#pragma once
#include "CameraKeyframe.h"
#include "CameraSystem/CameraConfig.h"
#include "Camera.h"
#include "Quaternion.h"
#include "Transform.h"
#include <vector>
#include <string>
#include <memory>

/// <summary>
/// キーフレーム間補間でカメラを動かすアニメーション
/// </summary>
class CameraAnimation {
public: //構造体
    enum class PlayState {
        STOPPED,
        PLAYING,
        PAUSED
    };

    enum class StartMode {
        JUMP_CUT,        ///< 即座に最初のキーフレームへ
        SMOOTH_BLEND     ///< 現在位置から最初のキーフレームまで補間
    };

public: //メンバー関数
    CameraAnimation();
    ~CameraAnimation();

    void Update(float deltaTime);

    /// <summary>
    /// キーフレームを追加。時刻順にソートし総時間を更新
    /// </summary>
    /// <param name="keyframe">追加するキーフレーム</param>
    void AddKeyframe(const CameraKeyframe& keyframe);

    /// <summary>
    /// 現在のカメラ状態から指定時刻のキーフレームを生成して追加。camera_ 未設定時は何もしない
    /// </summary>
    /// <param name="time">キーフレーム時刻（秒）</param>
    /// <param name="interpolation">このキーフレームから次への補間方法</param>
    void AddKeyframeFromCurrentCamera(float time,
        CameraKeyframe::InterpolationType interpolation = CameraKeyframe::InterpolationType::LINEAR);

    /// <summary>
    /// 指定 index のキーフレームを削除し総時間を更新。範囲外は無視
    /// </summary>
    /// <param name="index">削除するキーフレームの添字</param>
    void RemoveKeyframe(size_t index);

    /// <summary>
    /// 指定 index のキーフレームを置換。時刻順に再ソートし総時間を更新。範囲外は無視
    /// </summary>
    /// <param name="index">置換するキーフレームの添字</param>
    /// <param name="keyframe">新しい内容</param>
    void EditKeyframe(size_t index, const CameraKeyframe& keyframe);

    void ClearKeyframes();

    void Play();
    void Pause();

    /// <summary>
    /// 停止し時間を0に戻す
    /// </summary>
    void Stop();

    /// <summary>
    /// FOV 復元なしで停止（アニメーション切り替え時用）
    /// </summary>
    void StopWithoutRestore();

    /// <summary>
    /// 現在時刻のみ0に戻す（状態はそのまま）
    /// </summary>
    void Reset();

    bool LoadFromJson(const std::string& filepath);
    bool SaveToJson(const std::string& filepath) const;

#ifdef _DEBUG
    void DrawImGui();
#endif

    //==========================================================================
    //Setter
    //==========================================================================
    void SetCamera(Tako::Camera* camera) { camera_ = camera; }

    /// <summary>
    /// 相対座標の基準ターゲット（nullptr で解除）
    /// </summary>
    void SetTarget(const Tako::Transform* target) { targetTransform_ = target; }

    void SetLooping(bool loop) { isLooping_ = loop; }
    void SetPlaySpeed(float speed) { playSpeed_ = speed; }
    void SetAnimationName(const std::string& name) { animationName_ = name; }

    /// <summary>
    /// 現在時刻を設定（シーク）
    /// </summary>
    void SetCurrentTime(float time);

    /// <summary>
    /// キーフレームをカメラに適用（index 省略時は選択中キーフレーム）
    /// </summary>
    void ApplyKeyframeToCamera(int index = -1);

    void SetStartMode(StartMode mode) { startMode_ = mode; }
    void SetBlendDuration(float duration) { blendDuration_ = duration; }

    //==========================================================================
    //Getter
    //==========================================================================
    [[nodiscard]] size_t GetKeyframeCount() const { return keyframes_.size(); }
    [[nodiscard]] const CameraKeyframe& GetKeyframe(size_t index) const { return keyframes_[index]; }
    [[nodiscard]] float GetDuration() const { return duration_; }
    [[nodiscard]] float GetPlaybackTime() const { return currentTime_; }
    [[nodiscard]] PlayState GetPlayState() const { return playState_; }
    [[nodiscard]] bool IsLooping() const { return isLooping_; }
    [[nodiscard]] const std::string& GetAnimationName() const { return animationName_; }
    [[nodiscard]] bool IsEditingKeyframe() const;
    [[nodiscard]] int GetSelectedKeyframeIndex() const;
    [[nodiscard]] const Tako::Transform* GetTarget() const { return targetTransform_; }
    [[nodiscard]] StartMode GetStartMode() const { return startMode_; }
    [[nodiscard]] float GetBlendDuration() const { return blendDuration_; }
    [[nodiscard]] bool IsBlending() const { return isBlending_; }

    /// <summary>
    /// ブレンド進行度を取得（0.0～1.0）
    /// </summary>
    [[nodiscard]] float GetBlendProgress() const { return blendProgress_; }

private: //非公開関数
    void SortKeyframes();
    void UpdateDuration();

    /// <summary>
    /// time を挟む前後キーフレームの index を求める
    /// </summary>
    /// <param name="time">対象時刻（秒）</param>
    /// <param name="prevIndex">出力。time 以下で最大時刻のキーフレーム添字</param>
    /// <param name="nextIndex">出力。prevIndex の次の添字。末尾超過時はループなら0、非ループなら prevIndex</param>
    /// <returns>キーフレームが2個以上あれば true、未満なら false</returns>
    bool FindKeyframeIndices(float time, size_t& prevIndex, size_t& nextIndex) const;

    /// <summary>
    /// prev/next を係数 t で補間しカメラに適用。座標系は prev 側を採用
    /// </summary>
    /// <param name="prev">補間元キーフレーム</param>
    /// <param name="next">補間先キーフレーム</param>
    /// <param name="t">補間係数（0.0～1.0、イージング適用後）。位置/FOVは線形、回転は Slerp</param>
    void InterpolateKeyframes(const CameraKeyframe& prev, const CameraKeyframe& next, float t);

    /// <summary>
    /// 補間係数 t にイージングを適用
    /// </summary>
    /// <param name="t">入力係数（0.0～1.0）</param>
    /// <param name="type">補間方法。CUBIC_BEZIER は線形にフォールバック</param>
    /// <returns>イージング後の係数</returns>
    float ApplyEasing(float t, CameraKeyframe::InterpolationType type) const;

    /// <summary>
    /// オイラー角からクォータニオンへ変換（回転順序 Y*X*Z）
    /// </summary>
    /// <param name="euler">オイラー角（ラジアン）</param>
    /// <returns>対応するクォータニオン</returns>
    Tako::Quaternion EulerToQuaternion(const Tako::Vector3& euler) const;

    /// <summary>
    /// クォータニオンからオイラー角へ変換
    /// </summary>
    /// <param name="q">入力クォータニオン</param>
    /// <returns>オイラー角（ラジアン）。ジンバルロック時は y を±π/2に固定</returns>
    Tako::Vector3 QuaternionToEuler(const Tako::Quaternion& q) const;

    /// <summary>
    /// 選択解除しカメラを元の値に戻す
    /// </summary>
    void ClearDeselectState();

    /// <summary>
    /// 座標系を解決しキーフレームをカメラへ直接適用
    /// </summary>
    void ApplyKeyframeDirectly(const CameraKeyframe& kf);

private: //メンバー変数
    //基本情報
    std::string                 animationName_   = "Untitled";
    std::vector<CameraKeyframe> keyframes_;
    Tako::Camera*               camera_          = nullptr;
    const Tako::Transform*      targetTransform_ = nullptr;     ///< 相対座標の基準

    //再生状態
    float     currentTime_ = 0.0f;                ///< 秒
    float     duration_    = 0.0f;                ///< 秒
    float     playSpeed_   = 1.0f;                ///< 1.0 が標準
    PlayState playState_   = PlayState::STOPPED;
    bool      isLooping_   = false;

    //ブレンド
    StartMode startMode_     = StartMode::JUMP_CUT;
    float     blendDuration_ = 0.5f;                 ///< 秒
    float     blendProgress_ = 0.0f;                 ///< 0.0～1.0
    bool      isBlending_    = false;

    //ブレンド開始時のカメラ状態
    Tako::Vector3 blendStartPosition_;
    Tako::Vector3 blendStartRotation_;
    float         blendStartFov_;

    //FOV 復元
    float originalFov_;     ///< アニメーション開始前の FOV
    bool  hasOriginalFov_;

#ifdef _DEBUG
    int            selectedKeyframeIndex_ = -1;
    bool           showTimeline_          = true;
    bool           autoSortKeyframes_     = true;
    CameraKeyframe tempKeyframe_;                  ///< 編集用の一時バッファ
#endif
};
