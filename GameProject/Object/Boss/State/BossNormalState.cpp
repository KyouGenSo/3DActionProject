#include "BossNormalState.h"
#include "../Boss.h"
#include "BehaviorTree.h"

BossNormalState::BossNormalState()
    : BossState("Normal")
{
}

void BossNormalState::Enter(Boss* boss)
{
    Tako::BehaviorTree* bt = boss->GetBehaviorTree();
    if (bt) {
        bt->Reset();
    }
}

void BossNormalState::Update(Boss* boss, float deltaTime)
{
    Tako::BehaviorTree* bt = boss->GetBehaviorTree();
    if (bt) {
        bt->Tick(deltaTime);
    }
}

void BossNormalState::Exit(Boss* boss)
{
    // 攻撃中断時のエフェクト等を消す
    boss->ResetActionState();

    // Running 中のアクションを中断
    Tako::BehaviorTree* bt = boss->GetBehaviorTree();
    if (bt) {
        bt->Reset();
    }
}
