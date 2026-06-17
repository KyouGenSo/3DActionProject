#pragma once
#include "Vector3.h"
#include <string>

// 前方宣言
namespace Tako {
    class EmitterManager;
}

/// <summary>
/// 被弾・パリィ成功時のシェイク/振動/Vignette/パーティクルをまとめて発生させる
/// </summary>
class DamageFeedback
{
public: //構造体
    struct HitParams {
        float         shakeIntensity    = 0.8f;
        float         vibrationLow      = 0.2f;                  ///< ゲームパッド低周波モーター
        float         vibrationHigh     = 0.3f;                  ///< ゲームパッド高周波モーター
        float         vibrationDuration = 0.25f;                 ///< 秒
        float         vignettePower     = 0.4f;
        float         vignetteRange     = 45.0f;
        Tako::Vector3 vignetteColor     = { 1.0f, 0.0f, 0.0f };  ///< 赤
        float         vignetteDuration  = 0.25f;                 ///< 秒
    };

    struct ParryParams {
        float         shakeIntensity    = 0.2f;
        float         vibrationLow      = 0.15f;                     ///< 低周波モーター
        float         vibrationHigh     = 0.3f;                      ///< 高周波モーター
        float         vibrationDuration = 0.15f;                     ///< 秒
        float         vignettePower     = 0.4f;
        float         vignetteRange     = 45.0f;
        Tako::Vector3 vignetteColor     = { 0.058f, 0.447f, 1.0f };  ///< 青
        float         vignetteDuration  = 0.3f;                      ///< 秒
        std::string   emitterBaseName   = "parry_success";
        float         emitterDuration   = 0.5f;                      ///< 秒
    };

public: //メンバー関数
    /// <summary>
    /// 被弾フィードバック（カメラシェイク・振動・赤Vignette）を発生させる
    /// </summary>
    /// <param name="params">演出の強度・時間などのパラメータ</param>
    static void TriggerHitFeedback(const HitParams& params = HitParams{});

    /// <summary>
    /// パリィ成功フィードバック（パーティクル・シェイク・振動・青Vignette）を発生させる
    /// </summary>
    /// <param name="position">パーティクルを出す位置</param>
    /// <param name="emitterManager">パーティクル発生元。nullptr ならパーティクルを省略</param>
    /// <param name="params">演出の強度・時間などのパラメータ</param>
    static void TriggerParryFeedback(
        const Tako::Vector3& position,
        Tako::EmitterManager* emitterManager,
        const ParryParams& params = ParryParams{});

private: //非公開関数
    DamageFeedback() = delete;
    ~DamageFeedback() = delete;
};
