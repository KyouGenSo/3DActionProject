#include "PauseMenu.h"
#include "Sprite.h"
#include "Input.h"
#include "WinApp.h"
#include "EnginePaths.h"
#include <functional>

#ifdef _DEBUG
#include "imgui.h"
#endif

using namespace Tako;

PauseMenu::~PauseMenu()
{
    // ダングリングポインタ防止
    if (winApp_ && onResizeId_ != 0) {
        winApp_->UnregisterOnResizeFunc(onResizeId_);
    }
}

void PauseMenu::Initialize()
{
    // 半透明黒背景オーバーレイ
    overlaySprite_ = std::make_unique<Sprite>();
    overlaySprite_->Initialize(EnginePaths::TexturePath("white.dds"));
    overlaySprite_->SetPos({ 0.0f, 0.0f });
    overlaySprite_->SetSize({ static_cast<float>(WinApp::clientWidth),  static_cast<float>(WinApp::clientHeight) });
    overlaySprite_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

    titleSprite_ = std::make_unique<Sprite>();
    titleSprite_->Initialize("PauseMenu_Text.dds");
    titleSprite_->SetPos({ 960.0f, 200.0f });
    titleSprite_->SetSize({ 400.0f, 100.0f });
    titleSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    const char* buttonTextures[kButtonCount] = {
        "PauseButton_Resume.dds",
        "PauseButton_Title.dds",
        "PauseButton_Exit.dds"
    };

    const float buttonY[kButtonCount] = { 400.0f, 550.0f, 700.0f };

    for (int i = 0; i < kButtonCount; ++i) {
        buttonSprites_[i] = std::make_unique<Sprite>();
        buttonSprites_[i]->Initialize(buttonTextures[i]);
        buttonSprites_[i]->SetPos({ 960.0f, buttonY[i] });
        buttonSprites_[i]->SetSize({ 300.0f, 100.0f });
        buttonSprites_[i]->SetAnchorPoint({ 0.5f, 0.5f });
    }

    // DPAD 操作ガイドスプライト（ControllerUI と同じ座標・サイズ）
    dpadGuideSprite_ = std::make_unique<Sprite>();
    dpadGuideSprite_->Initialize("button/DPAD_Neutral.dds");
    dpadGuideSprite_->SetPos({ 323.0f, 869.0f });
    dpadGuideSprite_->SetSize({ 150.0f, 150.0f });

    dpadUpSprite_ = std::make_unique<Sprite>();
    dpadUpSprite_->Initialize("button/DPAD_Up.dds");
    dpadUpSprite_->SetPos({ 323.0f, 869.0f });
    dpadUpSprite_->SetSize({ 150.0f, 150.0f });

    dpadDownSprite_ = std::make_unique<Sprite>();
    dpadDownSprite_->Initialize("button/DPAD_Down.dds");
    dpadDownSprite_->SetPos({ 323.0f, 869.0f });
    dpadDownSprite_->SetSize({ 150.0f, 150.0f });

    // A ボタンスプライト（ControllerUI と同じ座標・サイズ）
    aButtonUpSprite_ = std::make_unique<Sprite>();
    aButtonUpSprite_->Initialize("button/A_Button_Up.dds");
    aButtonUpSprite_->SetPos({ 1565.0f, 948.0f });
    aButtonUpSprite_->SetSize({ 60.0f, 60.0f });

    aButtonDownSprite_ = std::make_unique<Sprite>();
    aButtonDownSprite_->Initialize("button/A_Button_Down.dds");
    aButtonDownSprite_->SetPos({ 1565.0f, 948.0f });
    aButtonDownSprite_->SetSize({ 60.0f, 60.0f });

    // 選択テキスト（ControllerUI と同じ座標・サイズ）
    sentakuSprite_ = std::make_unique<Sprite>();
    sentakuSprite_->Initialize("sentaku.dds");
    sentakuSprite_->SetPos({ 460.0f, 910.0f });
    sentakuSprite_->SetSize({ 150.0f, 50.0f });

    // 決定テキスト（ControllerUI と同じ座標・サイズ）
    ketteiSprite_ = std::make_unique<Sprite>();
    ketteiSprite_->Initialize("kettei.dds");
    ketteiSprite_->SetPos({ 1518.0f, 1000.0f });
    ketteiSprite_->SetSize({ 150.0f, 50.0f });

    Reset();

    winApp_ = WinApp::GetInstance();
    if (winApp_) {
        onResizeId_ = winApp_->RegisterOnResizeFunc(
            std::bind(&PauseMenu::OnResize, this, std::placeholders::_1)
        );

        // 現在のウィンドウサイズに合わせて位置調整（シーン再入場対応）
        Vector2 currentSize = { static_cast<float>(WinApp::clientWidth), static_cast<float>(WinApp::clientHeight) };
        if (currentSize.x != kBaseWidth || currentSize.y != kBaseHeight) {
            OnResize(currentSize);
        }
    }
}

PauseMenu::Action PauseMenu::Update()
{
    Input* input = Input::GetInstance();

    // 画面サイズに合わせる
    overlaySprite_->SetSize({ static_cast<float>(WinApp::clientWidth),  static_cast<float>(WinApp::clientHeight) });

    isDPadUpPressed_ = input->PushButton(GamepadButton::DPad_Up);
    isDPadDownPressed_ = input->PushButton(GamepadButton::DPad_Down);
    isAPressed_ = input->PushButton(GamepadButton::A);

    // DPAD 上下で選択移動
    if (input->TriggerButton(GamepadButton::DPad_Up)) {
        selectedIndex_ = (selectedIndex_ - 1 + kButtonCount) % kButtonCount;
        UpdateButtonColors();
    }
    if (input->TriggerButton(GamepadButton::DPad_Down)) {
        selectedIndex_ = (selectedIndex_ + 1) % kButtonCount;
        UpdateButtonColors();
    }

    if (input->TriggerButton(GamepadButton::A)) {
        switch (selectedIndex_) {
        case 0: return Action::Resume;
        case 1: return Action::ToTitle;
        case 2: return Action::ExitGame;
        }
    }

    overlaySprite_->Update();
    titleSprite_->Update();
    for (auto& button : buttonSprites_) {
        button->Update();
    }
    dpadGuideSprite_->Update();
    dpadUpSprite_->Update();
    dpadDownSprite_->Update();
    aButtonUpSprite_->Update();
    aButtonDownSprite_->Update();
    ketteiSprite_->Update();
    sentakuSprite_->Update();

    return Action::None;
}

void PauseMenu::Draw()
{
    overlaySprite_->Draw();

    titleSprite_->Draw();

    for (auto& button : buttonSprites_) {
        button->Draw();
    }

    // === 操作説明 UI ===

    sentakuSprite_->Draw();

    if (isDPadUpPressed_) {
        dpadUpSprite_->Draw();
    } else if (isDPadDownPressed_) {
        dpadDownSprite_->Draw();
    } else {
        dpadGuideSprite_->Draw();
    }

    ketteiSprite_->Draw();

    if (isAPressed_) {
        aButtonDownSprite_->Draw();
    } else {
        aButtonUpSprite_->Draw();
    }
}

void PauseMenu::Reset()
{
    selectedIndex_ = 0;
    UpdateButtonColors();
}

void PauseMenu::UpdateButtonColors()
{
    for (int i = 0; i < kButtonCount; ++i) {
        if (i == selectedIndex_) {
            buttonSprites_[i]->SetColor({ kSelectedColorR, kSelectedColorG, kSelectedColorB, 1.0f });
        } else {
            buttonSprites_[i]->SetColor({ kUnselectedColorR, kUnselectedColorG, kUnselectedColorB, 1.0f });
        }
    }
}

void PauseMenu::OnResize(const Vector2& newSize)
{
    float scaleX = newSize.x / kBaseWidth;
    float scaleY = newSize.y / kBaseHeight;

    overlaySprite_->SetPos({ 0.0f, 0.0f });

    titleSprite_->SetPos({ 960.0f * scaleX, 200.0f * scaleY });

    const float buttonY[kButtonCount] = { 400.0f, 550.0f, 700.0f };
    for (int i = 0; i < kButtonCount; ++i) {
        buttonSprites_[i]->SetPos({ 960.0f * scaleX, buttonY[i] * scaleY });
    }

    dpadGuideSprite_->SetPos({ 323.0f * scaleX, 869.0f * scaleY });

    dpadUpSprite_->SetPos({ 323.0f * scaleX, 869.0f * scaleY });

    dpadDownSprite_->SetPos({ 323.0f * scaleX, 869.0f * scaleY });

    aButtonUpSprite_->SetPos({ 1565.0f * scaleX, 948.0f * scaleY });

    aButtonDownSprite_->SetPos({ 1565.0f * scaleX, 948.0f * scaleY });

    sentakuSprite_->SetPos({ 460.0f * scaleX, 910.0f * scaleY });

    ketteiSprite_->SetPos({ 1518.0f * scaleX, 1000.0f * scaleY });
}

void PauseMenu::DrawImGui()
{
#ifdef _DEBUG
    if (ImGui::TreeNode("PauseMenu")) {
        ImGui::Text("Selected Index: %d", selectedIndex_);
        ImGui::Text("Input States:");
        ImGui::Text("  DPad Up: %s", isDPadUpPressed_ ? "Pressed" : "Released");
        ImGui::Text("  DPad Down: %s", isDPadDownPressed_ ? "Pressed" : "Released");
        ImGui::Text("  A Button: %s", isAPressed_ ? "Pressed" : "Released");

        ImGui::Separator();

        if (ImGui::TreeNode("Overlay Sprite")) {
            overlaySprite_->DrawImGui();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Title Sprite (PAUSE)")) {
            titleSprite_->DrawImGui();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Button Sprites")) {
            const char* buttonNames[kButtonCount] = { "Resume", "Title", "Exit" };
            for (int i = 0; i < kButtonCount; ++i) {
                if (ImGui::TreeNode(buttonNames[i])) {
                    buttonSprites_[i]->DrawImGui();
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("DPAD Guide Sprites")) {
            if (ImGui::TreeNode("DPAD Neutral")) {
                dpadGuideSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("DPAD Up")) {
                dpadUpSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("DPAD Down")) {
                dpadDownSprite_->DrawImGui();
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("A Button Sprites")) {
            if (ImGui::TreeNode("A Button Up")) {
                aButtonUpSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("A Button Down")) {
                aButtonDownSprite_->DrawImGui();
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Text Sprites")) {
            if (ImGui::TreeNode("Kettei (決定)")) {
                ketteiSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Sentaku (選択)")) {
                sentakuSprite_->DrawImGui();
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
#endif
}
