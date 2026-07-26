// Engine includes
#include "GameScene.h"
#include "WinApp.h"
#include "Object3dBasic.h"
#include "SpriteBasic.h"
#include "Input.h"
#include "LineRenderer.h"
#include "FrameTimer.h"
#include "GPUParticle.h"
#include "SceneManager.h"
#include "EmitterManager.h"
#include "ForceFieldManager.h"
#include "Object3d.h"
#include "Model.h"
#include "ShadowRenderer.h"
#include "CollisionManager.h"
#include "GlobalVariables.h"
#include "PostEffectManager.h"
#include "DecalManager.h"
#include "EnginePaths.h"

// Game includes
#include "../Collision/CollisionTypeIdDef.h"
#include "CameraSystem/CameraManager.h"
#include "CameraSystem/Controller/ThirdPersonController.h"
#include "CameraSystem/Controller/TopDownController.h"
#include "CameraSystem/Controller/CameraAnimationController.h"
#include "Object/Player/State/PlayerState.h"
#include "Object/Player/State/PlayerStateMachine.h"
#include "Common/GameConst.h"

#include <algorithm>
#include <cmath>

// Debug includes
#ifdef _DEBUG
#include "ImGui.h"
#include "DebugCamera.h"
#include "DebugUIManager.h"
#include "CameraSystem/CameraDebugUI.h"
#endif

using namespace Tako;

namespace {
    constexpr BYTE  kCameraModeToggleKey = DIK_P;
    constexpr float kFadeDuration        = 0.3f;
    constexpr float kGroundUvScale       = 100.0f;
    constexpr int   kVortexEmitterCount  = 4;
}

void GameScene::Initialize()
{
    /// ================================== ///
    ///              初期化処理             ///
    /// ================================== ///

    /// ----------------------エンジンクラス初期化---------------------------------------------------------///
    CollisionManager::GetInstance()->Initialize();

    emitterManager_ = std::make_unique<EmitterManager>(GPUParticle::GetInstance());

    // シーンプリセット統合保存のため EmitterManager に連携
    forceFieldManager_ = std::make_unique<ForceFieldManager>(GPUParticle::GetInstance());
    emitterManager_->SetForceFieldManager(forceFieldManager_.get());

    inputHandler_ = std::make_unique<InputHandler>();
    inputHandler_->Initialize();

    controllerUI_ = std::make_unique<ControllerUI>();
    controllerUI_->Initialize();

    pauseMenu_ = std::make_unique<PauseMenu>();
    pauseMenu_->Initialize();

    InitializePostEffect();

    InitializeDebugOption();

    /// ----------------------シーンの描画設定---------------------------------------------------------///
    GlobalVariables* gvScene = GlobalVariables::GetInstance();
    float shadowMaxDist = gvScene->GetValueFloat("GameScene", "ShadowMaxDistance");
    ShadowRenderer::GetInstance()->SetMaxShadowDistance(shadowMaxDist);
    float lightZ = gvScene->GetValueFloat("GameScene", "DirectionalLightZ");
    Object3dBasic::GetInstance()->SetDirectionalLightDirection(Vector3(0.0f, -1.0f, lightZ));


    InitializeObject3d();

    InitializeCameraSystem();

    InitializeEmitterManager();

    InitializeEffectManager();

    SetCollisionMask();

    SetCameraAnimation();
}

void GameScene::Finalize()
{
#ifdef _DEBUG
    DebugUIManager::GetInstance()->ClearGameObjects();
    CameraDebugUI::CleanupAnimationEditor();
#endif

    if (player_) {
        player_->Finalize();
    }
    if (boss_) {
        boss_->Finalize();
    }

    if (cameraManager_) {
        cameraManager_->Finalize();
    }

    // CollisionManager::Reset() 前に確定させる
    if (projectileManager_) {
        projectileManager_->Clear();
    }

    CollisionManager::GetInstance()->Reset();

    DecalManager::GetInstance()->ClearDecals();

    PostEffectManager::GetInstance()->ClearEffectChain();

    pauseMenu_.reset();
}

void GameScene::Update()
{
    /// ================================== ///
    ///              更新処理               ///
    /// ================================== ///

#ifdef _DEBUG
    // P キーでカメラモード切り替え
    if (Input::GetInstance()->TriggerKey(kCameraModeToggleKey)) {
        cameraMode_ = !cameraMode_;
    }

#endif

    // ポーズトグル判定の前に実行する必要があるため、ポーズ中でも更新する
    inputHandler_->Update();

    CheckPause();

    if (isPaused_) {
        UpdatePause();
        return;
    }

    UpdateGameFlow();

    UpdateCameraMode();

    UpdateInput();

    UpdateObjects();

    UpdateProjectiles();

    UpdateEffects();

    CollisionManager::GetInstance()->CheckAllCollisions();
}

void GameScene::UpdateGameFlow()
{
    // 開始演出が終わったらボスの一時停止を解除
    if (animationController_->GetPlayState() != CameraAnimation::PlayState::PLAYING && !isStart_) {
        isStart_ = true;
        boss_->SetIsPause(false);
    }

    // ゲームクリア判定と演出開始
    if (boss_->IsDead() && !clearEffectManager_->IsPlaying() && !clearEffectManager_->IsComplete()) {
        cameraManager_->DeactivateAllControllers();
        cameraManager_->ActivateController("Animation");
        animationController_->SwitchAnimation("clear_anim");
        animationController_->Play();
        boss_->SetIsPause(true);
        player_->SetScale(Vector3(0.f, 0.f, 0.f)); // プレイヤーを非表示
        clearEffectManager_->Start();
    }

    // ゲームオーバー判定と演出開始
    if (player_->IsDead() && !overEffectManager_->IsPlaying() && !overEffectManager_->IsComplete()) {
        cameraManager_->DeactivateAllControllers();
        cameraManager_->ActivateController("Animation");
        animationController_->SwitchAnimation("over_anim");
        animationController_->Play();
        boss_->SetIsPause(true);
        overEffectManager_->Start();
    }
}

void GameScene::UpdateObjects()
{
    skyBox_->Update();
    ground_->Update();
    player_->Update();
    boss_->Update(FrameTimer::GetInstance()->GetDeltaTime());
    controllerUI_->Update();
    cameraManager_->Update(FrameTimer::GetInstance()->GetDeltaTime());
}

void GameScene::UpdateProjectiles()
{
    // ボス・プレイヤーが貯めた弾リクエストを ProjectileManager に渡して生成
    projectileManager_->SpawnBossBullets(boss_->ConsumePendingBullets());
    projectileManager_->SpawnPenetratingBossBullets(boss_->ConsumePendingPenetratingBullets());
    projectileManager_->SpawnPlayerBullets(player_->ConsumePendingBullets());

    projectileManager_->Update(FrameTimer::GetInstance()->GetDeltaTime());
}

void GameScene::UpdateEffects()
{
    float deltaTime = FrameTimer::GetInstance()->GetDeltaTime();

    bool isDashing = false;
    if (player_ && player_->GetStateMachine() && player_->GetStateMachine()->GetCurrentState()) {
        isDashing = (player_->GetStateMachine()->GetCurrentState()->GetName() == "Dash");
    }
    dashEffectManager_->Update(deltaTime, player_->GetTranslate(), isDashing);

    bossBorderManager_->Update(boss_->GetPhase(), boss_->GetTranslate());

    emitterManager_->Update();

    overEffectManager_->Update(deltaTime);
    if (overEffectManager_->IsComplete()) {
        SceneManager::GetInstance()->ChangeScene("over", "Fade", kFadeDuration);
    }

    clearEffectManager_->Update(deltaTime);
    if (clearEffectManager_->IsComplete()) {
        SceneManager::GetInstance()->ChangeScene("clear", "Fade", kFadeDuration);
    }
}

void GameScene::Draw()
{
    /// ================================== ///
    ///              描画処理               ///
    /// ================================== ///

    //-------------------SkyBox の描画-------------------//
    skyBox_->Draw();

    //------------------シャドウマップの描画------------------//
    if (ShadowRenderer::GetInstance()->IsEnabled()) {
        ShadowRenderer::GetInstance()->BeginShadowPass();
        ground_->Draw();
        player_->Draw();
        boss_->DrawShadow();
        ShadowRenderer::GetInstance()->EndShadowPass();
    }

    //------------------背景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();



    //-------------------Model の描画-------------------//
    Object3dBasic::GetInstance()->SetCommonRenderSetting();

    ground_->Draw();
    player_->Draw();
    boss_->Draw();

    //------------------前景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();



#ifdef _DEBUG
    CollisionManager::GetInstance()->DrawColliders();
#endif

}

void GameScene::DrawWithoutEffect()
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

    player_->DrawSprite();
    boss_->DrawSprite();

    controllerUI_->Draw();

    // 最前面に描画
    if (isPaused_) {
        pauseMenu_->Draw();
    }
}

void GameScene::DrawImGui()
{
#ifdef _DEBUG

#endif // DEBUG
}

void GameScene::UpdateCameraMode()
{
    if (player_->IsDead() || boss_->IsDead() || !isStart_) {
        return;
    }

    if (boss_->GetPhase() == 1) {
        cameraMode_ = false;
        // 移動制限を解除しステージ全体を移動可能にする
        player_->ClearDynamicBounds();
    }
    else if (boss_->GetPhase() == 2) {
        cameraMode_ = true;
        // ボス中心の戦闘エリアに移動制限
        Vector3 bossPos = boss_->GetTransform().translate;
        player_->SetDynamicBoundsFromCenter(bossPos, GameConst::kBossPhase2AreaSize, GameConst::kBossPhase2AreaSize);
    }

    if (cameraMode_) {
        cameraManager_->ActivateController("ThirdPerson");
    }
    else {
        cameraManager_->ActivateController("TopDown");
    }

    player_->SetMode(cameraMode_);

}

void GameScene::UpdateInput()
{
    // カメラアニメーション再生中やデバッグカメラ操作中は入力をリセット
    if (animationController_->GetPlayState() == CameraAnimation::PlayState::PLAYING
#ifdef  _DEBUG
        || Object3dBasic::GetInstance()->GetDebug()
#endif
        ) {
        inputHandler_->ResetInputs();
    }
}

void GameScene::InitializeDebugOption()
{
#ifdef _DEBUG
    DebugCamera::GetInstance()->Initialize();
    Object3dBasic::GetInstance()->SetDebug(false);
    LineRenderer::GetInstance()->SetDebug(false);
    GPUParticle::GetInstance()->SetIsDebug(false);

    DebugUIManager::GetInstance()->SetSceneName("GameScene");

    DebugUIManager::GetInstance()->RegisterGameObject("Player",
        [this]() { if (player_) player_->DrawImGui(); });
    DebugUIManager::GetInstance()->RegisterGameObject("Boss",
        [this]() { if (boss_) boss_->DrawImGui(); });

    DebugUIManager::GetInstance()->RegisterGameObject("CameraSystem",
        []() { CameraDebugUI::Draw(); });

    DebugUIManager::GetInstance()->RegisterGameObject("CameraAnimationEditor",
        []() {
            CameraDebugUI::DrawAnimationEditorOnly();
            CameraDebugUI::UpdateAnimationEditor(
                FrameTimer::GetInstance()->GetDeltaTime());
        });

    DebugUIManager::GetInstance()->RegisterGameObject("ControllerUI",
        [this]() { if (controllerUI_) controllerUI_->DrawImGui(); });

    DebugUIManager::GetInstance()->RegisterGameObject("PauseMenu",
        [this]() { if (pauseMenu_) pauseMenu_->DrawImGui(); });

    DebugUIManager::GetInstance()->SetEmitterManager(emitterManager_.get());
    DebugUIManager::GetInstance()->SetForceFieldManager(forceFieldManager_.get());
#endif
}

void GameScene::InitializePostEffect()
{
    RGBSplitParam rgbParam{
        .redOffset = Vector2(-0.01f, 0.0f),
        .greenOffset = Vector2(0.01f, 0.0f),
        .blueOffset = Vector2(0.0f, 0.0f),
        .intensity = 0.1f };
    DepthOutlineParam outlineParam{ .outlineThickness = 0.4f };
    PostEffectManager::GetInstance()->AddEffectToChain("DepthBasedOutline");
    PostEffectManager::GetInstance()->AddEffectToChain("RGBSplit");
    PostEffectManager::GetInstance()->SetEffectParam("DepthBasedOutline", outlineParam);
    PostEffectManager::GetInstance()->SetEffectParam("RGBSplit", rgbParam);
}

void GameScene::InitializeObject3d()
{
    skyBox_ = std::make_unique<SkyBox>();
    skyBox_->Initialize(EnginePaths::TexturePath("my_skybox.dds"));

    groundUvTransform_.translate = Vector3(0.0f, 0.0f, 0.0f);
    groundUvTransform_.rotate = Vector3(0.0f, 0.0f, 0.0f);
    groundUvTransform_.scale = Vector3(kGroundUvScale, kGroundUvScale, kGroundUvScale);
    ground_ = std::make_unique<Object3d>();
    ground_->Initialize();
    ground_->SetModel("ground_black.gltf");
    ground_->SetUvTransform(groundUvTransform_);
    ground_->SetEnableHighlight(false);

    //-----------Player の初期化----------------//
    player_ = std::make_unique<Player>();
    player_->Initialize();
    player_->SetCamera((*Object3dBasic::GetInstance()->GetCamera()));
    player_->SetInputHandler(inputHandler_.get());

    //-----------Boss の初期化--------------------//
    boss_ = std::make_unique<Boss>();
    boss_->Initialize();
    boss_->SetPlayer(player_.get());
    // 開始演出が終わるまで一時停止
    boss_->SetIsPause(true);
    player_->SetBoss(boss_.get());

    controllerUI_->SetBoss(boss_.get());
}

void GameScene::InitializeCameraSystem()
{
    cameraManager_ = CameraManager::GetInstance();
    cameraManager_->Initialize((*Object3dBasic::GetInstance()->GetCamera()));

    // ThirdPersonController を登録
    auto tpController = std::make_unique<ThirdPersonController>();
    tpController->SetTarget(&player_->GetTransform());
    // ボスをセカンダリターゲットにして注視を有効化
    tpController->SetSecondaryTarget(&boss_->GetTransform());
    tpController->EnableLookAtTarget(true);
    cameraManager_->RegisterController("ThirdPerson", std::move(tpController));

    // TopDownController を登録
    auto tdController = std::make_unique<TopDownController>();
    tdController->SetTarget(&player_->GetTransform());
    std::vector<const Transform*> additionalTargets = { &boss_->GetTransform() };
    tdController->SetAdditionalTargets(additionalTargets);
    cameraManager_->RegisterController("TopDown", std::move(tdController));

    // CameraAnimationController を登録
    auto animController = std::make_unique<CameraAnimationController>();
    animationController_ = animController.get();
    cameraManager_->RegisterController("Animation", std::move(animController));
}

void GameScene::SetCollisionMask()
{
    CollisionManager* collisionManager = CollisionManager::GetInstance();

    collisionManager->SetCollisionMask(
        static_cast<uint32_t>(CollisionTypeId::PLAYER_ATTACK),
        static_cast<uint32_t>(CollisionTypeId::BOSS),
        true
    );

    collisionManager->SetCollisionMask(
        static_cast<uint32_t>(CollisionTypeId::PLAYER),
        static_cast<uint32_t>(CollisionTypeId::BOSS_ATTACK),
        true
    );

    collisionManager->SetCollisionMask(
        static_cast<uint32_t>(CollisionTypeId::PLAYER_PROJECTILE),
        static_cast<uint32_t>(CollisionTypeId::BOSS),
        true
    );

    collisionManager->SetCollisionMask(
        static_cast<uint32_t>(CollisionTypeId::PLAYER),
        static_cast<uint32_t>(CollisionTypeId::BOSS_PROJECTILE),
        true
    );

    collisionManager->SetCollisionMask(
        static_cast<uint32_t>(CollisionTypeId::PLAYER_PROJECTILE),
        static_cast<uint32_t>(CollisionTypeId::BOSS_PROJECTILE),
        true
    );
}

void GameScene::InitializeEmitterManager()
{
    emitterManager_->LoadScenePreset("gamescene_preset");

    // フェーズ2まで境界線は非表示
    emitterManager_->SetEmitterActive("boss_border_left", false);
    emitterManager_->SetEmitterActive("boss_border_right", false);
    emitterManager_->SetEmitterActive("boss_border_front", false);
    emitterManager_->SetEmitterActive("boss_border_back", false);

    // ボス近接攻撃予兆エフェクト
    emitterManager_->LoadPreset("boss_attack_sign", "boss_melee_attack_sign");
    emitterManager_->SetEmitterActive("boss_melee_attack_sign", false);

    emitterManager_->LoadPreset("can_attack_sign", "can_attack_sign");

    // ===== Phase1 リパルス・ショックウェーブ用エミッター =====
    emitterManager_->LoadPreset("boss_repel_ring", "boss_repel_ring");
    emitterManager_->SetEmitterActive("boss_repel_ring", false);
    emitterManager_->LoadPreset("boss_repel_flash", "boss_repel_flash");
    emitterManager_->SetEmitterActive("boss_repel_flash", false);

    // ===== Phase2 ヴォルテックス・テンペスト用エミッター =====
    for (int i = 0; i < kVortexEmitterCount; ++i) {
        const std::string emitterName = "boss_vortex_" + std::to_string(i);
        emitterManager_->LoadPreset("boss_vortex", emitterName);
        emitterManager_->SetEmitterActive(emitterName, false);
    }

    // ===== 追従レーザー用エミッター（本体＋チャージ）=====
    emitterManager_->LoadPreset("boss_razer_trail", "boss_tracking_laser");
    emitterManager_->SetEmitterActive("boss_tracking_laser", false);
    emitterManager_->LoadPreset("boss_charge_effect", "boss_tracking_laser_charge");
    emitterManager_->SetEmitterActive("boss_tracking_laser_charge", false);

    boss_->SetEmitterManager(emitterManager_.get());

    // ボスモデルをスポーン形状とする MeshEmitter
    boss_->InitializeAuraEmitter();

    // テレポート演出用 MeshEmitter
    boss_->InitializeBodyParticleEmitter();

    boss_->SetForceFieldManager(forceFieldManager_.get());

    player_->SetForceFieldManager(forceFieldManager_.get());

    player_->SetEmitterManager(emitterManager_.get());

    emitterManager_->LoadPreset("parry_true", "parry_effect");
    emitterManager_->SetEmitterActive("parry_effect", false);
    emitterManager_->SetEmitterActive("parry_success", false);
}

void GameScene::InitializeEffectManager()
{
    overEffectManager_ = std::make_unique<OverEffectManager>(emitterManager_.get());
    overEffectManager_->SetTarget(player_.get());

    clearEffectManager_ = std::make_unique<ClearEffectManager>(emitterManager_.get());
    clearEffectManager_->SetTarget(boss_.get());

    bossBorderManager_ = std::make_unique<BossBorderParticleManager>(emitterManager_.get(), GameConst::kBossPhase2AreaSize);

    dashEffectManager_ = std::make_unique<DashEffectManager>(emitterManager_.get());
    dashEffectManager_->InitializePosition(player_->GetTranslate());

    projectileManager_ = std::make_unique<ProjectileManager>(emitterManager_.get());

    // プレイヤー弾を ForceField の影響下に置く
    projectileManager_->SetForceFieldManager(forceFieldManager_.get());
}

void GameScene::SetCameraAnimation()
{
    // ゲーム開始アニメーションを再生
    animationController_->LoadAnimationFromFile("game_start");
    cameraManager_->ActivateController("Animation");
    animationController_->SwitchAnimation("game_start");
    animationController_->Play();

    animationController_->LoadAnimationFromFile("over_anim");
    animationController_->SetAnimationTargetByName("over_anim", player_->GetTransformPtr());

    animationController_->LoadAnimationFromFile("clear_anim");
    animationController_->SetAnimationTargetByName("clear_anim", boss_->GetTransformPtr());
}

void GameScene::CheckPause()
{
    // ゲーム開始後、演出中以外、生存中のみポーズ可能
    if (isStart_ && !player_->IsDead() && !boss_->IsDead() &&
        animationController_->GetPlayState() != CameraAnimation::PlayState::PLAYING) {
        if (inputHandler_->IsPaused()) {
            isPaused_ = !isPaused_;
            controllerUI_->SetIsPaused(isPaused_);
            if (isPaused_) {
                PostEffectManager::GetInstance()->SetEffectParam("GaussianBlur", GaussianBlurParam{ .sigma = 20.0f, .kernelSize = 30 });
                PostEffectManager::GetInstance()->AddEffectToChain("GaussianBlur");
                player_->SetIsPause(true);
                boss_->SetIsPause(true);
                pauseMenu_->Reset();
            }
            else {
                PostEffectManager::GetInstance()->RemoveEffectFromChain("GaussianBlur");
                player_->SetIsPause(false);
                boss_->SetIsPause(false);
            }
        }
    }
}

void GameScene::UpdatePause()
{
    PauseMenu::Action action = pauseMenu_->Update();
    switch (action) {
    case PauseMenu::Action::Resume:
        isPaused_ = false;
        PostEffectManager::GetInstance()->RemoveEffectFromChain("GaussianBlur");
        controllerUI_->SetIsPaused(false);
        player_->SetIsPause(false);
        boss_->SetIsPause(false);
        break;
    case PauseMenu::Action::ToTitle:
        SceneManager::GetInstance()->ChangeScene("title", "Fade", kFadeDuration);
        break;
    case PauseMenu::Action::ExitGame:
        PostQuitMessage(0);
        break;
    default:
        break;
    }
}