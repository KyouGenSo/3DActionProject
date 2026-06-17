#pragma once
#include <array>

#include "PlayerState.h"

class Boss;

/// <summary>
/// 攻撃状態クラス（ターゲット検索→移動→攻撃、コンボ管理、先行入力対応）
/// </summary>
class AttackState : public PlayerState
{
private: //定数
    static constexpr int kMaxComboCount = 4;

private: //構造体
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
        float startAngle;      ///< ラジアン
        float swingAngle;      ///< 振り幅（ラジアン）
        float swingDirection;  ///< +1.0f / -1.0f
        float attackDuration;  ///< 秒
        float damage;
        SwingAxis axis;
    };

    /// <summary>
    /// 攻撃フェーズ
    /// </summary>
    enum AttackPhase {
        SearchTarget,
        MoveToTarget,
        ExecuteAttack,
        Recovery         ///< コンボ完走時のみ
    };

public: //メンバー関数
    AttackState() : PlayerState("Attack") {}

    void Enter(Player* player) override;
    void Update(Player* player, float deltaTime) override;
    void Exit(Player* player) override;
    void HandleInput(Player* player) override;
    void DrawImGui(Player* player) override;

    //==========================================
    //Getter
    //==========================================
    AttackPhase GetPhase() const { return phase_; }
    Boss* GetTargetEnemy() const { return targetEnemy_; }
    float GetPhaseTimer() const { return phaseTimer_; }
    int GetComboIndex() const { return comboIndex_; }
    int GetMaxCombo() const { return maxCombo_; }
    float GetMaxMoveTime() const { return maxMoveTime_; }
    float GetRecoveryTime() const { return recoveryTime_; }
    bool HasBufferedInput() const { return hasBufferedInput_; }
    const ComboData& GetCurrentComboData() const { return combos_[comboIndex_]; }

private: //非公開関数
    void LoadComboData();

    /// <summary>
    /// ブロック角度設定、コライダー有効化などのコンボ初期化
    /// </summary>
    void InitializeComboAttack(Player* player);

    /// <summary>
    /// 指定フェーズへ遷移（タイマーリセット込み）
    /// </summary>
    void TransitionToPhase(AttackPhase newPhase);

    void SearchForTarget(Player* player);
    void ProcessMoveToTarget(Player* player, float deltaTime);
    void ProcessExecuteAttack(Player* player, float deltaTime);

    /// <summary>
    /// 先行入力とコンボ位置から次状態を決定
    /// </summary>
    void OnExecuteAttackComplete(Player* player);

    void ProcessRecovery(Player* player, float deltaTime);

    /// <summary>
    /// コンボの回転軸に応じてブロック位置を計算
    /// </summary>
    void UpdateBlockPosition(Player* player);

private: //メンバー変数
    //フェーズ
    AttackPhase phase_         = SearchTarget;
    class Boss* targetEnemy_   = nullptr;
    float       phaseTimer_    = 0.0f;          ///< 各フェーズ共用
    float       maxSearchTime_ = 0.1f;
    float       maxMoveTime_   = 0.1f;
    float       recoveryTime_  = 0.5f;          ///< コンボ完走時の硬直

    //コンボ
    int  comboIndex_       = 0;      ///< 0-3
    int  maxCombo_         = 4;
    bool hasBufferedInput_ = false;  ///< 先行入力フラグ

    std::array<ComboData, kMaxComboCount> combos_{};

    //攻撃ブロック回転制御
    float blockAngle_  = 0.0f;
    float blockRadius_ = 4.0f;  ///< プレイヤーからの距離
    float blockScale_  = 0.5f;
};
