#pragma once
#include "PlayerState.h"

/// <summary>
/// ダッシュ状態クラス（短時間の高速移動）
/// </summary>
class DashState : public PlayerState
{
public: //メンバー関数
    DashState() : PlayerState("Dash") {}

    void Enter(Player* player) override;
    void Update(Player* player, float deltaTime) override;
    void Exit(Player* player) override;
    void HandleInput(Player* player) override;
    void DrawImGui(Player* player) override;

    //==============================
    //Setter
    //==============================
    void SetDuration(float duration) { duration_ = duration; }
    void SetSpeed(float speed) { speed_ = speed; }

    //==============================
    //Getter
    //==============================
    float GetTimer() const { return timer_; }
    float GetDuration() const { return duration_; }
    float GetSpeed() const { return speed_; }

private: //メンバー変数
    float timer_    = 0.0f;
    float duration_ = 0.05f;    ///< 秒
    float speed_    = 10.0f;
};
