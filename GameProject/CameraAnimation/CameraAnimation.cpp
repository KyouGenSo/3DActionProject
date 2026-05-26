#include "CameraAnimation.h"
#include "Vec3Func.h"
#include "QuatFunc.h"
#include "CameraSystem/CameraConfig.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <numbers>
#include <format>
#include <DirectXMath.h>

#ifdef _DEBUG
#include "ImGuiManager.h"
#include "DebugUIManager.h"
#endif

using namespace Tako;

/// <summary>
/// コンストラクタ
/// </summary>
CameraAnimation::CameraAnimation() {
    keyframes_.reserve(CameraConfig::Animation::KEYFRAME_RESERVE_COUNT); // 予め領域を確保

    // FOV 復元用変数の初期化
    originalFov_ = CameraConfig::Animation::DEFAULT_FOV_DEGREES;
    hasOriginalFov_ = false;
}

/// <summary>
/// デストラクタ
/// </summary>
CameraAnimation::~CameraAnimation() {
    // 特に処理なし
}

/// <summary>
/// 更新処理
/// </summary>
void CameraAnimation::Update(float deltaTime) {
    // カメラが設定されていない場合は何もしない
    if (!camera_) {
        return;
    }

    // 再生中でない場合は何もしない
    if (playState_ != PlayState::PLAYING) {
        return;
    }

    // ブレンド中の処理
    if (isBlending_) {
        blendProgress_ += deltaTime / blendDuration_;

        if (blendProgress_ >= 1.0f) {
            // ブレンド完了
            blendProgress_ = 1.0f;
            isBlending_ = false;
        }

        // 最初のキーフレームまで補間
        if (!keyframes_.empty()) {
            const CameraKeyframe& firstKf = keyframes_[0];
            float t = ApplyEasing(blendProgress_, CameraKeyframe::InterpolationType::EASE_IN_OUT);

            // 位置の補間（ターゲット相対の場合も考慮）
            Vector3 targetPosition = firstKf.position;
            if (firstKf.coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE && targetTransform_) {
                targetPosition = Vec3::Add(targetTransform_->translate, firstKf.position);
            }
            Vector3 position = Vec3::Lerp(blendStartPosition_, targetPosition, t);

            // 回転の補間（クォータニオンで Slerp）
            Quaternion q1 = EulerToQuaternion(blendStartRotation_);
            Quaternion q2 = EulerToQuaternion(firstKf.rotation);
            Quaternion qResult = Quat::Slerp(q1, q2, t);
            Vector3 rotation = QuaternionToEuler(qResult);

            // FOV の補間
            float fov = Vec3::Lerp(blendStartFov_, firstKf.fov, t);

            // カメラに適用
            camera_->SetTranslate(position);
            camera_->SetRotate(rotation);
            camera_->SetFovY(fov);
        }

        // ブレンド中は通常のアニメーション処理をスキップ
        return;
    }

    // キーフレームが2つ以上ない場合はアニメーション不可
    if (keyframes_.size() < 2) {
        return;
    }

    // 時間を進める
    currentTime_ += deltaTime * playSpeed_;

    // ループ処理または停止
    if (currentTime_ >= duration_) {
        if (isLooping_) {
            // ループ再生
            currentTime_ = fmodf(currentTime_, duration_);
        }
        else {
            // ワンショット再生の終了
            currentTime_ = duration_;
            playState_ = PlayState::STOPPED;

            // FOV を復元
            if (hasOriginalFov_ && camera_) {
                camera_->SetFovY(CameraConfig::STANDARD_FOV);
                hasOriginalFov_ = false;
            }
        }
    }

    // 負の時間の処理（逆再生対応）
    if (currentTime_ < 0.0f) {
        if (isLooping_) {
            currentTime_ = duration_ + fmodf(currentTime_, duration_);
        }
        else {
            currentTime_ = 0.0f;
            playState_ = PlayState::STOPPED;

            // FOV を復元
            if (hasOriginalFov_ && camera_) {
                camera_->SetFovY(CameraConfig::STANDARD_FOV);
                hasOriginalFov_ = false;
            }
        }
    }

    // キーフレーム間の補間を実行
    size_t prevIndex = 0, nextIndex = 0;
    if (FindKeyframeIndices(currentTime_, prevIndex, nextIndex)) {
        const CameraKeyframe& prev = keyframes_[prevIndex];
        const CameraKeyframe& next = keyframes_[nextIndex];

        // 補間係数を計算（0.0～1.0）
        float timeDiff = next.time - prev.time;
        float t = 0.0f;
        if (timeDiff > 0.0f) {
            t = (currentTime_ - prev.time) / timeDiff;
            t = std::clamp(t, 0.0f, 1.0f);

            // イージング関数を適用
            t = ApplyEasing(t, prev.interpolation);
        }

        // キーフレーム間を補間してカメラに適用
        InterpolateKeyframes(prev, next, t);
    }
}

/// <summary>
/// キーフレームの追加
/// </summary>
void CameraAnimation::AddKeyframe(const CameraKeyframe& keyframe) {
    keyframes_.push_back(keyframe);

    // 自動ソートが有効な場合
#ifdef _DEBUG
    if (autoSortKeyframes_) {
#endif
        SortKeyframes();
#ifdef _DEBUG
    }
#endif

    UpdateDuration();
}

/// <summary>
/// 現在のカメラ状態からキーフレームを追加
/// </summary>
void CameraAnimation::AddKeyframeFromCurrentCamera(float time,
    CameraKeyframe::InterpolationType interpolation) {

    if (!camera_) {
        return;
    }

    CameraKeyframe keyframe;
    keyframe.time = time;
    keyframe.position = camera_->GetTranslate();
    keyframe.rotation = camera_->GetRotate();
    keyframe.fov = camera_->GetFovY();
    keyframe.interpolation = interpolation;

    AddKeyframe(keyframe);
}

/// <summary>
/// キーフレームの削除
/// </summary>
void CameraAnimation::RemoveKeyframe(size_t index) {
    if (index >= keyframes_.size()) {
        return;
    }

    keyframes_.erase(keyframes_.begin() + index);
    UpdateDuration();
}

/// <summary>
/// キーフレームの編集
/// </summary>
void CameraAnimation::EditKeyframe(size_t index, const CameraKeyframe& keyframe) {
    if (index >= keyframes_.size()) {
        return;
    }

    keyframes_[index] = keyframe;

#ifdef _DEBUG
    if (autoSortKeyframes_) {
#endif
        SortKeyframes();
#ifdef _DEBUG
    }
#endif

    UpdateDuration();
}

/// <summary>
/// すべてのキーフレームをクリア
/// </summary>
void CameraAnimation::ClearKeyframes() {
    keyframes_.clear();
    duration_ = 0.0f;
    currentTime_ = 0.0f;
    playState_ = PlayState::STOPPED;
}

/// <summary>
/// 再生開始
/// </summary>
void CameraAnimation::Play() {
    if (keyframes_.empty() || !camera_) {
        return; // キーフレームが不足またはカメラ未設定
    }

    playState_ = PlayState::PLAYING;

    // 元の FOV を保存
    originalFov_ = camera_->GetFovY();
    hasOriginalFov_ = true;

    // 再生開始時に選択状態を解除（カメラを元に戻す）
    ClearDeselectState();

    // 開始モードに応じた処理
    if (startMode_ == StartMode::JUMP_CUT) {
        // 即座に最初のキーフレームに移動
        currentTime_ = 0.0f;
        isBlending_ = false;

        // 最初のキーフレームを即座に適用（1つしかない場合も対応）
        if (!keyframes_.empty()) {
            ApplyKeyframeDirectly(keyframes_[0]);
        }
    }
    else {
        // SMOOTH_BLEND: 現在のカメラ状態を保存してブレンド開始
        blendStartPosition_ = camera_->GetTranslate();
        blendStartRotation_ = camera_->GetRotate();
        blendStartFov_ = camera_->GetFovY();
        blendProgress_ = 0.0f;
        isBlending_ = true;
        currentTime_ = 0.0f;
    }
}

/// <summary>
/// 一時停止
/// </summary>
void CameraAnimation::Pause() {
    if (playState_ == PlayState::PLAYING) {
        playState_ = PlayState::PAUSED;
    }
}

/// <summary>
/// 停止（時間を0にリセット）
/// </summary>
void CameraAnimation::Stop() {
    playState_ = PlayState::STOPPED;
    currentTime_ = 0.0f;

    // FOV を復元
    if (hasOriginalFov_ && camera_) {
        camera_->SetFovY(originalFov_);
        hasOriginalFov_ = false;
    }

    // 停止時に選択状態を解除（カメラを元に戻す）
    ClearDeselectState();
}

/// <summary>
/// FOV 復元なしで停止（アニメーション切り替え時用）
/// </summary>
void CameraAnimation::StopWithoutRestore() {
    playState_ = PlayState::STOPPED;
    currentTime_ = 0.0f;

    // FOV 復元をスキップ（フラグのみリセット）
    hasOriginalFov_ = false;

    // 停止時に選択状態を解除（カメラを元に戻す）
    ClearDeselectState();
}

/// <summary>
/// 現在時刻をリセット
/// </summary>
void CameraAnimation::Reset() {
    currentTime_ = 0.0f;
}

/// <summary>
/// 現在時刻の設定（シーク）
/// </summary>
void CameraAnimation::SetCurrentTime(float time) {
    currentTime_ = std::clamp(time, 0.0f, duration_);

    // プレビュー/スクラブ時も補間を実行（再生状態に関係なく）
    if (!camera_ || keyframes_.size() < 2) {
        return;
    }

    // キーフレーム間の補間を実行
    size_t prevIndex = 0, nextIndex = 0;
    if (FindKeyframeIndices(currentTime_, prevIndex, nextIndex)) {
        const CameraKeyframe& prev = keyframes_[prevIndex];
        const CameraKeyframe& next = keyframes_[nextIndex];

        // 補間係数を計算（0.0～1.0）
        float timeDiff = next.time - prev.time;
        float t = 0.0f;
        if (timeDiff > 0.0f) {
            t = (currentTime_ - prev.time) / timeDiff;
            t = std::clamp(t, 0.0f, 1.0f);

            // イージング関数を適用
            t = ApplyEasing(t, prev.interpolation);
        }

        // キーフレーム間を補間してカメラに適用
        InterpolateKeyframes(prev, next, t);
    }
}

/// <summary>
/// キーフレームを時間でソート
/// </summary>
void CameraAnimation::SortKeyframes() {
    std::sort(keyframes_.begin(), keyframes_.end(),
        [](const CameraKeyframe& a, const CameraKeyframe& b) {
            return a.time < b.time;
        });
}

/// <summary>
/// アニメーションの総時間を更新
/// </summary>
void CameraAnimation::UpdateDuration() {
    if (keyframes_.empty()) {
        duration_ = 0.0f;
        return;
    }

    // 最後のキーフレームの時刻が総時間
    duration_ = keyframes_.back().time;
}

/// <summary>
/// 現在時刻に対応する2つのキーフレームを検索
/// </summary>
bool CameraAnimation::FindKeyframeIndices(float time, size_t& prevIndex, size_t& nextIndex) const {
    if (keyframes_.size() < 2) {
        return false;
    }

    // 時刻以下の最大のキーフレームを探す
    prevIndex = 0;
    for (size_t i = 0; i < keyframes_.size(); ++i) {
        if (keyframes_[i].time <= time) {
            prevIndex = i;
        }
        else {
            break;
        }
    }

    // 次のキーフレームを設定
    nextIndex = prevIndex + 1;
    if (nextIndex >= keyframes_.size()) {
        // 最後のキーフレームを超えた場合
        if (isLooping_ && keyframes_.size() > 1) {
            // ループ時は最初のキーフレームに戻る
            nextIndex = 0;
        }
        else {
            // ループしない場合は最後のキーフレームを維持
            nextIndex = prevIndex;
        }
    }

    return true;
}

/// <summary>
/// キーフレーム間の補間
/// </summary>
void CameraAnimation::InterpolateKeyframes(const CameraKeyframe& prev, const CameraKeyframe& next, float t) {
    if (!camera_) {
        return;
    }

    // 位置の補間
    Vector3 position = Vec3::Lerp(prev.position, next.position, t);

    // 座標系タイプを考慮（両方のキーフレームが同じ座標系である必要がある）
    // 異なる場合は前のキーフレームの座標系を優先
    CameraKeyframe::CoordinateType coordinateType = prev.coordinateType;

    // TARGET_RELATIVE モードの場合、ターゲット位置を加算
    if (coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE && targetTransform_) {
        // position はオフセットとして扱う
        position = Vec3::Add(targetTransform_->translate, position);
    }
    // ターゲットが設定されていない場合は、ワールド座標として扱う

    // 回転の補間（クォータニオンで Slerp）
    Quaternion q1 = EulerToQuaternion(prev.rotation);
    Quaternion q2 = EulerToQuaternion(next.rotation);
    Quaternion qResult = Quat::Slerp(q1, q2, t);
    Vector3 rotation = QuaternionToEuler(qResult);

    // FOV の補間（線形補間）
    float fov = Vec3::Lerp(prev.fov, next.fov, t);

    // カメラに適用
    camera_->SetTranslate(position);
    camera_->SetRotate(rotation);
    camera_->SetFovY(fov);
}

/// <summary>
/// イージング関数の適用
/// </summary>
float CameraAnimation::ApplyEasing(float t, CameraKeyframe::InterpolationType type) const {
    switch (type) {
    case CameraKeyframe::InterpolationType::LINEAR:
        return t;

    case CameraKeyframe::InterpolationType::EASE_IN:
        return t * t;

    case CameraKeyframe::InterpolationType::EASE_OUT:
        return 1.0f - (1.0f - t) * (1.0f - t);

    case CameraKeyframe::InterpolationType::EASE_IN_OUT:
        if (t < 0.5f) {
            return 2.0f * t * t;
        }
        else {
            return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
        }

    case CameraKeyframe::InterpolationType::CUBIC_BEZIER:
        // TODO: カスタムベジェカーブの実装
        // 現在は線形補間にフォールバック
        return t;

    default:
        return t;
    }
}

/// <summary>
/// オイラー角をクォータニオンに変換
/// </summary>
Quaternion CameraAnimation::EulerToQuaternion(const Vector3& euler) const {
    // 各軸周りの回転をクォータニオンで作成
    Quaternion qx = Quat::MakeRotateAxisAngle(Vector3(1.0f, 0.0f, 0.0f), euler.x);
    Quaternion qy = Quat::MakeRotateAxisAngle(Vector3(0.0f, 1.0f, 0.0f), euler.y);
    Quaternion qz = Quat::MakeRotateAxisAngle(Vector3(0.0f, 0.0f, 1.0f), euler.z);

    // 回転順序: Y * X * Z
    Quaternion result = Quat::Multiply(qy, qx);
    result = Quat::Multiply(result, qz);

    return result;
}

/// <summary>
/// クォータニオンをオイラー角に変換
/// </summary>
Vector3 CameraAnimation::QuaternionToEuler(const Quaternion& q) const {
    Vector3 euler;

    // クォータニオンから回転行列の要素を計算
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    euler.x = std::atan2f(sinr_cosp, cosr_cosp);

    // Pitch (Y 軸回転)
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (std::abs(sinp) >= 1.0f) {
        euler.y = std::copysignf(std::numbers::pi_v<float> / 2.0f, sinp); // ジンバルロック時
    }
    else {
        euler.y = std::asinf(sinp);
    }

    // Yaw (Z 軸回転)
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    euler.z = std::atan2f(siny_cosp, cosy_cosp);

    return euler;
}

/// <summary>
/// 現在キーフレームを編集中か判定
/// </summary>
bool CameraAnimation::IsEditingKeyframe() const {
#ifdef _DEBUG
    return selectedKeyframeIndex_ >= 0 && selectedKeyframeIndex_ < static_cast<int>(keyframes_.size());
#else
    return false;
#endif
}

/// <summary>
/// 選択中のキーフレームインデックスを取得
/// </summary>
int CameraAnimation::GetSelectedKeyframeIndex() const {
#ifdef _DEBUG
    return selectedKeyframeIndex_;
#else
    return -1;
#endif
}

/// <summary>
/// 指定したキーフレームをカメラに適用
/// </summary>
void CameraAnimation::ApplyKeyframeToCamera(int index) {
    if (!camera_) {
        return;
    }

#ifdef _DEBUG
    // インデックスが指定されていない場合は選択中のキーフレームを使用
    if (index < 0) {
        index = selectedKeyframeIndex_;
    }

    // 編集中の場合は編集中のキーフレーム（tempKeyframe_）を使用
    if (index == selectedKeyframeIndex_ && index >= 0) {
        camera_->SetTranslate(tempKeyframe_.position);
        camera_->SetRotate(tempKeyframe_.rotation);
        camera_->SetFovY(tempKeyframe_.fov);
        return;
    }
#endif

    // 有効なインデックスかチェック
    if (index >= 0 && index < static_cast<int>(keyframes_.size())) {
        const CameraKeyframe& keyframe = keyframes_[index];
        camera_->SetTranslate(keyframe.position);
        camera_->SetRotate(keyframe.rotation);
        camera_->SetFovY(keyframe.fov);
    }
}

/// <summary>
/// 選択解除時の処理（カメラを元の値に戻す）
/// </summary>
void CameraAnimation::ClearDeselectState() {
#ifdef _DEBUG
    if (selectedKeyframeIndex_ >= 0 && selectedKeyframeIndex_ < static_cast<int>(keyframes_.size())) {
        // 元のキーフレームの値に戻す
        const CameraKeyframe& original = keyframes_[selectedKeyframeIndex_];
        if (camera_) {
            camera_->SetTranslate(original.position);
            camera_->SetRotate(original.rotation);
            camera_->SetFovY(original.fov);
        }
    }
    selectedKeyframeIndex_ = -1;
#endif
}

/// <summary>
/// キーフレームをカメラに直接適用（内部用）
/// </summary>
void CameraAnimation::ApplyKeyframeDirectly(const CameraKeyframe& kf) {
    if (!camera_) {
        return;
    }

    // 座標系タイプに応じて位置を設定
    Vector3 position = kf.position;
    if (kf.coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE && targetTransform_) {
        // ターゲット相対座標の場合、ターゲット位置にオフセットを加算
        position = Vec3::Add(targetTransform_->translate, kf.position);
    }

    // カメラに適用
    camera_->SetTranslate(position);
    camera_->SetRotate(kf.rotation);
    camera_->SetFovY(kf.fov);
}

/// <summary>
/// JSON ファイルから読み込み
/// </summary>
bool CameraAnimation::LoadFromJson(const std::string& filepath) {
    try {
        // JSON ファイルパスを構築
        std::filesystem::path jsonPath = "resources/Json/CameraAnimations/" + filepath;
        if (!jsonPath.has_extension()) {
            jsonPath += ".json";
        }

        // ファイルを開く
        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            // ファイルが開けなかった
            return false;
        }

        // JSON パース
        nlohmann::json json;
        file >> json;
        file.close();

        // データを読み込み
        animationName_ = json.value("animation_name", "Untitled");
        isLooping_ = json.value("loop", false);
        playSpeed_ = json.value("play_speed", 1.0f);

        // 開始モード設定を読み込み（後方互換性のためデフォルト値を設定）
        int startModeInt = json.value("start_mode", static_cast<int>(StartMode::JUMP_CUT));
        startMode_ = static_cast<StartMode>(startModeInt);
        blendDuration_ = json.value("blend_duration", CameraConfig::Animation::DEFAULT_BLEND_DURATION);

        // キーフレームをクリア
        keyframes_.clear();

        // キーフレーム配列を読み込み
        if (json.contains("keyframes")) {
            for (const auto& kf : json["keyframes"]) {
                CameraKeyframe keyframe = kf.get<CameraKeyframe>();
                keyframes_.push_back(keyframe);
            }
        }

        // キーフレームをソートして総時間を更新
        SortKeyframes();
        UpdateDuration();

#ifdef _DEBUG
        DebugUIManager::GetInstance()->AddLog(
            " CameraAnimation: Loaded animation" + animationName_ + " from " + jsonPath.string(),
            DebugUIManager::LogType::Info);
#endif

        // 読み込み成功
        return true;

    }
    catch (const std::exception& e) {
        // エラー処理
        (void)e; // 警告回避
        return false;
    }
}

/// <summary>
/// JSON ファイルに保存
/// </summary>
bool CameraAnimation::SaveToJson(const std::string& filepath) const {
    try {
        // 保存用 JSON 作成
        nlohmann::json json;
        json["animation_name"] = animationName_;
        json["duration"] = duration_;
        json["loop"] = isLooping_;
        json["play_speed"] = playSpeed_;

        // 開始モード設定を保存
        json["start_mode"] = static_cast<int>(startMode_);
        json["blend_duration"] = blendDuration_;

        // キーフレーム配列を保存
        json["keyframes"] = nlohmann::json::array();
        for (const auto& kf : keyframes_) {
            json["keyframes"].push_back(kf);
        }

        // 保存先ディレクトリを確認・作成
        std::filesystem::path dirPath = "resources/Json/CameraAnimations";
        if (!std::filesystem::exists(dirPath)) {
            std::filesystem::create_directories(dirPath);
        }

        // JSON ファイルパスを構築
        std::filesystem::path jsonPath = dirPath / filepath;
        if (!jsonPath.has_extension()) {
            jsonPath += ".json";
        }

        // ファイルに書き込み
        std::ofstream file(jsonPath);
        if (!file.is_open()) {
            return false;
        }

        // インデント付きで書き込み
        file << json.dump(4);
        file.close();

#ifdef _DEBUG
        // Debug ログ出力
        DebugUIManager::GetInstance()->AddLog(
            " CameraAnimation: Saved animation " + animationName_ + " to " + jsonPath.string(),
            DebugUIManager::LogType::Info);
#endif

        // 保存成功
        return true;

    }
    catch (const std::exception& e) {
        // エラー処理
        (void)e; // 警告回避
        return false;
    }
}

#ifdef _DEBUG
/// <summary>
/// ImGui でのデバッグ表示
/// </summary>
void CameraAnimation::DrawImGui() {
    ImGui::Separator();

    // アニメーション情報
    ImGui::Text("Animation: %s", animationName_.c_str());
    ImGui::Text("Duration: %.2f seconds", duration_);
    ImGui::Text("Current Time: %.2f", currentTime_);
    ImGui::Text("Keyframes: %zu", keyframes_.size());

    ImGui::Separator();

    // 再生コントロール
    ImGui::Text("Playback Controls");

    // 再生状態の表示
    const char* stateStr = "STOPPED";
    if (playState_ == PlayState::PLAYING) stateStr = "PLAYING";
    else if (playState_ == PlayState::PAUSED) stateStr = "PAUSED";
    ImGui::Text("State: %s", stateStr);

    // ボタン
    if (ImGui::Button("Play")) {
        Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        Pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        Stop();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        Reset();
    }

    // 再生設定
    ImGui::Checkbox("Loop", &isLooping_);
    ImGui::SliderFloat("Play Speed", &playSpeed_,
        CameraConfig::Animation::MIN_PLAY_SPEED,
        CameraConfig::Animation::MAX_PLAY_SPEED, "%.2f");

    ImGui::Separator();

    // キーフレーム管理
    if (ImGui::CollapsingHeader("Keyframe Management")) {
        // Esc キーで選択解除
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ClearDeselectState();
        }

        // 現在のカメラ状態を追加
        if (camera_) {
            if (ImGui::Button("Add Keyframe from Current Camera")) {
                AddKeyframeFromCurrentCamera(currentTime_);
            }

            // 手動でキーフレーム追加
            static float newKeyTime = 0.0f;
            static int interpType = 0;
            static int coordType = 0;
            ImGui::DragFloat("New Keyframe Time", &newKeyTime,
                CameraConfig::Animation::KEYFRAME_DRAG_STEP, 0.0f, FLT_MAX);
            ImGui::Combo("Interpolation", &interpType,
                "LINEAR\0EASE_IN\0EASE_OUT\0EASE_IN_OUT\0");
            ImGui::Combo("Coordinate Type", &coordType,
                "WORLD\0TARGET_RELATIVE\0");

            // ターゲット設定状態の表示
            if (targetTransform_) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Target: Set");
                if (coordType == 1) { // TARGET_RELATIVE
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                        "Position will be offset from target");
                }
            }
            else {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Target: Not Set");
                if (coordType == 1) { // TARGET_RELATIVE
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                        "Warning: Target not set, will use world coordinates");
                }
            }

            if (ImGui::Button("Add Custom Keyframe")) {
                CameraKeyframe kf;
                kf.time = newKeyTime;
                // TARGET_RELATIVE モードかつターゲットが設定されている場合、現在位置からオフセットを計算
                if (coordType == 1 && targetTransform_) {
                    kf.position = Vec3::Subtract(camera_->GetTranslate(), targetTransform_->translate);
                }
                else {
                    kf.position = camera_->GetTranslate();
                }
                kf.rotation = camera_->GetRotate();
                kf.fov = camera_->GetFovY();
                kf.interpolation = static_cast<CameraKeyframe::InterpolationType>(interpType);
                kf.coordinateType = static_cast<CameraKeyframe::CoordinateType>(coordType);
                AddKeyframe(kf);
            }
        }

        ImGui::Checkbox("Auto Sort Keyframes", &autoSortKeyframes_);

        if (ImGui::Button("Sort Keyframes")) {
            SortKeyframes();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All Keyframes")) {
            ClearKeyframes();
        }

        ImGui::Separator();

        // 選択中の表示と解除ボタン
        if (selectedKeyframeIndex_ >= 0) {
            ImGui::Text("Selected: Keyframe %d", selectedKeyframeIndex_);
            ImGui::SameLine();
            if (ImGui::Button("Deselect")) {
                ClearDeselectState();
            }
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press ESC to deselect");
        }
        else {
            ImGui::Text("No keyframe selected");
        }

        // キーフレームリスト
        if (ImGui::BeginChild("Keyframe List", ImVec2(0, 200), true)) {
            for (size_t i = 0; i < keyframes_.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));

                bool isSelected = (selectedKeyframeIndex_ == static_cast<int>(i));
                const char* coordTypeStr = (keyframes_[i].coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE)
                    ? "[REL]" : "[WLD]";
                std::string label = std::format("{} KF {}: {:.2f}s", coordTypeStr, i, keyframes_[i].time);

                // Selectable のサイズを制限して削除ボタンのスペースを確保
                float availWidth = ImGui::GetContentRegionAvail().x;
                if (ImGui::Selectable(label.c_str(), isSelected, 0, ImVec2(availWidth - 30, 0))) {
                    selectedKeyframeIndex_ = static_cast<int>(i);
                    tempKeyframe_ = keyframes_[i];
                    // 選択したキーフレームを即座にカメラに適用
                    ApplyKeyframeToCamera(selectedKeyframeIndex_);
                }

                // 削除ボタン
                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    ImGui::PopID();  // break する前に PopID を呼ぶ
                    RemoveKeyframe(i);
                    if (selectedKeyframeIndex_ == static_cast<int>(i)) {
                        selectedKeyframeIndex_ = -1;
                    }
                    else if (selectedKeyframeIndex_ > static_cast<int>(i)) {
                        selectedKeyframeIndex_--;
                    }
                    break; // ループを抜ける（削除後のインデックスずれを防ぐ）
                }

                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        // 選択中のキーフレームを編集
        if (selectedKeyframeIndex_ >= 0 && selectedKeyframeIndex_ < static_cast<int>(keyframes_.size())) {
            ImGui::Separator();
            ImGui::Text("Edit Keyframe %d", selectedKeyframeIndex_);

            // 時間
            if (ImGui::DragFloat("Time", &tempKeyframe_.time, 0.1f, 0.0f, duration_)) {
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

            // 位置
            if (ImGui::DragFloat3("Position", &tempKeyframe_.position.x, 0.1f)) {
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

            // 回転（度単位で表示）
            Vector3 rotationDegrees = {
                DirectX::XMConvertToDegrees(tempKeyframe_.rotation.x),
                DirectX::XMConvertToDegrees(tempKeyframe_.rotation.y),
                DirectX::XMConvertToDegrees(tempKeyframe_.rotation.z)
            };
            if (ImGui::DragFloat3("Rotation (deg)", &rotationDegrees.x, 1.0f)) {
                tempKeyframe_.rotation = {
                    DirectX::XMConvertToRadians(rotationDegrees.x),
                    DirectX::XMConvertToRadians(rotationDegrees.y),
                    DirectX::XMConvertToRadians(rotationDegrees.z)
                };
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

            // FOV（度単位で表示）
            float fovDegrees = DirectX::XMConvertToDegrees(tempKeyframe_.fov);
            if (ImGui::DragFloat("FOV (deg)", &fovDegrees, 0.5f,
                CameraConfig::Animation::FOV_MIN_DEGREES,
                CameraConfig::Animation::FOV_MAX_DEGREES)) {
                tempKeyframe_.fov = DirectX::XMConvertToRadians(fovDegrees);
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

            // 座標系タイプ
            int coordType = static_cast<int>(tempKeyframe_.coordinateType);
            if (ImGui::Combo("Coordinate Type", &coordType,
                "WORLD\0TARGET_RELATIVE\0")) {
                tempKeyframe_.coordinateType = static_cast<CameraKeyframe::CoordinateType>(coordType);
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

            // ターゲット相対モードの場合の説明
            if (tempKeyframe_.coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE) {
                ImGui::TextWrapped("Position is relative offset from target");
                if (!targetTransform_) {
                    ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f),
                        "Warning: No target set, will use world coordinates");
                }
            }

            int interpType = static_cast<int>(tempKeyframe_.interpolation);
            if (ImGui::Combo("Interpolation Type", &interpType,
                "LINEAR\0EASE_IN\0EASE_OUT\0EASE_IN_OUT\0")) {
                tempKeyframe_.interpolation = static_cast<CameraKeyframe::InterpolationType>(interpType);
            }

            if (ImGui::Button("Apply Changes")) {
                EditKeyframe(selectedKeyframeIndex_, tempKeyframe_);
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                tempKeyframe_ = keyframes_[selectedKeyframeIndex_];
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }
        }
    }

    ImGui::Separator();

    // ファイル操作
    if (ImGui::CollapsingHeader("File Operations")) {
        static char filename[128] = "";
        ImGui::InputText("Filename", filename, sizeof(filename));

        if (ImGui::Button("Save to JSON")) {
            if (strlen(filename) > 0) {
                if (SaveToJson(filename)) {
                    ImGui::Text("Saved successfully!");
                }
                else {
                    ImGui::Text("Save failed!");
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load from JSON")) {
            if (strlen(filename) > 0) {
                if (LoadFromJson(filename)) {
                    ImGui::Text("Loaded successfully!");
                }
                else {
                    ImGui::Text("Load failed!");
                }
            }
        }

        // アニメーション名の編集
        static char animName[128] = "";
        if (ImGui::InputText("Animation Name", animName, sizeof(animName),
            ImGuiInputTextFlags_EnterReturnsTrue)) {
            animationName_ = animName;
        }
    }
}
#endif