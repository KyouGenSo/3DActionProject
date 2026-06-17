#include "ControllerUI.h"
#include "WinApp.h"
#include "Input.h"
#include "Object/Boss/Boss.h"
#include <cmath>
#include <DirectXMath.h>
#include <format>
#include <functional>

#ifdef _DEBUG
#include "imgui.h"
#endif

using namespace Tako;

ControllerUI::~ControllerUI()
{
    // ダングリングポインタ防止
    if (winApp_ && onResizeId_ != 0) {
        winApp_->UnregisterOnResizeFunc(onResizeId_);
    }
}

void ControllerUI::Initialize()
{
    // ボタンスプライト初期化
    aButtonUpSprite_ = std::make_unique<Sprite>();
    aButtonUpSprite_->Initialize("button/A_Button_Up.dds");
    aButtonUpSprite_->SetPos({ 1565.0f, 948.0f });
    aButtonUpSprite_->SetSize({ 60.0f, 60.0f });

    aButtonDownSprite_ = std::make_unique<Sprite>();
    aButtonDownSprite_->Initialize("button/A_Button_Down.dds");
    aButtonDownSprite_->SetPos({ 1565.0f, 948.0f });
    aButtonDownSprite_->SetSize({ 60.0f, 60.0f });

    bButtonUpSprite_ = std::make_unique<Sprite>();
    bButtonUpSprite_->Initialize("button/B_Button_Up.dds");
    bButtonUpSprite_->SetPos({ 1640.0f, 869.0f });
    bButtonUpSprite_->SetSize({ 60.0f, 60.0f });

    bButtonDownSprite_ = std::make_unique<Sprite>();
    bButtonDownSprite_->Initialize("button/B_Button_Down.dds");
    bButtonDownSprite_->SetPos({ 1640.0f, 869.0f });
    bButtonDownSprite_->SetSize({ 60.0f, 60.0f });

    xButtonUpSprite_ = std::make_unique<Sprite>();
    xButtonUpSprite_->Initialize("button/X_Button_Up.dds");
    xButtonUpSprite_->SetPos({ 1488.0f, 869.0f });
    xButtonUpSprite_->SetSize({ 60.0f, 60.0f });

    xButtonDownSprite_ = std::make_unique<Sprite>();
    xButtonDownSprite_->Initialize("button/X_Button_Down.dds");
    xButtonDownSprite_->SetPos({ 1488.0f, 869.0f });
    xButtonDownSprite_->SetSize({ 60.0f, 60.0f });

    yButtonUpSprite_ = std::make_unique<Sprite>();
    yButtonUpSprite_->Initialize("button/Y_Button_Up.dds");
    yButtonUpSprite_->SetPos({ 1565.0f, 803.0f });
    yButtonUpSprite_->SetSize({ 60.0f, 60.0f });

    yButtonDownSprite_ = std::make_unique<Sprite>();
    yButtonDownSprite_->Initialize("button/Y_Button_Down.dds");
    yButtonDownSprite_->SetPos({ 1565.0f, 803.0f });
    yButtonDownSprite_->SetSize({ 60.0f, 60.0f });

    // ジョイスティックスプライト初期化（8方向）
    for (int i = 0; i < 8; ++i) {
        lJoystickSprites_[i] = std::make_unique<Sprite>();
        lJoystickSprites_[i]->Initialize(std::format("joystick/L_Joystick_{:02d}.dds", i + 1));
        lJoystickSprites_[i]->SetPos({ 323.0f, 869.0f });
        lJoystickSprites_[i]->SetSize({ 150.0f, 150.0f });

        rJoystickSprites_[i] = std::make_unique<Sprite>();
        rJoystickSprites_[i]->Initialize(std::format("joystick/R_Joystick_{:02d}.dds", i + 1));
        rJoystickSprites_[i]->SetPos({ 791.0f, 869.0f });
        rJoystickSprites_[i]->SetSize({ 150.0f, 150.0f });
    }

    // アクションアイコン初期化
    kougekiSprite_ = std::make_unique<Sprite>();
    kougekiSprite_->Initialize("kougeki.dds");
    kougekiSprite_->SetPos({ 1349.0f, 871.0f });
    kougekiSprite_->SetSize({ 150.0f, 50.0f });

    dashSprite_ = std::make_unique<Sprite>();
    dashSprite_->Initialize("dash.dds");
    dashSprite_->SetPos({ 1518.0f, 997.0f });
    dashSprite_->SetSize({ 150.0f, 50.0f });

    parrySprite_ = std::make_unique<Sprite>();
    parrySprite_->Initialize("parry.dds");
    parrySprite_->SetPos({ 1701.0f, 871.0f });
    parrySprite_->SetSize({ 150.0f, 50.0f });

    shagekiSprite_ = std::make_unique<Sprite>();
    shagekiSprite_->Initialize("shageki.dds");
    shagekiSprite_->SetPos({ 913.0f, 892.0f });
    shagekiSprite_->SetSize({ 150.0f, 50.0f });

    idouSprite_ = std::make_unique<Sprite>();
    idouSprite_->Initialize("idou.dds");
    idouSprite_->SetPos({ 450.0f, 892.0f });
    idouSprite_->SetSize({ 150.0f, 50.0f });

    // ポーズ操作ヒント初期化
    pauseHintIconSprite_ = std::make_unique<Sprite>();
    pauseHintIconSprite_->Initialize("button/Menu_Button_Up.dds");
    pauseHintIconSprite_->SetPos({ 30.0f, 157.0f });
    pauseHintIconSprite_->SetSize({ 80.0f, 80.0f });

    pauseHintTextSprite_ = std::make_unique<Sprite>();
    pauseHintTextSprite_->Initialize("PauseMenu_Text.dds");
    pauseHintTextSprite_->SetPos({ 124.0f, 120.0f });
    pauseHintTextSprite_->SetSize({ 240.0f, 120.0f });

    // リサイズコールバック登録
    winApp_ = WinApp::GetInstance();
    if (winApp_) {
        onResizeId_ = winApp_->RegisterOnResizeFunc(
            std::bind(&ControllerUI::OnResize, this, std::placeholders::_1)
        );

        // 現在のウィンドウサイズに合わせて位置調整（シーン再入場対応）
        Vector2 currentSize = { static_cast<float>(WinApp::clientWidth), static_cast<float>(WinApp::clientHeight) };
        if (currentSize.x != kBaseWidth || currentSize.y != kBaseHeight) {
            OnResize(currentSize);
        }
    }
}

void ControllerUI::Update()
{
    Input* input = Input::GetInstance();

    isAPressed_ = input->PushButton(GamepadButton::A);
    isBPressed_ = input->PushButton(GamepadButton::B);
    isXPressed_ = input->PushButton(GamepadButton::X);
    isYPressed_ = input->PushButton(GamepadButton::Y);

    Vector2 leftStick = input->GetLeftStick();
    Vector2 rightStick = input->GetRightStick();
    leftStickDir_ = GetStickDirectionIndex(leftStick);
    rightStickDir_ = GetStickDirectionIndex(rightStick);

    aButtonUpSprite_->Update();
    aButtonDownSprite_->Update();
    bButtonUpSprite_->Update();
    bButtonDownSprite_->Update();
    xButtonUpSprite_->Update();
    xButtonDownSprite_->Update();
    yButtonUpSprite_->Update();
    yButtonDownSprite_->Update();

    for (int i = 0; i < 8; ++i) {
        lJoystickSprites_[i]->Update();
        rJoystickSprites_[i]->Update();
    }

    kougekiSprite_->Update();
    dashSprite_->Update();
    parrySprite_->Update();
    shagekiSprite_->Update();
    idouSprite_->Update();

    pauseHintIconSprite_->Update();
    pauseHintTextSprite_->Update();
}

int ControllerUI::GetStickDirectionIndex(const Vector2& stick) const
{
    float magnitude = std::sqrt(stick.x * stick.x + stick.y * stick.y);

    // デッドゾーン未満は上(0)を返す
    if (magnitude < stickDeadzone_) {
        return 0;
    }

    // atan2(x, y) で「上」を0度基準にする
    float angle = DirectX::XMConvertToDegrees(std::atan2(stick.x, stick.y));

    // 境界を方向の「間」に置くため 22.5度オフセット
    angle += 22.5f;
    if (angle < 0.0f) {
        angle += 360.0f;
    }

    // 45度ずつ8方向に分類
    int index = static_cast<int>(angle / 45.0f) % 8;
    return index;
}

void ControllerUI::Draw()
{
    // ポーズ中の操作説明 UI は PauseMenu 側が描画する
    if (isPaused_) {
        pauseHintIconSprite_->Draw();
        pauseHintTextSprite_->Draw();
        return;
    }

    // === 以下、通常時の描画 ===

    bool isPhase2 = boss_ && boss_->GetPhase() == 2;

    if (isAPressed_) {
        aButtonDownSprite_->Draw();
    } else {
        aButtonUpSprite_->Draw();
    }

    if (isBPressed_) {
        bButtonDownSprite_->Draw();
    } else {
        bButtonUpSprite_->Draw();
    }

    if (isXPressed_) {
        xButtonDownSprite_->Draw();
    } else {
        xButtonUpSprite_->Draw();
    }

    if (isYPressed_) {
        yButtonDownSprite_->Draw();
    } else {
        yButtonUpSprite_->Draw();
    }

    lJoystickSprites_[leftStickDir_]->Draw();

    // 右ジョイスティックと射撃はフェーズ2では非表示
    if (!isPhase2) {
        rJoystickSprites_[rightStickDir_]->Draw();
    }

    kougekiSprite_->Draw();
    dashSprite_->Draw();
    parrySprite_->Draw();

    if (!isPhase2) {
        shagekiSprite_->Draw();
    }

    idouSprite_->Draw();

    pauseHintIconSprite_->Draw();
    pauseHintTextSprite_->Draw();
}

void ControllerUI::DrawImGui()
{
#ifdef _DEBUG
    if (ImGui::TreeNode("ControllerUI")) {
        ImGui::Text("Button States:");
        ImGui::Text("  A: %s", isAPressed_ ? "Pressed" : "Released");
        ImGui::Text("  B: %s", isBPressed_ ? "Pressed" : "Released");
        ImGui::Text("  X: %s", isXPressed_ ? "Pressed" : "Released");
        ImGui::Text("  Y: %s", isYPressed_ ? "Pressed" : "Released");

        ImGui::Separator();
        ImGui::Text("Stick Directions:");
        const char* dirNames[] = { "Up", "UpLeft", "Left", "DownLeft", "Down", "DownRight", "Right", "UpRight" };
        ImGui::Text("  Left Stick: %s (index: %d)", dirNames[leftStickDir_], leftStickDir_);
        ImGui::Text("  Right Stick: %s (index: %d)", dirNames[rightStickDir_], rightStickDir_);

        ImGui::Separator();
        ImGui::SliderFloat("Deadzone", &stickDeadzone_, 0.0f, 1.0f);

        if (ImGui::TreeNode("Button Sprites")) {
            if (ImGui::TreeNode("A Button Up")) {
                aButtonUpSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("A Button Down")) {
                aButtonDownSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("B Button Up")) {
                bButtonUpSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("B Button Down")) {
                bButtonDownSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("X Button Up")) {
                xButtonUpSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("X Button Down")) {
                xButtonDownSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Y Button Up")) {
                yButtonUpSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Y Button Down")) {
                yButtonDownSprite_->DrawImGui();
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Joystick Sprites")) {
            for (int i = 0; i < 8; ++i) {
                std::string lName = std::format("L Joystick {}", i + 1);
                if (ImGui::TreeNode(lName.c_str())) {
                    lJoystickSprites_[i]->DrawImGui();
                    ImGui::TreePop();
                }
            }
            for (int i = 0; i < 8; ++i) {
                std::string rName = std::format("R Joystick {}", i + 1);
                if (ImGui::TreeNode(rName.c_str())) {
                    rJoystickSprites_[i]->DrawImGui();
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Action Icons")) {
            if (ImGui::TreeNode("Kougeki")) {
                kougekiSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Dash")) {
                dashSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Parry")) {
                parrySprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Shageki")) {
                shagekiSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Idou")) {
                idouSprite_->DrawImGui();
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Pause Hint")) {
            if (ImGui::TreeNode("Icon")) {
                pauseHintIconSprite_->DrawImGui();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Text")) {
                pauseHintTextSprite_->DrawImGui();
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
#endif
}

void ControllerUI::OnResize(const Vector2& newSize)
{
    float scaleX = newSize.x / kBaseWidth;
    float scaleY = newSize.y / kBaseHeight;

    aButtonUpSprite_->SetPos({ 1565.0f * scaleX, 948.0f * scaleY });
    aButtonDownSprite_->SetPos({ 1565.0f * scaleX, 948.0f * scaleY });

    bButtonUpSprite_->SetPos({ 1640.0f * scaleX, 869.0f * scaleY });
    bButtonDownSprite_->SetPos({ 1640.0f * scaleX, 869.0f * scaleY });

    xButtonUpSprite_->SetPos({ 1488.0f * scaleX, 869.0f * scaleY });
    xButtonDownSprite_->SetPos({ 1488.0f * scaleX, 869.0f * scaleY });

    yButtonUpSprite_->SetPos({ 1565.0f * scaleX, 803.0f * scaleY });
    yButtonDownSprite_->SetPos({ 1565.0f * scaleX, 803.0f * scaleY });

    for (int i = 0; i < 8; ++i) {
        lJoystickSprites_[i]->SetPos({ 323.0f * scaleX, 869.0f * scaleY });

        rJoystickSprites_[i]->SetPos({ 791.0f * scaleX, 869.0f * scaleY });
    }

    kougekiSprite_->SetPos({ 1349.0f * scaleX, 871.0f * scaleY });

    dashSprite_->SetPos({ 1518.0f * scaleX, 997.0f * scaleY });

    parrySprite_->SetPos({ 1701.0f * scaleX, 871.0f * scaleY });

    shagekiSprite_->SetPos({ 913.0f * scaleX, 892.0f * scaleY });

    idouSprite_->SetPos({ 450.0f * scaleX, 892.0f * scaleY });

    pauseHintIconSprite_->SetPos({ 30.0f * scaleX, 157.0f * scaleY });

    pauseHintTextSprite_->SetPos({ 124.0f * scaleX, 120.0f * scaleY });
}
