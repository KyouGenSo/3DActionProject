// Engine includes
#include "GameScene.h"
#include "WinApp.h"
#include "Object3dBasic.h"
#include "SpriteBasic.h"
#include "Input.h"
#include "Draw2D.h"
#include "FrameTimer.h"
#include "GPUParticle.h"
#include "SceneManager.h"
#include "EmitterManager.h"
#include "Object3d.h"
#include "Model.h"
#include "ShadowRenderer.h"
#include "CollisionManager.h"
#include "GlobalVariables.h"
#include "PostEffectManager.h"
#include "DecalManager.h"

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

void GameScene::Initialize()
{
    /// ================================== ///
    ///              初期化処理             ///
    /// ================================== ///

    /// ----------------------エンジンクラス初期化---------------------------------------------------------///
    // CollisionManager の初期化
    CollisionManager::GetInstance()->Initialize();

    // EmitterManager の生成
    emitterManager_ = std::make_unique<EmitterManager>(GPUParticle::GetInstance());

    // Input Handler の初期化
    inputHandler_ = std::make_unique<InputHandler>();
    inputHandler_->Initialize();

    // コントローラー UI の初期化
    controllerUI_ = std::make_unique<ControllerUI>();
    controllerUI_->Initialize();

    // ポーズメニューの初期化
    pauseMenu_ = std::make_unique<PauseMenu>();
    pauseMenu_->Initialize();

    InitializePostEffect();

    InitializeDebugOption();

    /// ----------------------シーンの描画設定---------------------------------------------------------///
    // GlobalVariables を取得
    GlobalVariables* gvScene = GlobalVariables::GetInstance();
    // シャドウマッピンの最大描画距離の設定
    float shadowMaxDist = gvScene->GetValueFloat("GameScene", "ShadowMaxDistance");
    ShadowRenderer::GetInstance()->SetMaxShadowDistance(shadowMaxDist);
    // 平行光源の方向の設定
    float lightZ = gvScene->GetValueFloat("GameScene", "DirectionalLightZ");
    Object3dBasic::GetInstance()->SetDirectionalLightDirection(Vector3(0.0f, -1.0f, lightZ));


    // 3D オブジェクトの初期化
    InitializeObject3d();

    // カメラシステムの初期化
    InitializeCameraSystem();

    // エミッターマネージャーの初期化
    InitializeEmitterManger();

    // エフェクトマネージャーの初期化
    InitializeEffectManager();

    // 衝突マスクの設定
    SetCollisionMask();

    // カメラアニメーション設定
    SetCameraAnimation();
}

void GameScene::Finalize()
{
#ifdef _DEBUG
    // デバッグ情報の登録解除
    DebugUIManager::GetInstance()->ClearDebugInfo();
    // ゲームオブジェクトの登録解除
    DebugUIManager::GetInstance()->ClearGameObjects();
    // AnimationEditor のクリーンアップ
    CameraDebugUI::CleanupAnimationEditor();
#endif

    // オブジェクトの終了処理
    if (player_) {
        player_->Finalize();
    }
    if (boss_) {
        boss_->Finalize();
    }

    // CameraManager のクリーンアップ
    if (cameraManager_) {
        cameraManager_->Finalize();
    }

    // CollisionManager のリセット
    CollisionManager::GetInstance()->Reset();

    // デカールの登録をクリア
    DecalManager::GetInstance()->ClearDecals();

    // PostEffec をクリア
    PostEffectManager::GetInstance()->ClearEffectChain();

    // ポーズメニューのクリア
    pauseMenu_.reset();
}

void GameScene::Update()
{
    /// ================================== ///
    ///              更新処理               ///
    /// ================================== ///

#ifdef _DEBUG
    // P キーでカメラモード切り替え
    if (Input::GetInstance()->TriggerKey(DIK_P)) {
        cameraMode_ = !cameraMode_;
    }

#endif

    // ポーズ中でも入力は更新（メニュー操作に必要、かつポーズトグル判定の前に実行する必要がある）
    inputHandler_->Update();

    // ポーズ入力チェック（ゲーム開始後、演出中以外、生存中のみ）
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
            } else {
                PostEffectManager::GetInstance()->RemoveEffectFromChain("GaussianBlur");
                player_->SetIsPause(false);
                boss_->SetIsPause(false);
            }
        }
    }

    // ポーズ中はメニュー更新のみ
    if (isPaused_) {
        UpdatePause();
        return;
    }

    // ゲーム開始演出終了後、ボスの一時停止を解除
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

    // カメラモードの更新
    UpdateCameraMode();

    // 入力の更新
    UpdateInput();

    // オブジェクトの更新処理
    skyBox_->Update();
    ground_->Update();
    player_->Update();
    boss_->Update(FrameTimer::GetInstance()->GetDeltaTime());
    controllerUI_->Update();
    cameraManager_->Update(FrameTimer::GetInstance()->GetDeltaTime());

    // ボス・プレイヤーが貯めた弾リクエストを ProjectileManager に流し、生成させる
    projectileManager_->SpawnBossBullets(boss_->ConsumePendingBullets());
    projectileManager_->SpawnPenetratingBossBullets(boss_->ConsumePendingPenetratingBullets());
    projectileManager_->SpawnPlayerBullets(player_->ConsumePendingBullets());

    // プロジェクタイル（弾）の更新＆非アクティブ弾の削除
    float deltaTime = FrameTimer::GetInstance()->GetDeltaTime();
    projectileManager_->Update(deltaTime);

    // ダッシュエフェクトの更新
    bool isDashing = false;
    if (player_ && player_->GetStateMachine() && player_->GetStateMachine()->GetCurrentState()) {
        isDashing = (player_->GetStateMachine()->GetCurrentState()->GetName() == "Dash");
    }
    dashEffectManager_->Update(deltaTime, player_->GetTranslate(), isDashing);

    // ボーダーパーティクルの更新
    bossBorderManager_->Update(boss_->GetPhase(), boss_->GetTranslate());

    // エミッターマネージャーの更新
    emitterManager_->Update();

    // ゲームオーバー演出の更新
    overEffectManager_->Update(deltaTime);
    if (overEffectManager_->IsComplete()) {
        SceneManager::GetInstance()->ChangeScene("over", "Fade", 0.3f);
    }

    // ゲームクリア演出の更新
    clearEffectManager_->Update(deltaTime);
    if (clearEffectManager_->IsComplete()) {
        SceneManager::GetInstance()->ChangeScene("clear", "Fade", 0.3f);
    }

    // 衝突判定の実行
    CollisionManager::GetInstance()->CheckAllCollisions();
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
        boss_->Draw();
        ShadowRenderer::GetInstance()->EndShadowPass();
    }

    //------------------背景 Sprite の描画------------------//
    // スプライト共通描画設定
    SpriteBasic::GetInstance()->SetCommonRenderSetting();



    //-------------------Model の描画-------------------//
    // 3D モデル共通描画設定
    Object3dBasic::GetInstance()->SetCommonRenderSetting();

    ground_->Draw();
    player_->Draw();
    boss_->Draw();

    //------------------前景 Sprite の描画------------------//
    // スプライト共通描画設定
    SpriteBasic::GetInstance()->SetCommonRenderSetting();



#ifdef _DEBUG
    // コライダーのデバッグ描画
    CollisionManager::GetInstance()->DrawColliders();
#endif

}

void GameScene::DrawWithoutEffect()
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

    player_->DrawSprite();
    boss_->DrawSprite();

    // コントローラー UI 描画
    controllerUI_->Draw();

    // ポーズメニュー描画（最前面）
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
        // フェーズ1: 動的制限を解除（ステージ全体を移動可能）
        player_->ClearDynamicBounds();
    }
    else if (boss_->GetPhase() == 2) {
        cameraMode_ = true;
        // フェーズ2: ボス中心の戦闘エリアに移動制限
        Vector3 bossPos = boss_->GetTransform().translate;
        player_->SetDynamicBoundsFromCenter(bossPos, GameConst::kBossPhase2AreaSize, GameConst::kBossPhase2AreaSize);
    }

    if (cameraMode_) {
        cameraManager_->ActivateController("ThirdPerson");
    }
    else {
        cameraManager_->ActivateController("TopDown");
    }

    // カメラモードを Player に設定
    player_->SetMode(cameraMode_);

}

void GameScene::UpdateInput()
{
    // 入力更新は Update()冒頭で実行済み
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
    Draw2D::GetInstance()->SetDebug(false);
    GPUParticle::GetInstance()->SetIsDebug(false);

    // DebugUIManager にシーン名を設定
    DebugUIManager::GetInstance()->SetSceneName("GameScene");

    // ゲームオブジェクトを DebugUIManager に登録
    DebugUIManager::GetInstance()->RegisterGameObject("Player",
        [this]() { if (player_) player_->DrawImGui(); });
    DebugUIManager::GetInstance()->RegisterGameObject("Boss",
        [this]() { if (boss_) boss_->DrawImGui(); });

    // CameraSystem デバッグ UI 登録
    DebugUIManager::GetInstance()->RegisterGameObject("CameraSystem",
        []() { CameraDebugUI::Draw(); });

    // CameraAnimationEditor デバッグ UI 登録
    DebugUIManager::GetInstance()->RegisterGameObject("CameraAnimationEditor",
        []() {
            CameraDebugUI::DrawAnimationEditorOnly();
            // 更新処理も呼び出す
            CameraDebugUI::UpdateAnimationEditor(
                FrameTimer::GetInstance()->GetDeltaTime());
        });

    // コントローラー UI の DebugUI 登録
    DebugUIManager::GetInstance()->RegisterGameObject("ControllerUI",
        [this]() { if (controllerUI_) controllerUI_->DrawImGui(); });

    // ポーズメニューの DebugUI 登録
    DebugUIManager::GetInstance()->RegisterGameObject("PauseMenu",
        [this]() { if (pauseMenu_) pauseMenu_->DrawImGui(); });

    DebugUIManager::GetInstance()->SetEmitterManager(emitterManager_.get());
#endif
}

void GameScene::InitializePostEffect()
{
    // PostEffect の初期化
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
    // SkyBox の初期化
    skyBox_ = std::make_unique<SkyBox>();
    skyBox_->Initialize("my_skybox.dds");

    // 床モデルの UV 変換設定
    groundUvTransform_.translate = Vector3(0.0f, 0.0f, 0.0f);
    groundUvTransform_.rotate = Vector3(0.0f, 0.0f, 0.0f);
    groundUvTransform_.scale = Vector3(100.0f, 100.0f, 100.0f);
    // 床モデルの初期化
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
    // ボスにプレイヤーの参照を設定
    boss_->SetPlayer(player_.get());
    // ゲーム開始時の演出が終わるまで一時停止状態に設定
    boss_->SetIsPause(true);
    // プレイヤーにボスの参照を設定
    player_->SetBoss(boss_.get());

    // コントローラー UI にボス参照を設定（フェーズ判定用）
    controllerUI_->SetBoss(boss_.get());
}

void GameScene::InitializeCameraSystem()
{
    // カメラマネージャーの初期化
    cameraManager_ = CameraManager::GetInstance();
    cameraManager_->Initialize((*Object3dBasic::GetInstance()->GetCamera()));

    // ThirdPersonController を登録
    auto tpController = std::make_unique<ThirdPersonController>();
    thirdPersonController_ = tpController.get();
    thirdPersonController_->SetTarget(&player_->GetTransform());
    // ボスをセカンダリターゲットとして設定し、注視機能を有効化
    thirdPersonController_->SetSecondaryTarget(&boss_->GetTransform());
    thirdPersonController_->EnableLookAtTarget(true);
    cameraManager_->RegisterController("ThirdPerson", std::move(tpController));

    // TopDownController を登録
    auto tdController = std::make_unique<TopDownController>();
    topDownController_ = tdController.get();
    topDownController_->SetTarget(&player_->GetTransform());
    std::vector<const Transform*> additionalTargets = { &boss_->GetTransform() };
    topDownController_->SetAdditionalTargets(additionalTargets);
    cameraManager_->RegisterController("TopDown", std::move(tdController));

    // CameraAnimationController を登録
    auto animController = std::make_unique<CameraAnimationController>();
    animationController_ = animController.get();
    cameraManager_->RegisterController("Animation", std::move(animController));
}

void GameScene::SetCollisionMask()
{
    // CollisionManager を取得
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

void GameScene::InitializeEmitterManger()
{
    // シーンのエミッターをまとめて読み込む
    emitterManager_->LoadScenePreset("gamescene_preset");

    // ボスフェーズ2用の境界線初期状態は無効化（ボスフェーズ2まで非表示）
    emitterManager_->SetEmitterActive("boss_border_left", false);
    emitterManager_->SetEmitterActive("boss_border_right", false);
    emitterManager_->SetEmitterActive("boss_border_front", false);
    emitterManager_->SetEmitterActive("boss_border_back", false);

    // ボス近接攻撃予兆エフェクトの読み込みと初期化
    emitterManager_->LoadPreset("boss_attack_sign", "boss_melee_attack_sign");
    emitterManager_->SetEmitterActive("boss_melee_attack_sign", false);

    // ボスのフェーズチェンジエフェクトの読み込み
    emitterManager_->LoadPreset("can_attack_sign", "can_attack_sign");

    // ボスに EmitterManager を設定
    boss_->SetEmitterManager(emitterManager_.get());

    // プレイヤーに EmitterManager を設定
    player_->SetEmitterManager(emitterManager_.get());

    // パリィエフェクトの初期状態を設定（gamescene_preset で読み込み済み）
    emitterManager_->SetEmitterActive("parry_effect", false);
    emitterManager_->SetEmitterActive("parry_success", false);
}

void GameScene::InitializeEffectManager()
{
    // ゲームオーバー演出マネージャー
    overEffectManager_ = std::make_unique<OverEffectManager>(emitterManager_.get());
    overEffectManager_->SetTarget(player_.get());

    // ゲームクリア演出マネージャー
    clearEffectManager_ = std::make_unique<ClearEffectManager>(emitterManager_.get());
    clearEffectManager_->SetTarget(boss_.get());

    // ボーダーパーティクルマネージャー
    bossBorderManager_ = std::make_unique<BossBorderParticleManager>(emitterManager_.get(), GameConst::kBossPhase2AreaSize);

    // ダッシュエフェクトマネージャー
    dashEffectManager_ = std::make_unique<DashEffectManager>(emitterManager_.get());
    dashEffectManager_->InitializePosition(player_->GetTranslate());

    // プロジェクタイル（弾）管理マネージャー
    projectileManager_ = std::make_unique<ProjectileManager>(emitterManager_.get());
}

void GameScene::SetCameraAnimation()
{
    // ゲーム開始アニメーションを再生
    animationController_->LoadAnimationFromFile("game_start");
    cameraManager_->ActivateController("Animation");
    animationController_->SwitchAnimation("game_start");
    animationController_->Play();

    // オーバー演出アニメーションの読み込みと設定
    animationController_->LoadAnimationFromFile("over_anim");
    animationController_->SetAnimationTargetByName("over_anim", player_->GetTransformPtr());

    // クリア演出アニメーションの読み込みと設定
    animationController_->LoadAnimationFromFile("clear_anim");
    animationController_->SetAnimationTargetByName("clear_anim", boss_->GetTransformPtr());
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
        SceneManager::GetInstance()->ChangeScene("title", "Fade", 0.3f);
        break;
    case PauseMenu::Action::ExitGame:
        PostQuitMessage(0);
        break;
    default:
        break;
    }
}