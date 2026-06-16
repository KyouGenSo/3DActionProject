#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの連続追尾射撃アクションノード
/// プレイヤー方向に連続で弾を発射する攻撃パターン。発射中もプレイヤー方向を追尾し続ける。
/// </summary>
class BTBossRapidFire : public AttackNode {
public:
    BTBossRapidFire();
    virtual ~BTBossRapidFire() = default;

    // パラメータ取得・設定
    float GetChargeTime() const { return chargeTime_; }
    void  SetChargeTime(float time) { chargeTime_ = time; }
    int   GetBulletCount() const { return bulletCount_; }
    void  SetBulletCount(int count) { bulletCount_ = count; }
    float GetFireInterval() const { return fireInterval_; }
    void  SetFireInterval(float interval) { fireInterval_ = interval; }
    float GetBulletSpeed() const { return bulletSpeed_; }
    void  SetBulletSpeed(float speed) { bulletSpeed_ = speed; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }

protected:
    /// <summary>
    /// 固有攻撃ロジック本体（チャージ → プレイヤー追尾しながら連続発射 → 硬直の制御）
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
    /// プレイヤー方向に向けて弾を1発生成・発射する
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    void FireBullet(Tako::BTBlackboard* blackboard);

    /// <summary>
    /// 現在のボス位置からプレイヤー位置への正規化済み方向ベクトルを算出する
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <returns>ボスからプレイヤーへの正規化方向ベクトル</returns>
    Tako::Vector3 CalculateDirectionToPlayer(Tako::BTBlackboard* blackboard);

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float chargeTime_ = 0.9f;        ///< 射撃前の準備時間
    int   bulletCount_ = 5;          ///< 発射する弾の数
    float fireInterval_ = 0.15f;     ///< 発射間隔（秒）
    float recoveryTime_ = 0.5f;      ///< 射撃後の硬直時間
    float bulletSpeed_ = 20.0f;      ///< 弾の速度

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;     ///< 状態の総時間（OnInitialize で算出）
    int   firedCount_ = 0;           ///< 発射済み弾数
    float timeSinceLastFire_ = 0.0f; ///< 前回発射からの経過時間

    //=========================================================================================
    // 演出
    //=========================================================================================
    BulletSignEffect bulletSignEffect_;
};
