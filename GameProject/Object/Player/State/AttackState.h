#pragma once
#include <array>

#include "PlayerState.h"

/// <summary>
/// 攻撃状態クラス
/// ターゲット検索、移動、攻撃実行、4コンボ管理を制御
/// プリインプット（先行入力）対応
/// </summary>
class AttackState : public PlayerState
{
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr int kMaxComboCount = 4;  ///< 最大コンボ数

private: // 構造体定義
    /// <summary>
    /// 攻撃ブロックの回転軸
    /// </summary>
    enum class SwingAxis {
        Horizontal, ///< 水平回転（XZ 平面）
        Vertical    ///< 垂直回転（プレイヤー向き基準の YZ 平面）
    };

    /// <summary>
    /// コンボごとの攻撃データ
    /// </summary>
    struct ComboData {
        float startAngle;      ///< 開始角度（ラジアン）
        float swingAngle;      ///< 振り幅（ラジアン）
        float swingDirection;  ///< 振り方向（+1.0f/-1.0f）
        float attackDuration;  ///< 攻撃持続時間（秒）
        float damage;          ///< ダメージ量
        SwingAxis axis;        ///< 回転軸（水平/垂直）
    };

public:
    AttackState() : PlayerState("Attack") {}

    /// <summary>
    /// 攻撃状態開始時の処理
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    void Enter(Player* player) override;

    /// <summary>
    /// 攻撃状態の更新処理
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    /// <param name="deltaTime">前フレームからの経過時間</param>
    void Update(Player* player, float deltaTime) override;

    /// <summary>
    /// 攻撃状態終了時の処理
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    void Exit(Player* player) override;

    /// <summary>
    /// 入力処理（プリインプット対応）
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    void HandleInput(Player* player) override;

private:
    /// <summary>
    /// 攻撃フェーズ
    /// </summary>
    enum AttackPhase {
        SearchTarget,    ///< ターゲット検索フェーズ
        MoveToTarget,    ///< ターゲットへの移動フェーズ
        ExecuteAttack,   ///< 攻撃実行フェーズ
        Recovery         ///< 硬直フェーズ（4コンボ完走時のみ）
    };

    AttackPhase phase_ = SearchTarget;                  ///< 現在の攻撃フェーズ
    class Boss* targetEnemy_ = nullptr;                 ///< 攻撃対象のボス
    float phaseTimer_ = 0.0f;                           ///< 統一タイマー（各フェーズで使用）
    float maxSearchTime_ = 0.1f;                        ///< 最大検索時間
    float maxMoveTime_ = 0.1f;                          ///< 最大移動時間
    float recoveryTime_ = 0.5f;                         ///< 硬直時間（4コンボ完走時）

    int comboIndex_ = 0;                                ///< 現在のコンボインデックス（0-3）
    int maxCombo_ = 4;                                  ///< 最大コンボ数
    bool hasBufferedInput_ = false;                     ///< プリインプット（先行入力）フラグ

    // コンボデータ配列
    std::array<ComboData, kMaxComboCount> combos_{};

    // 攻撃ブロック回転制御
    float blockAngle_ = 0.0f;                           ///< 現在のブロック回転角度
    float blockRadius_ = 4.0f;                          ///< プレイヤーからの距離
    float blockScale_ = 0.5f;                           ///< ブロックのスケール

    /// <summary>
    /// GlobalVariables からコンボデータを読み込み
    /// </summary>
    void LoadComboData();

    /// <summary>
    /// コンボ攻撃の初期化
    /// ブロック角度の設定、コライダーの有効化など
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    void InitializeComboAttack(Player* player);

    /// <summary>
    /// 指定フェーズへ遷移（タイマーリセット込み）
    /// </summary>
    /// <param name="newPhase">遷移先のフェーズ</param>
    void TransitionToPhase(AttackPhase newPhase);

    /// <summary>
    /// ターゲット検索処理
    /// 攻撃範囲内の最も近い敵を検索してターゲットに設定する
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    void SearchForTarget(Player* player);

    /// <summary>
    /// ターゲットへの移動処理
    /// ターゲットに向かって移動し、攻撃範囲内に到達したら攻撃実行フェーズに遷移
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    /// <param name="deltaTime">前フレームからの経過時間</param>
    void ProcessMoveToTarget(Player* player, float deltaTime);

    /// <summary>
    /// 攻撃実行処理
    /// アニメーション再生、ダメージ判定、コンボ管理を行う
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    /// <param name="deltaTime">前フレームからの経過時間</param>
    void ProcessExecuteAttack(Player* player, float deltaTime);

    /// <summary>
    /// 攻撃実行完了時の処理
    /// プリインプットの有無、コンボインデックスに応じて次の状態を決定
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    void OnExecuteAttackComplete(Player* player);

    /// <summary>
    /// 硬直フェーズの処理
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    /// <param name="deltaTime">前フレームからの経過時間</param>
    void ProcessRecovery(Player* player, float deltaTime);

    /// <summary>
    /// ブロックの位置を更新
    /// プレイヤーの位置と向きに基づいてブロックの位置を計算
    /// 水平回転と垂直回転を現在のコンボ設定に基づいて切り替え
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    void UpdateBlockPosition(Player* player);

public:
    /// <summary>
    /// 現在の攻撃フェーズを取得（デバッグ用）
    /// </summary>
    /// <returns>現在の攻撃フェーズ</returns>
    AttackPhase GetPhase() const { return phase_; }

    /// <summary>
    /// 現在のターゲット敵を取得（デバッグ用）
    /// </summary>
    /// <returns>ターゲット敵のポインタ（nullptr の場合もある）</returns>
    Boss* GetTargetEnemy() const { return targetEnemy_; }

    /// <summary>
    /// フェーズタイマーを取得（デバッグ用）
    /// </summary>
    /// <returns>現在のフェーズタイマー値（秒）</returns>
    float GetPhaseTimer() const { return phaseTimer_; }

    /// <summary>
    /// ImGui デバッグ情報の描画
    /// </summary>
    /// <param name="player">プレイヤーインスタンス</param>
    void DrawImGui(Player* player) override;

    // DrawImGui 用のゲッター追加
    int GetComboIndex() const { return comboIndex_; }
    int GetMaxCombo() const { return maxCombo_; }
    float GetMaxMoveTime() const { return maxMoveTime_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    bool HasBufferedInput() const { return hasBufferedInput_; }
    const ComboData& GetCurrentComboData() const { return combos_[comboIndex_]; }
};
