#pragma once
#include "AttackNode.h"
#include "../../../../Collision/MeteorImpactCollider.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <vector>

class Boss;
class Player;

/// <summary>
/// ステージ全域のランダム位置へ予兆デカール付きの弾を間隔スケジュールで降らせる弾雨攻撃。
/// 弾ごとにライフサイクルが独立し、複数弾の予兆・落下がオーバーラップする。
/// </summary>
class BTBossBulletRain : public AttackNode {
private: //定数
    static constexpr int   kMaxPoolSize         = 50;  ///< 同時進行スロット数の上限
    static constexpr int   kMaxPlacementRetries = 30;
    static constexpr float kDecalBaseAlpha      = 0.3f;
    static constexpr float kBlinkAlphaMin       = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;
    static constexpr float kDecalFallingAlpha   = 0.6f;   ///< 落下中の点滅停止・確定表示アルファ

private: //構造体
    enum class BulletState { Waiting, Warning, Blinking, Falling, Impact, Done };

    struct Bullet {
        BulletState   state       = BulletState::Waiting;
        float         stateTimer  = 0.0f;                  ///< 現在ステートの経過秒
        Tako::Vector3 impactPos{};                         ///< 着弾点 (y=0)
    };

public: //メンバー関数
    BTBossBulletRain();
    virtual ~BTBossBulletRain();

    //=======================================
    //Setter
    //=======================================
    void  SetChargeTime(float time) { chargeTime_ = time; }
    void  SetRainDuration(float duration) { rainDuration_ = duration; }
    void  SetSpawnInterval(float interval) { spawnInterval_ = interval; }
    void  SetWarningDuration(float time) { warningDuration_ = time; }
    void  SetBlinkDuration(float time) { blinkDuration_ = time; }
    void  SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }
    void  SetFallHeight(float height) { fallHeight_ = height; }
    void  SetFallSpeed(float speed) { fallSpeed_ = speed; }
    void  SetImpactRadius(float radius) { impactRadius_ = radius; }
    void  SetImpactActiveDuration(float time) { impactActiveDuration_ = time; }
    void  SetColliderY(float y) { colliderY_ = y; }
    void  SetDamage(float damage) { damage_ = damage; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    void  SetPlayerAimProbability(float probability) { playerAimProbability_ = probability; }
    void  SetPlayerAimRadius(float radius) { playerAimRadius_ = radius; }
    void  SetStageMargin(float margin) { stageMargin_ = margin; }
    void  SetMinSeparation(float separation) { minSeparation_ = separation; }

    //=======================================
    //Getter
    //=======================================
    float GetChargeTime() const { return chargeTime_; }
    float GetRainDuration() const { return rainDuration_; }
    float GetSpawnInterval() const { return spawnInterval_; }
    float GetWarningDuration() const { return warningDuration_; }
    float GetBlinkDuration() const { return blinkDuration_; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    float GetFallHeight() const { return fallHeight_; }
    float GetFallSpeed() const { return fallSpeed_; }
    float GetImpactRadius() const { return impactRadius_; }
    float GetImpactActiveDuration() const { return impactActiveDuration_; }
    float GetColliderY() const { return colliderY_; }
    float GetDamage() const { return damage_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    float GetPlayerAimProbability() const { return playerAimProbability_; }
    float GetPlayerAimRadius() const { return playerAimRadius_; }
    float GetStageMargin() const { return stageMargin_; }
    float GetMinSeparation() const { return minSeparation_; }

protected: //非公開関数
    /// <summary>
    /// チャージ後、spawnInterval_ ごとに弾のライフサイクルを開始し全弾を更新する
    /// </summary>
    /// <param name="blackboard">プレイヤー狙い抽選に使用</param>
    /// <param name="boss">落下弾を発射するボス</param>
    /// <param name="deltaTime">前フレームからの経過秒</param>
    /// <returns>全弾終了と硬直完了で Success、進行中は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnCleanup() override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private: //非公開関数
    /// <summary>
    /// 再利用可能なスロット（Waiting / Done）を探す
    /// </summary>
    /// <returns>空きスロットのインデックス。全スロット進行中なら -1</returns>
    int FindFreeSlot() const;

    /// <summary>
    /// 着弾位置を抽選して弾を Warning 状態にし、Decal とコライダーを配置する
    /// </summary>
    /// <param name="index">開始するスロットのインデックス</param>
    /// <param name="blackboard">プレイヤー位置の取得元</param>
    void ActivateBullet(int index, Tako::BTBlackboard* blackboard);

    /// <summary>
    /// ステージ内の着弾位置を抽選（playerAimProbability_ の確率でプレイヤー付近を狙う）
    /// </summary>
    /// <param name="player">狙い弾の目標。nullptr なら常に一様ランダム</param>
    /// <returns>着弾位置 (y=0)</returns>
    Tako::Vector3 SampleImpactPosition(Player* player);

    /// <summary>
    /// 弾1発のステートマシンを進行させる
    /// </summary>
    /// <param name="index">更新する弾のインデックス</param>
    /// <param name="boss">落下弾を発射するボス</param>
    /// <param name="deltaTime">前フレームからの経過秒</param>
    void UpdateBullet(int index, Boss* boss, float deltaTime);

private: //メンバー変数
    //パラメータ
    float chargeTime_           = 0.0f;
    float rainDuration_         = 10.0f;
    float spawnInterval_        = 0.1f;
    float warningDuration_      = 0.5f;
    float blinkDuration_        = 0.5f;
    float blinkFrequency_       = 10.0f;
    float fallHeight_           = 40.0f;
    float fallSpeed_            = 100.0f;
    float impactRadius_         = 3.5f;
    float impactActiveDuration_ = 0.15f;
    float colliderY_            = 1.0f;
    float damage_               = 10.0f;
    float recoveryTime_         = 2.0f;
    float playerAimProbability_ = 0.1f;
    float playerAimRadius_      = 10.0f;
    float stageMargin_          = 5.0f;
    float minSeparation_        = 10.0f;

    //ランタイム状態
    float            totalDuration_    = 0.0f;
    float            recoveryStart_    = 0.0f;
    float            fallTime_         = 0.0f;
    int              totalSpawnCount_  = 0;
    int              poolSize_         = 0;
    int              nextSpawnIndex_   = 0;
    BulletSignEffect bulletSignEffect_;

    //スロットプール
    std::vector<Bullet>                                bullets_;
    std::vector<std::unique_ptr<Tako::Decal>>          decals_;
    std::vector<std::unique_ptr<MeteorImpactCollider>> colliders_;
    std::vector<Tako::Transform>                       colliderTransforms_;
};
