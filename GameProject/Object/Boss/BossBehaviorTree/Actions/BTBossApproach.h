#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"
#include "Vector3.h"

class Boss;
class Player;

/// <summary>
/// プレイヤー方向へイージング移動で接近し、一定距離で停止する
/// </summary>
class BTBossApproach : public Tako::BTNode {
private: //定数
    static constexpr float kDirectionEpsilon = 0.01f;
    static constexpr float kArrivalThreshold = 0.5f;

public: //メンバー関数
    BTBossApproach();
    virtual ~BTBossApproach() = default;

    /// <summary>
    /// プレイヤー手前 targetDistance_ までイージング移動する
    /// </summary>
    /// <param name="blackboard">boss / player ポインタを保持する共有ストレージ</param>
    /// <returns>boss/player 未取得で Failure、目標到達で Success、移動中は Running</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;

    void Reset() override;

    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("approachSpeed")) {
            approachSpeed_ = params["approachSpeed"];
        }
        if (params.contains("targetDistance")) {
            targetDistance_ = params["targetDistance"];
        }
    }

    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    bool DrawImGui() override;
#endif

    //====================================
    //Setter
    //====================================
    void SetApproachSpeed(float speed) { approachSpeed_ = speed; }
    void SetTargetDistance(float distance) { targetDistance_ = distance; }

    //====================================
    //Getter
    //====================================
    float GetApproachSpeed() const { return approachSpeed_; }
    float GetTargetDistance() const { return targetDistance_; }

private: //非公開関数
    /// <summary>
    /// 開始/目標位置とエリア制限後の所要時間を算出し、プレイヤー方向へ旋回する
    /// </summary>
    /// <param name="boss">移動・旋回させるボス</param>
    /// <param name="player">接近対象のプレイヤー</param>
    void InitializeApproach(Boss* boss, Player* player);

    /// <summary>
    /// elapsedTime_ / approachDuration_ の進捗で開始位置から目標位置へ補間移動する
    /// </summary>
    /// <param name="boss">移動させるボス</param>
    /// <param name="deltaTime">未使用</param>
    void UpdateApproachMovement(Boss* boss, float deltaTime);

private: //メンバー変数
    //パラメータ
    float approachSpeed_  = 80.0f;
    float targetDistance_ = 12.0f;  ///< プレイヤーからの停止距離

    //状態管理
    Tako::Vector3 startPosition_;
    Tako::Vector3 targetPosition_;
    float         elapsedTime_      = 0.0f;
    float         approachDuration_ = 0.0f;  ///< 距離から動的計算
    bool          isFirstExecute_   = true;
};
