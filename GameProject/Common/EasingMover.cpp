#include "EasingMover.h"
#include "EaseFunc.h"
#include <algorithm>

using namespace Tako;

void EasingMover::Initialize(const Vector3& start, const Vector3& target, float duration)
{
    startPosition_ = start;
    targetPosition_ = target;
    duration_ = duration;
    elapsedTime_ = 0.0f;
    isInitialized_ = true;
}

void EasingMover::InitializeWithSpeed(const Vector3& start, const Vector3& target, float speed)
{
    startPosition_ = start;
    targetPosition_ = target;

    Vector3 diff = target - start;
    diff.y = 0.0f;  // Y 軸は無視して水平距離で計算
    float distance = diff.Length();

    duration_ = (speed > 0.0f) ? distance / speed : 0.0f;
    elapsedTime_ = 0.0f;
    isInitialized_ = true;
}

Vector3 EasingMover::Update(float deltaTime)
{
    if (!isInitialized_ || duration_ <= 0.0f) {
        return targetPosition_;
    }

    elapsedTime_ += deltaTime;
    float t = std::clamp(elapsedTime_ / duration_, 0.0f, 1.0f);

    float easedT = ApplyEasing(t);

    return Vector3::Lerp(startPosition_, targetPosition_, easedT);
}

void EasingMover::Reset()
{
    isInitialized_ = false;
    elapsedTime_ = 0.0f;
    duration_ = 0.0f;
}

bool EasingMover::HasReached() const
{
    if (!isInitialized_) return true;
    if (duration_ <= 0.0f) return true;
    return elapsedTime_ >= duration_;
}

bool EasingMover::IsInitialized() const
{
    return isInitialized_;
}

float EasingMover::GetProgress() const
{
    if (duration_ <= 0.0f) return 1.0f;
    return std::clamp(elapsedTime_ / duration_, 0.0f, 1.0f);
}

void EasingMover::SetEasingType(EasingType type)
{
    easingType_ = type;
}

const Vector3& EasingMover::GetTargetPosition() const
{
    return targetPosition_;
}

const Vector3& EasingMover::GetStartPosition() const
{
    return startPosition_;
}

float EasingMover::ApplyEasing(float t) const
{
    switch (easingType_) {
    case EasingType::Linear:
        return Ease::Linear(t);
    case EasingType::SmoothStep:
        return Ease::SmoothStep(t);
    case EasingType::EaseOut:
        return Ease::OutQuad(t);
    default:
        return t;
    }
}
