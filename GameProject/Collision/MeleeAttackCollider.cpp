#include "MeleeAttackCollider.h"
#include "../Object/Player/Player.h"
#include "../Object/Boss/Boss.h"
#include "CollisionTypeIdDef.h"

using namespace Tako;

MeleeAttackCollider::MeleeAttackCollider(Player* player)
    : player_(player) {

    SetTypeID(static_cast<uint32_t>(CollisionTypeId::PLAYER_ATTACK));
    SetActive(false);
}

void MeleeAttackCollider::OnCollisionStay(Collider* other) {
    if (!other) return;

#ifdef _DEBUG
    collisionCount_++;
#endif

    uint32_t typeID = other->GetTypeID();

    if (typeID == static_cast<uint32_t>(CollisionTypeId::BOSS)) {
        Boss* enemy = static_cast<Boss*>(other->GetOwner());
        if (enemy && !detectedEnemy_) {
            detectedEnemy_ = enemy;
            if (canDamage_) {
                // ノックバック方向（プレイヤー→ボス）。水平面のみ
                Tako::Vector3 knockbackDir = enemy->GetTransform().translate - player_->GetTransform().translate;
                knockbackDir.y = 0.0f;
                if (knockbackDir.Length() > 0.01f) {
                    knockbackDir = knockbackDir.Normalize();
                }
                enemy->OnMeleeAttackHit(attackDamage_, knockbackDir, knockbackEnabled_);
                canDamage_ = false;
            }
        }
    }
}

void MeleeAttackCollider::Reset() {
    detectedEnemy_ = nullptr;
#ifdef _DEBUG
    collisionCount_ = 0;
#endif
}

void MeleeAttackCollider::Damage()
{
    canDamage_ = true;
}