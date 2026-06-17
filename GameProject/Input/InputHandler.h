#pragma once
#include <memory>
#include "vector2.h"

class Player;

/// <summary>
/// キーボード・ゲームパッド入力を統合し、アクション状態を提供
/// </summary>
class InputHandler
{
public: //メンバー関数
    InputHandler();
    ~InputHandler();

    void Initialize();
    void Update();
    void ResetInputs();

    //=====================================
    //Getter
    //=====================================
    bool IsMoving() const;
    bool IsDashing() const;
    bool IsAttacking() const;
    bool IsShooting() const;
    bool IsParrying() const;
    bool IsPaused() const;

    /// <summary>
    /// 左スティックとWASDを加算した移動入力ベクトルを返す
    /// </summary>
    /// <returns>移動方向。x=左右(右が正)、y=前後(前が正)。未正規化で斜め入力時は長さが1超になりうる</returns>
    Tako::Vector2 GetMoveDirection() const;

    /// <summary>
    /// 右スティックの照準方向を返す
    /// </summary>
    /// <returns>正規化された照準方向。デッドゾーン内・未接続時はゼロベクトル</returns>
    Tako::Vector2 GetAimDirection() const;

private: //メンバー変数
    //入力状態
    bool isMoving_    = false;
    bool isDashing_   = false;
    bool isAttacking_ = false;
    bool isShooting_  = false;
    bool isParrying_  = false;
    bool isPaused_    = false;

    //入力ベクトル
    Tako::Vector2 moveDirection_;
    Tako::Vector2 aimDirection_;
};