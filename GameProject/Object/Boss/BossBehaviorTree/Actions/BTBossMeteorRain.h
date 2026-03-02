#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
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
/// フェーズ2でボスが弾を上方向に発射し、フェーズ2エリア内のランダムな位置に着弾する
/// Charge(予備動作) → Launch(弾発射演出) → Warning(Decal表示) → Blink(点滅警告) → Impact(着弾ダメージ) → Recovery(硬直)
/// </summary>
class BTBossMeteorRain : public BTNode {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossMeteorRain();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossMeteorRain();

    /// <summary>
    /// ノードの実行
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <returns>実行結果</returns>
    BTNodeStatus Execute(BTBlackboard* blackboard) override;

    /// <summary>
    /// ノードのリセット
    /// </summary>
    void Reset() override;

    // パラメータ取得・設定
    float GetChargeTime() const { return chargeTime_; }
    void SetChargeTime(float time) { chargeTime_ = time; }
    float GetLaunchDuration() const { return launchDuration_; }
    void SetLaunchDuration(float time) { launchDuration_ = time; }
    float GetWarningDuration() const { return warningDuration_; }
    void SetWarningDuration(float time) { warningDuration_ = time; }
    float GetBlinkDuration() const { return blinkDuration_; }
    void SetBlinkDuration(float time) { blinkDuration_ = time; }
    float GetImpactDuration() const { return impactDuration_; }
    void SetImpactDuration(float time) { impactDuration_ = time; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    int GetMinImpacts() const { return minImpacts_; }
    void SetMinImpacts(int count) { minImpacts_ = count; }
    int GetMaxImpacts() const { return maxImpacts_; }
    void SetMaxImpacts(int count) { maxImpacts_ = count; }
    float GetImpactRadius() const { return impactRadius_; }
    void SetImpactRadius(float radius) { impactRadius_ = radius; }
    float GetDamage() const { return damage_; }
    void SetDamage(float damage) { damage_ = damage; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    void SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }
    float GetLaunchSpeed() const { return launchSpeed_; }
    void SetLaunchSpeed(float speed) { launchSpeed_ = speed; }
    float GetLaunchSpreadXZ() const { return launchSpreadXZ_; }
    void SetLaunchSpreadXZ(float spread) { launchSpreadXZ_ = spread; }
    float GetHorizontalSpeed() const { return horizontalSpeed_; }
    void SetHorizontalSpeed(float speed) { horizontalSpeed_ = speed; }
    int GetHorizontalBulletCount() const { return horizontalBulletCount_; }
    void SetHorizontalBulletCount(int count) { horizontalBulletCount_ = count; }
    float GetFallSpeed() const { return fallSpeed_; }
    void SetFallSpeed(float speed) { fallSpeed_ = speed; }
    float GetFallHeight() const { return fallHeight_; }
    void SetFallHeight(float height) { fallHeight_ = height; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("chargeTime")) {
            chargeTime_ = params["chargeTime"];
        }
        if (params.contains("launchDuration")) {
            launchDuration_ = params["launchDuration"];
        }
        if (params.contains("warningDuration")) {
            warningDuration_ = params["warningDuration"];
        }
        if (params.contains("blinkDuration")) {
            blinkDuration_ = params["blinkDuration"];
        }
        if (params.contains("impactDuration")) {
            impactDuration_ = params["impactDuration"];
        }
        if (params.contains("recoveryTime")) {
            recoveryTime_ = params["recoveryTime"];
        }
        if (params.contains("minImpacts")) {
            minImpacts_ = params["minImpacts"];
        }
        if (params.contains("maxImpacts")) {
            maxImpacts_ = params["maxImpacts"];
        }
        if (params.contains("impactRadius")) {
            impactRadius_ = params["impactRadius"];
        }
        if (params.contains("damage")) {
            damage_ = params["damage"];
        }
        if (params.contains("blinkFrequency")) {
            blinkFrequency_ = params["blinkFrequency"];
        }
        if (params.contains("launchSpeed")) {
            launchSpeed_ = params["launchSpeed"];
        }
        if (params.contains("launchSpreadXZ")) {
            launchSpreadXZ_ = params["launchSpreadXZ"];
        }
        if (params.contains("fallSpeed")) {
            fallSpeed_ = params["fallSpeed"];
        }
        if (params.contains("fallHeight")) {
            fallHeight_ = params["fallHeight"];
        }
        if (params.contains("horizontalSpeed")) {
            horizontalSpeed_ = params["horizontalSpeed"];
        }
        if (params.contains("horizontalBulletCount")) {
            horizontalBulletCount_ = params["horizontalBulletCount"];
        }
    }

    /// <summary>
    /// パラメータを JSON として抽出
    /// </summary>
    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    /// <summary>
    /// ImGui でパラメータ編集 UI を描画
    /// </summary>
    bool DrawImGui() override;
#endif

private: // プライベートメンバー関数
    /// <summary>
    /// メテオレイン攻撃の初期化（着弾数決定、位置生成、Decal/コライダー初期化）
    /// </summary>
    /// <param name="boss">ボス</param>
    void InitializeMeteorRain(Boss* boss);

    /// <summary>
    /// ランダムな着弾位置を生成
    /// </summary>
    /// <param name="bossPos">ボスの位置</param>
    void GenerateImpactPositions(const Tako::Vector3& bossPos);

    /// <summary>
    /// 上方向に弾を発射（視覚演出）
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="index">着弾インデックス</param>
    void LaunchBullet(Boss* boss, int index);

    /// <summary>
    /// Blinking フェーズの更新（sin波点滅）
    /// </summary>
    /// <param name="phaseElapsed">フェーズ内の経過時間</param>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// Impact フェーズの開始（コライダー有効化、Decal非表示）
    /// </summary>
    /// <param name="boss">ボス</param>
    void BeginImpactPhase(Boss* boss);

    /// <summary>
    /// Impact フェーズの終了（コライダー無効化）
    /// </summary>
    /// <param name="boss">ボス</param>
    void EndImpactPhase(Boss* boss);

    /// <summary>
    /// リソースのクリーンアップ
    /// </summary>
    void Cleanup();

private:
    /// <summary>
    /// 着弾数の上限
    /// </summary>
    static constexpr int kMaxImpacts = 20;

    /// <summary>
    /// Decal の基本アルファ値
    /// </summary>
    static constexpr float kDecalBaseAlpha = 0.3f;

    /// <summary>
    /// 点滅アルファ値の最小値
    /// </summary>
    static constexpr float kBlinkAlphaMin = 0.15f;

    /// <summary>
    /// 点滅アルファ値の振幅
    /// </summary>
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

    //=========================================================================================
    // パラメータ
    //=========================================================================================

    // === 時間制御 ===
    float chargeTime_ = 0.5f;         ///< 予備動作（チャージ）時間
    float launchDuration_ = 0.5f;     ///< 弾発射演出時間
    float warningDuration_ = 1.0f;    ///< Decal予兆表示時間
    float blinkDuration_ = 0.8f;      ///< Decal点滅警告時間
    float impactDuration_ = 0.3f;     ///< 着弾ダメージ判定時間
    float recoveryTime_ = 0.8f;       ///< 硬直時間

    // === 攻撃パラメータ ===
    int minImpacts_ = 3;              ///< 最小着弾数
    int maxImpacts_ = 6;              ///< 最大着弾数
    float impactRadius_ = 4.0f;       ///< 各着弾地点の半径
    float damage_ = 15.0f;            ///< ダメージ量
    float blinkFrequency_ = 10.0f;    ///< 点滅周波数 (Hz)

    // === 弾発射パラメータ ===
    float launchSpeed_ = 30.0f;       ///< 弾の上方向速度
    float launchSpreadXZ_ = 5.0f;     ///< 発射時のXZ散らし量
    float horizontalSpeed_ = 20.0f;   ///< 水平方向の弾速度
    int horizontalBulletCount_ = 10;  ///< 水平方向の弾数

    // === 落下弾パラメータ ===
    float fallSpeed_ = 40.0f;         ///< 弾の落下速度
    float fallHeight_ = 40.0f;        ///< 弾のスポーン高さ（着弾位置の上方）

    //=========================================================================================
    // 状態管理
    //=========================================================================================

    float elapsedTime_ = 0.0f;        ///< 経過時間
    float totalDuration_ = 0.0f;      ///< 状態の総時間
    bool isFirstExecute_ = true;      ///< 初回実行フラグ
    bool hasBegunImpact_ = false;     ///< 着弾開始済みフラグ
    bool hasEndedImpact_ = false;     ///< 着弾終了済みフラグ
    bool enteredRecovery_ = false;    ///< 硬直開始フラグ
    bool decalsShown_ = false;        ///< Decal表示済みフラグ
    int bulletsLaunched_ = 0;         ///< 発射済み弾数
    int horizontalBulletsLaunched_ = 0; ///< 水平弾発射済みカウンター
    BulletSignEffect bulletSignEffect_;  ///< 予備動作エフェクト

    //=========================================================================================
    // 着弾管理（動的サイズ）
    //=========================================================================================

    /// <summary>
    /// 着弾数（実行時に決定）
    /// </summary>
    int impactCount_ = 0;

    /// <summary>
    /// 各着弾地点の座標
    /// </summary>
    std::vector<Tako::Vector3> impactPositions_;

    //=========================================================================================
    // Decal
    //=========================================================================================

    /// <summary>
    /// 各着弾地点の範囲表示用 Decal
    /// </summary>
    std::vector<std::unique_ptr<Tako::Decal>> impactDecals_;

    //=========================================================================================
    // コライダー
    //=========================================================================================

    /// <summary>
    /// 各着弾地点のダメージ判定用コライダー
    /// </summary>
    std::vector<std::unique_ptr<MeteorImpactCollider>> impactColliders_;

    /// <summary>
    /// 各着弾地点コライダー用の Transform（コライダーが Transform* を参照するため保持）
    /// reserve() してから全追加後にポインタを渡すこと（再配置対策）
    /// </summary>
    std::vector<Tako::Transform> colliderTransforms_;

};
