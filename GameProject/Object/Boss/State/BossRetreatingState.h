#pragma once
#include "BossState.h"
#include "Vector3.h"

class Player;

/// <summary>
/// ボスの離脱状態（外部イベント駆動）
/// 非硬直中の被弾で強制遷移し、プレイヤーから離れるよう後退移動
/// 移動完了後に Normal へ復帰
/// </summary>
class BossRetreatingState : public BossState {
public:
	BossRetreatingState();
	~BossRetreatingState() override = default;

	void Enter(Boss* boss) override;
	void Update(Boss* boss, float deltaTime) override;
	void Exit(Boss* boss) override;

private:
	/// <summary>
	/// 離脱移動の更新
	/// </summary>
	void UpdateRetreatMovement(Boss* boss);

	/// <summary>
	/// エリア内に収まる位置を計算
	/// </summary>
	Tako::Vector3 ClampToArea(const Tako::Vector3& position, Boss* boss);

	/// <summary>
	/// 最適な離脱方向を探索（壁回避）
	/// </summary>
	Tako::Vector3 FindBestRetreatDirection(const Tako::Vector3& primaryDirection, float retreatDistance, Boss* boss);

	/// <summary>
	/// 指定方向での移動距離を評価
	/// </summary>
	float EvaluateDirection(const Tako::Vector3& direction, float retreatDistance, Boss* boss);

	// 定数
	static constexpr float kDirectionEpsilon = 0.01f;
	static constexpr float kArrivalThreshold = 0.5f;
	static constexpr float kEasingCoeffA = 3.0f;
	static constexpr float kEasingCoeffB = 2.0f;
	static constexpr float kMinRetreatDistance = 10.0f;

	// パラメータ
	float retreatSpeed_ = 250.0f;
	float targetDistance_ = 60.0f;
	Tako::Vector3 startPosition_;
	Tako::Vector3 targetPosition_;
	float elapsedTime_ = 0.0f;
	float retreatDuration_ = 0.0f;
};
