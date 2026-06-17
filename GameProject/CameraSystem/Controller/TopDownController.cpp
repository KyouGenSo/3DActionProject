#include "TopDownController.h"
#include "Vec3Func.h"
#include <algorithm>
#include <numeric>

using namespace Tako;

TopDownController::TopDownController() {
    currentHeight_ = CameraConfig::TopDown::INITIAL_HEIGHT;
    currentBackOffset_ = CameraConfig::TopDown::INITIAL_BACK_OFFSET;
}

void TopDownController::Update(float deltaTime) {
    if (!isActive_ || !camera_ || !primaryTarget_) {
        return;
    }

    if (camera_) {
        camera_->SetFovY(standardFov_);
    }

    UpdateCameraPosition();
}

void TopDownController::Activate() {
    isActive_ = true;

    if (camera_) {
        camera_->SetFovY(standardFov_);
    }

    if (primaryTarget_) {
        Reset();
    }
}

void TopDownController::Reset() {
    if (!primaryTarget_ || !camera_) {
        return;
    }

    interpolatedTargetPos_ = CalculateFocusPoint();

    UpdateCameraPosition();
}

Vector3 TopDownController::CalculateFocusPoint() const {
    if (!primaryTarget_) {
        return Vector3{};
    }

    std::vector<Vector3> allPositions;
    allPositions.push_back(primaryTarget_->translate);

    for (const auto* target : additionalTargets_) {
        if (target) {
            allPositions.push_back(target->translate);
        }
    }

    // 全ターゲットの平均位置を中心とする
    if (!allPositions.empty()) {
        Vector3 sum = std::accumulate(allPositions.begin(), allPositions.end(),
            Vector3{ 0.0f, 0.0f, 0.0f },
            [](const Vector3& a, const Vector3& b) {
                return Vec3::Add(a, b);
            });
        return Vec3::Multiply(sum, 1.0f / static_cast<float>(allPositions.size()));
    }

    return primaryTarget_->translate;
}

float TopDownController::CalculateMaxTargetDistance() const {
    if (!primaryTarget_ || additionalTargets_.empty()) {
        return 0.0f;
    }

    float maxDistance = 0.0f;

    // プライマリと各追加ターゲット間の距離
    for (const auto* target : additionalTargets_) {
        if (target) {
            Vector3 diff = Vec3::Subtract(primaryTarget_->translate, target->translate);
            float distance = static_cast<float>(Vec3::Length(diff));
            maxDistance = std::max(maxDistance, distance);
        }
    }

    // 追加ターゲット同士の距離も考慮
    for (size_t i = 0; i < additionalTargets_.size(); ++i) {
        for (size_t j = i + 1; j < additionalTargets_.size(); ++j) {
            if (additionalTargets_[i] && additionalTargets_[j]) {
                Vector3 diff = Vec3::Subtract(additionalTargets_[i]->translate,
                    additionalTargets_[j]->translate);
                float distance = static_cast<float>(Vec3::Length(diff));
                maxDistance = std::max(maxDistance, distance);
            }
        }
    }

    return maxDistance;
}

void TopDownController::CalculateCameraParameters(float targetDistance,
    float& outHeight,
    float& outBackOffset) const {
    // ターゲット間距離が大きいほど高く・後方に引く
    outHeight = baseHeight_ + targetDistance * heightMultiplier_;
    outHeight = std::clamp(outHeight, minHeight_, maxHeight_);

    outBackOffset = baseBackOffset_ - targetDistance * backOffsetMultiplier_;
    outBackOffset = std::clamp(outBackOffset, minBackOffset_, maxBackOffset_);
}

void TopDownController::UpdateCameraPosition() {
    Vector3 focusPoint = CalculateFocusPoint();

    interpolatedTargetPos_ = Vec3::Lerp(interpolatedTargetPos_, focusPoint, followSmoothness_);

    float targetDistance = CalculateMaxTargetDistance();

    float targetHeight = baseHeight_;
    float targetBackOffset = baseBackOffset_;

    if (targetDistance > 0.0f) {
        CalculateCameraParameters(targetDistance, targetHeight, targetBackOffset);
    }

    currentHeight_ = Vec3::Lerp(currentHeight_, targetHeight, followSmoothness_);
    currentBackOffset_ = Vec3::Lerp(currentBackOffset_, targetBackOffset, followSmoothness_);

    Vector3 cameraPos = interpolatedTargetPos_;
    cameraPos.y = currentHeight_;
    cameraPos.z += currentBackOffset_;

    camera_->SetTranslate(cameraPos);

    // 俯瞰角度に固定
    camera_->SetRotate(Vector3(cameraAngleX_, 0.0f, 0.0f));
}