#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// プレイヤー方向へ扇状に複数弾を一斉発射する。
/// </summary>
class BTBossShoot : public AttackNode {
private:
    static constexpr float kDirectionEpsilon = 0.01f;
    static constexpr float kAngleEpsilon = 0.001f;

public:
    BTBossShoot();
    virtual ~BTBossShoot() = default;

    [[nodiscard]] float GetChargeTime() const { return chargeTime_; }
    void SetChargeTime(float time) { chargeTime_ = time; }

    [[nodiscard]] float GetBulletSpeed() const { return bulletSpeed_; }
    void SetBulletSpeed(float speed) { bulletSpeed_ = speed; }

    [[nodiscard]] float GetSpreadAngle() const { return spreadAngle_; }
    void SetSpreadAngle(float angle) { spreadAngle_ = angle; }

    [[nodiscard]] float GetRecoveryTime() const { return recoveryTime_; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }

    [[nodiscard]] int GetBulletCount() const { return bulletCount_; }
    void SetBulletCount(int count) { bulletCount_ = count; }

protected:
    /// <summary>
    /// チャージ後に一斉発射し硬直へ。総時間経過で終了。
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
    /// プレイヤー方向を中心に bulletCount_ 発を扇状に一斉発射する。
    /// </summary>
    void FireBullets(Tako::BTBlackboard* blackboard);

    /// <summary>
    /// 基準方向を XZ 平面で回転させた発射方向を返す。
    /// </summary>
    /// <param name="baseDirection">基準方向（正規化済み）</param>
    /// <param name="angleOffset">回転角（ラジアン）</param>
    /// <returns>正規化された発射方向</returns>
    Tako::Vector3 CalculateBulletDirection(const Tako::Vector3& baseDirection, float angleOffset);

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float chargeTime_ = 0.9f;
    float recoveryTime_ = 0.5f;
    float totalDuration_ = 1.0f;     ///< OnInitialize で算出
    float bulletSpeed_ = 20.0f;
    float spreadAngle_ = 0.2618f;    ///< ラジアン、約 15 度
    int   bulletCount_ = 3;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    bool hasFired_ = false;

    //=========================================================================================
    // 演出
    //=========================================================================================
    BulletSignEffect bulletSignEffect_;
};
