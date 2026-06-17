#pragma once
#include "TransitionEffectBase.h"
#include "Vector3.h"
#include <cstdint>

class Boss;

/// <summary>
/// ボス撃破時の斬撃エフェクト増加と消滅演出を制御
/// </summary>
class ClearEffectManager : public TransitionEffectBase
{
public: //構造体
    enum class Phase {
        Idle,
        SlashBuildup,
        Explosion,
        ScaleDown,
        Complete        ///< シーン遷移可能
    };

    struct Params {
        float startDelay = 0.5f;                 ///< 秒
        uint32_t slashMaxCount = 100;            ///< 斬撃エミッター最大数
        float slashMaxRadius = 10.0f;
        uint32_t slashCountIncrement = 1;        ///< 1フレームあたりの増加量
        float slashRadiusIncrement = 0.05f;      ///< 1フレームあたりの増加量
        float scaleDecreaseRate = 5.0f;          ///< /秒
        float shakeDuration = 0.4f;              ///< 秒
    };

public: //メンバー関数
    explicit ClearEffectManager(Tako::EmitterManager* emitterManager);
    ~ClearEffectManager() override = default;

    /// <summary>
    /// 演出を初期状態から開始する（再生中の呼び出しは無視）
    /// </summary>
    void Start() override;

    /// <summary>
    /// フェーズ（斬撃増加→爆発→縮小→完了）を1ステップ進める
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過時間（秒）</param>
    void Update(float deltaTime) override;

    /// <summary>
    /// タイマー・フェーズ・斬撃カウンタを初期値へ戻す
    /// </summary>
    void Reset() override;

    //==================================
    //Setter
    //==================================
    void SetTarget(Boss* boss) { target_ = boss; }
    void SetParams(const Params& params) { params_ = params; }

    //==================================
    //Getter
    //==================================
    const Params& GetParams() const { return params_; }
    Phase GetPhase() const { return phase_; }

private: //メンバー変数
    Boss*    target_             = nullptr;
    Phase    phase_              = Phase::Idle;
    uint32_t currentSlashCount_  = 1;
    float    currentSlashRadius_ = 2.0f;
    bool     explosionFired_     = false;
    Params   params_;
};
