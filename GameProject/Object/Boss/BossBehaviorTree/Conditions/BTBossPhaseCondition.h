#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

/// <summary>
/// 現在のボスフェーズを指定値と比較する条件ノード
/// </summary>
class BTBossPhaseCondition : public Tako::BTNode {
public:
    enum class Comparison {
        Equal = 0,
        NotEqual = 1,
        GreaterOrEqual = 2,
        LessOrEqual = 3
    };

    BTBossPhaseCondition();
    virtual ~BTBossPhaseCondition() = default;

    /// <summary>
    /// 現在のボスフェーズが targetPhase に対する比較条件を満たすかを判定する
    /// </summary>
    /// <param name="blackboard">"boss" ポインタを保持するブラックボード</param>
    /// <returns>比較条件を満たせば Success。満たさない、または boss が未設定なら Failure</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;
    void Reset() override;
    void ApplyParameters(const nlohmann::json& params) override;
    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    bool DrawImGui() override;
#endif

    // Getters/Setters
    uint32_t GetTargetPhase() const { return targetPhase_; }
    void SetTargetPhase(uint32_t phase) { targetPhase_ = phase; }

    Comparison GetComparison() const { return comparison_; }
    void SetComparison(Comparison comp) { comparison_ = comp; }

private:
    /// <summary>
    /// 現在フェーズと targetPhase_ を comparison_ に従って比較する
    /// </summary>
    /// <param name="currentPhase">現在のボスフェーズ（1 or 2）</param>
    /// <returns>比較条件を満たせば true</returns>
    bool EvaluateCondition(uint32_t currentPhase) const;

    uint32_t targetPhase_ = 2;
    Comparison comparison_ = Comparison::Equal;
};
