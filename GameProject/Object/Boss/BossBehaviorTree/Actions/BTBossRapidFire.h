#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの連続追尾射撃アクションノード
/// プレイヤー方向に連続で弾を発射する攻撃パターン。発射中もプレイヤー方向を追尾し続ける。
/// </summary>
class BTBossRapidFire : public AttackNode {
public:
    BTBossRapidFire();
    virtual ~BTBossRapidFire() = default;

    // パラメータ取得・設定
    float GetChargeTime() const { return chargeTime_; }
    void  SetChargeTime(float time) { chargeTime_ = time; }
    int   GetBulletCount() const { return bulletCount_; }
    void  SetBulletCount(int count) { bulletCount_ = count; }
    float GetFireInterval() const { return fireInterval_; }
    void  SetFireInterval(float interval) { fireInterval_ = interval; }
    float GetBulletSpeed() const { return bulletSpeed_; }
    void  SetBulletSpeed(float speed) { bulletSpeed_ = speed; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }

protected:
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;
    void OnApplyParameters(const nlohmann::json& params) override;
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private:
    void AimAtPlayer(BTBlackboard* blackboard, float deltaTime);
    void FireBullet(BTBlackboard* blackboard);
    Tako::Vector3 CalculateDirectionToPlayer(BTBlackboard* blackboard);

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float chargeTime_ = 0.9f;        ///< 射撃前の準備時間
    int   bulletCount_ = 5;          ///< 発射する弾の数
    float fireInterval_ = 0.15f;     ///< 発射間隔（秒）
    float recoveryTime_ = 0.5f;      ///< 射撃後の硬直時間
    float bulletSpeed_ = 20.0f;      ///< 弾の速度

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;     ///< 状態の総時間（OnInitialize で算出）
    int   firedCount_ = 0;           ///< 発射済み弾数
    float timeSinceLastFire_ = 0.0f; ///< 前回発射からの経過時間

    //=========================================================================================
    // 演出
    //=========================================================================================
    BulletSignEffect bulletSignEffect_;
};
