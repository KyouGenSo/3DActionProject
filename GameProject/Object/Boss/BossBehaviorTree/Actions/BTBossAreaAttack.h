#pragma once
#include "AttackNode.h"
#include "../../../../Collision/BossAreaAttackCollider.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <array>
#include <string>

class Boss;

/// <summary>
/// フェーズ2エリアを4象限に分割し、ランダムに1〜3象限を攻撃する。
/// プレイヤーのいる象限は確定で含める。
/// </summary>
class BTBossAreaAttack : public AttackNode {
public:
    BTBossAreaAttack();
    virtual ~BTBossAreaAttack();

    // パラメータ取得・設定
    float GetWarningDuration() const { return warningDuration_; }
    void  SetWarningDuration(float time) { warningDuration_ = time; }
    float GetBlinkDuration() const { return blinkDuration_; }
    void  SetBlinkDuration(float time) { blinkDuration_ = time; }
    float GetAttackDuration() const { return attackDuration_; }
    void  SetAttackDuration(float time) { attackDuration_ = time; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    int   GetMinQuadrants() const { return minQuadrants_; }
    void  SetMinQuadrants(int count) { minQuadrants_ = count; }
    int   GetMaxQuadrants() const { return maxQuadrants_; }
    void  SetMaxQuadrants(int count) { maxQuadrants_ = count; }
    float GetDamage() const { return damage_; }
    void  SetDamage(float damage) { damage_ = damage; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    void  SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }

private:
    /// <summary>
    /// Warning → Blinking → Attack → Recovery のフェーズを進行させる
    /// </summary>
    /// <param name="blackboard">未使用</param>
    /// <param name="boss">攻撃させるボス</param>
    /// <param name="deltaTime">前フレームからの経過秒</param>
    /// <returns>全フェーズ完了で Success、進行中は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnCleanup() override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>
    /// 攻撃象限をランダム選択して activeQuadrants_ を更新（プレイヤー象限は確定）
    /// </summary>
    void SelectRandomQuadrants(Tako::BTBlackboard* blackboard);

    /// <summary>
    /// プレイヤーがボス基準でどの象限にいるか判定する
    /// </summary>
    /// <param name="blackboard">boss / player ポインタを保持する共有ストレージ</param>
    /// <returns>象限インデックス 0..3（zIndex*2 + xIndex）</returns>
    int GetPlayerQuadrant(Tako::BTBlackboard* blackboard) const;

    /// <summary>
    /// 指定象限の中心ワールド座標を求める
    /// </summary>
    /// <param name="quadrantIndex">象限インデックス 0..3</param>
    /// <param name="bossPos">象限分割の原点となるボス位置</param>
    /// <returns>象限中心のワールド座標</returns>
    Tako::Vector3 GetQuadrantCenter(int quadrantIndex, const Tako::Vector3& bossPos) const;

    /// <summary>
    /// 点滅フェーズの Decal アルファを sin で更新する
    /// </summary>
    /// <param name="phaseElapsed">点滅フェーズ開始からの経過秒</param>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// 攻撃開始: Decal 非表示、コライダーとエミッタを有効化する
    /// </summary>
    /// <param name="boss">エミッタ取得元のボス</param>
    void BeginAttackPhase(Boss* boss);

    /// <summary>
    /// 攻撃終了: Decal・コライダー・エミッタを停止する
    /// </summary>
    /// <param name="boss">エミッタ取得元のボス</param>
    void EndAttackPhase(Boss* boss);

    //=========================================================================================
    // 定数
    //=========================================================================================

    static constexpr int   kQuadrantCount = 4;
    static constexpr float kDecalBaseAlpha = 0.3f;
    static constexpr float kBlinkAlphaMin = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;  ///< kBlinkAlphaMin からの増加分
    static constexpr float kColliderHeight = 10.0f;

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float warningDuration_ = 1.5f;
    float blinkDuration_ = 1.0f;
    float attackDuration_ = 0.5f;
    float recoveryTime_ = 0.5f;
    int   minQuadrants_ = 1;
    int   maxQuadrants_ = 3;
    float damage_ = 15.0f;
    float blinkFrequency_ = 10.0f;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;
    bool  hasBegunAttack_ = false;
    bool  hasEndedAttack_ = false;
    std::array<bool, kQuadrantCount> activeQuadrants_{};

    //=========================================================================================
    // Decal / コライダー / パーティクル
    //=========================================================================================
    std::array<std::unique_ptr<Tako::Decal>, kQuadrantCount> quadrantDecals_;
    std::array<std::unique_ptr<BossAreaAttackCollider>, kQuadrantCount> quadrantColliders_;
    std::array<Tako::Transform, kQuadrantCount> colliderTransforms_;

    bool particlesInitialized_ = false;
    std::array<std::string, kQuadrantCount> emitterNames_;
};
