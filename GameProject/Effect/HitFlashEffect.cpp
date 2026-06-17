#include "HitFlashEffect.h"
#include "Object3d.h"

using namespace Tako;

void HitFlashEffect::Start(const Vector4& flashColor, float duration)
{
    isActive_ = true;
    timer_ = 0.0f;
    duration_ = duration;
    flashColor_ = flashColor;
}

void HitFlashEffect::Update(float deltaTime, Object3d* target, const Vector4& originalColor)
{
    if (!isActive_ || !target) return;

    timer_ += deltaTime;

    if (timer_ <= duration_) {
        target->SetMaterialColor(flashColor_);
    }
    else {
        isActive_ = false;
        target->SetMaterialColor(originalColor);
    }
}
