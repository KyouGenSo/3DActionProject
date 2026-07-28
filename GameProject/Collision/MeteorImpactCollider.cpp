#include "MeteorImpactCollider.h"
#include "../Object/Boss/Boss.h"
#include "CollisionTypeIdDef.h"
#include "PlayerHitResolver.h"

using namespace Tako;

MeteorImpactCollider::MeteorImpactCollider(Boss* boss)
    : boss_(boss) {
    SetTypeID(static_cast<uint32_t>(CollisionTypeId::BOSS_ATTACK));
    SetActive(false);
}

void MeteorImpactCollider::OnCollisionEnter(Collider* other) {
    const bool isPlayer = other && other->GetTypeID() == static_cast<uint32_t>(CollisionTypeId::PLAYER);
    if (sharedHitGuard_ && isPlayer) {
        if (*sharedHitGuard_) return;
        *sharedHitGuard_ = true;
    }
    ResolvePlayerHit(other, damage_);
}
