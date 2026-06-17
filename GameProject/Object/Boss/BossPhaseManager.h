#pragma once
#include <cstdint>

/// <summary>
/// HP 変化に応じてボスのフェーズ遷移とライフを管理する
/// </summary>
class BossPhaseManager
{
public: //メンバー関数
    BossPhaseManager() = default;
    ~BossPhaseManager() = default;

    /// <summary>
    /// HP 関連パラメータを設定して状態を初期化する
    /// </summary>
    /// <param name="maxHp">最大 HP</param>
    /// <param name="phase2Threshold">フェーズ2へ移行可能になる HP 閾値</param>
    /// <param name="phase2InitialHp">フェーズ2開始時の HP</param>
    void Initialize(float maxHp, float phase2Threshold, float phase2InitialHp);

    /// <summary>
    /// 現在 HP を見てフェーズ移行可能フラグの設定とライフ減算・撃破判定を行う
    /// </summary>
    /// <param name="currentHp">現在の HP</param>
    void Update(float currentHp);

    /// <summary>
    /// フェーズ変更要求を消費する。要求があれば内部フェーズを 2 に進める
    /// </summary>
    /// <returns>移行可能だった場合 true（同時にフラグをクリア）。それ以外は false</returns>
    bool ConsumePhaseChangeRequest();

    void Reset();

    //==============================
    //Setter
    //==============================
    void SetPhase(uint32_t phase) { if (phase >= 1 && phase <= 2) phase_ = phase; }
    void SetLife(uint8_t life) { life_ = life; }
    void SetDead(bool dead) { isDead_ = dead; }

    //==============================
    //Getter
    //==============================
    uint8_t GetPhase() const { return phase_; }
    uint8_t GetLife() const { return life_; }
    bool IsReadyToChangePhase() const { return isReadyToChangePhase_; }
    bool IsDead() const { return isDead_; }
    float GetPhase2Threshold() const { return phase2Threshold_; }
    float GetPhase2InitialHp() const { return phase2InitialHp_; }
    float GetMaxHp() const { return maxHp_; }

private: //メンバー変数
    uint8_t phase_                = 1;      ///< 1 or 2
    uint8_t life_                 = 1;
    bool    isReadyToChangePhase_ = false;
    bool    isDead_               = false;

    float maxHp_           = 200.0f;
    float phase2Threshold_ = 105.0f;
    float phase2InitialHp_ = 100.0f;
};
