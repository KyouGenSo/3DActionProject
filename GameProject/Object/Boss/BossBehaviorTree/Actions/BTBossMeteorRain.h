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
/// ボスのメテオレイン攻撃アクションノード
/// フェーズ2でボスが弾を上方向に発射し、フェーズ2エリア内のランダムな位置に着弾する。
/// Charge → Launch → Warning → Blink → Impact → Recovery
/// </summary>
class BTBossMeteorRain : public AttackNode {
public:
    BTBossMeteorRain();
    virtual ~BTBossMeteorRain();

    // パラメータ取得・設定
    float GetChargeTime() const { return chargeTime_; }
    void  SetChargeTime(float time) { chargeTime_ = time; }
    float GetLaunchDuration() const { return launchDuration_; }
    void  SetLaunchDuration(float time) { launchDuration_ = time; }
    float GetWarningDuration() const { return warningDuration_; }
    void  SetWarningDuration(float time) { warningDuration_ = time; }
    float GetBlinkDuration() const { return blinkDuration_; }
    void  SetBlinkDuration(float time) { blinkDuration_ = time; }
    float GetImpactDuration() const { return impactDuration_; }
    void  SetImpactDuration(float time) { impactDuration_ = time; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    int   GetMinImpacts() const { return minImpacts_; }
    void  SetMinImpacts(int count) { minImpacts_ = count; }
    int   GetMaxImpacts() const { return maxImpacts_; }
    void  SetMaxImpacts(int count) { maxImpacts_ = count; }
    float GetImpactRadius() const { return impactRadius_; }
    void  SetImpactRadius(float radius) { impactRadius_ = radius; }
    float GetDamage() const { return damage_; }
    void  SetDamage(float damage) { damage_ = damage; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    void  SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }
    float GetLaunchSpeed() const { return launchSpeed_; }
    void  SetLaunchSpeed(float speed) { launchSpeed_ = speed; }
    float GetLaunchSpreadXZ() const { return launchSpreadXZ_; }
    void  SetLaunchSpreadXZ(float spread) { launchSpreadXZ_ = spread; }
    float GetHorizontalSpeed() const { return horizontalSpeed_; }
    void  SetHorizontalSpeed(float speed) { horizontalSpeed_ = speed; }
    int   GetHorizontalBulletCount() const { return horizontalBulletCount_; }
    void  SetHorizontalBulletCount(int count) { horizontalBulletCount_ = count; }
    float GetFallSpeed() const { return fallSpeed_; }
    void  SetFallSpeed(float speed) { fallSpeed_ = speed; }
    float GetFallHeight() const { return fallHeight_; }
    void  SetFallHeight(float height) { fallHeight_ = height; }

protected:
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;
    void OnCleanup() override;
    void OnApplyParameters(const nlohmann::json& params) override;
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private:
    void GenerateImpactPositions(const Tako::Vector3& bossPos);
    void LaunchBullet(Boss* boss, int index);
    void UpdateBlinkingPhase(float phaseElapsed);
    void BeginImpactPhase(Boss* boss);
    void EndImpactPhase(Boss* boss);

    static constexpr int kMaxImpacts = 20;
    static constexpr int kMaxPlacementRetries = 30;
    static constexpr float kDecalBaseAlpha = 0.3f;
    static constexpr float kBlinkAlphaMin = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float chargeTime_ = 0.5f;
    float launchDuration_ = 0.5f;
    float warningDuration_ = 1.0f;
    float blinkDuration_ = 0.8f;
    float impactDuration_ = 0.3f;
    float recoveryTime_ = 0.8f;
    int   minImpacts_ = 3;
    int   maxImpacts_ = 6;
    float impactRadius_ = 4.0f;
    float damage_ = 15.0f;
    float blinkFrequency_ = 10.0f;
    float launchSpeed_ = 30.0f;
    float launchSpreadXZ_ = 5.0f;
    float horizontalSpeed_ = 20.0f;
    int   horizontalBulletCount_ = 10;
    float fallSpeed_ = 40.0f;
    float fallHeight_ = 40.0f;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;
    bool  hasBegunImpact_ = false;
    bool  hasEndedImpact_ = false;
    bool  decalsShown_ = false;
    int   bulletsLaunched_ = 0;
    int   horizontalBulletsLaunched_ = 0;
    BulletSignEffect bulletSignEffect_;

    //=========================================================================================
    // 着弾管理（動的サイズ）
    //=========================================================================================
    int impactCount_ = 0;
    std::vector<Tako::Vector3> impactPositions_;
    std::vector<std::unique_ptr<Tako::Decal>> impactDecals_;
    std::vector<std::unique_ptr<MeteorImpactCollider>> impactColliders_;
    std::vector<Tako::Transform> colliderTransforms_;
};
