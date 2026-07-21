#include "MyGame.h"

#include"Audio.h"
#include"Input.h"
#include "scene/SceneFactory.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "Draw2D.h"
#include "PostEffectManager.h"
#include "FrameTimer.h"
#include "GlobalVariables.h"
#include "ModelManager.h"
#include "GPUParticle.h"
#include "SpriteBasic.h"
#include "TransitionManager.h"

#include <numbers>

#ifdef _DEBUG
#include "DebugUIManager.h"
#endif

using namespace Tako;

void MyGame::Initialize()
{
    winApp_->SetWindowSize(1920, 1080);

    winApp_->SetWindowTitle(L"LE4A_12_キョウ_ゲンソ_Slash");

    TakoFramework::Initialize();

    sceneFactory_ = std::make_unique<SceneFactory>();
    SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());
    SceneManager::GetInstance()->ChangeScene("title", 0.0f);

    LoadTextures();

    RegisterGlobalVariables();

    GlobalVariables::GetInstance()->LoadFiles();
}

void MyGame::Finalize()
{
    TakoFramework::Finalize();
}

void MyGame::Update()
{
    // F11キーでフルスクリーン切り替え
    if (Input::GetInstance()->TriggerKey(DIK_F11)) {
        ToggleFullScreen();
    }

    TakoFramework::Update();

}

void MyGame::Draw()
{
    TakoFramework::Draw();
}

void MyGame::RegisterGlobalVariables()
{
    RegisterInputVariables();
    RegisterGameSceneVariables();
    RegisterPlayerVariables();
    RegisterBossVariables();
    RegisterProjectileVariables();
    RegisterStateVariables();
}

void MyGame::RegisterInputVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("Input");
    gv->AddItem("Input", "TriggerThreshold", 0.5f);
}

void MyGame::RegisterGameSceneVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("GameScene");
    gv->AddItem("GameScene", "ShadowMaxDistance", 100.0f);
    gv->AddItem("GameScene", "DirectionalLightZ", -0.05f);

    gv->CreateGroup("DashEffect");
    gv->AddItem("DashEffect", "LerpSpeed", 35.0f);

    gv->CreateGroup("CameraShake");
    gv->AddItem("CameraShake", "Duration", 0.3f);
    gv->AddItem("CameraShake", "Intensity", 0.5f);
}

void MyGame::RegisterPlayerVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("Player");
    gv->AddItem("Player", "BodyColliderSize", 3.2f);
    gv->AddItem("Player", "MeleeColliderX", 5.0f);
    gv->AddItem("Player", "MeleeColliderY", 2.0f);
    gv->AddItem("Player", "MeleeColliderZ", 17.0f);
    gv->AddItem("Player", "MeleeColliderOffsetZ", 10.0f);
    gv->AddItem("Player", "MoveInputDeadzone", 0.1f);
    gv->AddItem("Player", "RotationLerpSpeed", 0.2f);
    gv->AddItem("Player", "Speed", 0.5f);
    gv->AddItem("Player", "InitialY", 2.5f);
    gv->AddItem("Player", "InitialZ", -120.0f);
    gv->AddItem("Player", "AttackStartDistance", 5.0f);
    gv->AddItem("Player", "AttackMoveRotationLerp", 0.3f);
    gv->AddItem("Player", "BossLookatLerp", 1.15f);
    gv->AddItem("Player", "AttackMoveSpeed", 50.0f);
}

void MyGame::RegisterBossVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("Boss");
    gv->AddItem("Boss", "BodyColliderSize", 3.2f);
    gv->AddItem("Boss", "HitEffectDuration", 0.1f);
    gv->AddItem("Boss", "ShakeDuration", 0.3f);
    gv->AddItem("Boss", "ShakeIntensity", 0.2f);

    gv->CreateGroup("BossMeleeAttackCollider");
    gv->AddItem("BossMeleeAttackCollider", "Damage", 10.0f);
    gv->AddItem("BossMeleeAttackCollider", "ColliderSizeX", 2.0f);
    gv->AddItem("BossMeleeAttackCollider", "ColliderSizeY", 2.0f);
    gv->AddItem("BossMeleeAttackCollider", "ColliderSizeZ", 2.0f);
    gv->AddItem("BossMeleeAttackCollider", "OffsetZ", 3.0f);
}

void MyGame::RegisterProjectileVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("BossBullet");
    gv->AddItem("BossBullet", "ColliderRadius", 1.0f);
    gv->AddItem("BossBullet", "Damage", 10.0f);
    gv->AddItem("BossBullet", "Lifetime", 5.0f);

    gv->CreateGroup("PenetratingBossBullet");
    gv->AddItem("PenetratingBossBullet", "ColliderRadius", 1.0f);
    gv->AddItem("PenetratingBossBullet", "Damage", 15.0f);
    gv->AddItem("PenetratingBossBullet", "Lifetime", 5.0f);

    gv->CreateGroup("PlayerBullet");
    gv->AddItem("PlayerBullet", "Damage", 10.0f);
    gv->AddItem("PlayerBullet", "Lifetime", 3.0f);
    gv->AddItem("PlayerBullet", "ColliderRadius", 0.5f);
    gv->AddItem("PlayerBullet", "Speed", 30.0f);
}

void MyGame::RegisterStateVariables()
{
    RegisterAttackStateVariables();
    RegisterDashStateVariables();
    RegisterParryStateVariables();
    RegisterShootStateVariables();
}

void MyGame::RegisterAttackStateVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("AttackState");
    gv->AddItem("AttackState", "SearchTime", 0.1f);
    gv->AddItem("AttackState", "MoveTime", 0.1f);
    gv->AddItem("AttackState", "BlockRadius", 4.0f);
    gv->AddItem("AttackState", "BlockScale", 0.5f);
    gv->AddItem("AttackState", "RecoveryTime", 0.5f);
    gv->AddItem("AttackState", "MaxCombo", 4);

    // Combo0: 左振り（水平）
    gv->AddItem("AttackState", "Combo0_StartAngle", -1.5708f);
    gv->AddItem("AttackState", "Combo0_SwingAngle", std::numbers::pi_v<float>);
    gv->AddItem("AttackState", "Combo0_SwingDirection", 1.0f);
    gv->AddItem("AttackState", "Combo0_AttackDuration", 0.15f);
    gv->AddItem("AttackState", "Combo0_Damage", 5.0f);
    gv->AddItem("AttackState", "Combo0_Axis", 0);

    // Combo1: 右振り（水平）
    gv->AddItem("AttackState", "Combo1_StartAngle", 1.5708f);
    gv->AddItem("AttackState", "Combo1_SwingAngle", std::numbers::pi_v<float>);
    gv->AddItem("AttackState", "Combo1_SwingDirection", -1.0f);
    gv->AddItem("AttackState", "Combo1_AttackDuration", 0.15f);
    gv->AddItem("AttackState", "Combo1_Damage", 5.0f);
    gv->AddItem("AttackState", "Combo1_Axis", 0);

    // Combo2: 縦切り（垂直）
    gv->AddItem("AttackState", "Combo2_StartAngle", -1.5708f);
    gv->AddItem("AttackState", "Combo2_SwingAngle", std::numbers::pi_v<float>);
    gv->AddItem("AttackState", "Combo2_SwingDirection", 1.0f);
    gv->AddItem("AttackState", "Combo2_AttackDuration", 0.2f);
    gv->AddItem("AttackState", "Combo2_Damage", 5.0f);
    gv->AddItem("AttackState", "Combo2_Axis", 1);

    // Combo3: 大回転（水平360度）
    gv->AddItem("AttackState", "Combo3_StartAngle", 0.0f);
    gv->AddItem("AttackState", "Combo3_SwingAngle", std::numbers::pi_v<float> * 2);
    gv->AddItem("AttackState", "Combo3_SwingDirection", -1.0f);
    gv->AddItem("AttackState", "Combo3_AttackDuration", 0.3f);
    gv->AddItem("AttackState", "Combo3_Damage", 10.0f);
    gv->AddItem("AttackState", "Combo3_Axis", 0);
}

void MyGame::RegisterDashStateVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("DashState");
    gv->AddItem("DashState", "Duration", 0.05f);
    gv->AddItem("DashState", "Speed", 10.0f);
    gv->AddItem("DashState", "DashCooldown", 0.5f);
}

void MyGame::RegisterParryStateVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("ParryState");
    gv->AddItem("ParryState", "ParryDuration", 0.5f);
    gv->AddItem("ParryState", "ParrySuccessHealAmount", 5.0f);
    gv->AddItem("ParryState", "ParryCooldown", 1.0f);
}

void MyGame::RegisterShootStateVariables()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    gv->CreateGroup("ShootState");
    gv->AddItem("ShootState", "FireRate", 0.2f);
    gv->AddItem("ShootState", "MoveSpeedMultiplier", 0.5f);
    gv->AddItem("ShootState", "AimRotationLerp", 0.3f);
    gv->AddItem("ShootState", "AimAssistEnabled", true);
    gv->AddItem("ShootState", "AimAssistAngle", 30.0f);
    gv->AddItem("ShootState", "AimAssistStrength", 1.0f);
}

void MyGame::LoadTextures()
{
    LoadTitleTextures();
    LoadButtonTextures();
    LoadJoystickTextures();
    LoadActionIconTextures();
}

void MyGame::LoadTitleTextures()
{
    TextureManager* tm = TextureManager::GetInstance();

    // タイトルテキストアニメーション用
    tm->LoadTexture("title_text/title_text_1.dds");
    tm->LoadTexture("title_text/title_text_2.dds");
    tm->LoadTexture("title_text/title_text_3.dds");
    tm->LoadTexture("title_text/title_text_4.dds");
    tm->LoadTexture("title_text/title_text_5.dds");
    tm->LoadTexture("title_text/title_text_6.dds");
    tm->LoadTexture("title_text/title_text_7.dds");
    tm->LoadTexture("title_text/title_text_8.dds");
    tm->LoadTexture("title_text/title_text_9.dds");
    tm->LoadTexture("title_text/title_text_10.dds");

    // シーン遷移用テキスト
    tm->LoadTexture("titlescene_button.dds");
    tm->LoadTexture("game_button_text.dds");
    tm->LoadTexture("gameClear_Text.dds");
    tm->LoadTexture("gameOver_Text.dds");

    // ポーズメニュー用
    tm->LoadTexture("PauseButton_Resume.dds");
    tm->LoadTexture("PauseButton_Title.dds");
    tm->LoadTexture("PauseButton_Exit.dds");
    tm->LoadTexture("PauseMenu_Text.dds");
}

void MyGame::LoadButtonTextures()
{
    TextureManager* tm = TextureManager::GetInstance();
    tm->LoadTexture("button/A_Button_Down.dds");
    tm->LoadTexture("button/A_Button_Up.dds");
    tm->LoadTexture("button/B_Button_Down.dds");
    tm->LoadTexture("button/B_Button_Up.dds");
    tm->LoadTexture("button/X_Button_Down.dds");
    tm->LoadTexture("button/X_Button_Up.dds");
    tm->LoadTexture("button/Y_Button_Down.dds");
    tm->LoadTexture("button/Y_Button_Up.dds");
    tm->LoadTexture("button/Menu_Button_Up.dds");
    tm->LoadTexture("button/DPAD_Neutral.dds");
    tm->LoadTexture("button/DPAD_Up.dds");
    tm->LoadTexture("button/DPAD_Down.dds");
}

void MyGame::LoadJoystickTextures()
{
    TextureManager* tm = TextureManager::GetInstance();

    // 左スティック
    tm->LoadTexture("joystick/L_Joystick_01.dds");
    tm->LoadTexture("joystick/L_Joystick_02.dds");
    tm->LoadTexture("joystick/L_Joystick_03.dds");
    tm->LoadTexture("joystick/L_Joystick_04.dds");
    tm->LoadTexture("joystick/L_Joystick_05.dds");
    tm->LoadTexture("joystick/L_Joystick_06.dds");
    tm->LoadTexture("joystick/L_Joystick_07.dds");
    tm->LoadTexture("joystick/L_Joystick_08.dds");

    // 右スティック
    tm->LoadTexture("joystick/R_Joystick_01.dds");
    tm->LoadTexture("joystick/R_Joystick_02.dds");
    tm->LoadTexture("joystick/R_Joystick_03.dds");
    tm->LoadTexture("joystick/R_Joystick_04.dds");
    tm->LoadTexture("joystick/R_Joystick_05.dds");
    tm->LoadTexture("joystick/R_Joystick_06.dds");
    tm->LoadTexture("joystick/R_Joystick_07.dds");
    tm->LoadTexture("joystick/R_Joystick_08.dds");
}

void MyGame::LoadActionIconTextures()
{
    TextureManager* tm = TextureManager::GetInstance();
    tm->LoadTexture("kougeki.dds");
    tm->LoadTexture("dash.dds");
    tm->LoadTexture("parry.dds");
    tm->LoadTexture("shageki.dds");
    tm->LoadTexture("idou.dds");
    tm->LoadTexture("kettei.dds");
    tm->LoadTexture("sentaku.dds");
}
