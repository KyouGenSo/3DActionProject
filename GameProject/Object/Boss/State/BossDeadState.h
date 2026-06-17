#pragma once
#include "BossState.h"

/// <summary>
/// HP=0 で遷移する死亡状態（復帰なし）
/// </summary>
class BossDeadState : public BossState {
public:
	BossDeadState();
	~BossDeadState() override = default;

	void Enter(Boss* boss) override;
	void Update(Boss* boss, float deltaTime) override;
	void Exit(Boss* boss) override;
};
