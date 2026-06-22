#pragma once
#include "AttackNode.h"
#include "../../../../Collision/BossAreaAttackCollider.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <string>

class Boss;

/// <summary>
/// ボス→プレイヤーへ伸びる矩形 Decal で予告し、発生まで追従して
/// その線に沿ってレーザー（Box エミッター）を撃つ遠距離攻撃。
/// 発生時にデカール追従を止め、ビームを凍結する。
/// </summary>
class BTBossTrackingLaser : public AttackNode {
private: //定数
    static constexpr float kDecalBaseAlpha = 0.3f;
    static constexpr float kBlinkAlphaMin = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

public: //メンバー関数
    BTBossTrackingLaser();
    virtual ~BTBossTrackingLaser();

    //==================================
    //Setter
    //==================================
    void SetAimDuration(float time) { aimDuration_ = time; }
    void SetBlinkDuration(float time) { blinkDuration_ = time; }
    void SetAttackDuration(float time) { attackDuration_ = time; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    void SetBeamWidth(float width) { beamWidth_ = width; }
    void SetEndOffset(float offset) { endOffset_ = offset; }
    void SetBeamHeight(float height) { beamHeight_ = height; }
    void SetColliderHeight(float height) { colliderHeight_ = height; }
    void SetLaserEmitDuration(float time) { laserEmitDuration_ = time; }
    void SetDamage(float damage) { damage_ = damage; }
    void SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }

    //==================================
    //Getter
    //==================================
    float GetAimDuration() const { return aimDuration_; }
    float GetBlinkDuration() const { return blinkDuration_; }
    float GetAttackDuration() const { return attackDuration_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    float GetBeamWidth() const { return beamWidth_; }
    float GetEndOffset() const { return endOffset_; }
    float GetBeamHeight() const { return beamHeight_; }
    float GetColliderHeight() const { return colliderHeight_; }
    float GetLaserEmitDuration() const { return laserEmitDuration_; }
    float GetDamage() const { return damage_; }
    float GetBlinkFrequency() const { return blinkFrequency_; }

protected: //メンバー関数
    /// <summary>
    /// Aim → Blink → Attack → Recovery のフェーズ制御。
    /// Aim/Blink 中はプレイヤーへ追従し、Attack でビームを凍結する。
    /// </summary>
    /// <returns>総時間到達で FinishAttack の結果、未到達は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// Decal・OBB コライダー・レーザープリセットを準備する
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
    /// ボス→プレイヤー方向からビームの中心・yaw（ラジアン）・全長を求める。
    /// 全長はボス-プレイヤー距離に endOffset_ を加えた値。
    /// </summary>
    /// <returns>水平距離が閾値以下で false（縮退）</returns>
    bool ComputeBeam(Boss* boss, Tako::BTBlackboard* blackboard,
                     Tako::Vector3& outCenter, float& outYawRad, float& outLength) const;

    /// <summary>
    /// 算出済みのビーム形状を Decal に反映する
    /// </summary>
    void ApplyBeamToDecal(const Tako::Vector3& center, float yawRad, float length);

    /// <summary>
    /// Decal の透明度をサインカーブで点滅させる
    /// </summary>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// ビームを凍結し OBB とレーザーエミッターを起動する
    /// </summary>
    void BeginAttackPhase(Boss* boss);

    /// <summary>
    /// OBB 無効化とエミッター停止
    /// </summary>
    void EndAttackPhase(Boss* boss);

private: //メンバー変数
    //パラメータ
    float aimDuration_       = 1.0f;
    float blinkDuration_     = 0.6f;
    float attackDuration_    = 0.25f;
    float recoveryTime_      = 0.5f;
    float beamWidth_         = 2.0f;
    float endOffset_         = 3.0f;
    float beamHeight_        = 1.0f;
    float colliderHeight_    = 3.0f;
    float laserEmitDuration_ = 0.05f;
    float damage_            = 15.0f;
    float blinkFrequency_    = 10.0f;

    //ランタイム状態
    float totalDuration_  = 0.0f;
    bool  hasBegunAttack_ = false;
    bool  hasEndedAttack_ = false;
    bool  hasStoppedEmit_ = false;

    //発生時に凍結するビーム形状
    Tako::Vector3 firedCenter_{};
    float         firedYawRad_ = 0.0f;
    float         firedLength_ = 0.0f;

    //Decal / コライダー / パーティクル
    std::unique_ptr<Tako::Decal>             beamDecal_;
    std::unique_ptr<BossAreaAttackCollider>  beamCollider_;
    Tako::Transform                          colliderTransform_;
    bool                                     particleInitialized_ = false;
    std::string                              emitterName_;
    Tako::Vector3                            presetBoxSize_{};
};
