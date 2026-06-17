#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

/// <summary>
/// プレイヤーとの水平距離が指定範囲内かを判定する条件ノード
/// </summary>
class BTBossDistanceCondition : public Tako::BTNode {
public: //メンバー関数
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

    //=============================
    //Setter
    //=============================
    void SetMinDistance(float dist) { minDistance_ = dist; }
    void SetMaxDistance(float dist) { maxDistance_ = dist; }

    //=============================
    //Getter
    //=============================
    float GetMinDistance() const { return minDistance_; }
    float GetMaxDistance() const { return maxDistance_; }

private: //メンバー変数
    float minDistance_ = 0.0f;
    float maxDistance_ = 15.0f;
};
