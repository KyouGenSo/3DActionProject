#include "ShakeEffect.h"
#include "RandomEngine.h"
#include "Vec3Func.h"

using namespace Tako;

void ShakeEffect::Start(float intensity, float duration)
{
    isActive_ = true;
    timer_ = 0.0f;

    // 0以下の場合はデフォルト値を使用
    intensity_ = (intensity > 0.0f) ? intensity : defaultIntensity_;
    duration_ = (duration > 0.0f) ? duration : defaultDuration_;
}

void ShakeEffect::Update(float deltaTime)
{
    if (!isActive_) {
        offset_ = { 0.0f, 0.0f, 0.0f };
        return;
    }

    timer_ += deltaTime;

    if (timer_ >= duration_) {
        Stop();
        return;
    }

    // 減衰係数（1.0→0.0へ線形減衰）
    float decay = Vec3::Lerp(1.0f, 0.0f, timer_ / duration_);

    // ランダムオフセット生成
    RandomEngine* rng = RandomEngine::GetInstance();
    offset_.x = rng->GetFloat(-intensity_, intensity_) * decay;
    offset_.y = rng->GetFloat(-intensity_, intensity_) * decay;
    offset_.z = rng->GetFloat(-intensity_, intensity_) * decay;
}

void ShakeEffect::Stop()
{
    isActive_ = false;
    timer_ = 0.0f;
    offset_ = { 0.0f, 0.0f, 0.0f };
}
