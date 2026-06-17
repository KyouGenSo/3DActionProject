#include "HPBarUI.h"

#include <algorithm>
#include "WinApp.h"

using namespace Tako;

void HPBarUI::Initialize(
    const std::string& texture,
    const Vector2& size,
    float screenXRatio,
    float screenYRatio,
    const Vector4& barColor,
    const Vector4& bgColor)
{
    baseSize_ = size;
    screenXRatio_ = screenXRatio;
    screenYRatio_ = screenYRatio;
    isDualBar_ = false;

    Vector2 screenPos = CalculateScreenPosition();

    barSprite_ = std::make_unique<Sprite>();
    barSprite_->Initialize(texture);
    barSprite_->SetSize(baseSize_);
    barSprite_->SetAnchorPoint(Vector2(1.0f, 0.0f));  // 右上アンカー
    barSprite_->SetColor(barColor);
    barSprite_->SetPos(screenPos);

    bgSprite_ = std::make_unique<Sprite>();
    bgSprite_->Initialize(texture);
    bgSprite_->SetSize(baseSize_);
    bgSprite_->SetAnchorPoint(Vector2(1.0f, 0.0f));
    bgSprite_->SetColor(bgColor);
    bgSprite_->SetPos(screenPos);
}

void HPBarUI::InitializeDual(
    const std::string& texture,
    const Vector2& size,
    float screenXRatio,
    float screenYRatio,
    const Vector4& bar1Color,
    const Vector4& bar2Color,
    const Vector4& bgColor)
{
    baseSize_ = size;
    screenXRatio_ = screenXRatio;
    screenYRatio_ = screenYRatio;
    isDualBar_ = true;

    Vector2 screenPos = CalculateScreenPosition();

    barSprite_ = std::make_unique<Sprite>();
    barSprite_->Initialize(texture);
    barSprite_->SetSize(baseSize_);
    barSprite_->SetAnchorPoint(Vector2(0.0f, 0.0f));  // 左上アンカー
    barSprite_->SetColor(bar1Color);
    barSprite_->SetPos(screenPos);

    bar2Sprite_ = std::make_unique<Sprite>();
    bar2Sprite_->Initialize(texture);
    bar2Sprite_->SetSize(baseSize_);
    bar2Sprite_->SetAnchorPoint(Vector2(0.0f, 0.0f));
    bar2Sprite_->SetColor(bar2Color);
    bar2Sprite_->SetPos(screenPos);

    bgSprite_ = std::make_unique<Sprite>();
    bgSprite_->Initialize(texture);
    bgSprite_->SetSize(baseSize_);
    bgSprite_->SetAnchorPoint(Vector2(0.0f, 0.0f));
    bgSprite_->SetColor(bgColor);
    bgSprite_->SetPos(screenPos);
}

void HPBarUI::Update(float currentValue, float maxValue)
{
    if (!barSprite_ || !bgSprite_) return;

    float ratio = (maxValue > 0.0f) ? (currentValue / maxValue) : 0.0f;
    ratio = std::clamp(ratio, 0.0f, 1.0f);

    Vector2 screenPos = CalculateScreenPosition();

    barSprite_->SetSize(Vector2(baseSize_.x * ratio, baseSize_.y));
    barSprite_->SetPos(screenPos);
    barSprite_->Update();

    bgSprite_->SetPos(screenPos);
    bgSprite_->Update();
}

void HPBarUI::UpdateDual(float currentHp, float maxHp, float phase2Threshold, uint32_t phase)
{
    if (!barSprite_ || !bar2Sprite_ || !bgSprite_) return;

    Vector2 screenPos = CalculateScreenPosition();

    if (phase == 1) {
        // フェーズ1: 青バー(barSprite_)が HP 減少で縮小、赤バー(bar2Sprite_)はフルサイズ
        float phase1Hp = currentHp - phase2Threshold;
        float phase1MaxHp = maxHp - phase2Threshold;
        float phase1Ratio = (phase1MaxHp > 0.0f) ? (phase1Hp / phase1MaxHp) : 0.0f;
        phase1Ratio = std::clamp(phase1Ratio, 0.0f, 1.0f);

        barSprite_->SetSize(Vector2(baseSize_.x * phase1Ratio, baseSize_.y));
        bar2Sprite_->SetSize(baseSize_);  // フルサイズ
    }
    else {
        // フェーズ2: 青バーは0幅、赤バーが HP 減少で縮小
        float phase2Ratio = (phase2Threshold > 0.0f) ? (currentHp / phase2Threshold) : 0.0f;
        phase2Ratio = std::clamp(phase2Ratio, 0.0f, 1.0f);

        barSprite_->SetSize(Vector2(0.0f, baseSize_.y));
        bar2Sprite_->SetSize(Vector2(baseSize_.x * phase2Ratio, baseSize_.y));
    }

    // 両バーを同じ座標に配置（重なり表示）
    barSprite_->SetPos(screenPos);
    bar2Sprite_->SetPos(screenPos);
    bgSprite_->SetPos(screenPos);

    barSprite_->Update();
    bar2Sprite_->Update();
    bgSprite_->Update();
}

void HPBarUI::Draw()
{
    if (bgSprite_) bgSprite_->Draw();
    if (bar2Sprite_ && isDualBar_) bar2Sprite_->Draw();
    if (barSprite_) barSprite_->Draw();
}

void HPBarUI::SetPosition(float screenXRatio, float screenYRatio)
{
    screenXRatio_ = screenXRatio;
    screenYRatio_ = screenYRatio;
}

void HPBarUI::SetBarColor(const Vector4& color)
{
    if (barSprite_) barSprite_->SetColor(color);
}

void HPBarUI::SetAnchorPoint(const Vector2& anchor)
{
    if (barSprite_) barSprite_->SetAnchorPoint(anchor);
    if (bar2Sprite_) bar2Sprite_->SetAnchorPoint(anchor);
    if (bgSprite_) bgSprite_->SetAnchorPoint(anchor);
}

Vector2 HPBarUI::CalculateScreenPosition() const
{
    return Vector2(
        WinApp::clientWidth * screenXRatio_,
        WinApp::clientHeight * screenYRatio_);
}
