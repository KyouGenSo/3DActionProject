#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"
#include "../../Movement/BossRetreatExecutor.h"

class Boss;

/// <summary>
/// プレイヤーを向いたまま後方へイージング移動で離脱。移動本体は BossRetreatExecutor へ委譲。
/// </summary>
class BTBossRetreat : public Tako::BTNode {
public: //メンバー関数
    BTBossRetreat();
    virtual ~BTBossRetreat() = default;

    /// <summary>
    /// プレイヤーから離れる方向へ移動。
    /// </summary>
    /// <returns>ボス/プレイヤー不在で Failure、目標距離到達で Success、移動中は Running</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;
    void Reset() override;

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

    //====================================
    //Setter
    //====================================
    void SetRetreatSpeed(float speed) {
        auto p = executor_.GetParameters();
        p.retreatSpeed = speed;
        executor_.SetParameters(p);
    }
    void SetTargetDistance(float distance) {
        auto p = executor_.GetParameters();
        p.targetDistance = distance;
        executor_.SetParameters(p);
    }

    //====================================
    //Getter
    //====================================
    float GetRetreatSpeed()   const { return executor_.GetParameters().retreatSpeed; }
    float GetTargetDistance() const { return executor_.GetParameters().targetDistance; }

private: //メンバー変数
    Boss*               boss_           = nullptr;
    BossRetreatExecutor executor_{};
    bool                isFirstExecute_ = true;
};
