#pragma once
#include "BossState.h"

/// <summary>
/// 通常状態。BehaviorTree が行動を駆動する
/// </summary>
class BossNormalState : public BossState {
public: //メンバー関数
	BossNormalState();
	~BossNormalState() override = default;

	void Enter(Boss* boss) override;
	void Update(Boss* boss, float deltaTime) override;
	void Exit(Boss* boss) override;
};
