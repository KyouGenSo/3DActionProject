#include "BulletSignEffect.h"
#include "../Object/Boss/Boss.h"
#include "Vec3Func.h"
#include <cmath>

using namespace Tako;

void BulletSignEffect::Start(Boss* boss, float duration) {
    duration_ = duration;
    elapsedTime_ = 0.0f;
    isActive_ = true;

    Vector3 emitterPos = CalculateEmitterPosition(boss);
    boss->SetBulletSignEmitterPosition(emitterPos);
    boss->SetBulletSignEmitterScaleRangeX(kScaleMin);
    boss->SetBulletSignEmitterActive(true);
}

void BulletSignEffect::Update(Boss* boss, float deltaTime) {
    if (!isActive_) return;

    elapsedTime_ += deltaTime;

    float t = elapsedTime_ / duration_;
    if (t > 1.0f) t = 1.0f;
    float scale = Vec3::Lerp(kScaleMin, kScaleMax, t);

    boss->SetBulletSignEmitterScaleRangeX(scale);
    boss->SetBulletSignEmitterPosition(CalculateEmitterPosition(boss));
}

void BulletSignEffect::End(Boss* boss) {
    if (!isActive_) return;

    boss->SetBulletSignEmitterActive(false);
    isActive_ = false;
    elapsedTime_ = 0.0f;
}

Vector3 BulletSignEffect::CalculateEmitterPosition(Boss* boss) {
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 bossRotate = boss->GetTransform().rotate;
    Vector3 forward = Vector3(sinf(bossRotate.y), 0.0f, cosf(bossRotate.y));
    return bossPos + forward * kForwardDistance;
}
