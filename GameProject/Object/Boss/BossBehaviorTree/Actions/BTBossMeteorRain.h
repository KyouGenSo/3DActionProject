#pragma once
#include "AttackNode.h"
#include "../../../../Collision/MeteorImpactCollider.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <vector>
#include <string>

class Boss;

/// <summary>
/// 弾を上方へ発射し、フェーズ2エリア内のランダム位置へ着弾させる。
/// 水平方向への全方位弾も同時に放つ。
/// </summary>
class BTBossMeteorRain : public AttackNode {
private: //定数
    static constexpr int   kMaxImpacts          = 20;
    static constexpr int   kMaxPlacementRetries = 30;
    static constexpr float kDecalBaseAlpha      = 0.3f;
    static constexpr float kBlinkAlphaMin       = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

public: //メンバー関数
    BTBossMeteorRain();
    virtual ~BTBossMeteorRain();

    //=======================================
    //Setter
    //=======================================
    void  SetChargeTime(float time) { chargeTime_ = time; }
    void  SetLaunchDuration(float time) { launchDuration_ = time; }
    void  SetWarningDuration(float time) { warningDuration_ = time; }
    void  SetBlinkDuration(float time) { blinkDuration_ = time; }
    void  SetImpactDuration(float time) { impactDuration_ = time; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    void  SetMinImpacts(int count) { minImpacts_ = count; }
    void  SetMaxImpacts(int count) { maxImpacts_ = count; }
    void  SetImpactRadius(float radius) { impactRadius_ = radius; }
    void  SetDamage(float damage) { damage_ = damage; }
    void  SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }
    void  SetLaunchSpeed(float speed) { launchSpeed_ = speed; }
    void  SetLaunchSpreadXZ(float spread) { launchSpreadXZ_ = spread; }
    void  SetHorizontalSpeed(float speed) { horizontalSpeed_ = speed; }
    void  SetHorizontalBulletCount(int count) { horizontalBulletCount_ = count; }
    void  SetFallSpeed(float speed) { fallSpeed_ = speed; }
    void  SetFallHeight(float height) { fallHeight_ = height; }

    //=======================================
    //Getter
    //=======================================
    float GetChargeTime() const { return chargeTime_; }
    float GetLaunchDuration() const { return launchDuration_; }
    float GetWarningDuration() const { return warningDuration_; }
    float GetBlinkDuration() const { return blinkDuration_; }
    float GetImpactDuration() const { return impactDuration_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    int   GetMinImpacts() const { return minImpacts_; }
    int   GetMaxImpacts() const { return maxImpacts_; }
    float GetImpactRadius() const { return impactRadius_; }
    float GetDamage() const { return damage_; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    float GetLaunchSpeed() const { return launchSpeed_; }
    float GetLaunchSpreadXZ() const { return launchSpreadXZ_; }
    float GetHorizontalSpeed() const { return horizontalSpeed_; }
    int   GetHorizontalBulletCount() const { return horizontalBulletCount_; }
    float GetFallSpeed() const { return fallSpeed_; }
    float GetFallHeight() const { return fallHeight_; }

protected: //非公開関数
    /// <summary>
    /// Charge → Launch → Warning → Blink → Impact → Recovery のフェーズを進行させる
    /// </summary>
    /// <param name="blackboard">未使用</param>
    /// <param name="boss">発射・着弾させるボス</param>
    /// <param name="deltaTime">前フレームからの経過秒</param>
    /// <returns>全フェーズ完了で Success、進行中は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnCleanup() override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private: //非公開関数
    /// <summary>
    /// エリア内のランダムな着弾位置群を生成して impactPositions_ に格納
    /// </summary>
    /// <param name="bossPos">配置範囲の中心となるボス位置</param>
    void GenerateImpactPositions(const Tako::Vector3& bossPos);

    /// <summary>
    /// 上空へ弾を1発発射
    /// </summary>
    /// <param name="boss">発射元のボス</param>
    /// <param name="index">未使用</param>
    void LaunchBullet(Boss* boss, int index);

    /// <summary>
    /// 点滅フェーズの Decal アルファを sin で更新する
    /// </summary>
    /// <param name="phaseElapsed">点滅フェーズ開始からの経過秒</param>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// コライダー有効化と落下弾スポーン
    /// </summary>
    /// <param name="boss">落下弾を発射するボス</param>
    void BeginImpactPhase(Boss* boss);

    /// <summary>
    /// 着弾終了: Decal とコライダーを停止する
    /// </summary>
    /// <param name="boss">未使用</param>
    void EndImpactPhase(Boss* boss);

private: //メンバー変数
    //パラメータ
    float chargeTime_            = 0.5f;
    float launchDuration_        = 0.5f;
    float warningDuration_       = 1.0f;
    float blinkDuration_         = 0.8f;
    float impactDuration_        = 0.3f;
    float recoveryTime_          = 0.8f;
    int   minImpacts_            = 3;
    int   maxImpacts_            = 6;
    float impactRadius_          = 4.0f;
    float damage_                = 15.0f;
    float blinkFrequency_        = 10.0f;
    float launchSpeed_           = 30.0f;
    float launchSpreadXZ_        = 5.0f;
    float horizontalSpeed_       = 20.0f;
    int   horizontalBulletCount_ = 10;
    float fallSpeed_             = 40.0f;
    float fallHeight_            = 40.0f;

    //ランタイム状態
    float            totalDuration_             = 0.0f;
    bool             hasBegunImpact_            = false;
    bool             hasEndedImpact_            = false;
    bool             decalsShown_               = false;
    int              bulletsLaunched_           = 0;
    int              horizontalBulletsLaunched_ = 0;
    BulletSignEffect bulletSignEffect_;

    //着弾管理（動的サイズ）
    int                                                impactCount_        = 0;
    std::vector<Tako::Vector3>                         impactPositions_;
    std::vector<std::unique_ptr<Tako::Decal>>          impactDecals_;
    std::vector<std::unique_ptr<MeteorImpactCollider>> impactColliders_;
    std::vector<Tako::Transform>                       colliderTransforms_;
};
