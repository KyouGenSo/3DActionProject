#pragma once
#include "Vector4.h"

namespace Tako {
    class Object3d;
}

/// <summary>
/// ダメージ時に一時的に色を変える演出を管理
/// </summary>
class HitFlashEffect
{
public: //メンバー関数
    HitFlashEffect() = default;
    ~HitFlashEffect() = default;

    /// <summary>
    /// フラッシュ演出を開始し、タイマーをリセットする
    /// </summary>
    /// <param name="flashColor">点滅中に適用する色（RGBA）</param>
    /// <param name="duration">この色を保持する時間（秒）</param>
    void Start(const Tako::Vector4& flashColor, float duration = 0.1f);

    /// <summary>
    /// duration 内は flashColor を適用し、経過後に originalColor へ戻す
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過時間（秒）</param>
    /// <param name="target">色を設定する対象（nullptr なら何もしない）</param>
    /// <param name="originalColor">duration 経過後に戻す元の色（RGBA）</param>
    void Update(float deltaTime, Tako::Object3d* target, const Tako::Vector4& originalColor);

    void Stop() { isActive_ = false; timer_ = 0.0f; }

    //====================
    //Getter
    //====================
    bool IsActive() const { return isActive_; }
    float GetTimer() const { return timer_; }

private: //メンバー変数
    bool          isActive_     = false;
    float         timer_        = 0.0f;
    float         duration_     = 0.1f;
    Tako::Vector4 flashColor_{};
};
