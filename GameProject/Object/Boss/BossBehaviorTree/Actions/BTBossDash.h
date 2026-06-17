#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ランダム方向へエリア内で一定距離ダッシュする
/// </summary>
class BTBossDash : public Tako::BTNode {
private: //定数
    static constexpr float kDirectionEpsilon = 0.01f;

public: //メンバー関数
    BTBossDash();
    virtual ~BTBossDash() = default;

    /// <summary>
    /// ランダム方向へエリア内をダッシュ移動する
    /// </summary>
    /// <param name="blackboard">boss ポインタを保持する共有ストレージ</param>
    /// <returns>boss 未取得で Failure、dashDuration_ 経過で Success、移動中は Running</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;

    void Reset() override;

    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("dashSpeed")) {
            dashSpeed_ = params["dashSpeed"];
        }
        if (params.contains("dashDuration")) {
            dashDuration_ = params["dashDuration"];
        }
    }

    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    bool DrawImGui() override;
#endif

    //==================================
    //Setter
    //==================================
    void SetDashSpeed(float speed) { dashSpeed_ = speed; }
    void SetDashDuration(float duration) { dashDuration_ = duration; }

    //==================================
    //Getter
    //==================================
    float GetDashSpeed() const { return dashSpeed_; }
    float GetDashDuration() const { return dashDuration_; }

private: //非公開関数
    /// <summary>
    /// ランダム方向と距離を抽選し、エリア制限後の目標位置と所要時間を決めて旋回する
    /// </summary>
    /// <param name="boss">移動・旋回させるボス</param>
    void InitializeDash(Boss* boss);

    /// <summary>
    /// 開始位置から目標位置へ補間移動し、上下振動を加える
    /// </summary>
    /// <param name="boss">移動させるボス</param>
    /// <param name="deltaTime">未使用</param>
    void UpdateDashMovement(Boss* boss, float deltaTime);

private: //メンバー変数
    //ダッシュ方向・パラメータ
    Tako::Vector3 dashDirection_;
    float         dashSpeed_     = 60.0f;
    float         dashDuration_  = 0.5f;

    //ランタイム状態
    Tako::Vector3 startPosition_;
    Tako::Vector3 targetPosition_;
    float         elapsedTime_    = 0.0f;
    bool          isFirstExecute_ = true;

    //距離・振動パラメータ
    float minDistance_   = 10.0f;
    float maxDistance_   = 50.0f;
    float vibrationFreq_ = 50.0f;  ///< ダッシュ中の上下振動の周波数
    float vibrationAmp_  = 0.05f;  ///< ダッシュ中の上下振動の振幅
};
