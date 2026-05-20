#pragma once
#include "BossState.h"
#include "../Movement/BossRetreatExecutor.h"

/// <summary>
/// ボスの離脱状態 (外部イベント駆動)
/// 非硬直中の被弾で強制遷移し、プレイヤーから離れるよう後退移動。
/// 移動完了後に Normal へ復帰する。
/// 実装本体は BossRetreatExecutor に委譲し、本クラスは State 遷移制御のみを担う。
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
