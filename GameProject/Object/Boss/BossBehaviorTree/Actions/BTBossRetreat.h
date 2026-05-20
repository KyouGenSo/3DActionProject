#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"
#include "../../Movement/BossRetreatExecutor.h"

class Boss;

/// <summary>
/// ボスの離脱アクションノード
/// プレイヤーを向いたまま後方にイージング移動で離れる。
/// 実装本体は BossRetreatExecutor に委譲し、本クラスは BT ノードとしての制御 (初回検知、
/// BTNodeStatus 返却、JSON プリセット永続化) のみを担う。
/// </summary>
class BTBossRetreat : public Tako::BTNode {
public:
    BTBossRetreat();
    virtual ~BTBossRetreat() = default;

    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;
    void Reset() override;

    // パラメータアクセス (Executor へ委譲)
    float GetRetreatSpeed()   const { return executor_.GetParameters().retreatSpeed; }
    float GetTargetDistance() const { return executor_.GetParameters().targetDistance; }
    void  SetRetreatSpeed(float speed) {
        auto p = executor_.GetParameters();
        p.retreatSpeed = speed;
        executor_.SetParameters(p);
    }
    void SetTargetDistance(float distance) {
        auto p = executor_.GetParameters();
        p.targetDistance = distance;
        executor_.SetParameters(p);
    }

    void ApplyParameters(const nlohmann::json& params) override {
        executor_.ApplyJson(params);
    }

    nlohmann::json ExtractParameters() const override {
        return executor_.ToJson();
    }

#ifdef _DEBUG
    bool DrawImGui() override {
        return executor_.DrawImGui("##retreat");
    }
#endif

private:
    Boss*                boss_           = nullptr;  ///< Blackboard から取得したボス参照
    BossRetreatExecutor  executor_       {};         ///< 移動ロジック本体
    bool                 isFirstExecute_ = true;     ///< 初回実行フラグ
};
