#include "TitleScene.h"

#include "SceneManager.h"
#include "Object3dBasic.h"
#include "SpriteBasic.h"
#include "ModelManager.h"
#include "Input.h"
#include "Draw2D.h"
#include "GPUParticle.h"
#include "EnginePaths.h"
#include "Vec3Func.h"
#include <cmath>
#include <numbers>
#include <format>

#ifdef _DEBUG
#include"ImGui.h"
#include "DebugCamera.h"
#include "DebugUIManager.h"
#endif

using namespace Tako;

void TitleScene::Initialize()
{
    /// ================================== ///
    ///              初期化処理              ///
    /// ================================== ///

    emitterManager_ = std::make_unique<EmitterManager>(GPUParticle::GetInstance());

    // シーンプリセット統合保存のため連携
    forceFieldManager_ = std::make_unique<ForceFieldManager>(GPUParticle::GetInstance());
    emitterManager_->SetForceFieldManager(forceFieldManager_.get());

    InitializeDebugUI();
    InitializeCamera();
    InitializePostEffects();
    InitializeSprites();
    InitializeParticles();
}

void TitleScene::Finalize()
{
#ifdef _DEBUG
    DebugUIManager::GetInstance()->ClearGameObjects();
#endif

    PostEffectManager::GetInstance()->ClearEffectChain();
}

void TitleScene::Update()
{
    /// ================================== ///
    ///              更新処理               ///
    /// ================================== ///

    UpdateWindowResize();
    UpdateStartButtonBlink();
    UpdateTitleTextAnimation();
    UpdateSlashParticleAnimation();
    UpdateTitleEffectAnimation();
    UpdateInput();

    titleBG_->Update();
    startButtonText_->Update();

    if (currentFrame_ >= 0 && currentFrame_ < titleTextSprites_.size()) {
        titleTextSprites_[currentFrame_]->Update();
    }

    emitterManager_->Update();
}

void TitleScene::Draw()
{
    /// ================================== ///
    ///              描画処理               ///
    /// ================================== ///

    //------------------背景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();

    titleBG_->Draw();


    //-------------------Model の描画-------------------//
    Object3dBasic::GetInstance()->SetCommonRenderSetting();




    //------------------前景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();

    if (currentFrame_ >= 0 && currentFrame_ < titleTextSprites_.size()) {
        titleTextSprites_[currentFrame_]->Draw();
    }

    // タイトルテキストの上に描画
    if (isEffectPlaying_ && titleTextEffect_) {
        titleTextEffect_->Draw();
    }

}

void TitleScene::DrawWithoutEffect()
{
    /// ================================== ///
    ///              描画処理               ///
    /// ================================== ///

    //------------------背景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();




    //-------------------Model の描画-------------------//
    Object3dBasic::GetInstance()->SetCommonRenderSetting();





    //------------------前景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();


    startButtonText_->Draw();

}

void TitleScene::DrawImGui()
{
#ifdef _DEBUG

    /// ================================== ///
    ///             ImGui の描画              ///
    /// ================================== ///

    if (ImGui::Button("Play Animation")) {
        PlayTitleAnimation();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Animation")) {
        StopTitleAnimation();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Animation")) {
        ResetTitleAnimation();
    }

    ImGui::Separator();
    ImGui::Text("Animation Settings");
    ImGui::SliderInt("Animation Speed", &animationSpeed_, 1, 30);
    ImGui::Checkbox("Loop", &isLoop_);

    ImGui::Separator();
    ImGui::Text("Status");
    ImGui::Text("Current Frame: %d / %d", currentFrame_ + 1, static_cast<int>(titleTextSprites_.size()));
    ImGui::Text("Is Playing: %s", isPlaying_ ? "Yes" : "No");
    ImGui::Text("Animation Complete: %s", animationComplete_ ? "Yes" : "No");

    ImGui::Separator();
    ImGui::Text("Manual Frame Control");
    if (ImGui::SliderInt("Frame", &currentFrame_, 0, static_cast<int>(titleTextSprites_.size()) - 1)) {
        // 手動変更時はアニメーションを停止
        isPlaying_ = false;
    }

    ImGui::Checkbox("Enable Blinking", &isButtonBlinking_);

    ImGui::Separator();
    ImGui::Text("Blink Parameters");
    ImGui::SliderFloat("Blink Speed", &blinkSpeed_, 0.5f, 10.0f, "%.1f");
    ImGui::SliderFloat("Min Alpha", &blinkMinAlpha_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Max Alpha", &blinkMaxAlpha_, 0.0f, 1.0f, "%.2f");

    if (ImGui::Button("Reset Timer")) {
        blinkTimer_ = 0.0f;
    }

    ImGui::Separator();
    ImGui::Text("Current Status");
    ImGui::Text("Timer: %.2f", blinkTimer_);
    float currentAlpha = startButtonText_->GetColor().w;
    ImGui::Text("Current Alpha: %.2f", currentAlpha);

    ImGui::Separator();
    ImGui::Text("Presets");
    if (ImGui::Button("Slow Fade")) {
        blinkSpeed_ = 1.0f;
        blinkMinAlpha_ = 0.3f;
        blinkMaxAlpha_ = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fast Blink")) {
        blinkSpeed_ = 5.0f;
        blinkMinAlpha_ = 0.0f;
        blinkMaxAlpha_ = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Gentle Pulse")) {
        blinkSpeed_ = 2.0f;
        blinkMinAlpha_ = 0.5f;
        blinkMaxAlpha_ = 1.0f;
    }

    ImGui::Separator();
    ImGui::Text("Title Text Expansion Effect");

    if (ImGui::Button("Trigger Effect")) {
        isEffectPlaying_ = true;
        effectTriggered_ = false;  // 再度トリガー可能にする
        effectTimer_ = 0.0f;
        effectScale_ = 1.0f;
        effectAlpha_ = effectInitialAlpha_;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Effect")) {
        isEffectPlaying_ = false;
        effectAlpha_ = 0.0f;
    }

    ImGui::SliderFloat("Effect Duration", &effectDuration_, 0.5f, 5.0f, "%.1f sec");
    ImGui::SliderFloat("Max Scale", &effectMaxScale_, 1.0f, 3.0f, "%.1f");
    ImGui::SliderFloat("Initial Alpha", &effectInitialAlpha_, 0.0f, 1.0f, "%.2f");

    ImGui::Text("Effect Status");
    ImGui::Text("Is Playing: %s", isEffectPlaying_ ? "Yes" : "No");
    ImGui::Text("Timer: %.2f / %.2f", effectTimer_, effectDuration_);
    ImGui::Text("Current Scale: %.2f", effectScale_);
    ImGui::Text("Current Alpha: %.2f", effectAlpha_);

    if (ImGui::Button("Reset All")) {
        ResetTitleAnimation();
        isEffectPlaying_ = false;
        effectTriggered_ = false;
        effectTimer_ = 0.0f;
        effectScale_ = 1.0f;
        effectAlpha_ = 0.0f;
        titleTextEffect_->SetAlpha(0.0f);
    }

    ImGui::Separator();
    ImGui::Text("Slash Particle Animation");

    if (ImGui::Button("Start Particle Animation")) {
        isSlashEmitterAnimating_ = true;
        slashEmitterAnimTimer_ = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Particle Animation")) {
        isSlashEmitterAnimating_ = false;
    }

    ImGui::Text("Start Values:");
    int startCount = static_cast<int>(slashEmitterStartCount_);
    if (ImGui::SliderInt("Start Count", &startCount, 1, 200)) {
        slashEmitterStartCount_ = static_cast<uint32_t>(startCount);
    }
    ImGui::SliderFloat("Start Frequency", &slashEmitterStartFreq_, 0.001f, 1.0f, "%.3f");

    ImGui::Text("End Values:");
    int endCount = static_cast<int>(slashEmitterEndCount_);
    if (ImGui::SliderInt("End Count", &endCount, 1, 300)) {
        slashEmitterEndCount_ = static_cast<uint32_t>(endCount);
    }
    ImGui::SliderFloat("End Frequency", &slashEmitterEndFreq_, 0.001f, 1.0f, "%.3f");

    ImGui::Text("Current Status:");
    ImGui::Text("Is Animating: %s", isSlashEmitterAnimating_ ? "Yes" : "No");
    ImGui::Text("Timer: %.2f / %.2f", slashEmitterAnimTimer_, slashEmitterAnimDuration_);

    auto slashEmitter = emitterManager_->GetEmitterByName("slash");
    if (slashEmitter) {
        ImGui::Text("Current Count: %u", slashEmitter->GetParticleCount());
        ImGui::Text("Current Frequency: %.3f", slashEmitter->GetFrequency());
    }

#endif // _DEBUG
}

void TitleScene::PlayTitleAnimation()
{
    isPlaying_ = true;
    animationComplete_ = false;

    isSlashEmitterAnimating_ = true;
    slashEmitterAnimTimer_ = 0.0f;

}

void TitleScene::StopTitleAnimation()
{
    isPlaying_ = false;
}

void TitleScene::ResetTitleAnimation()
{
    currentFrame_ = 0;
    frameCounter_ = 0;
    isPlaying_ = false;
    animationComplete_ = false;

    isSlashEmitterAnimating_ = false;
    slashEmitterAnimTimer_ = 0.0f;

    // slash エミッターを初期値に戻す
    auto slashEmitter = emitterManager_->GetEmitterByName("slash");
    if (slashEmitter) {
        slashEmitter->SetParticleCount(slashEmitterStartCount_);
        slashEmitter->SetFrequency(slashEmitterStartFreq_);
    }
}

void TitleScene::InitializeDebugUI()
{
#ifdef _DEBUG
    DebugCamera::GetInstance()->Initialize();
    Object3dBasic::GetInstance()->SetDebug(false);
    Draw2D::GetInstance()->SetDebug(false);
    GPUParticle::GetInstance()->SetIsDebug(false);

    DebugUIManager::GetInstance()->RegisterGameObject("TitleScene",
        [this]() { this->DrawImGui(); });

    DebugUIManager::GetInstance()->RegisterGameObject("BackGround",
        [this]() { if (titleBG_) titleBG_->DrawImGui(); });

    for (int i = 0; i < 10; ++i) {
        DebugUIManager::GetInstance()->RegisterGameObject(std::format("TitleText{}", i + 1),
            [this, i]() {
                titleTextSprites_[i]->DrawImGui();
            });
    }

    DebugUIManager::GetInstance()->RegisterGameObject("StartButtonText",
        [this]() { if (startButtonText_) startButtonText_->DrawImGui(); });

    DebugUIManager::GetInstance()->SetEmitterManager(emitterManager_.get());
    DebugUIManager::GetInstance()->SetForceFieldManager(forceFieldManager_.get());
#endif
}

void TitleScene::InitializeCamera()
{
    (*Object3dBasic::GetInstance()->GetCamera())->SetRotate(Vector3(0.2f, 0.0f, 0.0f));
    (*Object3dBasic::GetInstance()->GetCamera())->SetTranslate(Vector3(0.0f, cameraY_ + offsetY, cameraZ_));
}

void TitleScene::InitializePostEffects()
{
    rgbSplitParam_.redOffset = Vector2(-0.01f, 0.f);
    rgbSplitParam_.greenOffset = Vector2(0.01f, 0.f);
    rgbSplitParam_.blueOffset = Vector2(0.0f, 0.f);
    rgbSplitParam_.intensity = 0.08f;

    vignetteParam_.color = Vector3(1.f, 1.f, 1.f);
    vignetteParam_.power = 0.02f;
    vignetteParam_.range = 20.0f;

    PostEffectManager::GetInstance()->AddEffectToChain("RGBSplit");
    PostEffectManager::GetInstance()->AddEffectToChain("Vignette");
    PostEffectManager::GetInstance()->SetEffectParam("RGBSplit", rgbSplitParam_);
    PostEffectManager::GetInstance()->SetEffectParam("Vignette", vignetteParam_);
}

void TitleScene::InitializeSprites()
{
    titleBG_ = make_unique<Sprite>();
    titleBG_->Initialize(EnginePaths::TexturePath("black.dds"));
    titleBG_->SetPos(Vector2(0.f, 0.f));
    titleBG_->SetSize(Vector2(static_cast<float>(WinApp::clientWidth), static_cast<float>(WinApp::clientHeight)));

    // 10枚のアニメーション用画像
    titleTextSprites_.reserve(10);
    for (int i = 0; i < 10; ++i) {
        std::string texturePath = std::format("title_text/title_text_{}.dds", i + 1);
        auto sprite = make_unique<Sprite>();
        sprite->Initialize(texturePath);
        sprite->SetSize(Vector2(titleTextWidth_, titleTextHeight_));
        sprite->SetPos(Vector2(WinApp::clientWidth / 2.f - titleTextWidth_ / 2.f, titleTextY_));
        titleTextSprites_.push_back(std::move(sprite));
    }

    startButtonText_ = make_unique<Sprite>();
    startButtonText_->Initialize("titlescene_button.dds");
    startButtonText_->SetPos(Vector2(
        WinApp::clientWidth / 2.f - startButtonText_->GetSize().x / 2.f,
        WinApp::clientHeight - startButtonBottomOffset_));

    // 拡大フェードアウト用
    titleTextEffect_ = make_unique<Sprite>();
    titleTextEffect_->Initialize("title_text/title_text_10.dds");
    titleTextEffect_->SetSize(Vector2(titleTextWidth_, titleTextHeight_));
    titleTextEffect_->SetPos(Vector2(WinApp::clientWidth / 2.f - titleTextWidth_ / 2.f, titleTextY_));
    titleTextEffect_->SetAlpha(0.0f);
}

void TitleScene::InitializeParticles()
{
    emitterManager_->LoadScenePreset("title_preset");

    auto slashEmitter = emitterManager_->GetEmitterByName("slash");
    if (slashEmitter) {
        slashEmitter->SetParticleCount(slashEmitterStartCount_);
        slashEmitter->SetFrequency(slashEmitterStartFreq_);
    }
}

void TitleScene::UpdateWindowResize()
{
    titleBG_->SetSize(Vector2(static_cast<float>(WinApp::clientWidth), static_cast<float>(WinApp::clientHeight)));

    for (auto& sprite : titleTextSprites_) {
        sprite->SetPos(Vector2(WinApp::clientWidth / 2.f - sprite->GetSize().x / 2.f, titleTextY_));
    }

    startButtonText_->SetPos(Vector2(
        WinApp::clientWidth / 2.f - startButtonText_->GetSize().x / 2.f,
        WinApp::clientHeight - startButtonBottomOffset_));
}

void TitleScene::UpdateStartButtonBlink()
{
    if (!isButtonBlinking_) return;

    blinkTimer_ += 1.0f / 60.0f;
    if (blinkTimer_ > 1000.f) blinkTimer_ = 0.f; // オーバーフロー防止

    // sin の結果(-1〜1)を0〜1に正規化し、min〜max にマッピング
    float sineValue = std::sin(blinkTimer_ * blinkSpeed_ * std::numbers::pi_v<float>);
    float normalizedSine = (sineValue + 1.0f) * 0.5f;
    float alpha = blinkMinAlpha_ + (blinkMaxAlpha_ - blinkMinAlpha_) * normalizedSine;

    startButtonText_->SetAlpha(alpha);
}

void TitleScene::UpdateTitleTextAnimation()
{
    if (!isPlaying_) return;

    frameCounter_++;

    if (frameCounter_ >= animationSpeed_) {
        frameCounter_ = 0;
        currentFrame_++;

        if (currentFrame_ >= titleTextSprites_.size()) {
            if (isLoop_) {
                currentFrame_ = 0;
            }
            else {
                // 最後のフレームで停止
                currentFrame_ = static_cast<int>(titleTextSprites_.size()) - 1;
                isPlaying_ = false;
                animationComplete_ = true;
            }
        }
    }

    // 完了時にエフェクトを一度だけ開始
    if (animationComplete_ && !effectTriggered_ && !isLoop_) {
        isEffectPlaying_ = true;
        effectTriggered_ = true;
        effectTimer_ = 0.0f;
        effectScale_ = 1.0f;
        effectAlpha_ = effectInitialAlpha_;
    }
}

void TitleScene::UpdateSlashParticleAnimation()
{
    if (!isSlashEmitterAnimating_) return;

    slashEmitterAnimTimer_ += 1.0f / 60.0f;

    float progress = slashEmitterAnimTimer_ / slashEmitterAnimDuration_;

    if (progress >= sceneTransitionProgress_) {
        SceneManager::GetInstance()->ChangeScene("game", "Fade", 0.2f);
    }

    if (progress >= 1.0f) {
        progress = 1.0f;
        isSlashEmitterAnimating_ = false;
    }

    auto slashEmitter = emitterManager_->GetEmitterByName("slash");
    if (slashEmitter) {
        uint32_t currentCount = static_cast<uint32_t>(
            Vec3::Lerp(
                static_cast<float>(slashEmitterStartCount_),
                static_cast<float>(slashEmitterEndCount_),
                progress));

        float currentFreq = Vec3::Lerp(slashEmitterStartFreq_, slashEmitterEndFreq_, progress);

        slashEmitter->SetParticleCount(currentCount);
        slashEmitter->SetFrequency(currentFreq);
    }
}

void TitleScene::UpdateTitleEffectAnimation()
{
    if (!isEffectPlaying_) return;

    effectTimer_ += 1.0f / 60.0f;

    float progress = effectTimer_ / effectDuration_;

    if (progress >= 1.0f) {
        progress = 1.0f;
        isEffectPlaying_ = false;
        effectAlpha_ = 0.0f;
    }
    else {
        effectScale_ = Vec3::Lerp(1.0f, effectMaxScale_, progress);

        effectAlpha_ = Vec3::Lerp(effectInitialAlpha_, 0.0f, progress);
    }

    titleTextEffect_->SetSize(Vector2(titleTextWidth_ * effectScale_, titleTextHeight_ * effectScale_));
    // 拡大してもセンターに保つよう位置を補正
    float centerX = WinApp::clientWidth / 2.f;
    float centerY = titleTextY_ + titleTextHeight_ / 2.f;
    titleTextEffect_->SetPos(Vector2(
        centerX - (titleTextWidth_ / 2.f * effectScale_),
        centerY - (titleTextHeight_ / 2.f * effectScale_)
    ));
    titleTextEffect_->SetAlpha(effectAlpha_);
    titleTextEffect_->Update();
}

void TitleScene::UpdateInput()
{
    if (Input::GetInstance()->TriggerKey(DIK_SPACE) ||
        Input::GetInstance()->TriggerButton(GamepadButton::A)) {
        if (!isPlaying_ && !isSlashEmitterAnimating_) {
            PlayTitleAnimation();
        }
    }
}