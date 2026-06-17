#include "PlayerBullet.h"
#include "../../Collision/BulletCollider.h"
#include "../../Object/Boss/Boss.h"
#include "../../Common/GameConst.h"
#include "../../Common/ForceFieldAffectMask.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "CollisionManager.h"
#include "EmitterManager.h"
#include "ForceFieldManager.h"
#include "GlobalVariables.h"
#include <format>

using namespace Tako;

uint32_t PlayerBullet::id = 0;

PlayerBullet::PlayerBullet(EmitterManager* emitterManager) {
    GlobalVariables* gv = GlobalVariables::GetInstance();

    damage_ = gv->GetValueFloat("PlayerBullet", "Damage");
    lifeTime_ = gv->GetValueFloat("PlayerBullet", "Lifetime");

    // GlobalVariables 未登録時のフォールバック
    if (damage_ <= 0.0f) {
        damage_ = 10.0f;
    }
    if (lifeTime_ <= 0.0f) {
        lifeTime_ = 3.0f;
    }

    emitterManager_ = emitterManager;

    if (emitterManager_) {
        bulletEmitterName_ = std::format("player_bullet{}", id);
        explodeEmitterName_ = std::format("player_bullet_explode{}", id);
        emitterManager_->LoadPreset("player_bullet", bulletEmitterName_);
        emitterManager_->SetEmitterActive(bulletEmitterName_, false);
        emitterManager_->LoadPreset("player_bullet_explode", explodeEmitterName_);
        emitterManager_->SetEmitterActive(explodeEmitterName_, false);
        effectEmitterName_ = std::format("player_bullet_effect{}", id);
        emitterManager_->LoadPreset("player_bullet_effect", effectEmitterName_);
        emitterManager_->SetEmitterActive(effectEmitterName_, false);
    }

    id++;

    if (id > kIdResetThreshold) {
        id = 0;
    }
}

PlayerBullet::~PlayerBullet() = default;

void PlayerBullet::Initialize(const Vector3& position, const Vector3& velocity) {
    Projectile::Initialize(position, velocity);

    Projectile::SetDefaultModel();
    model_->Update();

    transform_.scale = Vector3(kInitialScale, kInitialScale, kInitialScale);

    Projectile::ActivateBulletEmitter(position);

    if (emitterManager_) {
        emitterManager_->SetEmitterActive(effectEmitterName_, true);
        emitterManager_->SetEmitterPosition(effectEmitterName_, position);
    }

    if (!collider_) {
        collider_ = std::make_unique<BulletCollider>(this,
            static_cast<uint32_t>(CollisionTypeId::BOSS),
            static_cast<uint32_t>(CollisionTypeId::BOSS_PROJECTILE));
        collider_->SetHitHandler([this](Tako::Collider* other) {
            Boss* boss = static_cast<Boss*>(other->GetOwner());
            if (!boss) return;
            if (!boss->IsInPhaseTransitionStun()) {
                boss->OnHit(GetDamage(), 0.5f);
            }
            SetActive(false);
        });
    }

    GlobalVariables* gv = GlobalVariables::GetInstance();
    float colliderRadius = gv->GetValueFloat("PlayerBullet", "ColliderRadius");
    if (colliderRadius <= 0.0f) {
        colliderRadius = 0.5f;
    }

    collider_->SetTransform(&transform_);
    collider_->SetRadius(colliderRadius);
    collider_->SetOffset(Vector3(0.0f, 0.0f, 0.0f));
    collider_->SetTypeID(static_cast<uint32_t>(CollisionTypeId::PLAYER_PROJECTILE));
    collider_->SetOwner(this);
    collider_->SetActive(true);
    collider_->Reset();

    CollisionManager::GetInstance()->AddCollider(collider_.get());
}

void PlayerBullet::Finalize() {
    if (collider_) {
        CollisionManager::GetInstance()->RemoveCollider(collider_.get());
    }

    // 追加エフェクトは一時化せず即時破棄（爆発演出は explode 側で完結）
    if (emitterManager_) {
        emitterManager_->RemoveEmitter(effectEmitterName_);
    }

    Projectile::FinalizeEmitters();
}

void PlayerBullet::Update(float deltaTime) {
    if (!isActive_) {
        return;
    }

    // GPU パーティクルと同じ力場を CPU 駆動の弾にも作用させる
    if (forceFieldManager_) {
        const Vector3 force = forceFieldManager_->EvaluateForceAt(
            transform_.translate, GameForceField::AffectBullets);
        velocity_ += force * deltaTime;
    }

    Projectile::Update(deltaTime);

    // 減速時に消滅（Repel 攻撃で吹き飛ばされたケース）。sqrt 回避のため2乗比較
    if (velocity_.LengthSquared() < kMinSpeedSquared) {
        isActive_ = false;
    }

    if (emitterManager_) {
        emitterManager_->SetEmitterPosition(bulletEmitterName_, transform_.translate);
        emitterManager_->SetEmitterPosition(explodeEmitterName_, transform_.translate);
        emitterManager_->SetEmitterPosition(effectEmitterName_, transform_.translate);
    }

    // エリア外に出たら非アクティブ化
    Vector3 pos = transform_.translate;
    if (pos.x < GameConst::kStageXMin || pos.x > GameConst::kStageXMax ||
        pos.z < GameConst::kStageZMin || pos.z > GameConst::kStageZMax ||
        pos.y < yBoundaryMin_ || pos.y > yBoundaryMax_) {
        isActive_ = false;
    }
}

