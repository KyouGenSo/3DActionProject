#pragma once
#include "PlayerState.h"
#include "Vector3.h"

/// <summary>
/// 射撃状態クラス（照準と発射）
/// </summary>
class ShootState : public PlayerState
{
public: //メンバー関数
    ShootState() : PlayerState("Shoot") {}

    void Enter(Player* player) override;
    void Update(Player* player, float deltaTime) override;
    void Exit(Player* player) override;
    void HandleInput(Player* player) override;
    void DrawImGui(Player* player) override;

    //==========================================
    //Setter
    //==========================================
    void SetFireRate(float rate) { fireRate_ = rate; }

    //==========================================
    //Getter
    //==========================================
    float GetFireRate() const { return fireRate_; }
    float GetFireRateTimer() const { return fireRateTimer_; }
    const Tako::Vector3& GetAimDirection() const { return aimDirection_; }

private: //非公開関数
    /// <summary>
    /// スティック入力とカメラ向きから照準方向を算出し、プレイヤーをその方向へ向ける
    /// </summary>
    /// <param name="player">対象プレイヤー（向きを更新する）</param>
    void CalculateAimDirection(Player* player);

    /// <summary>
    /// 現在の照準方向へ弾の生成リクエストを発行する
    /// </summary>
    /// <param name="player">発射元プレイヤー</param>
    void Fire(Player* player);

    /// <summary>
    /// 照準方向が敵方向から猶予角以内なら、強度に応じて敵方向へ引き寄せる（XZ平面のみ）
    /// </summary>
    /// <param name="player">対象プレイヤー（ターゲット取得元）</param>
    void ApplyAimAssist(Player* player);

private: //メンバー変数
    float fireRate_            = 0.2f;    ///< 秒間隔
    float fireRateTimer_       = 0.0f;
    float moveSpeedMultiplier_ = 0.5f;    ///< 射撃中の移動速度倍率

    bool  aimAssistEnabled_    = true;
    float aimAssistAngle_      = 30.0f;   ///< 猶予角（度数法）
    float aimAssistStrength_   = 1.0f;    ///< 0=補正なし〜1=完全スナップ

    Tako::Vector3 aimDirection_;
};
