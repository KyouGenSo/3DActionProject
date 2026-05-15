#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの大範囲射撃アクションノード
/// プレイヤー方向に向きながら、角度をスイープしながら弾を連射する。
/// 通常弾（速い）と貫通弾（遅い）を混ぜて発射する。
/// </summary>
class BTBossWideShoot : public AttackNode {
public:
    BTBossWideShoot();
    virtual ~BTBossWideShoot() = default;

    // パラメータ取得・設定
    float GetChargeTime() const { return chargeTime_; }
    void  SetChargeTime(float time) { chargeTime_ = time; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    float GetFiringDuration() const { return firingDuration_; }
    void  SetFiringDuration(float duration) { firingDuration_ = duration; }
    float GetSweepAngle() const { return sweepAngle_; }
    void  SetSweepAngle(float angle) { sweepAngle_ = angle; }
    int   GetBulletsPerSweep() const { return bulletsPerSweep_; }
    void  SetBulletsPerSweep(int count) { bulletsPerSweep_ = count; }
    int   GetSweepCount() const { return sweepCount_; }
    void  SetSweepCount(int count) { sweepCount_ = count; }
    float GetNormalBulletSpeed() const { return normalBulletSpeed_; }
    void  SetNormalBulletSpeed(float speed) { normalBulletSpeed_ = speed; }
    float GetPenetratingBulletSpeed() const { return penetratingBulletSpeed_; }
    void  SetPenetratingBulletSpeed(float speed) { penetratingBulletSpeed_ = speed; }
    int   GetPenetratingCount() const { return penetratingCount_; }
    void  SetPenetratingCount(int count) { penetratingCount_ = count; }

protected:
    /// <summary>
    /// 固有攻撃ロジック本体（チャージ → 角度スイープ連射 → 硬直の制御）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    /// <returns>Tako::BTNodeStatus::Running（攻撃継続中） / Tako::BTNodeStatus::Success（攻撃完了）</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理（totalDuration / fireInterval の算出、基準方向の決定、予兆エフェクト起動）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

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
    static constexpr float kDirectionEpsilon = 0.001f;
    static constexpr float kAngleEpsilon = 0.0001f;

    /// <summary>
    /// プレイヤー方向へボスを徐々に旋回させる
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    void AimAtPlayer(Tako::BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 現在のスイープ位置・弾種に基づき弾を1発生成・発射する
    /// </summary>
    /// <param name="boss">弾を発射するBossへのポインタ</param>
    void FireBullet(Boss* boss);

    /// <summary>
    /// 現在のスイープ進捗から基準方向に対する角度オフセットを算出する
    /// </summary>
    /// <returns>基準方向に対する角度オフセット（ラジアン）</returns>
    float GetCurrentAngleOffset() const;

    /// <summary>
    /// 現在のスイープ内位置から、発射する弾が貫通弾であるかを判定する
    /// </summary>
    /// <returns>貫通弾なら true、通常弾なら false</returns>
    bool  IsPenetratingBullet() const;

    /// <summary>
    /// 基準方向と角度オフセットから弾の進行方向ベクトルを算出する
    /// </summary>
    /// <param name="baseDirection">基準となる正規化済み方向ベクトル</param>
    /// <param name="angleOffset">基準方向からの角度オフセット（ラジアン）</param>
    /// <returns>計算された弾の進行方向ベクトル</returns>
    Tako::Vector3 CalculateBulletDirection(const Tako::Vector3& baseDirection, float angleOffset);

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float chargeTime_ = 0.8f;       ///< チャージ時間
    float recoveryTime_ = 0.5f;     ///< 硬直時間
    float firingDuration_ = 1.0f;   ///< 全体の発射時間（秒）
    float fireInterval_ = 0.0f;     ///< 発射間隔（OnInitialize で算出）
    float sweepAngle_ = 1.0472f;    ///< スイープ角度（約60度）
    int   bulletsPerSweep_ = 12;    ///< 1スイープあたりの弾数
    int   sweepCount_ = 2;          ///< スイープ回数
    float normalBulletSpeed_ = 40.0f;       ///< 通常弾速度
    float penetratingBulletSpeed_ = 15.0f;  ///< 貫通弾速度
    int   penetratingCount_ = 4;            ///< 1スイープあたりの貫通弾の数

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;    ///< 状態の総時間
    float timeSinceLastFire_ = 0.0f;
    int   currentSweep_ = 0;
    int   firedInSweep_ = 0;
    bool  hasEndedEffect_ = false;
    Tako::Vector3 baseDirection_;   ///< 発射基準方向

    //=========================================================================================
    // 演出
    //=========================================================================================
    BulletSignEffect bulletSignEffect_;
};
