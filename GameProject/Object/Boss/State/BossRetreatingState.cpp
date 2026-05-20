#include "BossRetreatingState.h"
#include "../Boss.h"
#include "BossStateMachine.h"
#include "../../Player/Player.h"
#include "BehaviorTree.h"
#include "BTBlackboard.h"

using namespace Tako;

BossRetreatingState::BossRetreatingState()
    : BossState("Retreating")
    , executor_(BossRetreatExecutor::Parameters{ 250.0f, 60.0f })
{
}

void BossRetreatingState::Enter(Boss* boss)
{
    Player* player = boss->GetBehaviorTree()->GetBlackboard()->GetPtr<Player>("player");
    executor_.Begin(boss, player);
}

void BossRetreatingState::Update(Boss* boss, float deltaTime)
{
    // 即時完了 / 既に到達済み: 1 フレームで Normal に復帰
    if (executor_.IsFinished(boss)) {
        executor_.SnapToTarget(boss);
        boss->GetStateMachine()->ChangeState("Normal");
        return;
    }

    executor_.Tick(boss, deltaTime);

    if (executor_.IsFinished(boss)) {
        executor_.SnapToTarget(boss);
        boss->GetStateMachine()->ChangeState("Normal");
    }
}

void BossRetreatingState::Exit(Boss* boss)
{
    (void)boss;
    executor_.Reset();
}
