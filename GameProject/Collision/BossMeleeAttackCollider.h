#pragma once
#include "OBBCollider.h"

class Boss;
class Player;

/// <summary>
/// ボス近接攻撃のダメージ判定
/// </summary>
class BossMeleeAttackCollider : public Tako::OBBCollider {
public: //メンバー関数
    BossMeleeAttackCollider(Boss* boss);
    ~BossMeleeAttackCollider() = default;

    /// <summary>
    /// プレイヤー初回命中時のみ、パリィ中なら成功扱い、否なら damage_ を適用
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionEnter(Tako::Collider* other) override;

    /// <summary>
    /// 何もしない。ダメージは初回ヒットの1回のみで継続ヒットしない
    /// </summary>
    /// <param name="other">衝突相手のコライダー（未使用）</param>
    void OnCollisionStay(Tako::Collider* other) override;

    /// <summary>
    /// 多重ヒット防止フラグをクリアし、次の攻撃で再びヒット可能にする
    /// </summary>
    void Reset();

    //==========================
    //Setter
    //==========================
    void SetDamage(float damage) { damage_ = damage; }

    //==========================
    //Getter
    //==========================
    float GetDamage() const { return damage_; }
    bool HasHitPlayer() const { return hasHitPlayer_; }

private: //メンバー変数
    Boss* boss_         = nullptr;
    float damage_       = 10.0f;
    bool  hasHitPlayer_ = false;    ///< 多重ヒット防止
};
