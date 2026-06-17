#pragma once
#include "PlayerState.h"

/// <summary>
/// 待機状態クラス
/// </summary>
class IdleState : public PlayerState
{
public:
	IdleState() : PlayerState("Idle") {}

	void Enter(Player* player) override;

	void Update(Player* player, float deltaTime) override;

	void Exit(Player* player) override;

	void HandleInput(Player* player) override;

	void DrawImGui(Player* player) override;

private:
	float idleTime_ = 0.0f;
};