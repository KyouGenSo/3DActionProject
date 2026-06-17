#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

/// <summary>
/// プレイヤーとの水平距離が指定範囲内かを判定する条件ノード
/// </summary>
class BTBossDistanceCondition : public Tako::BTNode {
public:
    BTBossDistanceCondition();
    virtual ~BTBossDistanceCondition() = default;

    /// <summary>
    /// プレイヤーとの水平距離（Y 無視）が [min, max] の範囲内かを判定する
    /// </summary>
    /// <param name="blackboard">"boss" と "player" ポインタを保持するブラックボード</param>
    /// <returns>範囲内なら Success。範囲外または boss/player が未設定なら Failure</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;
    void Reset() override;
    void ApplyParameters(const nlohmann::json& params) override;
    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    bool DrawImGui() override;
#endif

    // Getters/Setters
    float GetMinDistance() const { return minDistance_; }
    void SetMinDistance(float dist) { minDistance_ = dist; }

    float GetMaxDistance() const { return maxDistance_; }
    void SetMaxDistance(float dist) { maxDistance_ = dist; }

private:
    float minDistance_ = 0.0f;
    float maxDistance_ = 15.0f;
};
