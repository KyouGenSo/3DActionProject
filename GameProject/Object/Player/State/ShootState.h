#pragma once
#include "PlayerState.h"
#include "Vector3.h"

/// <summary>
/// 射撃状態クラス（照準と発射）
/// </summary>
class ShootState : public PlayerState
{
public:
	ShootState() : PlayerState("Shoot") {}

	void Enter(Player* player) override;

	void Update(Player* player, float deltaTime) override;

	void Exit(Player* player) override;

	void HandleInput(Player* player) override;

	void DrawImGui(Player* player) override;

	float GetFireRate() const { return fireRate_; }
	float GetFireRateTimer() const { return fireRateTimer_; }
	const Tako::Vector3& GetAimDirection() const { return aimDirection_; }

	void SetFireRate(float rate) { fireRate_ = rate; }

private:
	float fireRate_ = 0.2f;              ///< 秒間隔
	float fireRateTimer_ = 0.0f;
	float moveSpeedMultiplier_ = 0.5f;   ///< 射撃中の移動速度倍率
    Tako::Vector3 aimDirection_;

	/// <summary>
	/// スティック入力とカメラ向きから照準方向を算出し、プレイヤーをその方向へ向ける
	/// </summary>
	/// <param name="player">対象プレイヤー（向きを更新する）</param>
	void CalculateAimDirection(Player* player);

	/// <summary>
	/// 現在の照準方向へ弾の生成リクエストを発行する
	/// </summary>
	/// <param name="player">発射元プレイヤー</param>
	void Fire(Player* player);
};