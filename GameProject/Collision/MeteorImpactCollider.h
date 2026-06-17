#pragma once
#include "SphereCollider.h"

class Boss;
class Player;

/// <summary>
/// メテオレイン着弾地点の球範囲ダメージ判定
/// </summary>
class MeteorImpactCollider : public Tako::SphereCollider {
public: //メンバー関数
    MeteorImpactCollider(Boss* boss);

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
