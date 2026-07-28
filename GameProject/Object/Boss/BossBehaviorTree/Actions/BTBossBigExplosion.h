#pragma once
#include "AttackNode.h"
#include "../../../../Collision/MeteorImpactCollider.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>

class Boss;

/// <summary>
/// プレイヤー位置に円形 Decal で範囲を予告し、上空から落下する爆発体が
/// 着地した瞬間に爆発バースト・ダメージ判定・白黒フラッシュを同時発生させる攻撃。
/// </summary>
class BTBossBigExplosion : public AttackNode {
public: //定数
    static constexpr const char* kEmitterFallFlash   = "boss_big_explosion_1";
    static constexpr const char* kEmitterImpactSpark = "boss_big_explosion_2";
    static constexpr const char* kEmitterFallFire    = "boss_big_explosion_3";
    static constexpr const char* kEmitterImpactSmoke = "boss_big_explosion_4";

private: //定数
    static constexpr float kDecalBaseAlpha      = 0.3f;
    static constexpr float kBlinkAlphaMin       = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;
    static constexpr float kFallTimeoutMargin   = 1.0f;   ///< 落下タイムアウトの余裕秒

private: //構造体
    enum class Phase { Warning, Blinking, Falling, Impact, Recovery };

public: //メンバー関数
    BTBossBigExplosion();
    virtual ~BTBossBigExplosion();

    //======================================
    //Setter
    //======================================
    void SetWarningDuration(float time) { warningDuration_ = time; }
    void SetBlinkDuration(float time) { blinkDuration_ = time; }
    void SetImpactDuration(float time) { impactDuration_ = time; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    void SetSpawnHeight(float height) { spawnHeight_ = height; }
    void SetFallSpeed(float speed) { fallSpeed_ = speed; }
    void SetAttackRadius(float radius) { attackRadius_ = radius; }
    void SetDamage(float damage) { damage_ = damage; }
    void SetColliderY(float y) { colliderY_ = y; }
    void SetColliderActiveDuration(float time) { colliderActiveDuration_ = time; }
    void SetBurstDuration(float time) { burstDuration_ = time; }
    void SetImpactYOffset(float offset) { impactYOffset_ = offset; }
    void SetSyncEmitterRadius(bool sync) { syncEmitterRadius_ = sync; }
    void SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }
    void SetBwEnabled(bool enabled) { bwEnabled_ = enabled; }
    void SetBwDuration(float time) { bwDuration_ = time; }
    void SetBwThreshold(float threshold) { bwThreshold_ = threshold; }

    //======================================
    //Getter
    //======================================
    float GetWarningDuration() const { return warningDuration_; }
    float GetBlinkDuration() const { return blinkDuration_; }
    float GetImpactDuration() const { return impactDuration_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    float GetSpawnHeight() const { return spawnHeight_; }
    float GetFallSpeed() const { return fallSpeed_; }
    float GetAttackRadius() const { return attackRadius_; }
    float GetDamage() const { return damage_; }
    float GetColliderY() const { return colliderY_; }
    float GetColliderActiveDuration() const { return colliderActiveDuration_; }
    float GetBurstDuration() const { return burstDuration_; }
    float GetImpactYOffset() const { return impactYOffset_; }
    bool GetSyncEmitterRadius() const { return syncEmitterRadius_; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    bool GetBwEnabled() const { return bwEnabled_; }
    float GetBwDuration() const { return bwDuration_; }
    float GetBwThreshold() const { return bwThreshold_; }

protected: //メンバー関数
    /// <summary>
    /// Warning → Blinking → Falling → Impact → Recovery のフェーズ制御。
    /// Falling は位置駆動で、落下エミッタが地面 (y=0) に達した瞬間 Impact へ移る。
    /// </summary>
    /// <returns>Recovery 満了で FinishAttack の結果、未到達は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 着弾点を発動時のプレイヤー位置に固定し、Decal とコライダーを準備する
    /// </summary>
    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnCleanup() override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;

#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private: //非公開関数
    /// <summary>
    /// Decal の透明度をサインカーブで点滅させる
    /// </summary>
    void UpdateBlink(float deltaTime);

    /// <summary>
    /// 落下エミッタを上空の着弾点直上へ配置して射出を開始する
    /// </summary>
    void BeginFalling();

    /// <summary>
    /// 落下エフェクトを消し、爆発バースト・コライダー・白黒フラッシュを同一フレームで起動する
    /// </summary>
    void TriggerImpact();

    void EndBurst();

    void CloseCollider();

    /// <summary>
    /// 射出タイマーを周期満了へ進め、有効化直後のフレームで必ず射出させる
    /// </summary>
    void ForceImmediateEmit(const char* emitterName);

    void AdvancePhase(Phase next);

private: //メンバー変数
    //パラメータ
    float warningDuration_        = 0.0f;
    float blinkDuration_          = 0.0f;
    float impactDuration_         = 0.8f;
    float recoveryTime_           = 1.5f;
    float spawnHeight_            = 70.0f;  ///< 落下エミッタの発生高度
    float fallSpeed_              = 30.0f;
    float attackRadius_           = 50.0f;
    float damage_                 = 30.0f;
    float colliderY_              = 1.0f;   ///< 球コライダー中心の高さ
    float colliderActiveDuration_ = 0.2f;
    float burstDuration_          = 0.1f;   ///< 着地バーストエミッタの有効時間
    float impactYOffset_          = 10.0f;  ///< 着地バーストエミッタの高さ
    bool  syncEmitterRadius_      = false;  ///< ON で球エミッタ半径を attackRadius_ に同期（エディタ調整値を上書き）
    float blinkFrequency_         = 10.0f;
    bool  bwEnabled_              = true;
    float bwDuration_             = 0.22f;
    float bwThreshold_            = 0.02f;  ///< 白黒2値化の輝度しきい値

    //ランタイム状態
    Phase         phase_          = Phase::Warning;
    float         phaseTimer_     = 0.0f;
    float         blinkClock_     = 0.0f;
    float         fallHeight_     = 0.0f;
    bool          burstEnded_     = false;
    bool          colliderClosed_ = false;
    Tako::Vector3 targetPos_{};

    //Decal / コライダー
    std::unique_ptr<Tako::Decal>          decal_;
    std::unique_ptr<MeteorImpactCollider> collider_;
    Tako::Transform                       colliderTransform_;
};
