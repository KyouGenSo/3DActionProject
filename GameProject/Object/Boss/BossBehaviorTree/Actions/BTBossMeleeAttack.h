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
    /// <summary>
    /// 固有攻撃ロジック本体（Prepare → Execute → (Interval →) Recovery のフェーズ制御）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    /// <returns>BTNodeStatus::Running（攻撃継続中） / BTNodeStatus::Success（攻撃完了）</returns>
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理（コンボ抽選、totalDuration の算出、武器ブロックの生成と初期姿勢の設定）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;

    /// <summary>
    /// 固有クリーンアップ処理（武器ブロック解放とフェーズ状態のリセット）
    /// </summary>
    void OnCleanup() override;

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
    /// プレイヤー方向へボスを徐々に旋回させる
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    void AimAtPlayer(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 準備フェーズの更新処理（プレイヤー追尾と突進初期化）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    void ProcessPreparePhase(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 攻撃実行フェーズの更新処理（突進移動とブロック振り回し）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    void ProcessExecutePhase(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 硬直フェーズの開始処理（Boss を硬直状態へ遷移させる）
    /// </summary>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void ProcessRecoveryPhase(Boss* boss);

    /// <summary>
    /// コンボ間隔フェーズの更新処理（次の攻撃までの待機とプレイヤー追尾）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    void ProcessIntervalPhase(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 現在のコンボ段に応じた振り方向と初期角度を設定する
    /// </summary>
    void InitializeSwingForCurrentCombo();

    /// <summary>
    /// 現在のブロック角度に基づき武器ブロックの位置・姿勢を更新する
    /// </summary>
    /// <param name="boss">武器ブロックを保有するBossへのポインタ</param>
    void UpdateBlockPosition(Boss* boss);

    /// <summary>
    /// 指定座標をフェーズ2のプレイエリア境界内にクランプして返す
    /// </summary>
    /// <param name="position">クランプ対象のワールド座標</param>
    /// <returns>エリア境界内に収めた座標</returns>
    Tako::Vector3 ClampToArea(const Tako::Vector3& position);

    /// <summary>
    /// 突進の開始位置・目標位置・方向ベクトルを算出して初期化する
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
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
