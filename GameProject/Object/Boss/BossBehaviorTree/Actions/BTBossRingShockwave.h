#pragma once
#include "AttackNode.h"
#include "../../../../Collision/RingColliderGroup.h"
#include "Vector3.h"
#include <numbers>
#include <string>

class Boss;

/// <summary>
/// 半円リング衝撃波をボス前方へ発射する攻撃。予兆中はプレイヤーへ追従し、
/// 発射時に方向を固定して前進しながら初期スケールから最大スケールへ拡大する。
/// </summary>
class BTBossRingShockwave : public AttackNode {
private: //定数
    static constexpr float kBlinkAlphaMin       = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;
    static constexpr float kRingBaseAlpha       = 0.8f;
    static constexpr float kSweepRad            = std::numbers::pi_v<float>;  ///< 半円（メッシュの掃引角と一致）

public: //メンバー関数
    BTBossRingShockwave();
    virtual ~BTBossRingShockwave();

    //======================================
    //Setter
    //======================================
    void SetWarningTime(float time) { warningTime_ = time; }
    void SetMoveSpeed(float speed) { moveSpeed_ = speed; }
    void SetInitialScale(float scale) { initialScale_ = scale; }
    void SetMaxScale(float scale) { maxScale_ = scale; }
    void SetMaxDistance(float distance) { maxDistance_ = distance; }
    void SetDamage(float damage) { damage_ = damage; }
    void SetFadeTime(float time) { fadeTime_ = time; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    void SetColliderY(float y) { colliderY_ = y; }
    void SetColliderScale(float scale) { colliderScale_ = scale; }
    void SetSpawnHeight(float height) { spawnHeight_ = height; }
    void SetSpawnOffsetForward(float offset) { spawnOffsetForward_ = offset; }
    void SetSegmentCount(int count) { segmentCount_ = count; }
    void SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }
    void SetUvScrollU(float speed) { uvScrollU_ = speed; }
    void SetUvScrollV(float speed) { uvScrollV_ = speed; }

    //======================================
    //Getter
    //======================================
    float GetWarningTime() const { return warningTime_; }
    float GetMoveSpeed() const { return moveSpeed_; }
    float GetInitialScale() const { return initialScale_; }
    float GetMaxScale() const { return maxScale_; }
    float GetMaxDistance() const { return maxDistance_; }
    float GetDamage() const { return damage_; }
    float GetFadeTime() const { return fadeTime_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    float GetColliderY() const { return colliderY_; }
    float GetColliderScale() const { return colliderScale_; }
    float GetSpawnHeight() const { return spawnHeight_; }
    float GetSpawnOffsetForward() const { return spawnOffsetForward_; }
    int GetSegmentCount() const { return segmentCount_; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    float GetUvScrollU() const { return uvScrollU_; }
    float GetUvScrollV() const { return uvScrollV_; }

protected: //メンバー関数
    /// <summary>
    /// Warning → Travel → Fade → Recovery のフェーズ制御。
    /// Warning 中はプレイヤーへ追従し、Travel 突入時に方向を固定する。
    /// </summary>
    /// <returns>総時間到達で FinishAttack の結果、未到達は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// リングコライダー群の生成と初期方向・表示の準備
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
    /// 前進距離に応じた現在のリング外半径（initialScale → maxScale の線形補間）
    /// </summary>
    float ComputeCurrentScale() const;

    /// <summary>
    /// ボス所有のリングモデルへ現在の位置・向き・スケール・透明度を反映する
    /// </summary>
    void UpdateRingVisual(Boss* boss, float alpha);

private: //メンバー変数
    //パラメータ
    float warningTime_        = 1.0f;
    float moveSpeed_          = 45.0f;
    float initialScale_       = 15.0f;   ///< 発射時のリング外半径
    float maxScale_           = 60.0f;   ///< 最大到達時のリング外半径
    float maxDistance_        = 100.0f;  ///< 前進距離（到達で消滅開始）
    float damage_             = 10.0f;
    float fadeTime_           = 0.3f;
    float recoveryTime_       = 1.0f;
    float colliderY_          = -3.0f;  ///< 球コライダー中心の高さ
    float colliderScale_      = 0.1f;   ///< コライダー帯厚の倍率（帯中心は見た目のリングと一致）
    float spawnHeight_        = 0.1f;   ///< リングモデルの Y 座標
    float spawnOffsetForward_ = 0.0f;   ///< ボス前方への発生オフセット
    int   segmentCount_       = 16;     ///< 弧上の球コライダー数
    float blinkFrequency_     = 10.0f;
    float uvScrollU_          = 0.3f;   ///< テクスチャの円周方向スクロール速度（UV/秒）
    float uvScrollV_          = -1.2f;  ///< テクスチャの径方向スクロール速度（UV/秒）

    //ランタイム状態
    float       totalDuration_  = 0.0f;
    float       travelDuration_ = 0.0f;
    float       traveled_       = 0.0f;
    bool        hasLaunched_    = false;
    bool        hasEnded_       = false;
    std::string emitterName_;

    //発射時に固定する方向と原点
    Tako::Vector3 originPos_{};
    Tako::Vector3 lockedDir_{ 0.0f, 0.0f, 1.0f };
    float         lockedYaw_ = 0.0f;

    //コライダー
    RingColliderGroup ringColliders_;
};
