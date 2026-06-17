#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

/// <summary>
/// 現在 HP を最大 HP に対するパーセンテージで閾値比較する条件ノード
/// </summary>
class BTBossHPCondition : public Tako::BTNode {
public:
    enum class Comparison {
        Less = 0,
        LessOrEqual = 1,
        Greater = 2,
        GreaterOrEqual = 3
    };

    BTBossHPCondition();
    virtual ~BTBossHPCondition() = default;

    /// <summary>
    /// 現在 HP の割合（%）が閾値に対する比較条件を満たすかを判定する
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
    float GetThresholdPercent() const { return thresholdPercent_; }
    void SetThresholdPercent(float percent) { thresholdPercent_ = percent; }

    Comparison GetComparison() const { return comparison_; }
    void SetComparison(Comparison comp) { comparison_ = comp; }

private:
    /// <summary>
    /// 現在 HP 割合と閾値を comparison_ に従って比較する
    /// </summary>
    /// <param name="currentPercent">現在 HP の割合（0〜100）</param>
    /// <returns>比較条件を満たせば true</returns>
    bool EvaluateCondition(float currentPercent) const;

    float thresholdPercent_ = 50.0f;   ///< 閾値（パーセンテージ: 0〜100）
    Comparison comparison_ = Comparison::LessOrEqual;
};
