#include "BossBulletCollider.h"
#include "../Object/Player/Player.h"
#include "CollisionTypeIdDef.h"
#include "CollisionManager.h"

BossBulletCollider::BossBulletCollider(BossBullet* owner)
    : owner_(owner) {
}

void BossBulletCollider::OnCollisionEnter(Collider* other) {
    if (!owner_ || !owner_->IsActive()) {
        return;
    }

    // プレイヤーとの衝突判定
    if (other->GetTypeID() == static_cast<uint32_t>(CollisionTypeId::PLAYER)) {
        // 多重ヒット防止チェック
        void* targetPtr = other->GetOwner();
        if (hitTargets_.find(targetPtr) != hitTargets_.end()) {
            return;  // 既にヒット済み
        }

        // プレイヤーの取得
        Player* player = static_cast<Player*>(other->GetOwner());
        if (player) {
            hitPlayer_ = player;
            hitTargets_.insert(targetPtr);

            // パリィ判定
            if (player->IsParrying()) {
                player->OnParrySuccess();
                owner_->SetActive(false);
                return;
            }

            // プレイヤーにダメージを与える
            player->OnHit(owner_->GetDamage());
            hasDealtDamage_ = true;

            // 弾を非アクティブ化
            owner_->SetActive(false);
        }
    }
    else if (other->GetTypeID() == static_cast<uint32_t>(CollisionTypeId::PLAYER_PROJECTILE)) { // プレイヤーの弾との衝突判定
        // 弾を非アクティブ化
        owner_->SetActive(false);
    }
}

void BossBulletCollider::OnCollisionStay(Collider* other) {

}

void BossBulletCollider::OnCollisionExit(Collider* other) {
    // 衝突終了処理
    if (other->GetOwner() == hitPlayer_) {
        hitPlayer_ = nullptr;
    }
}

void BossBulletCollider::Reset() {
    hitTargets_.clear();
    hitPlayer_ = nullptr;
    hasDealtDamage_ = false;
}