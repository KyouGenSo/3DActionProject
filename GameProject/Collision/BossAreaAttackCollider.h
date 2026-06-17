#pragma once
#include "OBBCollider.h"

class Boss;
class Player;

/// <summary>
/// フェーズ2の分割矩形エリアによる範囲攻撃のダメージ判定
/// </summary>
class BossAreaAttackCollider : public Tako::OBBCollider {
public: //メンバー関数
    BossAreaAttackCollider(Boss* boss);

    /// <summary>
    /// プレイヤー命中時に damage_ を適用（パリィ中なら成功扱い）
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionEnter(Tako::Collider* other) override;

    //==========================
    //Setter
    //==========================
    void SetDamage(float damage) { damage_ = damage; }

    //==========================
    //Getter
    //==========================
    float GetDamage() const { return damage_; }

private: //メンバー変数
    Boss* boss_   = nullptr;
    float damage_ = 15.0f;
};
