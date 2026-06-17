#pragma once
#include "TransitionEffectBase.h"
#include "Vector3.h"

class Player;

/// <summary>
/// プレイヤー死亡時のエミッター発火とスケール減少を制御
/// </summary>
class OverEffectManager : public TransitionEffectBase
{
public: //構造体
    enum class Phase {
        Idle,
        WaitEmit1,
        WaitEmit2,
        ScaleDown,
        Complete    ///< シーン遷移可能
    };

    struct Params {
        float emit1Time = 2.0f;          ///< 秒
        float emit2Time = 2.8f;          ///< 秒
        float totalTime = 3.8f;          ///< 秒
        float scaleDecreaseRate = 5.0f;  ///< /秒
    };

public: //メンバー関数
    explicit OverEffectManager(Tako::EmitterManager* emitterManager);
    ~OverEffectManager() override = default;

    /// <summary>
    /// 演出を初期状態から開始する（再生中の呼び出しは無視）
    /// </summary>
    void Start() override;

    /// <summary>
    /// フェーズ（emit1待ち→emit2待ち→縮小→完了）を1ステップ進める
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過時間（秒）</param>
    void Update(float deltaTime) override;

    /// <summary>
    /// タイマー・フェーズ・発火フラグを初期値へ戻す
    /// </summary>
    void Reset() override;

    //==================================
    //Setter
    //==================================
    void SetTarget(Player* player) { target_ = player; }
    void SetParams(const Params& params) { params_ = params; }

    //==================================
    //Getter
    //==================================
    const Params& GetParams() const { return params_; }
    Phase GetPhase() const { return phase_; }

private: //メンバー変数
    Player* target_     = nullptr;
    Phase   phase_      = Phase::Idle;
    bool    emit1Fired_ = false;
    bool    emit2Fired_ = false;
    Params  params_;
};
