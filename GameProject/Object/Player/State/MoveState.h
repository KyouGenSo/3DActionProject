#pragma once
#include "PlayerState.h"

/// <summary>
/// 移動状態クラス（歩行・走行）
/// </summary>
class MoveState : public PlayerState
{
public: //メンバー関数
    MoveState() : PlayerState("Walk") {}

    void Enter(Player* player) override;
    void Update(Player* player, float deltaTime) override;
    void Exit(Player* player) override;
    void HandleInput(Player* player) override;
    void DrawImGui(Player* player) override;

private: //メンバー変数
    float moveTime_ = 0.0f;
};
