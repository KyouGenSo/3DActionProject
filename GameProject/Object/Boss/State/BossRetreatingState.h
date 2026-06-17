#pragma once
#include "BossState.h"
#include "../Movement/BossRetreatExecutor.h"

/// <summary>
/// 非硬直中の被弾で遷移する離脱状態。移動は BossRetreatExecutor に委譲し、完了後 Normal へ復帰
/// </summary>
class BossRetreatingState : public BossState {
public:
    BossRetreatingState();
    ~BossRetreatingState() override = default;

    void Enter(Boss* boss) override;
    void Update(Boss* boss, float deltaTime) override;
    void Exit(Boss* boss) override;

private:
    BossRetreatExecutor executor_;
};
