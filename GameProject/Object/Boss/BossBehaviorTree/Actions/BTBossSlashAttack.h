#pragma once
#include "AttackNode.h"
#include "../../../../Collision/MeteorImpactCollider.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <string>

class Boss;

/// <summary>
/// プレイヤー位置を狙い、円形 Decal で範囲を予告してから斬撃を発動する遠距離攻撃。
/// </summary>
class BTBossSlashAttack : public AttackNode {
public:
    BTBossSlashAttack();
    virtual ~BTBossSlashAttack();

    // パラメータ取得・設定
    float GetWarningDuration() const { return warningDuration_; }
    void  SetWarningDuration(float time) { warningDuration_ = time; }
    float GetBlinkDuration() const { return blinkDuration_; }
    void  SetBlinkDuration(float time) { blinkDuration_ = time; }
    float GetAttackDuration() const { return attackDuration_; }
    void  SetAttackDuration(float time) { attackDuration_ = time; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    float GetAttackRadius() const { return attackRadius_; }
    void  SetAttackRadius(float radius) { attackRadius_ = radius; }
    float GetDamage() const { return damage_; }
    void  SetDamage(float damage) { damage_ = damage; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    void  SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }

protected:
    /// <summary>
    /// Warning → Blinking → Attack → Recovery のフェーズ制御
    /// </summary>
    /// <returns>総時間到達で FinishAttack の結果、未到達は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 着弾点を確定し Decal とコライダーを準備する
    /// </summary>
    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnCleanup() override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>
    /// Decal の透明度をサインカーブで点滅させる
    /// </summary>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// コライダー有効化と斬撃パーティクル起動
    /// </summary>
    void BeginAttackPhase(Boss* boss);

    /// <summary>
    /// コライダー無効化と Decal 非表示化
    /// </summary>
    void EndAttackPhase(Boss* boss);

    static constexpr float kDecalBaseAlpha = 0.3f;
    static constexpr float kBlinkAlphaMin = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float warningDuration_ = 1.0f;
    float blinkDuration_ = 0.8f;
    float attackDuration_ = 0.3f;
    float recoveryTime_ = 0.5f;
    float attackRadius_ = 8.0f;
    float damage_ = 15.0f;
    float blinkFrequency_ = 10.0f;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;
    bool  hasBegunAttack_ = false;
    bool  hasEndedAttack_ = false;

    //=========================================================================================
    // Decal / コライダー / パーティクル
    //=========================================================================================
    std::unique_ptr<Tako::Decal> slashDecal_;
    std::unique_ptr<MeteorImpactCollider> slashCollider_;
    Tako::Transform colliderTransform_;
    bool particleInitialized_ = false;
    std::string emitterName_;
};
