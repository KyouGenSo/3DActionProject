#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

/// <summary>
/// HP 条件ノード
/// 現在のボス HP を最大 HP に対するパーセンテージで比較して成功/失敗を返す
/// </summary>
class BTBossHPCondition : public Tako::BTNode {
public:
    /// <summary>
    /// 比較タイプ
    /// </summary>
    enum class Comparison {
        Less = 0,            // HP が閾値未満
        LessOrEqual = 1,     // HP が閾値以下
        Greater = 2,         // HP が閾値超過
        GreaterOrEqual = 3   // HP が閾値以上
    };

    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossHPCondition();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossHPCondition() = default;

    /// <summary>
    /// ノードの実行
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <returns>実行結果</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;

    /// <summary>
    /// ノードのリセット
    /// </summary>
    void Reset() override;

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override;

    /// <summary>
    /// パラメータを JSON として抽出
    /// </summary>
    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    /// <summary>
    /// ImGui でパラメータ編集 UI を描画
    /// </summary>
    bool DrawImGui() override;
#endif

    // Getters/Setters
    float GetThresholdPercent() const { return thresholdPercent_; }
    void SetThresholdPercent(float percent) { thresholdPercent_ = percent; }

    Comparison GetComparison() const { return comparison_; }
    void SetComparison(Comparison comp) { comparison_ = comp; }

private:
    /// <summary>
    /// 比較条件を評価
    /// </summary>
    /// <param name="currentPercent">現在の HP パーセンテージ</param>
    /// <returns>条件を満たしていれば true</returns>
    bool EvaluateCondition(float currentPercent) const;

    // HP 閾値（パーセンテージ: 0〜100）
    float thresholdPercent_ = 50.0f;

    // 比較タイプ
    Comparison comparison_ = Comparison::LessOrEqual;
};
