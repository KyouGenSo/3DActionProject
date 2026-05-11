#pragma once
#include <numbers>
#include "AttackNode.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの近接攻撃アクションノード
/// 準備 → 攻撃 → 硬直の 3 フェーズ（コンボ時は 3 連撃 + 間隔）で武器ブロックを振る。
/// </summary>
class BTBossMeleeAttack : public AttackNode {
private:
    static constexpr float kDirectionEpsilon = 0.01f;
    static constexpr float kBlockStartAngle = -std::numbers::pi_v<float> / 2.0f;
    static constexpr float kAngleEpsilon = 0.001f;

    enum class MeleePhase {
        Prepare,    ///< 準備（プレイヤー方向を向く、予兆表示）
        Execute,    ///< 攻撃実行（ブロック回転、ダメージ判定）
        Interval,   ///< コンボ間隔（次の攻撃までの待機）
        Recovery    ///< 硬直
    };

public:
    BTBossMeleeAttack();
    virtual ~BTBossMeleeAttack() = default;

    // パラメータ取得・設定
    float GetPrepareTime() const { return prepareTime_; }
    void  SetPrepareTime(float time) { prepareTime_ = time; }
    float GetAttackDuration() const { return attackDuration_; }
    void  SetAttackDuration(float duration) { attackDuration_ = duration; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    float GetBlockRadius() const { return blockRadius_; }
    void  SetBlockRadius(float radius) { blockRadius_ = radius; }
    float GetBlockScale() const { return blockScale_; }
    void  SetBlockScale(float scale) { blockScale_ = scale; }
    float GetSwingAngle() const { return swingAngle_; }
    void  SetSwingAngle(float angle) { swingAngle_ = angle; }
    float GetRushDistance() const { return rushDistance_; }
    void  SetRushDistance(float distance) { rushDistance_ = distance; }
    float GetStopDistance() const { return stopDistance_; }
    void  SetStopDistance(float distance) { stopDistance_ = distance; }

protected:
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;
    void OnCleanup() override;
    void OnApplyParameters(const nlohmann::json& params) override;
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private:
    void AimAtPlayer(BTBlackboard* blackboard, float deltaTime);
    void ProcessPreparePhase(BTBlackboard* blackboard, float deltaTime);
    void ProcessExecutePhase(BTBlackboard* blackboard, float deltaTime);
    void ProcessRecoveryPhase(Boss* boss);
    void ProcessIntervalPhase(BTBlackboard* blackboard, float deltaTime);
    void InitializeSwingForCurrentCombo();
    void UpdateBlockPosition(Boss* boss);
    Tako::Vector3 ClampToArea(const Tako::Vector3& position);
    void InitializeRush(BTBlackboard* blackboard);

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float prepareTime_ = 1.0f;
    float attackDuration_ = 0.3f;
    float recoveryTime_ = 0.3f;
    float blockRadius_ = 8.0f;
    float blockScale_ = 0.5f;
    float swingAngle_ = static_cast<float>(std::numbers::pi);
    float rushDistance_ = 20.0f;
    float stopDistance_ = 5.0f;
    float comboInterval_ = 0.5f;
    float comboProbability_ = 0.5f;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    MeleePhase currentPhase_ = MeleePhase::Prepare;
    float totalDuration_ = 1.6f;
    float phaseTimer_ = 0.0f;
    float blockAngle_ = 0.0f;
    bool  colliderActivated_ = false;

    // 突進状態
    Tako::Vector3 startPosition_;
    Tako::Vector3 targetPosition_;
    Tako::Vector3 rushDirection_;
    bool rushInitialized_ = false;

    // コンボ管理
    bool  isComboMode_ = false;
    int   comboMaxCount_ = 1;
    int   comboIndex_ = 0;
    float currentSwingDirection_ = 1.0f;
};
