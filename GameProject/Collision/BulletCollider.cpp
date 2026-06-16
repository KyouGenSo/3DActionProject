#include "BulletCollider.h"
#include "../Object/Projectile/Projectile.h"

BulletCollider::BulletCollider(Projectile* owner, uint32_t targetTypeId, uint32_t cancelTypeId)
    : owner_(owner), targetTypeId_(targetTypeId), cancelTypeId_(cancelTypeId) {
}

void BulletCollider::OnCollisionEnter(Tako::Collider* other) {
    if (!owner_ || !owner_->IsActive()) {
        return;
    }

    uint32_t typeID = other->GetTypeID();

    // 対象へのヒット
    if (typeID == targetTypeId_) {
        // 多重ヒット防止
        void* targetPtr = other->GetOwner();
        if (hitTargets_.find(targetPtr) != hitTargets_.end()) {
            return;
        }
        hitTargets_.insert(targetPtr);

        if (hitHandler_) {
            hitHandler_(other);
        }
    }
    // 打ち消し対象との衝突で弾を消す
    else if (typeID == cancelTypeId_) {
        owner_->SetActive(false);
    }
}
