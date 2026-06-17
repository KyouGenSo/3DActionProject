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
private: //定数
    static constexpr float kDecalBaseAlpha = 0.3f;
    static constexpr float kBlinkAlphaMin = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

public: //メンバー関数
    BTBossSlashAttack();
    virtual ~BTBossSlashAttack();

    //==================================
    //Setter
    //==================================
    void SetWarningDuration(float time) { warningDuration_ = time; }
    void SetBlinkDuration(float time) { blinkDuration_ = time; }
    void SetAttackDuration(float time) { attackDuration_ = time; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    void SetAttackRadius(float radius) { attackRadius_ = radius; }
    void SetDamage(float damage) { damage_ = damage; }
    void SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }

    //==================================
    //Getter
    //==================================
    float GetWarningDuration() const { return warningDuration_; }
    float GetBlinkDuration() const { return blinkDuration_; }
    float GetAttackDuration() const { return attackDuration_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    float GetAttackRadius() const { return attackRadius_; }
    float GetDamage() const { return damage_; }
    float GetBlinkFrequency() const { return blinkFrequency_; }

protected: //メンバー関数
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

private: //非公開関数
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

private: //メンバー変数
    //パラメータ
    float warningDuration_ = 1.0f;
    float blinkDuration_   = 0.8f;
    float attackDuration_  = 0.3f;
    float recoveryTime_    = 0.5f;
    float attackRadius_    = 8.0f;
    float damage_          = 15.0f;
    float blinkFrequency_  = 10.0f;

    //ランタイム状態
    float totalDuration_  = 0.0f;
    bool  hasBegunAttack_ = false;
    bool  hasEndedAttack_ = false;

    //Decal / コライダー / パーティクル
    std::unique_ptr<Tako::Decal>          slashDecal_;
    std::unique_ptr<MeteorImpactCollider> slashCollider_;
    Tako::Transform                       colliderTransform_;
    bool                                  particleInitialized_ = false;
    std::string                           emitterName_;
};
