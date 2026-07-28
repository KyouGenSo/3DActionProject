#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

/// <summary>
/// プレイヤーがボス方向へ射撃中かを判定する条件ノード
/// </summary>
class BTBossPlayerShootingCondition : public Tako::BTNode {
public: //メンバー関数
    BTBossPlayerShootingCondition();
    virtual ~BTBossPlayerShootingCondition() = default;

    /// <summary>
    /// プレイヤーが射撃ステート中で、照準方向とボスへの方向（XZ 平面）の角度が閾値以内かを判定する
    /// </summary>
    /// <param name="blackboard">"boss" と "player" ポインタを保持するブラックボード</param>
    /// <returns>ボス方向へ射撃中なら Success。非射撃中・閾値外・boss/player が未設定なら Failure</returns>
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
    void SetAngleThreshold(float degrees) { angleThreshold_ = degrees; }

    //=============================
    //Getter
    //=============================
    float GetAngleThreshold() const { return angleThreshold_; }

private: //メンバー変数
    float angleThreshold_ = 30.0f;    ///< 許容角度（度数法）
};
