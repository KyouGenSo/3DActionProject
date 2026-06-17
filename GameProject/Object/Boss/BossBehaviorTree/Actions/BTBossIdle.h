#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

class Boss;

/// <summary>
/// プレイヤーを向きつつ idleDuration_ だけ待機する
/// </summary>
class BTBossIdle : public Tako::BTNode {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr float kDirectionEpsilon = 0.01f;

public:
    BTBossIdle();

    virtual ~BTBossIdle() = default;

    /// <summary>
    /// プレイヤーを向きつつ idleDuration_ だけ待機する
    /// </summary>
    /// <param name="blackboard">boss / player ポインタを保持する共有ストレージ</param>
    /// <returns>boss 未取得で Failure、idleDuration_ 経過で Success、待機中は Running</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;

    void Reset() override;

    void SetIdleDuration(float duration) { idleDuration_ = duration; }

    float GetIdleDuration() const { return idleDuration_; }

    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("idleDuration")) {
            idleDuration_ = params["idleDuration"];
        }
    }

    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    bool DrawImGui() override;
#endif

private:
    /// <summary>
    /// プレイヤー方向へ rotationSpeed_ で旋回する（1フレームの回転量を制限）
    /// </summary>
    /// <param name="blackboard">boss / player ポインタを保持する共有ストレージ</param>
    /// <param name="deltaTime">前フレームからの経過秒</param>
    void LookAtPlayer(Tako::BTBlackboard* blackboard, float deltaTime);


    float idleDuration_ = 2.0f;

    float rotationSpeed_ = 5.0f;   ///< ラジアン/秒

    float elapsedTime_ = 0.0f;

    bool isFirstExecute_ = true;
};