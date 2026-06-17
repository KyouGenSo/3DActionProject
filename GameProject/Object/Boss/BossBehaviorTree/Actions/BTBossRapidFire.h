#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// 発射中もプレイヤー方向を追尾し続ける連続射撃。
/// </summary>
class BTBossRapidFire : public AttackNode {
public:
    BTBossRapidFire();
    virtual ~BTBossRapidFire() = default;

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
    /// <summary>
    /// チャージ→連射→硬直のフェーズ制御。総時間経過まで Running、超過で攻撃終了。
    /// </summary>
    /// <returns>総時間到達で FinishAttack の結果、未到達は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>
    /// ボス位置からプレイヤー方向へ弾を 1 発生成する。
    /// </summary>
    void FireBullet(Tako::BTBlackboard* blackboard);

    /// <summary>
    /// ボスからプレイヤーへの XZ 平面方向。
    /// </summary>
    /// <returns>正規化方向。プレイヤー不在または近接時は (0,0,1)</returns>
    Tako::Vector3 CalculateDirectionToPlayer(Tako::BTBlackboard* blackboard);

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float chargeTime_ = 0.9f;
    int   bulletCount_ = 5;
    float fireInterval_ = 0.15f;     ///< 秒
    float recoveryTime_ = 0.5f;
    float bulletSpeed_ = 20.0f;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;     ///< OnInitialize で算出
    int   firedCount_ = 0;
    float timeSinceLastFire_ = 0.0f;

    //=========================================================================================
    // 演出
    //=========================================================================================
    BulletSignEffect bulletSignEffect_;
};
