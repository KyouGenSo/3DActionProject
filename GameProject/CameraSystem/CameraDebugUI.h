#pragma once
#include "CameraManager.h"
#include "Controller/ThirdPersonController.h"
#include "Controller/TopDownController.h"
#include "CameraAnimation/CameraAnimation.h"
#include "CameraAnimationEditor/CameraAnimationEditor.h"
#include <memory>

#ifdef _DEBUG

/// <summary>
/// カメラシステムの ImGui デバッグ UI
/// </summary>
class CameraDebugUI {
public:
    static void Draw();

    static void DrawManagerInfo();

    static void DrawFirstPersonControllerInfo(ThirdPersonController* controller);

    static void DrawTopDownControllerInfo(TopDownController* controller);

    static void DrawAnimationInfo(CameraAnimation* animation);

    /// <summary>
    /// アニメーションエディターのみを描画（DebugUIManager 用）
    /// </summary>
    static void DrawAnimationEditorOnly();

    static void InitializeAnimationEditor();

    static void CleanupAnimationEditor();

    static void UpdateAnimationEditor(float deltaTime);

private:
    static void DrawControllerSwitcher();

    static void DrawCameraState();

private:

    static bool showManagerInfo_;
    static bool showControllerInfo_;
    static bool showAnimationInfo_;

    static std::unique_ptr<CameraAnimationEditor> animationEditor_;
    static bool useAdvancedEditor_;
};

#endif // _DEBUG