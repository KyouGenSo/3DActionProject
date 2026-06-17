#include "BossBorderParticleManager.h"
#include "EmitterManager.h"

BossBorderParticleManager::BossBorderParticleManager(Tako::EmitterManager* emitterManager, float areaSize)
    : emitterManager_(emitterManager), areaSize_(areaSize)
{
}

void BossBorderParticleManager::Update(int bossPhase, const Tako::Vector3& bossPosition)
{
    bool shouldShowBorder = (bossPhase == 2);

    if (shouldShowBorder && !isActive_) {
        SetActive(true);
    }
    else if (!shouldShowBorder && isActive_) {
        SetActive(false);
    }

    if (isActive_) {
        UpdatePositions(bossPosition);
    }
}

void BossBorderParticleManager::SetActive(bool active)
{
    if (isActive_ == active) {
        return;
    }

    emitterManager_->SetEmitterActive("boss_border_left", active);
    emitterManager_->SetEmitterActive("boss_border_right", active);
    emitterManager_->SetEmitterActive("boss_border_front", active);
    emitterManager_->SetEmitterActive("boss_border_back", active);

    isActive_ = active;
}

void BossBorderParticleManager::UpdatePositions(const Tako::Vector3& bossPosition)
{
    // Y を0に固定し、ボスの XZ 周囲4方向に配置
    Tako::Vector3 basePos = Tako::Vector3(bossPosition.x, 0.0f, bossPosition.z);
    float areaSize = areaSize_;

    emitterManager_->SetEmitterPosition("boss_border_left",
        basePos + Tako::Vector3(0.0f, 0.0f, -areaSize));
    emitterManager_->SetEmitterPosition("boss_border_right",
        basePos + Tako::Vector3(0.0f, 0.0f, areaSize));
    emitterManager_->SetEmitterPosition("boss_border_front",
        basePos + Tako::Vector3(-areaSize, 0.0f, 0.0f));
    emitterManager_->SetEmitterPosition("boss_border_back",
        basePos + Tako::Vector3(areaSize, 0.0f, 0.0f));
}
