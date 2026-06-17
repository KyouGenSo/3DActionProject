#pragma once
#include "BossState.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// ノックバック移動と色点滅を行い、一定時間後に Normal へ復帰するスタン状態
/// </summary>
class BossStunnedState : public BossState {
private: //定数
	static constexpr float kDirectionEpsilon = 0.01f;

public: //メンバー関数
	BossStunnedState();
	~BossStunnedState() override = default;

	void Enter(Boss* boss) override;
	void Update(Boss* boss, float deltaTime) override;
	void Exit(Boss* boss) override;

	/// <summary>
	/// スタン中（4コンボ目ヒット時）にノックバックを有効化。次の Update で開始位置・目標位置を再計算する
	/// </summary>
	/// <param name="direction">ノックバック方向。長さが kDirectionEpsilon 未満ならボス後方へフォールバック</param>
	void EnableKnockback(const Tako::Vector3& direction);

private: //非公開関数
	void UpdateKnockback(Boss* boss, float deltaTime);

	void UpdateFlash(Boss* boss);

private: //メンバー変数
	float         stunDuration_      = 1.5f;
	float         knockbackDistance_ = 8.0f;
	float         knockbackDuration_ = 0.3f;
	float         flashInterval_     = 0.05f;
	float         flashDuration_     = 0.03f;
	Tako::Vector4 stunFlashColor_    = { 1.0f, 1.0f, 0.0f, 1.0f };

	Tako::Vector3 startPosition_;
	Tako::Vector3 targetPosition_;
	float         elapsedTime_         = 0.0f;
	float         flashTimer_          = 0.0f;
	float         knockbackTimer_      = 0.0f;
	bool          knockbackComplete_   = false;
	bool          knockbackWasSkipped_ = false;

	//スタン中に後から有効化されるノックバック
	bool          pendingKnockbackEnable_    = false;
	Tako::Vector3 pendingKnockbackDirection_;
};
