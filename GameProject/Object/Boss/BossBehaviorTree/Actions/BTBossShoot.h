#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの射撃アクションノード（チャージ → 扇状複数発射 → 硬直）
/// </summary>
class BTBossShoot : public AttackNode {
private:
    static constexpr float kDirectionEpsilon = 0.01f;
    static constexpr float kAngleEpsilon = 0.001f;

public:
    BTBossShoot();
    virtual ~BTBossShoot() = default;

    /// パラメータ取得・設定
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
    /// <summary>攻撃ロジック本体（AttackNode が初回 Execute 時の準備後に呼び出す）</summary>
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>初回実行時の独自初期化（totalDuration 計算と予兆エフェクト起動）</summary>
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;

    /// <summary>派生固有 JSON 適用（共通キーは AttackNode が処理済み）</summary>
    void OnApplyParameters(const nlohmann::json& params) override;

    /// <summary>派生固有 JSON 抽出（out にキーを追加）</summary>
    void OnExtractParameters(nlohmann::json& out) const override;

#ifdef _DEBUG
    /// <summary>派生固有 ImGui パラメータ編集 UI</summary>
    bool OnDrawImGui() override;
#endif

private:
    void AimAtPlayer(BTBlackboard* blackboard, float deltaTime);
    void FireBullets(BTBlackboard* blackboard);
    Tako::Vector3 CalculateBulletDirection(const Tako::Vector3& baseDirection, float angleOffset);

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float chargeTime_ = 0.9f;        ///< 射撃前の準備時間
    float recoveryTime_ = 0.5f;      ///< 射撃後の硬直時間
    float totalDuration_ = 1.0f;     ///< 状態の総時間（OnInitialize で算出）
    float bulletSpeed_ = 20.0f;      ///< 弾の速度
    float spreadAngle_ = 0.2618f;    ///< 扇状発射の角度（ラジアン、約 15 度）
    int   bulletCount_ = 3;          ///< 発射する弾数

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    bool hasFired_ = false;          ///< 弾が発射済みか

    //=========================================================================================
    // 演出
    //=========================================================================================
    BulletSignEffect bulletSignEffect_;
};
