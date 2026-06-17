#pragma once

namespace Tako {
    class EmitterManager;
}

/// <summary>
/// ゲームオーバー/クリアなど演出の共通インターフェース
/// </summary>
class TransitionEffectBase
{
public: //メンバー関数
    explicit TransitionEffectBase(Tako::EmitterManager* emitterManager);
    virtual ~TransitionEffectBase() = default;

    virtual void Start() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Reset();

    //=====================
    //Getter
    //=====================
    bool IsComplete() const { return isComplete_; }
    bool IsPlaying() const { return isPlaying_; }
    float GetTimer() const { return timer_; }

protected: //メンバー変数
    Tako::EmitterManager* emitterManager_ = nullptr;
    float                 timer_          = 0.0f;
    bool                  isPlaying_      = false;
    bool                  isComplete_     = false;
};
