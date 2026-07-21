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
class CameraAnimationController;

/// <summary>
/// ゲームメインシーン。プレイヤーとボスの戦闘を管理
/// </summary>
class GameScene : public Tako::BaseScene
{
public: //メンバー関数
    void Initialize() override;

    void Finalize() override;

    void Update() override;

    void Draw() override;
    void DrawWithoutEffect() override;

    void DrawImGui() override;

private: //非公開関数
    /// <summary>
    /// フェーズに応じてカメラモードと移動制限を切り替え
    /// </summary>
    void UpdateCameraMode();

    /// <summary>
    /// アニメーション再生中・デバッグカメラ中は入力をリセット
    /// </summary>
    void UpdateInput();

    /// <summary>
    /// 開始演出の終了処理とクリア・オーバーの判定、各演出の開始
    /// </summary>
    void UpdateGameFlow();

    void UpdateObjects();

    /// <summary>
    /// 弾リクエストをProjectileManagerに渡して生成・更新
    /// </summary>
    void UpdateProjectiles();

    /// <summary>
    /// 各種エフェクトの更新と演出完了時のシーン遷移
    /// </summary>
    void UpdateEffects();

    void InitializeDebugOption();

    void InitializePostEffect();

    void InitializeObject3d();

    void InitializeCameraSystem();

    void SetCollisionMask();

    void InitializeEmitterManager();

    void InitializeEffectManager();

    void SetCameraAnimation();

    void CheckPause();

    void UpdatePause();

private: //メンバー変数
    //エミッター・フォースフィールド
    std::unique_ptr<Tako::ForceFieldManager> forceFieldManager_;
    std::unique_ptr<Tako::EmitterManager>    emitterManager_;

    //シーンオブジェクト
    std::unique_ptr<Tako::SkyBox>   skyBox_;
    std::unique_ptr<Tako::Object3d> ground_;
    std::unique_ptr<Player>         player_;
    std::unique_ptr<Boss>           boss_;
    std::unique_ptr<InputHandler>   inputHandler_;

    //カメラ
    CameraManager*             cameraManager_       = nullptr;
    CameraAnimationController* animationController_ = nullptr;
    bool                       cameraMode_          = false;    ///< true: ThirdPerson, false: TopDown

    //地面UV
    Tako::Transform groundUvTransform_{};

    //弾管理
    std::unique_ptr<ProjectileManager> projectileManager_;

    //状態
    bool isStart_ = false;

    //エフェクト管理
    std::unique_ptr<OverEffectManager>         overEffectManager_;
    std::unique_ptr<ClearEffectManager>        clearEffectManager_;
    std::unique_ptr<BossBorderParticleManager> bossBorderManager_;
    std::unique_ptr<DashEffectManager>         dashEffectManager_;

    //UI・ポーズ
    std::unique_ptr<ControllerUI> controllerUI_;
    std::unique_ptr<PauseMenu>    pauseMenu_;
    bool                          isPaused_     = false;
};