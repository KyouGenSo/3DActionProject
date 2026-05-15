#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

/// <summary>
/// 距離条件ノード
/// プレイヤーとの距離が指定範囲内かを判定して成功/失敗を返す
/// </summary>
class BTBossDistanceCondition : public Tako::BTNode {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossDistanceCondition();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossDistanceCondition() = default;

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
    float GetMinDistance() const { return minDistance_; }
    void SetMinDistance(float dist) { minDistance_ = dist; }

    float GetMaxDistance() const { return maxDistance_; }
    void SetMaxDistance(float dist) { maxDistance_ = dist; }

private:
    // 最小距離（これ以上離れている必要がある）
    float minDistance_ = 0.0f;

    // 最大距離（これ以下の距離である必要がある）
    float maxDistance_ = 15.0f;
};
