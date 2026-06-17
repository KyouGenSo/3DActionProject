#include "DashEffectManager.h"
#include "EmitterManager.h"
#include "GlobalVariables.h"
#include "Vec3Func.h"
#include <cmath>

DashEffectManager::DashEffectManager(Tako::EmitterManager* emitterManager)
    : emitterManager_(emitterManager)
{
}

void DashEffectManager::Update(float deltaTime, const Tako::Vector3& playerPosition, bool isDashing)
{
    if (isDashing && !previousIsDashing_) {
        emitterManager_->SetEmitterActive(params_.emitterName, true);
        isActive_ = true;
        emitterPosition_ = playerPosition;
    }

    // ダッシュ終了後も追いつくまで補間を継続する
    if (isActive_) {
        // GlobalVariables の値があれば params より優先
        float lerpSpeed = Tako::GlobalVariables::GetInstance()->GetValueFloat("DashEffect", "LerpSpeed");
        if (lerpSpeed <= 0.0f) {
            lerpSpeed = params_.lerpSpeed;
        }

        // 指数減衰 t = 1 - e^(-speed * dt)。FPS非依存で同じ見た目になる
        float t = 1.0f - std::exp(-lerpSpeed * deltaTime);

        emitterPosition_ = Tako::Vec3::Lerp(emitterPosition_, playerPosition, t);

        emitterManager_->SetEmitterPosition(params_.emitterName, emitterPosition_);

        if (!isDashing) {
            float distanceSquared = playerPosition.DistanceSquared(emitterPosition_);

            if (distanceSquared < params_.stopThreshold * params_.stopThreshold) {
                emitterManager_->SetEmitterActive(params_.emitterName, false);
                isActive_ = false;
            }
        }
    }

    previousIsDashing_ = isDashing;
}

void DashEffectManager::InitializePosition(const Tako::Vector3& position)
{
    emitterPosition_ = position;
}
