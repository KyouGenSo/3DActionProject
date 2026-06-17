#include "BossPhaseTransitionStunState.h"
#include "../Boss.h"

BossPhaseTransitionStunState::BossPhaseTransitionStunState()
	: BossState("PhaseTransitionStun")
{
}

void BossPhaseTransitionStunState::Enter(Boss* boss)
{
	boss->SetCanAttackSignEmitterActive(true);
	boss->SetCanAttackSignEmitterPosition(boss->GetTransform().translate);

	flashTimer_ = 0.0f;
	boss->StartStunFlash(stunFlashColor_, flashDuration_);
}

void BossPhaseTransitionStunState::Update(Boss* boss, float deltaTime)
{
	// パーティクルをボスに追従
	boss->SetCanAttackSignEmitterPosition(boss->GetTransform().translate);

	flashTimer_ += deltaTime;
	if (flashTimer_ >= flashInterval_) {
		flashTimer_ = 0.0f;
		boss->StartStunFlash(stunFlashColor_, flashDuration_);
	}

	// フェーズ2移行は Boss::OnMeleeAttackHit() が CompletePhaseTransition() で行う
}

void BossPhaseTransitionStunState::Exit(Boss* boss)
{
	boss->SetCanAttackSignEmitterActive(false);
	flashTimer_ = 0.0f;
}
