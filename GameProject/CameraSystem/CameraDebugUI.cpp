#ifdef _DEBUG

#include "CameraDebugUI.h"
#include "CameraAnimationEditor/CameraAnimationEditor.h"
#include "Controller/CameraAnimationController.h"
#include "FrameTimer.h"
#include "ImGuiManager.h"
#include <DirectXMath.h>
#include <sstream>

using namespace Tako;

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

    if (ImGui::BeginTabBar("CameraDebugTabs")) {
        if (ImGui::BeginTabItem("Manager")) {
            DrawManagerInfo();
            ImGui::EndTabItem();
        }

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

    if (ImGui::BeginTable("ControllerTable", 3,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        std::string debugInfo = manager->GetDebugInfo();

        if (manager->GetControllerCount() > 0) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("ThirdPerson");
            ImGui::TableNextColumn();
            ImGui::Text("50");  // FOLLOW_DEFAULT priority
            ImGui::TableNextColumn();
            bool isFPActive = (manager->GetActiveControllerName() == "ThirdPerson");
            ImGui::TextColored(isFPActive ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                isFPActive ? "Active" : "Inactive");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("TopDown");
            ImGui::TableNextColumn();
            ImGui::Text("50");  // FOLLOW_DEFAULT priority
            ImGui::TableNextColumn();
            bool isTDActive = (manager->GetActiveControllerName() == "TopDown");
            ImGui::TextColored(isTDActive ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                isTDActive ? "Active" : "Inactive");

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

    ImGui::PushID("ThirdPerson");

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

    Vector3 offset = controller->GetOffset();
    float offsetArray[3] = { offset.x, offset.y, offset.z };
    if (ImGui::DragFloat3("Offset", offsetArray, 0.1f)) {
        controller->SetOffset(Vector3(offsetArray[0], offsetArray[1], offsetArray[2]));
    }

    static float fpRotateSpeed = CameraConfig::ThirdPerson::DEFAULT_ROTATE_SPEED;
    if (ImGui::SliderFloat("Rotate Speed", &fpRotateSpeed, 0.01f, 0.2f)) {
        controller->SetRotateSpeed(fpRotateSpeed);
    }

    static float fpSmoothness = CameraConfig::FOLLOW_SMOOTHNESS;
    if (ImGui::SliderFloat("Follow Smoothness", &fpSmoothness, 0.01f, 1.0f)) {
        controller->SetSmoothness(fpSmoothness);
    }

    if (ImGui::Button("Reset Camera")) {
        controller->Reset();
    }

    ImGui::PopID();
}

void CameraDebugUI::DrawTopDownControllerInfo(TopDownController* controller) {
    if (!controller) {
        return;
    }

    ImGui::PushID("TopDown");

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

    static float tdBaseHeight = CameraConfig::TopDown::BASE_HEIGHT;
    if (ImGui::DragFloat("Base Height", &tdBaseHeight, 0.5f, 5.0f, 100.0f)) {
        controller->SetBaseHeight(tdBaseHeight);
    }

    static float tdHeightMultiplier = CameraConfig::TopDown::HEIGHT_MULTIPLIER;
    if (ImGui::SliderFloat("Height Multiplier", &tdHeightMultiplier, 0.0f, 2.0f)) {
        controller->SetHeightMultiplier(tdHeightMultiplier);
    }

    static float tdAngleXDegrees = DirectX::XMConvertToDegrees(CameraConfig::TopDown::DEFAULT_ANGLE_X);
    if (ImGui::SliderFloat("Camera Angle (deg)", &tdAngleXDegrees, 0.0f, 90.0f)) {
        controller->SetCameraAngle(DirectX::XMConvertToRadians(tdAngleXDegrees));
    }

    static float tdSmoothness = CameraConfig::FOLLOW_SMOOTHNESS;
    if (ImGui::SliderFloat("Follow Smoothness", &tdSmoothness, 0.01f, 1.0f)) {
        controller->SetSmoothness(tdSmoothness);
    }

    ImGui::Text("Current Height: %.2f", controller->GetCurrentHeight());

    if (ImGui::Button("Reset Camera")) {
        controller->Reset();
    }

    ImGui::PopID();
}

void CameraDebugUI::DrawAnimationInfo(CameraAnimation* animation) {
    if (!animation) {
        return;
    }

    ImGui::PushID("AnimationInfo");

    ImGui::Text("=== Camera Animation ===");

    ImGui::Checkbox("Use Advanced Editor", &useAdvancedEditor_);

    if (useAdvancedEditor_) {
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

        if (animationEditor_ && animationEditor_->IsOpen()) {
            animationEditor_->Draw();
            animationEditor_->Update(0.016f); // 仮の deltaTime
        }

        ImGui::PopID();
        return;
    }

    ImGui::Text("Animation: %s", animation->GetAnimationName().c_str());
    ImGui::Text("Duration: %.2f seconds", animation->GetDuration());
    ImGui::Text("Current Time: %.2f", animation->GetPlaybackTime());
    ImGui::Text("Keyframes: %zu", animation->GetKeyframeCount());

    const char* stateStr = "STOPPED";
    auto playState = animation->GetPlayState();
    if (playState == CameraAnimation::PlayState::PLAYING) stateStr = "PLAYING";
    else if (playState == CameraAnimation::PlayState::PAUSED) stateStr = "PAUSED";
    ImGui::Text("State: %s", stateStr);

    ImGui::Separator();

    auto* animController = dynamic_cast<CameraAnimationController*>(
        CameraManager::GetInstance()->GetController("Animation"));

    if (ImGui::Button("Play")) {
        // コントローラー経由なら isActive_ も更新される
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

    bool isLooping = animation->IsLooping();
    if (ImGui::Checkbox("Loop", &isLooping)) {
        animation->SetLooping(isLooping);
    }

    static float playSpeed = 1.0f;
    if (ImGui::SliderFloat("Play Speed", &playSpeed, -2.0f, 2.0f, "%.2f")) {
        animation->SetPlaySpeed(playSpeed);
    }

    float currentTime = animation->GetPlaybackTime();
    if (ImGui::SliderFloat("Timeline", &currentTime, 0.0f,
        animation->GetDuration(), "%.2f")) {
        animation->SetCurrentTime(currentTime);
    }

    ImGui::PopID();
}

void CameraDebugUI::DrawControllerSwitcher() {
    CameraManager* manager = CameraManager::GetInstance();
    if (!manager) {
        return;
    }

    ImGui::Text("=== Controller Switcher ===");

    std::string activeName = manager->GetActiveControllerName();
    ImGui::Text("Current Active: %s",
        activeName.empty() ? "None" : activeName.c_str());

    ImGui::Separator();

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

    Vector3 pos = camera->GetTranslate();
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

    Vector3 rot = camera->GetRotate();
    ImGui::Text("Rotation: (%.1f°, %.1f°, %.1f°)",
        DirectX::XMConvertToDegrees(rot.x), DirectX::XMConvertToDegrees(rot.y), DirectX::XMConvertToDegrees(rot.z));

    float fov = DirectX::XMConvertToDegrees(camera->GetFovY());
    ImGui::Text("FOV: %.1f°", fov);

    ImGui::Text("Aspect Ratio: %.3f", camera->GetAspect());

    ImGui::Text("Near/Far: %.2f / %.1f",
        camera->GetNearClip(), camera->GetFarClip());
}

void CameraDebugUI::DrawAnimationEditorOnly() {
    if (!animationEditor_) {
        InitializeAnimationEditor();
    }

    if (animationEditor_) {
        if (!animationEditor_->IsOpen()) {
            animationEditor_->Open();
        }

        animationEditor_->Draw();
    }
    else {
        if (ImGui::Begin("Camera Animation Editor")) {
            ImGui::Text("⚠️ Animation Editor not available");
            ImGui::TextWrapped("Make sure AnimationController is registered and initialized with a valid CameraAnimation.");

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

    auto* animController = dynamic_cast<CameraAnimationController*>(
        manager->GetController("Animation"));
    if (!animController) return;

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