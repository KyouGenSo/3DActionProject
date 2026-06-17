#pragma once
#include <numbers>

#include "ICameraController.h"
#include "../CameraConfig.h"
#include "Vector3.h"
#include "Input.h"

/// <summary>
/// プレイヤー後方から追従し、ボス注視機能を持つ三人称カメラ
/// </summary>
class ThirdPersonController : public TargetedCameraController {
public:
    ThirdPersonController();

    ~ThirdPersonController() override = default;

    void Update(float deltaTime) override;

    bool IsActive() const override { return isActive_; }

    CameraControlPriority GetPriority() const override {
        return CameraControlPriority::FOLLOW_DEFAULT;
    }

    void Activate() override;

    void Deactivate() override { isActive_ = false; }

    void Reset();

    //==================== Setter ====================

    void SetOffset(const Tako::Vector3& offset) {
        offset_ = offset;
        offsetOrigin_ = offset;
    }

    void SetRotateSpeed(float speed) {
        rotateSpeed_ = speed;
    }

    /// <param name="smoothness">0.0-1.0</param>
    void SetSmoothness(float smoothness) {
        followSmoothness_ = smoothness;
    }

    /// <summary>
    /// 注視対象（ボスなど）を設定
    /// </summary>
    void SetSecondaryTarget(const Tako::Transform* target) {
        secondaryTarget_ = target;
    }

    void EnableLookAtTarget(bool enable) {
        enableLookAtTarget_ = enable;
    }

    //==================== Getter ====================

    const Tako::Vector3& GetOffset() const { return offset_; }

    const Tako::Vector3& GetInterpolatedTargetPosition() const {
        return interpolatedTargetPos_;
    }

private:
    void ProcessInput(float deltaTime);

    void UpdateRotation();

    void UpdatePosition();

    /// <summary>
    /// カメラ回転を考慮したオフセットを計算
    /// </summary>
    /// <returns>回転適用後のワールド空間オフセット</returns>
    Tako::Vector3 CalculateOffset() const;

    /// <summary>
    /// セカンダリターゲットへの注視回転を計算
    /// </summary>
    /// <returns>注視に必要な回転角（ラジアン、見下ろし角を加算）。ターゲット未設定なら現在のカメラ回転</returns>
    Tako::Vector3 CalculateLookAtRotation() const;

private:
    Tako::Input* input_ = nullptr;

    Tako::Vector3 interpolatedTargetPos_ = {};
    Tako::Vector3 offset_ = {
        CameraConfig::ThirdPerson::DEFAULT_OFFSET_X,
        CameraConfig::ThirdPerson::DEFAULT_OFFSET_Y,
        CameraConfig::ThirdPerson::DEFAULT_OFFSET_Z
    };
    Tako::Vector3 offsetOrigin_ = offset_;

    float destinationAngleX_ = CameraConfig::ThirdPerson::DEFAULT_ANGLE_X;  // ラジアン
    float destinationAngleY_ = 0.0f;  // ラジアン
    float destinationAngleZ_ = 0.0f;  // ラジアン

    // DEFAULT_ROTATE_SPEED（度）をラジアンに変換
    float rotateSpeed_ = CameraConfig::ThirdPerson::DEFAULT_ROTATE_SPEED * (std::numbers::pi_v<float> / 180.0f);
    float followSmoothness_ = CameraConfig::FOLLOW_SMOOTHNESS;
    float offsetLerpSpeed_ = CameraConfig::OFFSET_LERP_SPEED;
    float rotationLerpSpeed_ = CameraConfig::ROTATION_LERP_SPEED;
    float standardFov_ = CameraConfig::STANDARD_FOV;

    bool isRotating_ = false;

    const Tako::Transform* secondaryTarget_ = nullptr;  // 注視対象（ボスなど）
    bool enableLookAtTarget_ = false;
};