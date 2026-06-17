#include "ThirdPersonController.h"
#include "Vec3Func.h"
#include "Mat4x4Func.h"
#include <cmath>
#include <DirectXMath.h>

using namespace Tako;

ThirdPersonController::ThirdPersonController() {
    input_ = Input::GetInstance();
}

void ThirdPersonController::Update(float deltaTime) {
    if (!isActive_ || !camera_ || !primaryTarget_) {
        return;
    }

    if (camera_) {
        camera_->SetFovY(standardFov_);
    }

    ProcessInput(deltaTime);
    UpdateRotation();
    UpdatePosition();
}

void ThirdPersonController::Activate() {
    isActive_ = true;

    if (camera_) {
        camera_->SetFovY(standardFov_);
    }

    if (primaryTarget_) {
        Reset();
    }
}

void ThirdPersonController::Reset() {
    if (!primaryTarget_ || !camera_) {
        return;
    }

    interpolatedTargetPos_ = primaryTarget_->translate;

    // カメラをターゲットの向きに合わせる（ラジアン）
    camera_->SetRotate(Vector3(0.0f, primaryTarget_->rotate.y, 0.0f));
    destinationAngleY_ = primaryTarget_->rotate.y;
    destinationAngleX_ = CameraConfig::ThirdPerson::DEFAULT_ANGLE_X;
    destinationAngleZ_ = 0.0f;

    offset_ = offsetOrigin_;

    Vector3 offset = CalculateOffset();
    camera_->SetTranslate(interpolatedTargetPos_ + offset);
}

void ThirdPersonController::ProcessInput(float deltaTime) {
    // ターゲット注視モードの場合は手動回転を無効化
    if (enableLookAtTarget_ && secondaryTarget_) {
        isRotating_ = false;
        return;
    }

    if (!input_->RStickInDeadZone()) {
        isRotating_ = true;
        float rotateX = input_->GetRightStick().x;
        // rotateSpeed_ はラジアン/フレーム（約0.00087rad ≈ 0.05度/フレーム）
        destinationAngleY_ += rotateX * rotateSpeed_ *
            CameraConfig::ThirdPerson::GAMEPAD_ROTATE_MULTIPLIER;
    }
    else {
        isRotating_ = false;
    }

    // 右スティック押し込みでターゲット背後にリセット
    if (input_->TriggerButton(GamepadButton::R_Thumbstick)) {
        destinationAngleY_ = primaryTarget_->rotate.y;
    }

    if (input_->PushKey(DIK_LEFT)) {
        destinationAngleY_ -= rotateSpeed_;
    }
    if (input_->PushKey(DIK_RIGHT)) {
        destinationAngleY_ += rotateSpeed_;
    }
}

void ThirdPersonController::UpdateRotation() {
    Vector3 currentRotation = camera_->GetRotate();

    if (enableLookAtTarget_ && secondaryTarget_) {
        Vector3 lookAtRotation = CalculateLookAtRotation();

        destinationAngleX_ = lookAtRotation.x;
        destinationAngleY_ = lookAtRotation.y;
        destinationAngleZ_ = lookAtRotation.z;
    }

    // 目標角度へ最短経路で補間
    float angleY = Vec3::LerpShortAngle(currentRotation.y, destinationAngleY_, rotationLerpSpeed_);
    float angleX = Vec3::LerpShortAngle(currentRotation.x, destinationAngleX_, rotationLerpSpeed_);
    float angleZ = Vec3::LerpShortAngle(currentRotation.z, destinationAngleZ_, rotationLerpSpeed_);

    camera_->SetRotate(Vector3(angleX, angleY, angleZ));
}

void ThirdPersonController::UpdatePosition() {
    offset_.x = Vec3::Lerp(offset_.x, offsetOrigin_.x, offsetLerpSpeed_);
    offset_.y = Vec3::Lerp(offset_.y, offsetOrigin_.y, offsetLerpSpeed_);
    offset_.z = Vec3::Lerp(offset_.z, offsetOrigin_.z, offsetLerpSpeed_);

    interpolatedTargetPos_ = Vec3::Lerp(interpolatedTargetPos_,
        primaryTarget_->translate,
        followSmoothness_);

    Vector3 offset = CalculateOffset();
    camera_->SetTranslate(interpolatedTargetPos_ + offset);
}

Vector3 ThirdPersonController::CalculateOffset() const {
    Vector3 offset = offset_;

    // カメラの回転でオフセットを変換
    Matrix4x4 rotationMatrix = Mat4x4::MakeRotateXYZ(camera_->GetRotate());
    offset = Mat4x4::TransformNormal(rotationMatrix, offset);

    return offset;
}

Vector3 ThirdPersonController::CalculateLookAtRotation() const {
    if (!secondaryTarget_ || !primaryTarget_) {
        return camera_->GetRotate();
    }

    // プライマリからセカンダリへの方向ベクトル
    Vector3 direction = secondaryTarget_->translate - primaryTarget_->translate;

    // 水平角（Y 軸回転、ラジアン）
    float angleY = std::atan2(direction.x, direction.z);

    // 垂直角（X 軸回転、ラジアン）
    float horizontalDistance = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    float angleX = -std::atan2(direction.y, horizontalDistance);

    // 見下ろし角を加算
    angleX += CameraConfig::ThirdPerson::LOOK_DOWN_ANGLE;

    return Vector3(angleX, angleY, 0.0f);
}