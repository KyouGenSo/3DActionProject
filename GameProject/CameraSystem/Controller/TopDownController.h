#pragma once
#include "ICameraController.h"
#include "../CameraConfig.h"
#include "Vector3.h"

/// <summary>
/// 俯瞰視点の固定角度カメラコントローラー
/// </summary>
class TopDownController : public TargetedCameraController {
public: //メンバー関数
    TopDownController();
    ~TopDownController() override = default;

    void Update(float deltaTime) override;
    void Activate() override;
    void Deactivate() override { isActive_ = false; }
    void Reset();

    //========================================================
    //Setter
    //========================================================
    void SetBaseHeight(float height) {
        baseHeight_ = height;
    }

    /// <param name="angleX">ラジアン</param>
    void SetCameraAngle(float angleX) {
        cameraAngleX_ = angleX;
    }

    void SetHeightMultiplier(float multiplier) {
        heightMultiplier_ = multiplier;
    }

    /// <param name="smoothness">0.0-1.0</param>
    void SetSmoothness(float smoothness) {
        followSmoothness_ = smoothness;
    }

    //========================================================
    //Getter
    //========================================================
    bool IsActive() const override { return isActive_; }

    CameraControlPriority GetPriority() const override {
        return CameraControlPriority::FOLLOW_DEFAULT;
    }

    float GetCurrentHeight() const { return currentHeight_; }

    const Tako::Vector3& GetInterpolatedTargetPosition() const {
        return interpolatedTargetPos_;
    }

private: //非公開関数
    /// <summary>
    /// 複数ターゲットの中心点を算出
    /// </summary>
    /// <returns>全ターゲットの平均位置。primaryTarget_ が無ければゼロベクトル</returns>
    Tako::Vector3 CalculateFocusPoint() const;

    /// <summary>
    /// 全ターゲット間の最大距離を算出
    /// </summary>
    /// <returns>最大距離。追加ターゲットが無ければ 0</returns>
    float CalculateMaxTargetDistance() const;

    /// <summary>
    /// ターゲット間距離からカメラの高さと後退量を算出
    /// </summary>
    /// <param name="targetDistance">ターゲット間の最大距離</param>
    /// <param name="outHeight">算出した高さ（minHeight_〜maxHeight_ でクランプ）</param>
    /// <param name="outBackOffset">算出した後退量（負値、minBackOffset_〜maxBackOffset_ でクランプ）</param>
    void CalculateCameraParameters(float targetDistance,
        float& outHeight,
        float& outBackOffset) const;

    void UpdateCameraPosition();

private: //メンバー変数
    Tako::Vector3 interpolatedTargetPos_ = {};
    float         currentHeight_         = CameraConfig::TopDown::BASE_HEIGHT;
    float         currentBackOffset_     = CameraConfig::TopDown::BASE_BACK_OFFSET;

    float baseHeight_       = CameraConfig::TopDown::BASE_HEIGHT;
    float heightMultiplier_ = CameraConfig::TopDown::HEIGHT_MULTIPLIER;
    float minHeight_        = CameraConfig::TopDown::MIN_HEIGHT;
    float maxHeight_        = CameraConfig::TopDown::MAX_HEIGHT;

    float cameraAngleX_ = CameraConfig::TopDown::DEFAULT_ANGLE_X;

    float baseBackOffset_       = CameraConfig::TopDown::BASE_BACK_OFFSET;
    float backOffsetMultiplier_ = CameraConfig::TopDown::BACK_OFFSET_MULTIPLIER;
    float minBackOffset_        = CameraConfig::TopDown::MIN_BACK_OFFSET;
    float maxBackOffset_        = CameraConfig::TopDown::MAX_BACK_OFFSET;

    float followSmoothness_ = CameraConfig::FOLLOW_SMOOTHNESS;

    float standardFov_ = CameraConfig::STANDARD_FOV;
};
