#pragma once
#include"TakoFramework.h"
#include"Vector2.h"

/// <summary>
/// 3D アクションゲームのメインクラス
/// </summary>
class MyGame : public Tako::TakoFramework
{
public: //メンバー関数
    void Initialize() override;

    void Finalize() override;

    void Update() override;

    void Draw() override;

private: //非公開関数
    void RegisterGlobalVariables();

    void RegisterInputVariables();

    void RegisterGameSceneVariables();

    void RegisterPlayerVariables();

    void RegisterBossVariables();

    void RegisterProjectileVariables();

    void RegisterStateVariables();

    void RegisterAttackStateVariables();

    void RegisterDashStateVariables();

    void RegisterParryStateVariables();

    void RegisterShootStateVariables();

    void LoadTextures();

    void LoadTitleTextures();

    void LoadButtonTextures();

    void LoadJoystickTextures();

    void LoadActionIconTextures();
};