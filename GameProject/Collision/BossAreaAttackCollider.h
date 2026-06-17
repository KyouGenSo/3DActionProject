#pragma once
#include "OBBCollider.h"

class Boss;
class Player;

/// <summary>
/// フェーズ2の分割矩形エリアによる範囲攻撃のダメージ判定
/// </summary>
class BossAreaAttackCollider : public Tako::OBBCollider {
public:
    BossAreaAttackCollider(Boss* boss);

    /// <summary>
    /// プレイヤー命中時に damage_ を適用（パリィ中なら成功扱い）
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionEnter(Tako::Collider* other) override;

    void SetDamage(float damage) { damage_ = damage; }

    float GetDamage() const { return damage_; }

private:
    Boss* boss_ = nullptr;
    float damage_ = 15.0f;
};
