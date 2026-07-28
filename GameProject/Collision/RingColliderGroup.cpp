#include "RingColliderGroup.h"

#include "CollisionManager.h"

#include <algorithm>
#include <cmath>

using namespace Tako;

RingColliderGroup::~RingColliderGroup() {
    Finalize();
}

void RingColliderGroup::Initialize(Boss* boss, int segmentCount) {
    Finalize();

    segmentCount_ = std::clamp(segmentCount, kMinSegments, kMaxSegments);

    for (int i = 0; i < segmentCount_; ++i) {
        transforms_[i].translate = Vector3(0.0f, 0.0f, 0.0f);
        transforms_[i].rotate = Vector3(0.0f, 0.0f, 0.0f);
        transforms_[i].scale = Vector3(1.0f, 1.0f, 1.0f);

        colliders_[i] = std::make_unique<MeteorImpactCollider>(boss);
        colliders_[i]->SetTransform(&transforms_[i]);
        colliders_[i]->SetOffset(Vector3(0.0f, 0.0f, 0.0f));
        colliders_[i]->SetOwner(boss);
        colliders_[i]->SetSharedHitGuard(&hitGuard_);
        CollisionManager::GetInstance()->AddCollider(colliders_[i].get());
    }
}

void RingColliderGroup::Finalize() {
    for (auto& collider : colliders_) {
        if (collider) {
            CollisionManager::GetInstance()->RemoveCollider(collider.get());
            collider.reset();
        }
    }
    hitGuard_ = false;
}

void RingColliderGroup::Activate(float midRadius) {
    prevMidRadius_ = midRadius;
    hitGuard_ = false;
    for (int i = 0; i < segmentCount_; ++i) {
        if (colliders_[i]) colliders_[i]->SetActive(true);
    }
}

void RingColliderGroup::Deactivate() {
    for (int i = 0; i < segmentCount_; ++i) {
        if (colliders_[i]) colliders_[i]->SetActive(false);
    }
}

void RingColliderGroup::UpdateArc(const Vector3& center, float yawRad, float sweepRad,
                                  float midRadius, float ringWidth, float extraBand) {
    const float thetaSeg = sweepRad / static_cast<float>(segmentCount_);

    // 前フレーム半径から今フレーム半径までを覆う帯（スイープ帯）
    const float bandWidth = (midRadius - prevMidRadius_) + ringWidth + extraBand;
    const float rMid = (midRadius + prevMidRadius_) * 0.5f;

    // 帯の厚みと隣接球の中心間距離の半分（下回ると弧上に隙間ができる）の大きい方
    const float radius = std::max(bandWidth * 0.5f, rMid * std::sin(thetaSeg * 0.5f)) * kOverlapFactor;

    for (int i = 0; i < segmentCount_; ++i) {
        if (!colliders_[i]) continue;
        const float alpha = -sweepRad * 0.5f + thetaSeg * (static_cast<float>(i) + 0.5f);
        const float beta = yawRad + alpha;
        transforms_[i].translate = center + Vector3(std::sin(beta), 0.0f, std::cos(beta)) * rMid;
        colliders_[i]->SetRadius(radius);
    }

    prevMidRadius_ = midRadius;
}

void RingColliderGroup::SetDamage(float damage) {
    for (int i = 0; i < segmentCount_; ++i) {
        if (colliders_[i]) colliders_[i]->SetDamage(damage);
    }
}
