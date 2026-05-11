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
    /// <summary>
    /// 固有攻撃ロジック本体（Charge → Launch → Warning → Blink → Impact → Recovery のフェーズ制御）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    /// <returns>BTNodeStatus::Running（攻撃継続中） / BTNodeStatus::Success（攻撃完了）</returns>
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理（着弾位置の生成、Decal の準備、totalDuration の算出）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;

    /// <summary>
    /// 固有クリーンアップ処理（Decal / コライダー / パーティクル群の解放とフラグリセット）
    /// </summary>
    void OnCleanup() override;

    /// <summary>
    /// 固有のjsonパラメータ適用
    /// </summary>
    /// <param name="params">適用するjsonパラメータ</param>
    void OnApplyParameters(const nlohmann::json& params) override;

    /// <summary>
    /// 固有のjsonパラメータ抽出処理
    /// </summary>
    /// <param name="out">抽出したパラメータを格納するjsonオブジェクトへの参照</param>
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    /// <summary>
    /// 固有のImGuiデバッグ表示
    /// </summary>
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>
    /// フェーズ2エリア内のランダムな着弾位置群を生成して impactPositions_ に格納する
    /// </summary>
    /// <param name="bossPos">ボスの現在位置（生成範囲の基準）</param>
    void GenerateImpactPositions(const Tako::Vector3& bossPos);

    /// <summary>
    /// 上空に向けて弾を1発発射する（Launch フェーズで index 番目の弾を発射）
    /// </summary>
    /// <param name="boss">弾を発射するBossへのポインタ</param>
    /// <param name="index">発射する弾のインデックス</param>
    void LaunchBullet(Boss* boss, int index);

    /// <summary>
    /// 点滅フェーズの透明度更新（着弾位置 Decal の点滅警告）
    /// </summary>
    /// <param name="phaseElapsed">点滅フェーズの経過時間</param>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// 着弾フェーズの開始処理（コライダー有効化と着弾パーティクル起動）
    /// </summary>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void BeginImpactPhase(Boss* boss);

    /// <summary>
    /// 着弾フェーズの終了処理（コライダー無効化と Decal の非表示化）
    /// </summary>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
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
