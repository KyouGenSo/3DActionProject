#include "BossDeadState.h"
#include "../Boss.h"

BossDeadState::BossDeadState()
	: BossState("Dead")
{
}

void BossDeadState::Enter(Boss* boss)
{
	(void)boss;
	// 死亡演出なし
}

void BossDeadState::Update(Boss* boss, float deltaTime)
{
	(void)boss;
	(void)deltaTime;
	// シーン遷移は GameScene 側で管理
}

void BossDeadState::Exit(Boss* boss)
{
	(void)boss;
	// 復活（デバッグ用）時のクリーンアップ
}
