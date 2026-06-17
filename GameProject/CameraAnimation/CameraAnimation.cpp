#include "CameraAnimation.h"
#include "Vec3Func.h"
#include "QuatFunc.h"
#include "EaseFunc.h"
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

CameraAnimation::CameraAnimation() {
    keyframes_.reserve(CameraConfig::Animation::KEYFRAME_RESERVE_COUNT);

    originalFov_ = CameraConfig::Animation::DEFAULT_FOV_DEGREES;
    hasOriginalFov_ = false;
}

CameraAnimation::~CameraAnimation() {
}

void CameraAnimation::Update(float deltaTime) {
    if (!camera_) {
        return;
    }

    if (playState_ != PlayState::PLAYING) {
        return;
    }

    if (isBlending_) {
        blendProgress_ += deltaTime / blendDuration_;

        if (blendProgress_ >= 1.0f) {
            blendProgress_ = 1.0f;
            isBlending_ = false;
        }

        // 現在のカメラ状態から最初のキーフレームまで補間
        if (!keyframes_.empty()) {
            const CameraKeyframe& firstKf = keyframes_[0];
            float t = ApplyEasing(blendProgress_, CameraKeyframe::InterpolationType::EASE_IN_OUT);

            Vector3 targetPosition = firstKf.position;
            if (firstKf.coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE && targetTransform_) {
                targetPosition = Vec3::Add(targetTransform_->translate, firstKf.position);
            }
            Vector3 position = Vec3::Lerp(blendStartPosition_, targetPosition, t);

            // 回転は Slerp
            Quaternion q1 = EulerToQuaternion(blendStartRotation_);
            Quaternion q2 = EulerToQuaternion(firstKf.rotation);
            Quaternion qResult = Quat::Slerp(q1, q2, t);
            Vector3 rotation = QuaternionToEuler(qResult);

            float fov = Vec3::Lerp(blendStartFov_, firstKf.fov, t);

            camera_->SetTranslate(position);
            camera_->SetRotate(rotation);
            camera_->SetFovY(fov);
        }

        // ブレンド中は通常のアニメーション処理をスキップ
        return;
    }

    if (keyframes_.size() < 2) {
        return;
    }

    currentTime_ += deltaTime * playSpeed_;

    if (currentTime_ >= duration_) {
        if (isLooping_) {
            currentTime_ = fmodf(currentTime_, duration_);
        }
        else {
            currentTime_ = duration_;
            playState_ = PlayState::STOPPED;

            if (hasOriginalFov_ && camera_) {
                camera_->SetFovY(CameraConfig::STANDARD_FOV);
                hasOriginalFov_ = false;
            }
        }
    }

    // 逆再生で時間が負になった場合
    if (currentTime_ < 0.0f) {
        if (isLooping_) {
            currentTime_ = duration_ + fmodf(currentTime_, duration_);
        }
        else {
            currentTime_ = 0.0f;
            playState_ = PlayState::STOPPED;

            if (hasOriginalFov_ && camera_) {
                camera_->SetFovY(CameraConfig::STANDARD_FOV);
                hasOriginalFov_ = false;
            }
        }
    }

    size_t prevIndex = 0, nextIndex = 0;
    if (FindKeyframeIndices(currentTime_, prevIndex, nextIndex)) {
        const CameraKeyframe& prev = keyframes_[prevIndex];
        const CameraKeyframe& next = keyframes_[nextIndex];

        float timeDiff = next.time - prev.time;
        float t = 0.0f;
        if (timeDiff > 0.0f) {
            t = (currentTime_ - prev.time) / timeDiff;
            t = std::clamp(t, 0.0f, 1.0f);

            t = ApplyEasing(t, prev.interpolation);
        }

        InterpolateKeyframes(prev, next, t);
    }
}

void CameraAnimation::AddKeyframe(const CameraKeyframe& keyframe) {
    keyframes_.push_back(keyframe);

#ifdef _DEBUG
    if (autoSortKeyframes_) {
#endif
        SortKeyframes();
#ifdef _DEBUG
    }
#endif

    UpdateDuration();
}

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

void CameraAnimation::RemoveKeyframe(size_t index) {
    if (index >= keyframes_.size()) {
        return;
    }

    keyframes_.erase(keyframes_.begin() + index);
    UpdateDuration();
}

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

void CameraAnimation::ClearKeyframes() {
    keyframes_.clear();
    duration_ = 0.0f;
    currentTime_ = 0.0f;
    playState_ = PlayState::STOPPED;
}

void CameraAnimation::Play() {
    if (keyframes_.empty() || !camera_) {
        return;
    }

    playState_ = PlayState::PLAYING;

    originalFov_ = camera_->GetFovY();
    hasOriginalFov_ = true;

    ClearDeselectState();

    if (startMode_ == StartMode::JUMP_CUT) {
        currentTime_ = 0.0f;
        isBlending_ = false;

        if (!keyframes_.empty()) {
            ApplyKeyframeDirectly(keyframes_[0]);
        }
    }
    else {
        // SMOOTH_BLEND: 現在のカメラ状態を起点にブレンド開始
        blendStartPosition_ = camera_->GetTranslate();
        blendStartRotation_ = camera_->GetRotate();
        blendStartFov_ = camera_->GetFovY();
        blendProgress_ = 0.0f;
        isBlending_ = true;
        currentTime_ = 0.0f;
    }
}

void CameraAnimation::Pause() {
    if (playState_ == PlayState::PLAYING) {
        playState_ = PlayState::PAUSED;
    }
}

void CameraAnimation::Stop() {
    playState_ = PlayState::STOPPED;
    currentTime_ = 0.0f;

    if (hasOriginalFov_ && camera_) {
        camera_->SetFovY(originalFov_);
        hasOriginalFov_ = false;
    }

    ClearDeselectState();
}

void CameraAnimation::StopWithoutRestore() {
    playState_ = PlayState::STOPPED;
    currentTime_ = 0.0f;

    // FOV は復元せずフラグのみリセット
    hasOriginalFov_ = false;

    ClearDeselectState();
}

void CameraAnimation::Reset() {
    currentTime_ = 0.0f;
}

void CameraAnimation::SetCurrentTime(float time) {
    currentTime_ = std::clamp(time, 0.0f, duration_);

    // 再生状態に関係なくプレビュー/スクラブのため補間を実行
    if (!camera_ || keyframes_.size() < 2) {
        return;
    }

    size_t prevIndex = 0, nextIndex = 0;
    if (FindKeyframeIndices(currentTime_, prevIndex, nextIndex)) {
        const CameraKeyframe& prev = keyframes_[prevIndex];
        const CameraKeyframe& next = keyframes_[nextIndex];

        float timeDiff = next.time - prev.time;
        float t = 0.0f;
        if (timeDiff > 0.0f) {
            t = (currentTime_ - prev.time) / timeDiff;
            t = std::clamp(t, 0.0f, 1.0f);

            t = ApplyEasing(t, prev.interpolation);
        }

        InterpolateKeyframes(prev, next, t);
    }
}

void CameraAnimation::SortKeyframes() {
    std::sort(keyframes_.begin(), keyframes_.end(),
        [](const CameraKeyframe& a, const CameraKeyframe& b) {
            return a.time < b.time;
        });
}

void CameraAnimation::UpdateDuration() {
    if (keyframes_.empty()) {
        duration_ = 0.0f;
        return;
    }

    // 末尾キーフレームの時刻が総時間
    duration_ = keyframes_.back().time;
}

bool CameraAnimation::FindKeyframeIndices(float time, size_t& prevIndex, size_t& nextIndex) const {
    if (keyframes_.size() < 2) {
        return false;
    }

    // time 以下で最大の時刻を持つキーフレーム
    prevIndex = 0;
    for (size_t i = 0; i < keyframes_.size(); ++i) {
        if (keyframes_[i].time <= time) {
            prevIndex = i;
        }
        else {
            break;
        }
    }

    nextIndex = prevIndex + 1;
    if (nextIndex >= keyframes_.size()) {
        // 末尾を超えた場合、ループ時は先頭へ戻し、非ループ時は末尾を維持
        if (isLooping_ && keyframes_.size() > 1) {
            nextIndex = 0;
        }
        else {
            nextIndex = prevIndex;
        }
    }

    return true;
}

void CameraAnimation::InterpolateKeyframes(const CameraKeyframe& prev, const CameraKeyframe& next, float t) {
    if (!camera_) {
        return;
    }

    Vector3 position = Vec3::Lerp(prev.position, next.position, t);

    // 座標系が混在する場合は prev 側を優先
    CameraKeyframe::CoordinateType coordinateType = prev.coordinateType;

    // TARGET_RELATIVE では position をオフセットとしてターゲット位置に加算
    if (coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE && targetTransform_) {
        position = Vec3::Add(targetTransform_->translate, position);
    }

    // 回転は Slerp
    Quaternion q1 = EulerToQuaternion(prev.rotation);
    Quaternion q2 = EulerToQuaternion(next.rotation);
    Quaternion qResult = Quat::Slerp(q1, q2, t);
    Vector3 rotation = QuaternionToEuler(qResult);

    float fov = Vec3::Lerp(prev.fov, next.fov, t);

    camera_->SetTranslate(position);
    camera_->SetRotate(rotation);
    camera_->SetFovY(fov);
}

float CameraAnimation::ApplyEasing(float t, CameraKeyframe::InterpolationType type) const {
    switch (type) {
    case CameraKeyframe::InterpolationType::LINEAR:
        return Ease::Linear(t);

    case CameraKeyframe::InterpolationType::EASE_IN:
        return Ease::InQuad(t);

    case CameraKeyframe::InterpolationType::EASE_OUT:
        return Ease::OutQuad(t);

    case CameraKeyframe::InterpolationType::EASE_IN_OUT:
        return Ease::InOutQuad(t);

    case CameraKeyframe::InterpolationType::CUBIC_BEZIER:
        // TODO: カスタムベジェカーブ実装。現在は線形にフォールバック
        return t;

    default:
        return t;
    }
}

Quaternion CameraAnimation::EulerToQuaternion(const Vector3& euler) const {
    Quaternion qx = Quat::MakeRotateAxisAngle(Vector3(1.0f, 0.0f, 0.0f), euler.x);
    Quaternion qy = Quat::MakeRotateAxisAngle(Vector3(0.0f, 1.0f, 0.0f), euler.y);
    Quaternion qz = Quat::MakeRotateAxisAngle(Vector3(0.0f, 0.0f, 1.0f), euler.z);

    // 回転順序: Y * X * Z
    Quaternion result = Quat::Multiply(qy, qx);
    result = Quat::Multiply(result, qz);

    return result;
}

Vector3 CameraAnimation::QuaternionToEuler(const Quaternion& q) const {
    Vector3 euler;

    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    euler.x = std::atan2f(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (std::abs(sinp) >= 1.0f) {
        euler.y = std::copysignf(std::numbers::pi_v<float> / 2.0f, sinp); // ジンバルロック
    }
    else {
        euler.y = std::asinf(sinp);
    }

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    euler.z = std::atan2f(siny_cosp, cosy_cosp);

    return euler;
}

bool CameraAnimation::IsEditingKeyframe() const {
#ifdef _DEBUG
    return selectedKeyframeIndex_ >= 0 && selectedKeyframeIndex_ < static_cast<int>(keyframes_.size());
#else
    return false;
#endif
}

int CameraAnimation::GetSelectedKeyframeIndex() const {
#ifdef _DEBUG
    return selectedKeyframeIndex_;
#else
    return -1;
#endif
}

void CameraAnimation::ApplyKeyframeToCamera(int index) {
    if (!camera_) {
        return;
    }

#ifdef _DEBUG
    if (index < 0) {
        index = selectedKeyframeIndex_;
    }

    // 編集中のキーフレームは tempKeyframe_ を反映
    if (index == selectedKeyframeIndex_ && index >= 0) {
        camera_->SetTranslate(tempKeyframe_.position);
        camera_->SetRotate(tempKeyframe_.rotation);
        camera_->SetFovY(tempKeyframe_.fov);
        return;
    }
#endif

    if (index >= 0 && index < static_cast<int>(keyframes_.size())) {
        const CameraKeyframe& keyframe = keyframes_[index];
        camera_->SetTranslate(keyframe.position);
        camera_->SetRotate(keyframe.rotation);
        camera_->SetFovY(keyframe.fov);
    }
}

void CameraAnimation::ClearDeselectState() {
#ifdef _DEBUG
    if (selectedKeyframeIndex_ >= 0 && selectedKeyframeIndex_ < static_cast<int>(keyframes_.size())) {
        // 編集前の値に戻す
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

void CameraAnimation::ApplyKeyframeDirectly(const CameraKeyframe& kf) {
    if (!camera_) {
        return;
    }

    // TARGET_RELATIVE では position をオフセットとしてターゲット位置に加算
    Vector3 position = kf.position;
    if (kf.coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE && targetTransform_) {
        position = Vec3::Add(targetTransform_->translate, kf.position);
    }

    camera_->SetTranslate(position);
    camera_->SetRotate(kf.rotation);
    camera_->SetFovY(kf.fov);
}

bool CameraAnimation::LoadFromJson(const std::string& filepath) {
    try {
        std::filesystem::path jsonPath = "resources/Json/CameraAnimations/" + filepath;
        if (!jsonPath.has_extension()) {
            jsonPath += ".json";
        }

        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json json;
        file >> json;
        file.close();

        animationName_ = json.value("animation_name", "Untitled");
        isLooping_ = json.value("loop", false);
        playSpeed_ = json.value("play_speed", 1.0f);

        // 旧フォーマット互換: 欠落時はデフォルト値
        int startModeInt = json.value("start_mode", static_cast<int>(StartMode::JUMP_CUT));
        startMode_ = static_cast<StartMode>(startModeInt);
        blendDuration_ = json.value("blend_duration", CameraConfig::Animation::DEFAULT_BLEND_DURATION);

        keyframes_.clear();

        if (json.contains("keyframes")) {
            for (const auto& kf : json["keyframes"]) {
                CameraKeyframe keyframe = kf.get<CameraKeyframe>();
                keyframes_.push_back(keyframe);
            }
        }

        SortKeyframes();
        UpdateDuration();

#ifdef _DEBUG
        DebugUIManager::GetInstance()->AddLog(
            " CameraAnimation: Loaded animation" + animationName_ + " from " + jsonPath.string(),
            DebugUIManager::LogType::Info);
#endif

        return true;

    }
    catch (const std::exception& e) {
        (void)e;
        return false;
    }
}

bool CameraAnimation::SaveToJson(const std::string& filepath) const {
    try {
        nlohmann::json json;
        json["animation_name"] = animationName_;
        json["duration"] = duration_;
        json["loop"] = isLooping_;
        json["play_speed"] = playSpeed_;

        json["start_mode"] = static_cast<int>(startMode_);
        json["blend_duration"] = blendDuration_;

        json["keyframes"] = nlohmann::json::array();
        for (const auto& kf : keyframes_) {
            json["keyframes"].push_back(kf);
        }

        std::filesystem::path dirPath = "resources/Json/CameraAnimations";
        if (!std::filesystem::exists(dirPath)) {
            std::filesystem::create_directories(dirPath);
        }

        std::filesystem::path jsonPath = dirPath / filepath;
        if (!jsonPath.has_extension()) {
            jsonPath += ".json";
        }

        std::ofstream file(jsonPath);
        if (!file.is_open()) {
            return false;
        }

        file << json.dump(4);
        file.close();

#ifdef _DEBUG
        DebugUIManager::GetInstance()->AddLog(
            " CameraAnimation: Saved animation " + animationName_ + " to " + jsonPath.string(),
            DebugUIManager::LogType::Info);
#endif

        return true;

    }
    catch (const std::exception& e) {
        (void)e;
        return false;
    }
}

#ifdef _DEBUG
void CameraAnimation::DrawImGui() {
    ImGui::Separator();

    ImGui::Text("Animation: %s", animationName_.c_str());
    ImGui::Text("Duration: %.2f seconds", duration_);
    ImGui::Text("Current Time: %.2f", currentTime_);
    ImGui::Text("Keyframes: %zu", keyframes_.size());

    ImGui::Separator();

    ImGui::Text("Playback Controls");

    const char* stateStr = "STOPPED";
    if (playState_ == PlayState::PLAYING) stateStr = "PLAYING";
    else if (playState_ == PlayState::PAUSED) stateStr = "PAUSED";
    ImGui::Text("State: %s", stateStr);

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

    ImGui::Checkbox("Loop", &isLooping_);
    ImGui::SliderFloat("Play Speed", &playSpeed_,
        CameraConfig::Animation::MIN_PLAY_SPEED,
        CameraConfig::Animation::MAX_PLAY_SPEED, "%.2f");

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Keyframe Management")) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ClearDeselectState();
        }

        if (camera_) {
            if (ImGui::Button("Add Keyframe from Current Camera")) {
                AddKeyframeFromCurrentCamera(currentTime_);
            }

            static float newKeyTime = 0.0f;
            static int interpType = 0;
            static int coordType = 0;
            ImGui::DragFloat("New Keyframe Time", &newKeyTime,
                CameraConfig::Animation::KEYFRAME_DRAG_STEP, 0.0f, FLT_MAX);
            ImGui::Combo("Interpolation", &interpType,
                "LINEAR\0EASE_IN\0EASE_OUT\0EASE_IN_OUT\0");
            ImGui::Combo("Coordinate Type", &coordType,
                "WORLD\0TARGET_RELATIVE\0");

            if (targetTransform_) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Target: Set");
                if (coordType == 1) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                        "Position will be offset from target");
                }
            }
            else {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Target: Not Set");
                if (coordType == 1) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                        "Warning: Target not set, will use world coordinates");
                }
            }

            if (ImGui::Button("Add Custom Keyframe")) {
                CameraKeyframe kf;
                kf.time = newKeyTime;
                // TARGET_RELATIVE では現在位置からターゲット位置を引いてオフセット化
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

        if (ImGui::BeginChild("Keyframe List", ImVec2(0, 200), true)) {
            for (size_t i = 0; i < keyframes_.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));

                bool isSelected = (selectedKeyframeIndex_ == static_cast<int>(i));
                const char* coordTypeStr = (keyframes_[i].coordinateType == CameraKeyframe::CoordinateType::TARGET_RELATIVE)
                    ? "[REL]" : "[WLD]";
                std::string label = std::format("{} KF {}: {:.2f}s", coordTypeStr, i, keyframes_[i].time);

                // 削除ボタンのスペース確保のため Selectable 幅を制限
                float availWidth = ImGui::GetContentRegionAvail().x;
                if (ImGui::Selectable(label.c_str(), isSelected, 0, ImVec2(availWidth - 30, 0))) {
                    selectedKeyframeIndex_ = static_cast<int>(i);
                    tempKeyframe_ = keyframes_[i];
                    ApplyKeyframeToCamera(selectedKeyframeIndex_);
                }

                ImGui::SameLine();
                if (ImGui::SmallButton("X")) {
                    ImGui::PopID();  // break 前に PopID
                    RemoveKeyframe(i);
                    if (selectedKeyframeIndex_ == static_cast<int>(i)) {
                        selectedKeyframeIndex_ = -1;
                    }
                    else if (selectedKeyframeIndex_ > static_cast<int>(i)) {
                        selectedKeyframeIndex_--;
                    }
                    break; // 削除後のインデックスずれを防ぐ
                }

                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        if (selectedKeyframeIndex_ >= 0 && selectedKeyframeIndex_ < static_cast<int>(keyframes_.size())) {
            ImGui::Separator();
            ImGui::Text("Edit Keyframe %d", selectedKeyframeIndex_);

            if (ImGui::DragFloat("Time", &tempKeyframe_.time, 0.1f, 0.0f, duration_)) {
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

            if (ImGui::DragFloat3("Position", &tempKeyframe_.position.x, 0.1f)) {
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

            // 度数で表示・編集（内部はラジアン）
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

            // 度数で表示・編集（内部はラジアン）
            float fovDegrees = DirectX::XMConvertToDegrees(tempKeyframe_.fov);
            if (ImGui::DragFloat("FOV (deg)", &fovDegrees, 0.5f,
                CameraConfig::Animation::FOV_MIN_DEGREES,
                CameraConfig::Animation::FOV_MAX_DEGREES)) {
                tempKeyframe_.fov = DirectX::XMConvertToRadians(fovDegrees);
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

            int coordType = static_cast<int>(tempKeyframe_.coordinateType);
            if (ImGui::Combo("Coordinate Type", &coordType,
                "WORLD\0TARGET_RELATIVE\0")) {
                tempKeyframe_.coordinateType = static_cast<CameraKeyframe::CoordinateType>(coordType);
                ApplyKeyframeToCamera(selectedKeyframeIndex_);
            }

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

        static char animName[128] = "";
        if (ImGui::InputText("Animation Name", animName, sizeof(animName),
            ImGuiInputTextFlags_EnterReturnsTrue)) {
            animationName_ = animName;
        }
    }
}
#endif