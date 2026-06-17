#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ステージ中央へ移動し、全方位へ弾を一定時間ばらまく。
/// 通常弾（速い）と貫通弾（遅い）を penetratingRatio_ で混ぜる。
/// </summary>
class BTBossBarrage : public AttackNode {
private: //定数
    static constexpr float kDirectionEpsilon = 0.001f;

public: //メンバー関数
    BTBossBarrage();
    virtual ~BTBossBarrage() = default;

    //=============================================
    //Setter
    //=============================================
    void  SetMoveDuration(float time) { moveDuration_ = time; }
    void  SetChargeTime(float time) { chargeTime_ = time; }
    void  SetFiringDuration(float duration) { firingDuration_ = duration; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    void  SetFireInterval(float interval) { fireInterval_ = interval; }
    void  SetNormalBulletSpeedMin(float speed) { normalBulletSpeedMin_ = speed; }
    void  SetNormalBulletSpeedMax(float speed) { normalBulletSpeedMax_ = speed; }
    void  SetPenetratingBulletSpeedMin(float speed) { penetratingBulletSpeedMin_ = speed; }
    void  SetPenetratingBulletSpeedMax(float speed) { penetratingBulletSpeedMax_ = speed; }
    void  SetPenetratingRatio(float ratio) { penetratingRatio_ = ratio; }

    //=============================================
    //Getter
    //=============================================
    float GetMoveDuration() const { return moveDuration_; }
    float GetChargeTime() const { return chargeTime_; }
    float GetFiringDuration() const { return firingDuration_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    float GetFireInterval() const { return fireInterval_; }
    float GetNormalBulletSpeedMin() const { return normalBulletSpeedMin_; }
    float GetNormalBulletSpeedMax() const { return normalBulletSpeedMax_; }
    float GetPenetratingBulletSpeedMin() const { return penetratingBulletSpeedMin_; }
    float GetPenetratingBulletSpeedMax() const { return penetratingBulletSpeedMax_; }
    float GetPenetratingRatio() const { return penetratingRatio_; }

protected: //非公開関数
    /// <summary>
    /// Move → Charge → Firing → Recovery のフェーズを進行させる
    /// </summary>
    /// <param name="blackboard">未使用</param>
    /// <param name="boss">移動・発射させるボス</param>
    /// <param name="deltaTime">前フレームからの経過秒</param>
    /// <returns>全フェーズ完了で Success、進行中は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private: //非公開関数
    /// <summary>
    /// ステージ中央へイージング移動し、進行方向へ旋回する
    /// </summary>
    /// <param name="boss">移動・旋回させるボス</param>
    /// <param name="deltaTime">未使用</param>
    void UpdateMove(Boss* boss, float deltaTime);

    /// <summary>
    /// 通常弾/貫通弾を penetratingRatio_ で抽選し、ランダムな水平方向へ1発発射
    /// </summary>
    void FireRandomBullet(Boss* boss);

private: //メンバー変数
    //パラメータ
    float moveDuration_   = 0.5f;
    float chargeTime_     = 0.8f;
    float firingDuration_ = 3.0f;
    float recoveryTime_   = 0.5f;
    float fireInterval_   = 0.08f;

    float normalBulletSpeedMin_      = 20.0f;
    float normalBulletSpeedMax_      = 35.0f;
    float penetratingBulletSpeedMin_ = 10.0f;
    float penetratingBulletSpeedMax_ = 20.0f;
    float penetratingRatio_          = 0.3f;

    //ランタイム状態
    float         totalDuration_     = 0.0f;
    float         timeSinceLastFire_ = 0.0f;
    bool          hasEndedEffect_    = false;
    Tako::Vector3 startPosition_;
    Tako::Vector3 targetPosition_;

    //演出
    BulletSignEffect bulletSignEffect_;
};
