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
    ResolvePlayerHit(other, damage_);
}
