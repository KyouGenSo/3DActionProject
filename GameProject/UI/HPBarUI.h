#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "Vector2.h"
#include "Vector4.h"
#include "Sprite.h"

/// <summary>
/// HP バーの初期化・更新・描画を管理
/// </summary>
class HPBarUI
{
public: //メンバー関数
    HPBarUI() = default;
    ~HPBarUI() = default;

    /// <summary>
    /// 単一バー（前景バー＋背景）を生成し初期化する
    /// </summary>
    /// <param name="texture">バー・背景に共通で使うテクスチャ名</param>
    /// <param name="size">バーの基準サイズ（ピクセル）。HP満タン時の幅</param>
    /// <param name="screenXRatio">表示位置X。画面幅に対する比率(0.0-1.0)</param>
    /// <param name="screenYRatio">表示位置Y。画面高さに対する比率(0.0-1.0)</param>
    /// <param name="barColor">前景バーの色(RGBA)</param>
    /// <param name="bgColor">背景バーの色(RGBA)。既定は不透明白</param>
    void Initialize(
        const std::string& texture,
        const Tako::Vector2& size,
        float screenXRatio,
        float screenYRatio,
        const Tako::Vector4& barColor,
        const Tako::Vector4& bgColor = Tako::Vector4{ 1.f, 1.f, 1.f, 1.0f });

    /// <summary>
    /// 2段重ねバー（Boss フェーズ用）を生成し初期化する
    /// </summary>
    /// <param name="texture">バー・背景に共通で使うテクスチャ名</param>
    /// <param name="size">バーの基準サイズ（ピクセル）</param>
    /// <param name="screenXRatio">表示位置X。画面幅に対する比率(0.0-1.0)</param>
    /// <param name="screenYRatio">表示位置Y。画面高さに対する比率(0.0-1.0)</param>
    /// <param name="bar1Color">フェーズ1で減少する手前バーの色(RGBA)</param>
    /// <param name="bar2Color">フェーズ2で減少する奥バーの色(RGBA)</param>
    /// <param name="bgColor">背景バーの色(RGBA)。既定は不透明白</param>
    void InitializeDual(
        const std::string& texture,
        const Tako::Vector2& size,
        float screenXRatio,
        float screenYRatio,
        const Tako::Vector4& bar1Color,
        const Tako::Vector4& bar2Color,
        const Tako::Vector4& bgColor = Tako::Vector4{ 1.f, 1.f, 1.f, 1.0f });

    /// <summary>
    /// 現在値/最大値の比率でバー幅を更新する
    /// </summary>
    /// <param name="currentValue">現在値。maxValue で割った比率は0.0-1.0にクランプ</param>
    /// <param name="maxValue">最大値。0以下なら比率0扱い</param>
    void Update(float currentValue, float maxValue);

    /// <summary>
    /// フェーズに応じて2段バーの幅を更新する
    /// </summary>
    /// <param name="currentHp">現在HP</param>
    /// <param name="maxHp">最大HP</param>
    /// <param name="phase2Threshold">フェーズ2開始HP閾値。これ以上が手前バー、以下が奥バーの担当</param>
    /// <param name="phase">現在のフェーズ（1: 手前バー減少 / 2: 手前バー0幅・奥バー減少）</param>
    void UpdateDual(float currentHp, float maxHp, float phase2Threshold, uint32_t phase);

    void Draw();

    //=======================================================
    //Setter
    //=======================================================
    /// <param name="screenXRatio">画面幅に対する比率</param>
    /// <param name="screenYRatio">画面高さに対する比率</param>
    void SetPosition(float screenXRatio, float screenYRatio);

    void SetBarColor(const Tako::Vector4& color);

    /// <param name="anchor">0,0:左上 1,1:右下</param>
    void SetAnchorPoint(const Tako::Vector2& anchor);

private: //非公開関数
    Tako::Vector2 CalculateScreenPosition() const;

private: //メンバー変数
    //スプライト
    std::unique_ptr<Tako::Sprite> barSprite_;
    std::unique_ptr<Tako::Sprite> bar2Sprite_;  ///< 2段目（Dual 用）
    std::unique_ptr<Tako::Sprite> bgSprite_;

    //表示位置・サイズ
    Tako::Vector2 baseSize_;
    float         screenXRatio_ = 0.5f;
    float         screenYRatio_ = 0.05f;

    //状態
    bool isDualBar_ = false;
};
