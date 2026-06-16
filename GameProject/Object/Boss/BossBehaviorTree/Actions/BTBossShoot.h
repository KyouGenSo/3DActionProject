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
    /// <summary>
    /// 固有攻撃ロジック本体（チャージ → 扇状複数発射 → 硬直の制御）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    /// <returns>Tako::BTNodeStatus::Running（攻撃継続中） / Tako::BTNodeStatus::Success（攻撃完了）</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理（totalDuration の算出と予兆エフェクト起動）
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
    /// <summary>
    /// 扇状に bulletCount_ 発の弾を一斉発射する
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    void FireBullets(Tako::BTBlackboard* blackboard);

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
