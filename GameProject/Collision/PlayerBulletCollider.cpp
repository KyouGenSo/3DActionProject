#include "PlayerBulletCollider.h"
#include "../Object/Projectile/PlayerBullet.h"
#include "../Object/Boss/Boss.h"
#include "CollisionTypeIdDef.h"
#include "CollisionManager.h"
#include "CameraSystem/CameraManager.h"
#include "Object/Projectile/BossBullet.h"

using namespace Tako;

PlayerBulletCollider::PlayerBulletCollider(PlayerBullet* owner)
    : owner_(owner) {
}

void PlayerBulletCollider::OnCollisionEnter(Collider* other) {
    if (!owner_ || !owner_->IsActive()) {
        return;
    }

    // ボスとの衝突判定
    if (other->GetTypeID() == static_cast<uint32_t>(CollisionTypeId::BOSS)) {
        // 多重ヒット防止チェック
        void* targetPtr = other->GetOwner();
        if (hitTargets_.find(targetPtr) != hitTargets_.end()) {
            return;  // 既にヒット済み
        }

        // ボスの取得
        Boss* boss = static_cast<Boss*>(other->GetOwner());
        if (boss) {
            hitBoss_ = boss;
            hitTargets_.insert(targetPtr);

            // フェーズ移行スタン状態でなければダメージを与える
            if (!boss->IsInPhaseTransitionStun()) {
                boss->OnHit(owner_->GetDamage(), 0.5f);
                hasDealtDamage_ = true;
            }

            // 弾を非アクティブ化
            owner_->SetActive(false);
        }
    }
    else if (other->GetTypeID() == static_cast<uint32_t>(CollisionTypeId::BOSS_PROJECTILE)) { // ボスの弾との衝突判定
        owner_->SetActive(false);
    }
}

void PlayerBulletCollider::OnCollisionStay(Collider* other) {
    // 継続処理なし
}

void PlayerBulletCollider::OnCollisionExit(Collider* other) {
    // 衝突終了処理
    if (other->GetOwner() == hitBoss_) {
        hitBoss_ = nullptr;
    }
}

void PlayerBulletCollider::Reset() {
    hitTargets_.clear();
    hitBoss_ = nullptr;
    hasDealtDamage_ = false;
}
