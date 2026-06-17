#pragma once
#include "BaseScene.h"
#include "Transform.h"
#include  "SkyBox.h"
#include "Object/Boss/Boss.h"
#include "Object/Player/Player.h"
#include "Input/InputHandler.h"
#include "../Object/Projectile/ProjectileManager.h"
#include "../Effect/OverEffectManager.h"
#include "../Effect/ClearEffectManager.h"
#include "../Effect/BossBorderParticleManager.h"
#include "../Effect/DashEffectManager.h"
#include "UI/ControllerUI.h"
#include "UI/PauseMenu.h"

#include "Decal.h"

#include <memory>
#include <vector>

namespace Tako {
    class Object3d;
    class EmitterManager;
    class ForceFieldManager;
    class Sprite;
    class BoneTracker;
    enum class DecalShape;
}

class CameraManager;
class ThirdPersonController;
class TopDownController;
class CameraAnimationController;

/// <summary>
/// ゲームメインシーン。プレイヤーとボスの戦闘を管理
/// </summary>
class GameScene : public Tako::BaseScene
{
public: // メンバ関数

    void Initialize() override;

    void Finalize() override;

    void Update() override;

    void Draw() override;
    void DrawWithoutEffect() override;

    void DrawImGui() override;

    /// <summary>
    /// フェーズに応じてカメラモードと移動制限を切り替え
    /// </summary>
    void UpdateCameraMode();

    /// <summary>
    /// アニメーション再生中・デバッグカメラ中は入力をリセット
    /// </summary>
    void UpdateInput();

    void InitializeDebugOption();

    void InitializePostEffect();

    void InitializeObject3d();

    void InitializeCameraSystem();

    void SetCollisionMask();

    void InitializeEmitterManger();

    void InitializeEffectManager();

    void SetCameraAnimation();

    void CheckPause();

    void UpdatePause();

private: // メンバ変数

    std::unique_ptr<Tako::SkyBox> skyBox_;

    std::unique_ptr<Tako::Object3d> ground_;

    std::unique_ptr<Player> player_;

    std::unique_ptr<Boss> boss_;

    std::unique_ptr<InputHandler> inputHandler_;

    CameraManager* cameraManager_ = nullptr;
    ThirdPersonController* thirdPersonController_ = nullptr;
    TopDownController* topDownController_ = nullptr;
    CameraAnimationController* animationController_ = nullptr;
    bool cameraMode_ = false;                                   ///< true: ThirdPerson, false: TopDown

    Tako::Transform groundUvTransform_{};

    std::unique_ptr<Tako::EmitterManager> emitterManager_;
    std::unique_ptr<Tako::ForceFieldManager> forceFieldManager_;

    std::unique_ptr<ProjectileManager> projectileManager_;

    bool isStart_ = false;

    bool isDebug_ = false;

    std::unique_ptr<OverEffectManager> overEffectManager_;
    std::unique_ptr<ClearEffectManager> clearEffectManager_;
    std::unique_ptr<BossBorderParticleManager> bossBorderManager_;
    std::unique_ptr<DashEffectManager> dashEffectManager_;

    std::unique_ptr<ControllerUI> controllerUI_;
    std::unique_ptr<PauseMenu> pauseMenu_;
    bool isPaused_ = false;
};