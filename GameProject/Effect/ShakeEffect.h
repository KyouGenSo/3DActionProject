#pragma once
#include "Vector3.h"

/// <summary>
/// オブジェクトに揺れを適用する汎用クラス
/// </summary>
class ShakeEffect
{
public: //メンバー関数
    ShakeEffect() = default;
    ~ShakeEffect() = default;

    /// <summary>
    /// 揺れを開始する。intensity/duration が0以下なら各デフォルト値を使う
    /// </summary>
    /// <param name="intensity">オフセットの最大振れ幅。0以下で defaultIntensity_ を使用</param>
    /// <param name="duration">揺れの継続時間（秒）。0以下で defaultDuration_ を使用</param>
    void Start(float intensity = 0.0f, float duration = 0.3f);

    void Update(float deltaTime);
    void Stop();

    //=======================================
    //Setter
    //=======================================
    void SetDefaultIntensity(float intensity) { defaultIntensity_ = intensity; }
    void SetDefaultDuration(float duration) { defaultDuration_ = duration; }

    //=======================================
    //Getter
    //=======================================
    Tako::Vector3 GetOffset() const { return offset_; }
    bool IsActive() const { return isActive_; }
    float GetTimer() const { return timer_; }
    float GetDuration() const { return duration_; }
    float GetDefaultIntensity() const { return defaultIntensity_; }
    float GetDefaultDuration() const { return defaultDuration_; }

private: //メンバー変数
    //状態
    bool  isActive_  = false;
    float timer_     = 0.0f;
    float duration_  = 0.3f;
    float intensity_ = 0.2f;

    //デフォルト値
    float defaultIntensity_ = 0.2f;
    float defaultDuration_  = 0.3f;

    //出力
    Tako::Vector3 offset_{};
};
