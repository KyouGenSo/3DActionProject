#pragma once
#include <cstdint>

/// <summary>
/// ボスのフェーズ・ライフ管理クラス
/// HP 変化に応じたフェーズ遷移とライフ管理を行う
/// </summary>
class BossPhaseManager
{
public:
    BossPhaseManager() = default;
    ~BossPhaseManager() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="maxHp">最大 HP</param>
    /// <param name="phase2Threshold">フェーズ2開始閾値</param>
    /// <param name="phase2InitialHp">フェーズ2開始時の HP</param>
    void Initialize(float maxHp, float phase2Threshold, float phase2InitialHp);

    /// <summary>
    /// HP 変化に応じてフェーズとライフを更新
    /// </summary>
    /// <param name="currentHp">現在の HP</param>
    void Update(float currentHp);

    /// <summary>
    /// フェーズ変更要求を消費（変更後にフラグをリセット）
    /// </summary>
    /// <returns>フェーズ変更すべきなら true</returns>
    bool ConsumePhaseChangeRequest();

    /// <summary>
    /// リセット（復活時などに使用）
    /// </summary>
    void Reset();

    /// <summary>
    /// 現在のフェーズを取得
    /// </summary>
    uint8_t GetPhase() const { return phase_; }

    /// <summary>
    /// フェーズを設定
    /// </summary>
    /// <param name="phase">新しいフェーズ番号（1または2）</param>
    void SetPhase(uint32_t phase) { if (phase >= 1 && phase <= 2) phase_ = phase; }

    /// <summary>
    /// 現在のライフを取得
    /// </summary>
    uint8_t GetLife() const { return life_; }

    /// <summary>
    /// ライフを設定
    /// </summary>
    void SetLife(uint8_t life) { life_ = life; }

    /// <summary>
    /// フェーズ変更準備完了か
    /// </summary>
    bool IsReadyToChangePhase() const { return isReadyToChangePhase_; }

    /// <summary>
    /// 死亡しているか
    /// </summary>
    bool IsDead() const { return isDead_; }

    /// <summary>
    /// 死亡フラグを設定
    /// </summary>
    void SetDead(bool dead) { isDead_ = dead; }

    /// <summary>
    /// フェーズ2閾値を取得
    /// </summary>
    float GetPhase2Threshold() const { return phase2Threshold_; }

    /// <summary>
    /// フェーズ2開始時 HP を取得
    /// </summary>
    float GetPhase2InitialHp() const { return phase2InitialHp_; }

    /// <summary>
    /// 最大 HP を取得
    /// </summary>
    float GetMaxHp() const { return maxHp_; }

private:
    uint8_t phase_ = 1;               ///< 現在のフェーズ（1 or 2）
    uint8_t life_ = 1;                 ///< ライフカウント
    bool isReadyToChangePhase_ = false;///< フェーズ変更準備完了フラグ
    bool isDead_ = false;              ///< 死亡フラグ

    float maxHp_ = 200.0f;             ///< 最大 HP
    float phase2Threshold_ = 105.0f;   ///< フェーズ2開始閾値
    float phase2InitialHp_ = 100.0f;   ///< フェーズ2開始時の HP
};
