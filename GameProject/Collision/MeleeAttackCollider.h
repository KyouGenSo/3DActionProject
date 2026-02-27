#pragma once
#include "OBBCollider.h"

class Player;
class Boss;

/// <summary>
/// 近接攻撃用コライダークラス
/// プレイヤーの近接攻撃判定と敵へのダメージ処理を管理
/// </summary>
class MeleeAttackCollider : public Tako::OBBCollider {
private:
    Player* player_ = nullptr;  ///< このコライダーを所有するプレイヤーへのポインタ
    Boss* detectedEnemy_ = nullptr;  ///< 現在検出されている敵への参照
    bool canDamage = false;  ///< ダメージを与えられる状態かどうか
    float attackDamage_{};  ///< 攻撃ダメージ量（GlobalVariables から取得）
    bool knockbackEnabled_ = true;  ///< ノックバック有効フラグ（4コンボ目のみ true）

#ifdef _DEBUG
    int collisionCount_ = 0;  ///< デバッグ用：衝突検出回数
#endif

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="player">このコライダーを所有するプレイヤー</param>
    MeleeAttackCollider(Player* player);

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~MeleeAttackCollider() = default;

    /// <summary>
    /// 衝突継続中のコールバック
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionStay(Collider* other) override;

    /// <summary>
    /// コライダーの状態をリセット（新しい攻撃の開始時に呼び出す）
    /// detectedEnemy をクリア
    /// </summary>
    void Reset();

    /// <summary>
    /// 検出された敵全員にダメージを与える
    /// canDamage フラグが true の時のみ実行可能
    /// </summary>
    void Damage();

    /// <summary>
    /// 現在検出されている敵を取得
    /// </summary>
    /// <returns>検出された敵へのポインタ（いない場合は nullptr）</returns>
    Boss* GetDetectedEnemy() const { return detectedEnemy_; }

    /// <summary>
    /// ノックバック有効フラグを設定（4コンボ目のみ true）
    /// </summary>
    /// <param name="enabled">有効にする場合 true</param>
    void SetKnockbackEnabled(bool enabled) { knockbackEnabled_ = enabled; }

    /// <summary>
    /// 攻撃ダメージ量を設定
    /// </summary>
    /// <param name="damage">ダメージ量</param>
    void SetAttackDamage(float damage) { attackDamage_ = damage; }

#ifdef _DEBUG
    /// <summary>
    /// デバッグ用：衝突検出回数を取得
    /// </summary>
    /// <returns>衝突検出回数</returns>
    int GetCollisionCount() const { return collisionCount_; }
#endif
};