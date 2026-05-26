#ifdef _DEBUG

#include "CameraDebugUI.h"
#include "CameraAnimationEditor/CameraAnimationEditor.h"
#include "Controller/CameraAnimationController.h"
#include "FrameTimer.h"
#include "ImGuiManager.h"
#include <DirectXMath.h>
#include <sstream>

using namespace Tako;

// 静的メンバ変数の定義
bool CameraDebugUI::showManagerInfo_ = true;
bool CameraDebugUI::showControllerInfo_ = true;
bool CameraDebugUI::showAnimationInfo_ = true;
std::unique_ptr<CameraAnimationEditor> CameraDebugUI::animationEditor_ = nullptr;
bool CameraDebugUI::useAdvancedEditor_ = false;

void CameraDebugUI::Draw() {
    if (!ImGui::Begin("Camera System Debug")) {
        ImGui::End();
        return;
    }

    // メインタブ
    if (ImGui::BeginTabBar("CameraDebugTabs")) {
        // Manager タブ
        if (ImGui::BeginTabItem("Manager")) {
            DrawManagerInfo();
            ImGui::EndTabItem();
        }

        // Controllers タブ
        if (ImGui::BeginTabItem("Controllers")) {
            DrawControllerSwitcher();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void CameraDebugUI::DrawManagerInfo() {
    CameraManager* manager = CameraManager::GetInstance();
    if (!manager) {
        ImGui::Text("CameraManager not initialized");
        return;
    }

    // ステータス情報をボックスで囲む
    ImGui::BeginChild("StatusBox", ImVec2(0, 80), true);
    {
        ImGui::Text("🎯 Active Controller:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s",
            manager->GetActiveControllerName().c_str());

        ImGui::Text("📊 Total Controllers:");
        ImGui::SameLine();
        ImGui::Text("%zu", manager->GetControllerCount());
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Text("Controller List:");
    ImGui::Separator();

    // コントローラーリストをテーブルで表示
    if (ImGui::BeginTable("ControllerTable", 3,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {

        // テーブルヘッダー
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        // デバッグ情報をパースして表示
        std::string debugInfo = manager->GetDebugInfo();

        // 各行を解析して表示
        if (manager->GetControllerCount() > 0) {
            // ThirdPerson
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("ThirdPerson");
            ImGui::TableNextColumn();
            ImGui::Text("50");  // FOLLOW_DEFAULT priority
            ImGui::TableNextColumn();
            bool isFPActive = (manager->GetActiveControllerName() == "ThirdPerson");
            ImGui::TextColored(isFPActive ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                isFPActive ? "Active" : "Inactive");

            // TopDown
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("TopDown");
            ImGui::TableNextColumn();
            ImGui::Text("50");  // FOLLOW_DEFAULT priority
            ImGui::TableNextColumn();
            bool isTDActive = (manager->GetActiveControllerName() == "TopDown");
            ImGui::TextColored(isTDActive ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                isTDActive ? "Active" : "Inactive");

            // Animation
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Animation");
            ImGui::TableNextColumn();
            ImGui::Text("100");  // ANIMATION priority
            ImGui::TableNextColumn();
            bool isAnimActive = (manager->GetActiveControllerName() == "Animation");
            ImGui::TextColored(isAnimActive ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                isAnimActive ? "Active" : "Inactive");
        }

        ImGui::EndTable();
    }
}

void CameraDebugUI::DrawFirstPersonControllerInfo(ThirdPersonController* controller) {
    if (!controller) {
        return;
    }

    ImGui::PushID("ThirdPerson");  // 一意の ID スコープ開始

    ImGui::Text("=== ThirdPerson Controller ===");
    ImGui::Text("Active: %s", controller->IsActive() ? "Yes" : "No");

    if (!controller->IsActive()) {
        if (ImGui::Button("Activate")) {
            controller->Activate();
        }
    }
    else {
        if (ImGui::Button("Deactivate")) {
            controller->Deactivate();
        }
    }

    ImGui::Separator();

    // オフセット設定
    Vector3 offset = controller->GetOffset();
    float offsetArray[3] = { offset.x, offset.y, offset.z };
    if (ImGui::DragFloat3("Offset", offsetArray, 0.1f)) {
        controller->SetOffset(Vector3(offsetArray[0], offsetArray[1], offsetArray[2]));
    }

    // 回転速度
    static float fpRotateSpeed = CameraConfig::ThirdPerson::DEFAULT_ROTATE_SPEED;
    if (ImGui::SliderFloat("Rotate Speed", &fpRotateSpeed, 0.01f, 0.2f)) {
        controller->SetRotateSpeed(fpRotateSpeed);
    }

    // 追従の滑らかさ
    static float fpSmoothness = CameraConfig::FOLLOW_SMOOTHNESS;
    if (ImGui::SliderFloat("Follow Smoothness", &fpSmoothness, 0.01f, 1.0f)) {
        controller->SetSmoothness(fpSmoothness);
    }

    // リセットボタン
    if (ImGui::Button("Reset Camera")) {
        controller->Reset();
    }

    ImGui::PopID();  // ID スコープ終了
}

void CameraDebugUI::DrawTopDownControllerInfo(TopDownController* controller) {
    if (!controller) {
        return;
    }

    ImGui::PushID("TopDown");  // 一意の ID スコープ開始

    ImGui::Text("=== TopDown Controller ===");
    ImGui::Text("Active: %s", controller->IsActive() ? "Yes" : "No");

    if (!controller->IsActive()) {
        if (ImGui::Button("Activate")) {
            controller->Activate();
        }
    }
    else {
        if (ImGui::Button("Deactivate")) {
            controller->Deactivate();
        }
    }

    ImGui::Separator();

    // カメラ高さ設定
    static float tdBaseHeight = CameraConfig::TopDown::BASE_HEIGHT;
    if (ImGui::DragFloat("Base Height", &tdBaseHeight, 0.5f, 5.0f, 100.0f)) {
        controller->SetBaseHeight(tdBaseHeight);
    }

    // 高さ倍率
    static float tdHeightMultiplier = CameraConfig::TopDown::HEIGHT_MULTIPLIER;
    if (ImGui::SliderFloat("Height Multiplier", &tdHeightMultiplier, 0.0f, 2.0f)) {
        controller->SetHeightMultiplier(tdHeightMultiplier);
    }

    // カメラ角度
    static float tdAngleXDegrees = DirectX::XMConvertToDegrees(CameraConfig::TopDown::DEFAULT_ANGLE_X);
    if (ImGui::SliderFloat("Camera Angle (deg)", &tdAngleXDegrees, 0.0f, 90.0f)) {
        controller->SetCameraAngle(DirectX::XMConvertToRadians(tdAngleXDegrees));
    }

    // 追従の滑らかさ
    static float tdSmoothness = CameraConfig::FOLLOW_SMOOTHNESS;
    if (ImGui::SliderFloat("Follow Smoothness", &tdSmoothness, 0.01f, 1.0f)) {
        controller->SetSmoothness(tdSmoothness);
    }

    // 現在の高さ表示
    ImGui::Text("Current Height: %.2f", controller->GetCurrentHeight());

    // リセットボタン
    if (ImGui::Button("Reset Camera")) {
        controller->Reset();
    }

    ImGui::PopID();  // ID スコープ終了
}

void CameraDebugUI::DrawAnimationInfo(CameraAnimation* animation) {
    if (!animation) {
        return;
    }

    ImGui::PushID("AnimationInfo");  // 一意の ID スコープ開始

    ImGui::Text("=== Camera Animation ===");

    // エディター切り替えオプション
    ImGui::Checkbox("Use Advanced Editor", &useAdvancedEditor_);

    if (useAdvancedEditor_) {
        // 高度なエディターを使用
        if (!animationEditor_) {
            animationEditor_ = std::make_unique<CameraAnimationEditor>();
            CameraManager* manager = CameraManager::GetInstance();
            if (manager && manager->GetCamera()) {
                animationEditor_->Initialize(animation, manager->GetCamera());
            }
        }

        if (ImGui::Button("Open Animation Editor")) {
            animationEditor_->Open();
        }

        // エディターが開いている場合は描画
        if (animationEditor_ && animationEditor_->IsOpen()) {
            animationEditor_->Draw();
            animationEditor_->Update(0.016f); // 仮の deltaTime
        }

        ImGui::PopID();
        return;
    }

    // シンプルな UI--------------------------------------

    // アニメーション情報
    ImGui::Text("Animation: %s", animation->GetAnimationName().c_str());
    ImGui::Text("Duration: %.2f seconds", animation->GetDuration());
    ImGui::Text("Current Time: %.2f", animation->GetPlaybackTime());
    ImGui::Text("Keyframes: %zu", animation->GetKeyframeCount());

    // 再生状態
    const char* stateStr = "STOPPED";
    auto playState = animation->GetPlayState();
    if (playState == CameraAnimation::PlayState::PLAYING) stateStr = "PLAYING";
    else if (playState == CameraAnimation::PlayState::PAUSED) stateStr = "PAUSED";
    ImGui::Text("State: %s", stateStr);

    ImGui::Separator();

    // 再生コントロール
    // CameraManager から AnimationController を取得
    auto* animController = dynamic_cast<CameraAnimationController*>(
        CameraManager::GetInstance()->GetController("Animation"));

    if (ImGui::Button("Play")) {
        // コントローラー経由で呼び出し（isActive_フラグを更新するため）
        if (animController) {
            animController->Play();
        }
        else {
            animation->Play();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        if (animController) {
            animController->Pause();
        }
        else {
            animation->Pause();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        if (animController) {
            animController->Stop();
        }
        else {
            animation->Stop();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        if (animController) {
            animController->Reset();
        }
        else {
            animation->Reset();
        }
    }

    // ループ設定
    bool isLooping = animation->IsLooping();
    if (ImGui::Checkbox("Loop", &isLooping)) {
        animation->SetLooping(isLooping);
    }

    // 再生速度
    static float playSpeed = 1.0f;
    if (ImGui::SliderFloat("Play Speed", &playSpeed, -2.0f, 2.0f, "%.2f")) {
        animation->SetPlaySpeed(playSpeed);
    }

    // タイムラインスライダー
    float currentTime = animation->GetPlaybackTime();
    if (ImGui::SliderFloat("Timeline", &currentTime, 0.0f,
        animation->GetDuration(), "%.2f")) {
        animation->SetCurrentTime(currentTime);
    }

    ImGui::PopID();  // ID スコープ終了
}

void CameraDebugUI::DrawControllerSwitcher() {
    CameraManager* manager = CameraManager::GetInstance();
    if (!manager) {
        return;
    }

    ImGui::Text("=== Controller Switcher ===");

    // アクティブなコントローラーを表示
    std::string activeName = manager->GetActiveControllerName();
    ImGui::Text("Current Active: %s",
        activeName.empty() ? "None" : activeName.c_str());

    ImGui::Separator();

    // ThirdPerson/TopDown の簡単切り替え
    if (ImGui::Button("Activate ThirdPerson")) {
        manager->DeactivateAllControllers();
        manager->ActivateController("ThirdPerson");
    }
    ImGui::SameLine();
    if (ImGui::Button("Activate TopDown")) {
        manager->DeactivateAllControllers();
        manager->ActivateController("TopDown");
    }

    ImGui::Separator();

    // 各コントローラーの詳細情報
    if (ImGui::CollapsingHeader("ThirdPerson Controller Details")) {
        auto* fpController = dynamic_cast<ThirdPersonController*>(
            manager->GetController("ThirdPerson"));
        DrawFirstPersonControllerInfo(fpController);
    }

    if (ImGui::CollapsingHeader("TopDown Controller Details")) {
        auto* tdController = dynamic_cast<TopDownController*>(
            manager->GetController("TopDown"));
        DrawTopDownControllerInfo(tdController);
    }
}

void CameraDebugUI::DrawCameraState() {
    CameraManager* manager = CameraManager::GetInstance();
    if (!manager || !manager->GetCamera()) {
        return;
    }

    Camera* camera = manager->GetCamera();

    ImGui::Text("=== Camera State ===");

    // 位置
    Vector3 pos = camera->GetTranslate();
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

    // 回転（度単位）
    Vector3 rot = camera->GetRotate();
    ImGui::Text("Rotation: (%.1f°, %.1f°, %.1f°)",
        DirectX::XMConvertToDegrees(rot.x), DirectX::XMConvertToDegrees(rot.y), DirectX::XMConvertToDegrees(rot.z));

    // FOV（度単位）
    float fov = DirectX::XMConvertToDegrees(camera->GetFovY());
    ImGui::Text("FOV: %.1f°", fov);

    // アスペクト比
    ImGui::Text("Aspect Ratio: %.3f", camera->GetAspect());

    // ニア・ファー
    ImGui::Text("Near/Far: %.2f / %.1f",
        camera->GetNearClip(), camera->GetFarClip());
}

void CameraDebugUI::DrawAnimationEditorOnly() {
    // エディターが未初期化の場合は初期化を試みる
    if (!animationEditor_) {
        InitializeAnimationEditor();
    }

    // エディターが初期化されていれば描画
    if (animationEditor_) {
        // エディターが閉じていれば開く
        if (!animationEditor_->IsOpen()) {
            animationEditor_->Open();
        }

        // エディターの描画
        animationEditor_->Draw();
    }
    else {
        // エディターが初期化できない場合のメッセージ
        if (ImGui::Begin("Camera Animation Editor")) {
            ImGui::Text("⚠️ Animation Editor not available");
            ImGui::TextWrapped("Make sure AnimationController is registered and initialized with a valid CameraAnimation.");

            // 再初期化ボタン
            if (ImGui::Button("Try Initialize")) {
                InitializeAnimationEditor();
            }
            ImGui::End();
        }
    }
}

void CameraDebugUI::InitializeAnimationEditor() {

    animationEditor_.reset();

    CameraManager* manager = CameraManager::GetInstance();
    if (!manager) return;

    // AnimationController を取得
    auto* animController = dynamic_cast<CameraAnimationController*>(
        manager->GetController("Animation"));
    if (!animController) return;

    // エディターの初期化（CameraAnimationController を渡す）
    animationEditor_ = std::make_unique<CameraAnimationEditor>();
    animationEditor_->Initialize(animController, manager->GetCamera());
}

void CameraDebugUI::CleanupAnimationEditor()
{
    animationEditor_.reset();
}

void CameraDebugUI::UpdateAnimationEditor(float deltaTime) {
    if (animationEditor_ && animationEditor_->IsOpen()) {
        animationEditor_->Update(deltaTime);
    }
}

#endif // _DEBUG