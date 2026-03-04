#pragma once
#include <numbers>

#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの近接攻撃アクションノード
/// 準備→攻撃→硬直の3フェーズで武器ブロックを振る
/// </summary>
class BTBossMeleeAttack : public BTNode {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr float kDirectionEpsilon = 0.01f;   ///< 方向判定の閾値
    static constexpr float kBlockStartAngle = -std::numbers::pi_v<float> / 2.0f; ///< ブロック開始角度（-π/2、右側から開始）
    static constexpr float kAngleEpsilon = 0.001f;      ///< 角度判定の閾値

    //=========================================================================================
    // 列挙型
    //=========================================================================================
private:
    /// <summary>
    /// 攻撃フェーズ
    /// </summary>
    enum class MeleePhase {
        Prepare,    ///< 準備フェーズ（プレイヤー方向を向く、予兆表示）
        Execute,    ///< 攻撃実行フェーズ（ブロック回転、ダメージ判定）
        Interval,   ///< コンボ間隔フェーズ（次の攻撃までの待機）
        Recovery    ///< 硬直フェーズ
    };

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossMeleeAttack();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossMeleeAttack() = default;

    /// <summary>
    /// ノードの実行
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <returns>実行結果</returns>
    BTNodeStatus Execute(BTBlackboard* blackboard) override;

    /// <summary>
    /// ノードのリセット
    /// </summary>
    void Reset() override;

    // パラメータ取得・設定
    float GetPrepareTime() const { return prepareTime_; }
    void SetPrepareTime(float time) { prepareTime_ = time; }
    float GetAttackDuration() const { return attackDuration_; }
    void SetAttackDuration(float duration) { attackDuration_ = duration; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    float GetBlockRadius() const { return blockRadius_; }
    void SetBlockRadius(float radius) { blockRadius_ = radius; }
    float GetBlockScale() const { return blockScale_; }
    void SetBlockScale(float scale) { blockScale_ = scale; }
    float GetSwingAngle() const { return swingAngle_; }
    void SetSwingAngle(float angle) { swingAngle_ = angle; }
    float GetRushDistance() const { return rushDistance_; }
    void SetRushDistance(float distance) { rushDistance_ = distance; }
    float GetStopDistance() const { return stopDistance_; }
    void SetStopDistance(float distance) { stopDistance_ = distance; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("prepareTime")) {
            prepareTime_ = params["prepareTime"];
        }
        if (params.contains("attackDuration")) {
            attackDuration_ = params["attackDuration"];
        }
        if (params.contains("recoveryTime")) {
            recoveryTime_ = params["recoveryTime"];
        }
        if (params.contains("blockRadius")) {
            blockRadius_ = params["blockRadius"];
        }
        if (params.contains("blockScale")) {
            blockScale_ = params["blockScale"];
        }
        if (params.contains("swingAngle")) {
            swingAngle_ = params["swingAngle"];
        }
        if (params.contains("rushDistance")) {
            rushDistance_ = params["rushDistance"];
        }
        if (params.contains("stopDistance")) {
            stopDistance_ = params["stopDistance"];
        }
        if (params.contains("comboInterval")) {
            comboInterval_ = params["comboInterval"];
        }
        if (params.contains("comboProbability")) {
            comboProbability_ = params["comboProbability"];
        }
    }

    /// <summary>
    /// パラメータを JSON として抽出
    /// </summary>
    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    /// <summary>
    /// ImGui でパラメータ編集 UI を描画
    /// </summary>
    bool DrawImGui() override;
#endif

private:
    /// <summary>
    /// 攻撃パラメータの初期化
    /// </summary>
    /// <param name="boss">ボス</param>
    void InitializeMeleeAttack(Boss* boss);

    /// <summary>
    /// プレイヤー方向を向く処理
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="deltaTime">経過時間</param>
    void AimAtPlayer(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 準備フェーズの処理
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <param name="deltaTime">経過時間</param>
    void ProcessPreparePhase(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 攻撃実行フェーズの処理
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <param name="deltaTime">経過時間</param>
    void ProcessExecutePhase(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 硬直フェーズの処理
    /// </summary>
    /// <param name="boss">ボス</param>
    void ProcessRecoveryPhase(Boss* boss);

    /// <summary>
    /// コンボ間隔フェーズの処理
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <param name="deltaTime">経過時間</param>
    void ProcessIntervalPhase(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 現在のコンボインデックスに応じた振り方向を初期化
    /// </summary>
    void InitializeSwingForCurrentCombo();

    /// <summary>
    /// ブロック位置の更新（Mat4x4使用）
    /// </summary>
    /// <param name="boss">ボス</param>
    void UpdateBlockPosition(Boss* boss);

    /// <summary>
    /// エリア内に収まる位置を計算
    /// </summary>
    /// <param name="position">調整前の位置</param>
    /// <returns>エリア内に収まる位置</returns>
    Tako::Vector3 ClampToArea(const Tako::Vector3& position);

    /// <summary>
    /// 突進の初期化（Execute 開始時に呼ぶ）
    /// </summary>
    /// <param name="boss">ボス</param>
    void InitializeRush(BTBlackboard* blackboard);

    //=========================================================================================
    // メンバ変数
    //=========================================================================================
private:
    // フェーズ管理
    MeleePhase currentPhase_ = MeleePhase::Prepare;

    // 時間パラメータ
    float prepareTime_ = 1.0f;      ///< 準備時間
    float attackDuration_ = 0.3f;   ///< 攻撃持続時間
    float recoveryTime_ = 0.3f;     ///< 硬直時間
    float totalDuration_ = 1.6f;    ///< 総時間

    // ブロックパラメータ
    float blockRadius_ = 8.0f;      ///< ボスからの距離
    float blockScale_ = 0.5f;       ///< ブロックスケール
    float swingAngle_ =             ///< 振り幅（π = 180度）
        static_cast<float>(std::numbers::pi);
    float blockAngle_ = 0.0f;       ///< 現在のブロック角度

    // 状態管理
    float elapsedTime_ = 0.0f;      ///< 経過時間
    float phaseTimer_ = 0.0f;       ///< 現在フェーズのタイマー
    bool isFirstExecute_ = true;    ///< 初回実行フラグ
    bool colliderActivated_ = false;///< コライダー有効化済みフラグ

    // 突進パラメータ
    float rushDistance_ = 20.0f;    ///< 突進距離（ミス時）
    float stopDistance_ = 5.0f;     ///< ヒット時の停止距離（プレイヤーからの距離）

    // 突進状態管理
    Tako::Vector3 startPosition_;         ///< 突進開始位置
    Tako::Vector3 targetPosition_;        ///< 突進目標位置（Execute 開始時に固定）
    Tako::Vector3 rushDirection_;         ///< 突進方向
    bool rushInitialized_ = false;  ///< 突進初期化済みフラグ

    // コンボ管理
    bool isComboMode_ = false;           ///< コンボモードフラグ
    int comboMaxCount_ = 1;              ///< 最大攻撃回数（単発:1, コンボ:3）
    int comboIndex_ = 0;                 ///< 現在の攻撃回数（0-indexed）
    float currentSwingDirection_ = 1.0f; ///< 振り方向（+1:右→左, -1:左→右）

    // コンボパラメータ（GlobalVariables 連携）
    float comboInterval_ = 0.5f;         ///< コンボ間隔（デフォルト0.5秒）
    float comboProbability_ = 0.5f;      ///< コンボ発動確率（デフォルト50%）
};
