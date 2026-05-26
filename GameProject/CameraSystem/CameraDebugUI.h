#pragma once
#include "CameraManager.h"
#include "Controller/ThirdPersonController.h"
#include "Controller/TopDownController.h"
#include "CameraAnimation/CameraAnimation.h"
#include "CameraAnimationEditor/CameraAnimationEditor.h"
#include <memory>

#ifdef _DEBUG

/// <summary>
/// カメラシステムのデバッグ UI
/// ImGui を使用したデバッグ機能を提供
/// </summary>
class CameraDebugUI {
public:
    /// <summary>
    /// デバッグ UI を描画
    /// </summary>
    static void Draw();

    /// <summary>
    /// CameraManager のデバッグ情報を描画
    /// </summary>
    static void DrawManagerInfo();

    /// <summary>
    /// FirstPersonController のデバッグ情報を描画
    /// </summary>
    /// <param name="controller">対象のコントローラー</param>
    static void DrawFirstPersonControllerInfo(ThirdPersonController* controller);

    /// <summary>
    /// TopDownController のデバッグ情報を描画
    /// </summary>
    /// <param name="controller">対象のコントローラー</param>
    static void DrawTopDownControllerInfo(TopDownController* controller);

    /// <summary>
    /// CameraAnimation のデバッグ情報を描画
    /// </summary>
    /// <param name="animation">対象のアニメーション</param>
    static void DrawAnimationInfo(CameraAnimation* animation);

    /// <summary>
    /// カメラアニメーションエディターのみを描画
    /// （DebugUIManager 用の独立した描画関数）
    /// </summary>
    static void DrawAnimationEditorOnly();

    /// <summary>
    /// アニメーションエディターの初期化
    /// </summary>
    static void InitializeAnimationEditor();

    /// <summary>
    /// アニメーションエディターをクリーンアップ
    /// </summary>
    static void CleanupAnimationEditor();

    /// <summary>
    /// アニメーションエディターの更新
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過時間</param>
    static void UpdateAnimationEditor(float deltaTime);

private:
    /// <summary>
    /// コントローラー切り替え UI を描画
    /// </summary>
    static void DrawControllerSwitcher();

    /// <summary>
    /// カメラ状態情報を描画
    /// </summary>
    static void DrawCameraState();

private:

    // UI 表示フラグ
    static bool showManagerInfo_;
    static bool showControllerInfo_;
    static bool showAnimationInfo_;

    // アニメーションエディター
    static std::unique_ptr<CameraAnimationEditor> animationEditor_;
    static bool useAdvancedEditor_;
};

#endif // _DEBUG