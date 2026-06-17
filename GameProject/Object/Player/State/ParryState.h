#pragma once
#include "PlayerState.h"

/// <summary>
/// パリィ状態クラス（防御・反撃タイミング管理）
/// </summary>
class ParryState : public PlayerState
{
public:
	ParryState() : PlayerState("Parry") {}

	void Enter(Player* player) override;

	void Update(Player* player, float deltaTime) override;

	void Exit(Player* player) override;

	void HandleInput(Player* player) override;

	/// <summary>
	/// パリィ成功時に攻撃へ遷移
	/// </summary>
	void OnParrySuccess(Player* player);

	void DrawImGui(Player* player) override;

	float GetParryTimer() const { return parryTimer_; }
	float GetParryDuration() const { return parryDuration_; }

	void SetParryDuration(float duration) { parryDuration_ = duration; }

private:
	float parryTimer_          = 0.0f;
	float parryDuration_       = 0.5f;          ///< 秒
};