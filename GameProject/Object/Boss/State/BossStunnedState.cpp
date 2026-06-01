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
	// タイマーリセット
	elapsedTime_ = 0.0f;
	flashTimer_ = 0.0f;
	knockbackTimer_ = 0.0f;
	knockbackComplete_ = false;
	knockbackWasSkipped_ = false;
	pendingKnockbackEnable_ = false;

	startPosition_ = boss->GetTransform().translate;

	// ノックバック情報をボスから取得
	Vector3 knockbackDir = boss->GetPendingStunDirection();
	bool withKnockback = boss->GetPendingStunWithKnockback();

	// ノックバック無効時は移動をスキップ（その場でスタン）
	if (!withKnockback) {
		targetPosition_ = startPosition_;
		knockbackComplete_ = true;
		knockbackWasSkipped_ = true;
		UpdateFlash(boss);
		return;
	}

	// 方向が有効かチェック
	if (knockbackDir.Length() < kDirectionEpsilon) {
		// フォールバック: ボスの後方に下がる
		float angle = boss->GetTransform().rotate.y;
		knockbackDir = Vector3(-sinf(angle), 0.0f, -cosf(angle));
	}

	knockbackDir.y = 0.0f;
	knockbackDir = knockbackDir.Normalize();

	// 目標位置を計算
	targetPosition_ = startPosition_ + knockbackDir * knockbackDistance_;
	targetPosition_ = BossMovement::ClampToBounds(targetPosition_, BossMovement::CalcStageBounds());

	// 初回フラッシュを即座に開始
	UpdateFlash(boss);
}

void BossStunnedState::Update(Boss* boss, float deltaTime)
{
	elapsedTime_ += deltaTime;

	// 中途ノックバック有効化（4コンボ目ヒット）
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

	// ノックバック処理
	if (!knockbackComplete_) {
		knockbackTimer_ += deltaTime;
		UpdateKnockback(boss, deltaTime);
	}

	// 色点滅処理（スタン中継続）
	flashTimer_ += deltaTime;
	if (flashTimer_ >= flashInterval_) {
		flashTimer_ = 0.0f;
		UpdateFlash(boss);
	}

	// スタン終了判定
	// フェーズ移行スタン中はタイムアウトしない（被弾で PhaseTransitionStunState に遷移済み）
	if (elapsedTime_ >= stunDuration_ && knockbackComplete_) {
		boss->GetStateMachine()->ChangeState("Normal");
	}
}

void BossStunnedState::Exit(Boss* boss)
{
	// 状態リセット
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

	// イージング（加速→減速）: smoothstep
	t = Ease::SmoothStep(t);

	Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
	boss->SetTranslate(newPosition);
}

void BossStunnedState::UpdateFlash(Boss* boss)
{
	boss->StartStunFlash(stunFlashColor_, flashDuration_);
}

