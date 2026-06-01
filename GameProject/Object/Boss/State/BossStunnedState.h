#pragma once
#include "BossState.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// ボスのスタン状態
/// ノックバック移動と色点滅を実行し、一定時間後に Normal へ復帰
/// BTBossStun のロジックをステートマシン用に移植
/// </summary>
class BossStunnedState : public BossState {
public:
	BossStunnedState();
	~BossStunnedState() override = default;

	void Enter(Boss* boss) override;
	void Update(Boss* boss, float deltaTime) override;
	void Exit(Boss* boss) override;

	/// <summary>
	/// スタン中にノックバックを有効化する（4コンボ目ヒット時）
	/// </summary>
	/// <param name="direction">ノックバック方向</param>
	void EnableKnockback(const Tako::Vector3& direction);

private:
	/// <summary>
	/// ノックバック移動の更新
	/// </summary>
	void UpdateKnockback(Boss* boss, float deltaTime);

	/// <summary>
	/// 色点滅の更新
	/// </summary>
	void UpdateFlash(Boss* boss);

	// 定数
	static constexpr float kDirectionEpsilon = 0.01f;

	// パラメータ
	float stunDuration_ = 1.5f;
	float knockbackDistance_ = 8.0f;
	float knockbackDuration_ = 0.3f;
	float flashInterval_ = 0.05f;
	float flashDuration_ = 0.03f;
	Tako::Vector4 stunFlashColor_ = { 1.0f, 1.0f, 0.0f, 1.0f };

	// 状態管理
	Tako::Vector3 startPosition_;
	Tako::Vector3 targetPosition_;
	float elapsedTime_ = 0.0f;
	float flashTimer_ = 0.0f;
	float knockbackTimer_ = 0.0f;
	bool knockbackComplete_ = false;
	bool knockbackWasSkipped_ = false;

	// 中途ノックバック有効化用
	bool pendingKnockbackEnable_ = false;
	Tako::Vector3 pendingKnockbackDirection_;
};
