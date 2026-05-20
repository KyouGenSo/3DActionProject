#include "BTBossRetreat.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"

using namespace Tako;

BTBossRetreat::BTBossRetreat() {
    name_ = "BossRetreat";
}

Tako::BTNodeStatus BTBossRetreat::Execute(Tako::BTBlackboard* blackboard) {
    boss_ = blackboard->GetPtr<Boss>("boss");
    if (!boss_) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    Player* player = blackboard->GetPtr<Player>("player");
    if (!player) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    const float deltaTime = blackboard->GetDeltaTime();

    if (isFirstExecute_) {
        executor_.Begin(boss_, player);
        isFirstExecute_ = false;

        // 既に目標距離以上離れている等で即時完了するケース
        if (executor_.IsFinished(boss_)) {
            isFirstExecute_ = true;
            executor_.Reset();
            status_ = Tako::BTNodeStatus::Success;
            return Tako::BTNodeStatus::Success;
        }
    }

    executor_.Tick(boss_, deltaTime);

    if (executor_.IsFinished(boss_)) {
        executor_.SnapToTarget(boss_);
        isFirstExecute_ = true;
        executor_.Reset();
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    status_ = Tako::BTNodeStatus::Running;
    return Tako::BTNodeStatus::Running;
}

void BTBossRetreat::Reset() {
    Tako::BTNode::Reset();
    executor_.Reset();
    isFirstExecute_ = true;
}
