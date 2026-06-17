#include "BossStunnedState.h"
#include "../Boss.h"
#include "BossStateMachine.h"
#include "../Movement/BossAreaBounds.h"
#include "EaseFunc.h"

#include <algorithm>
#include <cmath>

using namespace Tako;

BossStunnedState::BossStunnedState()
	: BossState("Stunned")
{
}

void BossStunnedState::Enter(Boss* boss)
{
	elapsedTime_ = 0.0f;
	flashTimer_ = 0.0f;
	knockbackTimer_ = 0.0f;
	knockbackComplete_ = false;
	knockbackWasSkipped_ = false;
	pendingKnockbackEnable_ = false;

	startPosition_ = boss->GetTransform().translate;

	Vector3 knockbackDir = boss->GetPendingStunDirection();
	bool withKnockback = boss->GetPendingStunWithKnockback();

	// ノックバック無効時は移動せずその場でスタン
	if (!withKnockback) {
		targetPosition_ = startPosition_;
		knockbackComplete_ = true;
		knockbackWasSkipped_ = true;
		UpdateFlash(boss);
		return;
	}

	// 方向無効時はボス後方へフォールバック
	if (knockbackDir.Length() < kDirectionEpsilon) {
		float angle = boss->GetTransform().rotate.y;
		knockbackDir = Vector3(-sinf(angle), 0.0f, -cosf(angle));
	}

	knockbackDir.y = 0.0f;
	knockbackDir = knockbackDir.Normalize();

	targetPosition_ = startPosition_ + knockbackDir * knockbackDistance_;
	targetPosition_ = BossMovement::ClampToBounds(targetPosition_, BossMovement::CalcStageBounds());

	UpdateFlash(boss);
}

void BossStunnedState::Update(Boss* boss, float deltaTime)
{
	elapsedTime_ += deltaTime;

	// 4コンボ目ヒットによる中途ノックバック有効化
	if (pendingKnockbackEnable_) {
		pendingKnockbackEnable_ = false;
		knockbackWasSkipped_ = false;
		knockbackComplete_ = false;
		knockbackTimer_ = 0.0f;
		startPosition_ = boss->GetTransform().translate;

		Vector3 knockbackDir = pendingKnockbackDirection_;
		if (knockbackDir.Length() < kDirectionEpsilon) {
			float angle = boss->GetTransform().rotate.y;
			knockbackDir = Vector3(-sinf(angle), 0.0f, -cosf(angle));
		}
		knockbackDir.y = 0.0f;
		knockbackDir = knockbackDir.Normalize();

		targetPosition_ = startPosition_ + knockbackDir * knockbackDistance_;
		targetPosition_ = BossMovement::ClampToBounds(targetPosition_, BossMovement::CalcStageBounds());
	}

	if (!knockbackComplete_) {
		knockbackTimer_ += deltaTime;
		UpdateKnockback(boss, deltaTime);
	}

	flashTimer_ += deltaTime;
	if (flashTimer_ >= flashInterval_) {
		flashTimer_ = 0.0f;
		UpdateFlash(boss);
	}

	// フェーズ移行スタン中はタイムアウトしない（被弾で PhaseTransitionStunState へ遷移済み）
	if (elapsedTime_ >= stunDuration_ && knockbackComplete_) {
		boss->GetStateMachine()->ChangeState("Normal");
	}
}

void BossStunnedState::Exit(Boss* boss)
{
	elapsedTime_ = 0.0f;
	flashTimer_ = 0.0f;
	knockbackTimer_ = 0.0f;
	knockbackComplete_ = false;
	knockbackWasSkipped_ = false;
	pendingKnockbackEnable_ = false;
}

void BossStunnedState::EnableKnockback(const Vector3& direction)
{
	pendingKnockbackEnable_ = true;
	pendingKnockbackDirection_ = direction;
}

void BossStunnedState::UpdateKnockback(Boss* boss, float deltaTime)
{
	(void)deltaTime;

	if (knockbackTimer_ >= knockbackDuration_) {
		knockbackComplete_ = true;
		boss->SetTranslate(targetPosition_);
		return;
	}

	float t = knockbackTimer_ / knockbackDuration_;
	t = std::clamp(t, 0.0f, 1.0f);

	t = Ease::SmoothStep(t);

	Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
	boss->SetTranslate(newPosition);
}

void BossStunnedState::UpdateFlash(Boss* boss)
{
	boss->StartStunFlash(stunFlashColor_, flashDuration_);
}

