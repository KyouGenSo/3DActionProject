#pragma once
#include "OBBCollider.h"

class Player;
class Boss;

/// <summary>
/// プレイヤー近接攻撃の判定と敵へのダメージ処理
/// </summary>
class MeleeAttackCollider : public Tako::OBBCollider {
public: //メンバー関数
    MeleeAttackCollider(Player* player);
    virtual ~MeleeAttackCollider() = default;

    /// <summary>
    /// ボス検出時、canDamage_ が立っていればダメージとノックバックを1回適用
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionStay(Collider* other) override;

    /// <summary>
    /// 検出済みの敵をクリアし、次の攻撃で再検出できるようにする
    /// </summary>
    void Reset();

    /// <summary>
    /// canDamage_ フラグを立てるだけ。実ダメージは次の OnCollisionStay のヒット時に適用される
    /// </summary>
    void Damage();

    //====================================
    //Setter
    //====================================
    /// <summary>
    /// ノックバック有効フラグを設定（4コンボ目のみ true）
    /// </summary>
    void SetKnockbackEnabled(bool enabled) { knockbackEnabled_ = enabled; }

    void SetAttackDamage(float damage) { attackDamage_ = damage; }

    //====================================
    //Getter
    //====================================
    Boss* GetDetectedEnemy() const { return detectedEnemy_; }

#ifdef _DEBUG
    int GetCollisionCount() const { return collisionCount_; }
#endif

private: //メンバー変数
    Player* player_           = nullptr;
    Boss*   detectedEnemy_    = nullptr;
    bool    canDamage_        = false;
    float   attackDamage_{};
    bool    knockbackEnabled_ = true;     ///< 4コンボ目のみ true

#ifdef _DEBUG
    int collisionCount_ = 0;
#endif
};
