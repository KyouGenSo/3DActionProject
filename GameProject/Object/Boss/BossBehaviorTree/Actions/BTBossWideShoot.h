#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// 角度をスイープしながら連射する大範囲射撃。通常弾（速い）と貫通弾（遅い）を混ぜて発射する。
/// </summary>
class BTBossWideShoot : public AttackNode {
private: //定数
    static constexpr float kDirectionEpsilon = 0.001f;
    static constexpr float kAngleEpsilon = 0.0001f;

public: //メンバー関数
    BTBossWideShoot();
    virtual ~BTBossWideShoot() = default;

    //==========================================
    //Setter
    //==========================================
    void SetChargeTime(float time) { chargeTime_ = time; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    void SetFiringDuration(float duration) { firingDuration_ = duration; }
    void SetSweepAngle(float angle) { sweepAngle_ = angle; }
    void SetBulletsPerSweep(int count) { bulletsPerSweep_ = count; }
    void SetSweepCount(int count) { sweepCount_ = count; }
    void SetNormalBulletSpeed(float speed) { normalBulletSpeed_ = speed; }
    void SetPenetratingBulletSpeed(float speed) { penetratingBulletSpeed_ = speed; }
    void SetPenetratingCount(int count) { penetratingCount_ = count; }

    //==========================================
    //Getter
    //==========================================
    float GetChargeTime() const { return chargeTime_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    float GetFiringDuration() const { return firingDuration_; }
    float GetSweepAngle() const { return sweepAngle_; }
    int   GetBulletsPerSweep() const { return bulletsPerSweep_; }
    int   GetSweepCount() const { return sweepCount_; }
    float GetNormalBulletSpeed() const { return normalBulletSpeed_; }
    float GetPenetratingBulletSpeed() const { return penetratingBulletSpeed_; }
    int   GetPenetratingCount() const { return penetratingCount_; }

protected: //メンバー関数
    /// <summary>
    /// チャージ→角度スイープ連射→硬直のフェーズ制御。総時間経過で終了。
    /// </summary>
    /// <returns>総時間到達で FinishAttack の結果、未到達は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;

#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private: //非公開関数
    void AimAtPlayer(Tako::BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 現在のスイープ角・弾種で 1 発生成する。貫通弾なら貫通弾、それ以外は通常弾。
    /// </summary>
    void FireBullet(Boss* boss);

    /// <summary>
    /// 現在の発射順に対応するスイープ角オフセット。
    /// </summary>
    /// <returns>角度（ラジアン）。currentSweep_ が奇数のスイープは符号反転</returns>
    float GetCurrentAngleOffset() const;

    /// <summary>
    /// 現在の発射が貫通弾かを判定する（スイープ内で等間隔に間引き）。
    /// </summary>
    /// <returns>貫通弾なら true</returns>
    bool  IsPenetratingBullet() const;

    /// <summary>
    /// 基準方向を XZ 平面で回転させた発射方向を返す。
    /// </summary>
    /// <param name="baseDirection">基準方向（正規化済み）</param>
    /// <param name="angleOffset">回転角（ラジアン）</param>
    /// <returns>正規化された発射方向</returns>
    Tako::Vector3 CalculateBulletDirection(const Tako::Vector3& baseDirection, float angleOffset);

private: //メンバー変数
    //パラメータ
    float chargeTime_             = 0.8f;
    float recoveryTime_           = 0.5f;
    float firingDuration_         = 1.0f;     ///< 全体の発射時間（秒）
    float fireInterval_           = 0.0f;     ///< OnInitialize で算出
    float sweepAngle_             = 1.0472f;  ///< ラジアン、約 60 度
    int   bulletsPerSweep_        = 12;
    int   sweepCount_             = 2;
    float normalBulletSpeed_      = 40.0f;
    float penetratingBulletSpeed_ = 15.0f;
    int   penetratingCount_       = 4;        ///< 1スイープあたり

    //ランタイム状態
    float         totalDuration_     = 0.0f;
    float         timeSinceLastFire_ = 0.0f;
    int           currentSweep_      = 0;
    int           firedInSweep_      = 0;
    bool          hasEndedEffect_    = false;
    Tako::Vector3 baseDirection_;

    //演出
    BulletSignEffect bulletSignEffect_;
};
