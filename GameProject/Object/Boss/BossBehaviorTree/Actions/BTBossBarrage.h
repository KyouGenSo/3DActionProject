#pragma once
#include "AttackNode.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの弾幕攻撃アクションノード
/// ステージ中央に移動し、周囲にランダムな方向でランダムな弾を一定時間撃ちまくる。
/// 通常弾（速い）と貫通弾（遅い）を混ぜて発射する。
/// </summary>
class BTBossBarrage : public AttackNode {
public:
    BTBossBarrage();
    virtual ~BTBossBarrage() = default;

    // パラメータ取得・設定
    float GetMoveDuration() const { return moveDuration_; }
    void  SetMoveDuration(float time) { moveDuration_ = time; }
    float GetChargeTime() const { return chargeTime_; }
    void  SetChargeTime(float time) { chargeTime_ = time; }
    float GetFiringDuration() const { return firingDuration_; }
    void  SetFiringDuration(float duration) { firingDuration_ = duration; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    float GetFireInterval() const { return fireInterval_; }
    void  SetFireInterval(float interval) { fireInterval_ = interval; }
    float GetNormalBulletSpeedMin() const { return normalBulletSpeedMin_; }
    void  SetNormalBulletSpeedMin(float speed) { normalBulletSpeedMin_ = speed; }
    float GetNormalBulletSpeedMax() const { return normalBulletSpeedMax_; }
    void  SetNormalBulletSpeedMax(float speed) { normalBulletSpeedMax_ = speed; }
    float GetPenetratingBulletSpeedMin() const { return penetratingBulletSpeedMin_; }
    void  SetPenetratingBulletSpeedMin(float speed) { penetratingBulletSpeedMin_ = speed; }
    float GetPenetratingBulletSpeedMax() const { return penetratingBulletSpeedMax_; }
    void  SetPenetratingBulletSpeedMax(float speed) { penetratingBulletSpeedMax_ = speed; }
    float GetPenetratingRatio() const { return penetratingRatio_; }
    void  SetPenetratingRatio(float ratio) { penetratingRatio_ = ratio; }

protected:
    /// <summary>
    /// 固有攻撃ロジック本体（中央移動 → チャージ → 弾幕発射 → 硬直の制御）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    /// <returns>Tako::BTNodeStatus::Running（攻撃継続中） / Tako::BTNodeStatus::Success（攻撃完了）</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理（totalDuration の算出、開始/目標位置の確定、チャージエフェクト起動）
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
    static constexpr float kEasingCoeffA = 3.0f;
    static constexpr float kEasingCoeffB = 2.0f;

    /// <summary>
    /// 移動フェーズの更新処理（startPosition_ から targetPosition_ へイージング移動）
    /// </summary>
    /// <param name="boss">移動対象のBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    void UpdateMove(Boss* boss, float deltaTime);

    /// <summary>
    /// 通常弾/貫通弾を penetratingRatio_ に応じてランダムに選択し、ランダムな水平方向へ1発発射
    /// </summary>
    /// <param name="boss">弾を発射するBossへのポインタ</param>
    void FireRandomBullet(Boss* boss);

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float moveDuration_ = 0.5f;
    float chargeTime_ = 0.8f;
    float firingDuration_ = 3.0f;
    float recoveryTime_ = 0.5f;
    float fireInterval_ = 0.08f;

    float normalBulletSpeedMin_ = 20.0f;
    float normalBulletSpeedMax_ = 35.0f;
    float penetratingBulletSpeedMin_ = 10.0f;
    float penetratingBulletSpeedMax_ = 20.0f;
    float penetratingRatio_ = 0.3f;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;
    float timeSinceLastFire_ = 0.0f;
    bool  hasEndedEffect_ = false;
    Tako::Vector3 startPosition_;
    Tako::Vector3 targetPosition_;

    //=========================================================================================
    // 演出
    //=========================================================================================
    BulletSignEffect bulletSignEffect_;
};
