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

  // エミッタマネージャ生成
    emitterManager_ = std::make_unique<EmitterManager>(GPUParticle::GetInstance());

    // フォースフィールドマネージャ生成（シーンプリセット統合保存のため連携）
    forceFieldManager_ = std::make_unique<ForceFieldManager>(GPUParticle::GetInstance());
    emitterManager_->SetForceFieldManager(forceFieldManager_.get());

    // 各種初期化処理を関数化して呼び出し
    InitializeDebugUI();       // デバッグ UI 初期化
    InitializeCamera();         // カメラ設定
    InitializePostEffects();    // ポストエフェクト設定
    InitializeSprites();        // スプライト初期化
    InitializeParticles();      // パーティクル初期化
}

void TitleScene::Finalize()
{
#ifdef _DEBUG
    // ゲームオブジェクトの登録解除
    DebugUIManager::GetInstance()->ClearGameObjects();
#endif

    PostEffectManager::GetInstance()->ClearEffectChain();
}

void TitleScene::Update()
{
    /// ================================== ///
    ///              更新処理               ///
    /// ================================== ///

  // 各種更新処理を関数化して呼び出し
    UpdateWindowResize();           // ウィンドウリサイズ処理
    UpdateStartButtonBlink();       // スタートボタン点滅更新
    UpdateTitleTextAnimation();     // タイトルテキストアニメーション更新
    UpdateSlashParticleAnimation(); // slash パーティクルアニメーション更新
    UpdateTitleEffectAnimation();   // タイトルエフェクトアニメーション更新
    UpdateInput();                  // 入力処理

    // 基本的なスプライトの更新
    titleBG_->Update();
    startButtonText_->Update();

    // 現在のフレームのスプライトを更新
    if (currentFrame_ >= 0 && currentFrame_ < titleTextSprites_.size()) {
        titleTextSprites_[currentFrame_]->Update();
    }

    // エミッターマネージャーの更新
    emitterManager_->Update();
}

void TitleScene::Draw()
{
    /// ================================== ///
    ///              描画処理               ///
    /// ================================== ///

    //------------------背景 Sprite の描画------------------//
    // スプライト共通描画設定
    SpriteBasic::GetInstance()->SetCommonRenderSetting();

    titleBG_->Draw();


    //-------------------Model の描画-------------------//
    // 3D モデル共通描画設定
    Object3dBasic::GetInstance()->SetCommonRenderSetting();




    //------------------前景 Sprite の描画------------------//
    // スプライト共通描画設定
    SpriteBasic::GetInstance()->SetCommonRenderSetting();

    // 現在のフレームのタイトルテキストを描画
    if (currentFrame_ >= 0 && currentFrame_ < titleTextSprites_.size()) {
        titleTextSprites_[currentFrame_]->Draw();
    }

    // 拡大フェードアウトエフェクトの描画（タイトルテキストの上に描画）
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
    // スプライト共通描画設定
    SpriteBasic::GetInstance()->SetCommonRenderSetting();




    //-------------------Model の描画-------------------//
    // 3D モデル共通描画設定
    Object3dBasic::GetInstance()->SetCommonRenderSetting();





    //------------------前景 Sprite の描画------------------//
    // スプライト共通描画設定
    SpriteBasic::GetInstance()->SetCommonRenderSetting();


    startButtonText_->Draw();

}

void TitleScene::DrawImGui()
{
#ifdef _DEBUG

    /// ================================== ///
    ///             ImGui の描画              ///
    /// ================================== ///

  // アニメーション制御ボタン
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

    // アニメーション設定
    ImGui::Separator();
    ImGui::Text("Animation Settings");
    ImGui::SliderInt("Animation Speed", &animationSpeed_, 1, 30);
    ImGui::Checkbox("Loop", &isLoop_);

    // 現在の状態表示
    ImGui::Separator();
    ImGui::Text("Status");
    ImGui::Text("Current Frame: %d / %d", currentFrame_ + 1, static_cast<int>(titleTextSprites_.size()));
    ImGui::Text("Is Playing: %s", isPlaying_ ? "Yes" : "No");
    ImGui::Text("Animation Complete: %s", animationComplete_ ? "Yes" : "No");

    // 手動フレーム制御
    ImGui::Separator();
    ImGui::Text("Manual Frame Control");
    if (ImGui::SliderInt("Frame", &currentFrame_, 0, static_cast<int>(titleTextSprites_.size()) - 1)) {
        // フレームを手動で変更した場合はアニメーションを停止
        isPlaying_ = false;
    }

    // スタートボタン点滅コントロール
    // 点滅有効/無効チェックボックス
    ImGui::Checkbox("Enable Blinking", &isButtonBlinking_);

    // 点滅パラメータの調整
    ImGui::Separator();
    ImGui::Text("Blink Parameters");
    ImGui::SliderFloat("Blink Speed", &blinkSpeed_, 0.5f, 10.0f, "%.1f");
    ImGui::SliderFloat("Min Alpha", &blinkMinAlpha_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Max Alpha", &blinkMaxAlpha_, 0.0f, 1.0f, "%.2f");

    // リセットボタン
    if (ImGui::Button("Reset Timer")) {
        blinkTimer_ = 0.0f;
    }

    // 現在の状態表示
    ImGui::Separator();
    ImGui::Text("Current Status");
    ImGui::Text("Timer: %.2f", blinkTimer_);
    float currentAlpha = startButtonText_->GetColor().w;
    ImGui::Text("Current Alpha: %.2f", currentAlpha);

    // プリセット設定
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

    // 拡大エフェクトコントロール
    ImGui::Separator();
    ImGui::Text("Title Text Expansion Effect");

    // エフェクト手動トリガー
    if (ImGui::Button("Trigger Effect")) {
        isEffectPlaying_ = true;
        effectTriggered_ = false;  // リセットして再度トリガー可能にする
        effectTimer_ = 0.0f;
        effectScale_ = 1.0f;
        effectAlpha_ = effectInitialAlpha_;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Effect")) {
        isEffectPlaying_ = false;
        effectAlpha_ = 0.0f;
    }

    // エフェクトパラメータ調整
    ImGui::SliderFloat("Effect Duration", &effectDuration_, 0.5f, 5.0f, "%.1f sec");
    ImGui::SliderFloat("Max Scale", &effectMaxScale_, 1.0f, 3.0f, "%.1f");
    ImGui::SliderFloat("Initial Alpha", &effectInitialAlpha_, 0.0f, 1.0f, "%.2f");

    // エフェクト状態表示
    ImGui::Text("Effect Status");
    ImGui::Text("Is Playing: %s", isEffectPlaying_ ? "Yes" : "No");
    ImGui::Text("Timer: %.2f / %.2f", effectTimer_, effectDuration_);
    ImGui::Text("Current Scale: %.2f", effectScale_);
    ImGui::Text("Current Alpha: %.2f", effectAlpha_);

    // アニメーションとエフェクトのリセット
    if (ImGui::Button("Reset All")) {
        ResetTitleAnimation();
        isEffectPlaying_ = false;
        effectTriggered_ = false;
        effectTimer_ = 0.0f;
        effectScale_ = 1.0f;
        effectAlpha_ = 0.0f;
        titleTextEffect_->SetAlpha(0.0f);
    }

    // Slash パーティクルエミッターアニメーションコントロール
    ImGui::Separator();
    ImGui::Text("Slash Particle Animation");

    // アニメーション制御
    if (ImGui::Button("Start Particle Animation")) {
        isSlashEmitterAnimating_ = true;
        slashEmitterAnimTimer_ = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Particle Animation")) {
        isSlashEmitterAnimating_ = false;
    }

    // パラメータ調整
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

    // 現在の状態表示
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

    // slash パーティクルエミッターのアニメーションも開始
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

    // slash パーティクルエミッターもリセット
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

    // TitleScene 自体のデバッグ UI 登録
    DebugUIManager::GetInstance()->RegisterGameObject("TitleScene",
        [this]() { this->DrawImGui(); });

    // 背景のデバッグ UI 登録
    DebugUIManager::GetInstance()->RegisterGameObject("BackGround",
        [this]() { if (titleBG_) titleBG_->DrawImGui(); });

    // タイトルテキストスプライトのデバッグ UI 登録（10枚）
    for (int i = 0; i < 10; ++i) {
        DebugUIManager::GetInstance()->RegisterGameObject(std::format("TitleText{}", i + 1),
            [this, i]() {
                titleTextSprites_[i]->DrawImGui();
            });
    }

    // スタートボタンテキストのデバッグ UI 登録
    DebugUIManager::GetInstance()->RegisterGameObject("StartButtonText",
        [this]() { if (startButtonText_) startButtonText_->DrawImGui(); });

    // エミッターマネージャーの設定
    DebugUIManager::GetInstance()->SetEmitterManager(emitterManager_.get());
    DebugUIManager::GetInstance()->SetForceFieldManager(forceFieldManager_.get());
#endif
}

void TitleScene::InitializeCamera()
{
    // カメラの回転と位置を設定
    (*Object3dBasic::GetInstance()->GetCamera())->SetRotate(Vector3(0.2f, 0.0f, 0.0f));
    (*Object3dBasic::GetInstance()->GetCamera())->SetTranslate(Vector3(0.0f, cameraY_ + offsetY, cameraZ_));
}

void TitleScene::InitializePostEffects()
{
    // RGBSplit エフェクトのパラメータ設定
    rgbSplitParam_.redOffset = Vector2(-0.01f, 0.f);
    rgbSplitParam_.greenOffset = Vector2(0.01f, 0.f);
    rgbSplitParam_.blueOffset = Vector2(0.0f, 0.f);
    rgbSplitParam_.intensity = 0.08f;

    // Vignette エフェクトのパラメータ設定
    vignetteParam_.color = Vector3(1.f, 1.f, 1.f);
    vignetteParam_.power = 0.02f;
    vignetteParam_.range = 20.0f;

    // ポストエフェクトチェーンに追加
    PostEffectManager::GetInstance()->AddEffectToChain("RGBSplit");
    PostEffectManager::GetInstance()->AddEffectToChain("Vignette");
    PostEffectManager::GetInstance()->SetEffectParam("RGBSplit", rgbSplitParam_);
    PostEffectManager::GetInstance()->SetEffectParam("Vignette", vignetteParam_);
}

void TitleScene::InitializeSprites()
{
    // 背景画像の初期化
    titleBG_ = make_unique<Sprite>();
    titleBG_->Initialize(EnginePaths::TexturePath("black.dds"));
    titleBG_->SetPos(Vector2(0.f, 0.f));
    titleBG_->SetSize(Vector2(static_cast<float>(WinApp::clientWidth), static_cast<float>(WinApp::clientHeight)));

    // タイトルテキストの初期化（10枚のアニメーション用画像）
    titleTextSprites_.reserve(10);  // 10枚分のメモリを確保
    for (int i = 0; i < 10; ++i) {
        std::string texturePath = std::format("title_text/title_text_{}.dds", i + 1);
        auto sprite = make_unique<Sprite>();
        sprite->Initialize(texturePath);
        sprite->SetSize(Vector2(titleTextWidth_, titleTextHeight_));
        sprite->SetPos(Vector2(WinApp::clientWidth / 2.f - titleTextWidth_ / 2.f, titleTextY_));
        titleTextSprites_.push_back(std::move(sprite));
    }

    // スタートボタンテキストの初期化
    startButtonText_ = make_unique<Sprite>();
    startButtonText_->Initialize("titlescene_button.dds");
    startButtonText_->SetPos(Vector2(
        WinApp::clientWidth / 2.f - startButtonText_->GetSize().x / 2.f,
        WinApp::clientHeight - startButtonBottomOffset_));

    // タイトルテキストエフェクトの初期化（拡大フェードアウト用）
    titleTextEffect_ = make_unique<Sprite>();
    titleTextEffect_->Initialize("title_text/title_text_10.dds");
    titleTextEffect_->SetSize(Vector2(titleTextWidth_, titleTextHeight_));
    titleTextEffect_->SetPos(Vector2(WinApp::clientWidth / 2.f - titleTextWidth_ / 2.f, titleTextY_));
    titleTextEffect_->SetAlpha(0.0f);  // 初期状態では非表示
}

void TitleScene::InitializeParticles()
{
    // エミッタマネージャからプリセットを読み込み
    emitterManager_->LoadScenePreset("title_preset");

    // slash エミッターの初期設定
    auto slashEmitter = emitterManager_->GetEmitterByName("slash");
    if (slashEmitter) {
        // 初期値を設定
        slashEmitter->SetParticleCount(slashEmitterStartCount_);
        slashEmitter->SetFrequency(slashEmitterStartFreq_);
    }
}

void TitleScene::UpdateWindowResize()
{
    // ウィンドウサイズに応じてスプライトの位置を調整
    titleBG_->SetSize(Vector2(static_cast<float>(WinApp::clientWidth), static_cast<float>(WinApp::clientHeight)));

    // タイトルテキストの位置更新（すべてのスプライトの位置を更新）
    for (auto& sprite : titleTextSprites_) {
        sprite->SetPos(Vector2(WinApp::clientWidth / 2.f - sprite->GetSize().x / 2.f, titleTextY_));
    }

    // スタートボタンの位置更新
    startButtonText_->SetPos(Vector2(
        WinApp::clientWidth / 2.f - startButtonText_->GetSize().x / 2.f,
        WinApp::clientHeight - startButtonBottomOffset_));
}

void TitleScene::UpdateStartButtonBlink()
{
    // スタートボタンの点滅アニメーション処理
    if (!isButtonBlinking_) return;

    // タイマーを更新（60FPS を想定して1/60秒ずつ加算）
    blinkTimer_ += 1.0f / 60.0f;
    if (blinkTimer_ > 1000.f) blinkTimer_ = 0.f; // タイマーのオーバーフロー防止

    // サイン波を使用してアルファ値を計算
    // sin 関数の結果（-1〜1）を0〜1の範囲に正規化し、指定範囲にマッピング
    float sineValue = std::sin(blinkTimer_ * blinkSpeed_ * std::numbers::pi_v<float>);
    float normalizedSine = (sineValue + 1.0f) * 0.5f;  // -1〜1 を 0〜1 に変換
    float alpha = blinkMinAlpha_ + (blinkMaxAlpha_ - blinkMinAlpha_) * normalizedSine;

    // アルファ値をスプライトに適用
    startButtonText_->SetAlpha(alpha);
}

void TitleScene::UpdateTitleTextAnimation()
{
    // アニメーション更新処理
    if (!isPlaying_) return;

    frameCounter_++;

    // 指定された速度でフレームを切り替え
    if (frameCounter_ >= animationSpeed_) {
        frameCounter_ = 0;
        currentFrame_++;

        // アニメーションの終端処理
        if (currentFrame_ >= titleTextSprites_.size()) {
            if (isLoop_) {
                // ループ再生の場合は最初に戻る
                currentFrame_ = 0;
            }
            else {
                // ループしない場合は最後のフレームで停止
                currentFrame_ = static_cast<int>(titleTextSprites_.size()) - 1;
                isPlaying_ = false;
                animationComplete_ = true;
            }
        }
    }

    // タイトルアニメーション完了時にエフェクトを開始
    if (animationComplete_ && !effectTriggered_ && !isLoop_) {
        // エフェクトを一度だけトリガー
        isEffectPlaying_ = true;
        effectTriggered_ = true;
        effectTimer_ = 0.0f;
        effectScale_ = 1.0f;
        effectAlpha_ = effectInitialAlpha_;
    }
}

void TitleScene::UpdateSlashParticleAnimation()
{
    // slash パーティクルエミッターのアニメーション更新
    if (!isSlashEmitterAnimating_) return;

    // タイマーを更新（60FPS を想定）
    slashEmitterAnimTimer_ += 1.0f / 60.0f;

    // 進行度を計算（0.0 〜 1.0）
    float progress = slashEmitterAnimTimer_ / slashEmitterAnimDuration_;

    if (progress >= sceneTransitionProgress_) {
        SceneManager::GetInstance()->ChangeScene("game", "Fade", 0.2f);
    }

    if (progress >= 1.0f) {
        // アニメーション終了
        progress = 1.0f;
        isSlashEmitterAnimating_ = false;
    }

    // エミッターを取得してパラメータを更新
    auto slashEmitter = emitterManager_->GetEmitterByName("slash");
    if (slashEmitter) {
        // count 値を線形補間
        uint32_t currentCount = static_cast<uint32_t>(
            Vec3::Lerp(
                static_cast<float>(slashEmitterStartCount_),
                static_cast<float>(slashEmitterEndCount_),
                progress));

        // frequency 値を線形補間
        float currentFreq = Vec3::Lerp(slashEmitterStartFreq_, slashEmitterEndFreq_, progress);

        // パラメータを適用
        slashEmitter->SetParticleCount(currentCount);
        slashEmitter->SetFrequency(currentFreq);
    }
}

void TitleScene::UpdateTitleEffectAnimation()
{
    // 拡大フェードアウトエフェクトの更新
    if (!isEffectPlaying_) return;

    // タイマーを更新（60FPS を想定）
    effectTimer_ += 1.0f / 60.0f;

    // 進行度を計算（0.0 〜 1.0）
    float progress = effectTimer_ / effectDuration_;

    if (progress >= 1.0f) {
        // エフェクト終了
        progress = 1.0f;
        isEffectPlaying_ = false;
        effectAlpha_ = 0.0f;
    }
    else {
        // スケールを線形補間で拡大
        effectScale_ = Vec3::Lerp(1.0f, effectMaxScale_, progress);

        // アルファ値を線形補間でフェードアウト
        effectAlpha_ = Vec3::Lerp(effectInitialAlpha_, 0.0f, progress);
    }

    // エフェクトスプライトに適用
    titleTextEffect_->SetSize(Vector2(titleTextWidth_ * effectScale_, titleTextHeight_ * effectScale_));
    // 拡大してもセンターに保つために位置を調整
    float centerX = WinApp::clientWidth / 2.f;
    float centerY = titleTextY_ + titleTextHeight_ / 2.f; // 元の位置 + 高さの半分
    titleTextEffect_->SetPos(Vector2(
        centerX - (titleTextWidth_ / 2.f * effectScale_),
        centerY - (titleTextHeight_ / 2.f * effectScale_)
    ));
    titleTextEffect_->SetAlpha(effectAlpha_);
    titleTextEffect_->Update();
}

void TitleScene::UpdateInput()
{
    // キーボードまたはゲームパッドの入力処理
    if (Input::GetInstance()->TriggerKey(DIK_SPACE) ||
        Input::GetInstance()->TriggerButton(GamepadButton::A)) {
        // スペースキー押下時にアニメーションを再生
        if (!isPlaying_ && !isSlashEmitterAnimating_) {
            PlayTitleAnimation();
        }
    }
}