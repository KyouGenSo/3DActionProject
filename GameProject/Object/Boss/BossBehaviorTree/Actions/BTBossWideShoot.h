#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの大範囲射撃アクションノード
/// プレイヤー方向に向きながら、角度をスイープしながら弾を連射
/// 通常弾（速い）と貫通弾（遅い）を混ぜて発射
/// </summary>
class BTBossWideShoot : public BTNode {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossWideShoot();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossWideShoot() = default;

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
    float GetRecoveryTime() const { return recoveryTime_; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    float GetFiringDuration() const { return firingDuration_; }
    void SetFiringDuration(float duration) { firingDuration_ = duration; }
    float GetSweepAngle() const { return sweepAngle_; }
    void SetSweepAngle(float angle) { sweepAngle_ = angle; }
    int GetBulletsPerSweep() const { return bulletsPerSweep_; }
    void SetBulletsPerSweep(int count) { bulletsPerSweep_ = count; }
    int GetSweepCount() const { return sweepCount_; }
    void SetSweepCount(int count) { sweepCount_ = count; }
    float GetNormalBulletSpeed() const { return normalBulletSpeed_; }
    void SetNormalBulletSpeed(float speed) { normalBulletSpeed_ = speed; }
    float GetPenetratingBulletSpeed() const { return penetratingBulletSpeed_; }
    void SetPenetratingBulletSpeed(float speed) { penetratingBulletSpeed_ = speed; }
    int GetPenetratingCount() const { return penetratingCount_; }
    void SetPenetratingCount(int count) { penetratingCount_ = count; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("chargeTime")) {
            chargeTime_ = params["chargeTime"];
        }
        if (params.contains("recoveryTime")) {
            recoveryTime_ = params["recoveryTime"];
        }
        if (params.contains("firingDuration")) {
            firingDuration_ = params["firingDuration"];
        }
        if (params.contains("sweepAngle")) {
            sweepAngle_ = params["sweepAngle"];
        }
        if (params.contains("bulletsPerSweep")) {
            bulletsPerSweep_ = params["bulletsPerSweep"];
        }
        if (params.contains("sweepCount")) {
            sweepCount_ = params["sweepCount"];
        }
        if (params.contains("normalBulletSpeed")) {
            normalBulletSpeed_ = params["normalBulletSpeed"];
        }
        if (params.contains("penetratingBulletSpeed")) {
            penetratingBulletSpeed_ = params["penetratingBulletSpeed"];
        }
        if (params.contains("penetratingCount")) {
            penetratingCount_ = params["penetratingCount"];
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

private:
    // 定数
    static constexpr float kDirectionEpsilon = 0.001f;  ///< 方向計算の閾値
    static constexpr float kAngleEpsilon = 0.0001f;     ///< 角度計算の閾値

    /// <summary>
    /// 射撃パラメータの初期化
    /// </summary>
    /// <param name="boss">ボス</param>
    void InitializeWideShoot(BTBlackboard* blackboard);

    /// <summary>
    /// プレイヤーを狙う処理
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <param name="deltaTime">経過時間</param>
    void AimAtPlayer(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 弾を1発発射
    /// </summary>
    /// <param name="boss">ボス</param>
    void FireBullet(Boss* boss);

    /// <summary>
    /// 現在の発射角度オフセットを取得
    /// </summary>
    /// <returns>角度オフセット（ラジアン）</returns>
    float GetCurrentAngleOffset() const;

    /// <summary>
    /// 現在の弾が貫通弾かどうか判定
    /// </summary>
    /// <returns>貫通弾なら true</returns>
    bool IsPenetratingBullet() const;

    /// <summary>
    /// 弾の発射方向を計算
    /// </summary>
    /// <param name="baseDirection">基準方向</param>
    /// <param name="angleOffset">角度オフセット</param>
    /// <returns>発射方向ベクトル</returns>
    Tako::Vector3 CalculateBulletDirection(const Tako::Vector3& baseDirection, float angleOffset);

    // === 時間制御 ===
    float chargeTime_ = 0.8f;       ///< チャージ時間
    float recoveryTime_ = 0.5f;     ///< 硬直時間
    float firingDuration_ = 1.0f;   ///< 全体の発射時間（秒）
    float fireInterval_ = 0.0f;     ///< 発射間隔（自動計算）

    // === 角度制御 ===
    float sweepAngle_ = 1.0472f;    ///< スイープ角度（約60度）

    // === 弾数制御 ===
    int bulletsPerSweep_ = 12;      ///< 1スイープあたりの弾数
    int sweepCount_ = 2;            ///< スイープ回数

    // === 速度制御 ===
    float normalBulletSpeed_ = 40.0f;       ///< 通常弾速度（速め）
    float penetratingBulletSpeed_ = 15.0f;  ///< 貫通弾速度（遅め）

    // === 弾種制御 ===
    int penetratingCount_ = 4;      ///< 1スイープあたりの貫通弾の数

    // === 状態管理 ===
    float totalDuration_ = 0.0f;    ///< 状態の総時間
    float elapsedTime_ = 0.0f;      ///< 経過時間
    float timeSinceLastFire_ = 0.0f;///< 前回発射からの経過時間
    int currentSweep_ = 0;          ///< 現在のスイープ回数
    int firedInSweep_ = 0;          ///< 現在のスイープで発射した弾数
    bool isFirstExecute_ = true;    ///< 初回実行フラグ
    bool hasEndedEffect_ = false;   ///< エフェクト終了フラグ

    // === 方向キャッシュ ===
    Tako::Vector3 baseDirection_;   ///< 発射基準方向（プレイヤー方向）

    // === エフェクト ===
    BulletSignEffect bulletSignEffect_;  ///< 射撃予兆エフェクト
};
