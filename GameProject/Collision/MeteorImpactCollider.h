#pragma once
#include "SphereCollider.h"

class Boss;
class Player;

/// <summary>
/// メテオレイン着弾用スフィアコライダー
/// SphereCollider を継承し、着弾地点の円形範囲でダメージ判定を行う
/// </summary>
class MeteorImpactCollider : public Tako::SphereCollider {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="boss">このコライダーを所有するボス</param>
    MeteorImpactCollider(Boss* boss);

    /// <summary>
    /// 衝突開始時のコールバック
    /// プレイヤーに接触した場合、パリィ判定後にダメージを与える
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionEnter(Tako::Collider* other) override;

    /// <summary>
    /// ダメージ量を設定
    /// </summary>
    /// <param name="damage">設定するダメージ量</param>
    void SetDamage(float damage) { damage_ = damage; }

    /// <summary>
    /// ダメージ量を取得
    /// </summary>
    /// <returns>現在のダメージ量</returns>
    float GetDamage() const { return damage_; }

private:
    Boss* boss_ = nullptr;           ///< このコライダーを所有するボス
    float damage_ = 15.0f;           ///< 攻撃ダメージ量
};
