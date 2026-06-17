#pragma once
#include "BossState.h"
#include "Vector4.h"

/// <summary>
/// HP 閾値で遷移し近接被弾を待つフェーズ移行スタン。被弾でフェーズ2へ移行し Normal へ復帰
/// </summary>
class BossPhaseTransitionStunState : public BossState {
public: //メンバー関数
	BossPhaseTransitionStunState();
	~BossPhaseTransitionStunState() override = default;

	void Enter(Boss* boss) override;
	void Update(Boss* boss, float deltaTime) override;
	void Exit(Boss* boss) override;

private: //メンバー変数
	//フラッシュパラメータ（BossStunnedState と同一値）
	float         flashInterval_  = 0.05f;
	float         flashDuration_  = 0.03f;
	Tako::Vector4 stunFlashColor_ = { 1.0f, 1.0f, 0.0f, 1.0f };

	float flashTimer_ = 0.0f;
};
