#include "BossPhaseManager.h"

void BossPhaseManager::Initialize(float maxHp, float phase2Threshold, float phase2InitialHp)
{
    maxHp_ = maxHp;
    phase2Threshold_ = phase2Threshold;
    phase2InitialHp_ = phase2InitialHp;
    Reset();
}

void BossPhaseManager::Update(float currentHp)
{
    if (currentHp <= phase2Threshold_ && phase_ == 1) {
        isReadyToChangePhase_ = true;
    }

    if (currentHp <= 0.0f && life_ > 0) {
        life_--;

        if (life_ == 0) {
            isDead_ = true;
        }
    }
}

bool BossPhaseManager::ConsumePhaseChangeRequest()
{
    if (isReadyToChangePhase_) {
        isReadyToChangePhase_ = false;
        phase_ = 2;
        return true;
    }
    return false;
}

void BossPhaseManager::Reset()
{
    phase_ = 1;
    life_ = 1;
    isReadyToChangePhase_ = false;
    isDead_ = false;
}
