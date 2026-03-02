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
    /// デストラクタ
    /// </summary>
    ~BossAreaAttackCollider() = default;

    /// <summary>
    /// 衝突開始時のコールバック
    /// プレイヤーに接触した場合ダメージを与える
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionEnter(Tako::Collider* other) override;

    /// <summary>
    /// 衝突継続中のコールバック
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionStay(Tako::Collider* other) override;

    /// <summary>
    /// コライダーの状態をリセット
    /// 次の攻撃に備えてヒットフラグをクリア
    /// </summary>
    void Reset();

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

    /// <summary>
    /// プレイヤーにヒットしたかどうかを取得
    /// </summary>
    /// <returns>ヒット済みなら true</returns>
    bool HasHitPlayer() const { return hasHitPlayer_; }

private:
    Boss* boss_ = nullptr;           ///< このコライダーを所有するボス
    float damage_ = 15.0f;           ///< 攻撃ダメージ量
    bool hasHitPlayer_ = false;      ///< 多重ヒット防止フラグ
};
