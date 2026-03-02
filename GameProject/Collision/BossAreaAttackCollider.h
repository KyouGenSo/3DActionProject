#pragma once
#include "OBBCollider.h"

class Boss;
class Player;

/// <summary>
/// ボスエリア攻撃用コライダークラス
/// フェーズ2で矩形エリアを分割した範囲攻撃のダメージ判定を管理
/// </summary>
class BossAreaAttackCollider : public Tako::OBBCollider {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="boss">このコライダーを所有するボス</param>
    BossAreaAttackCollider(Boss* boss);

    /// <summary>
    /// 衝突開始時のコールバック
    /// プレイヤーに接触した場合ダメージを与える
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
