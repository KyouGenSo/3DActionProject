#include "PenetratingBossBullet.h"
#include "../../Collision/BulletCollider.h"
#include "../../Object/Player/Player.h"
#include "../../Common/GameConst.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "CollisionManager.h"
#include "EmitterManager.h"
#include "RandomEngine.h"
#include "GlobalVariables.h"
#include <format>

using namespace Tako;

uint32_t PenetratingBossBullet::id = 0;

PenetratingBossBullet::PenetratingBossBullet(EmitterManager* emittermanager) {
    GlobalVariables* gv = GlobalVariables::GetInstance();

    damage_ = gv->GetValueFloat("PenetratingBossBullet", "Damage");
    lifeTime_ = gv->GetValueFloat("PenetratingBossBullet", "Lifetime");

    RandomEngine* rng = RandomEngine::GetInstance();

    rotationSpeed_ = Vector3(
        rng->GetFloat(rotationSpeedMin_, rotationSpeedMax_),
        rng->GetFloat(rotationSpeedMin_, rotationSpeedMax_),
        rng->GetFloat(rotationSpeedMin_, rotationSpeedMax_)
    );

    emitterManager_ = emittermanager;

    if (emitterManager_) {
        bulletEmitterName_ = std::format("boss_penetrate_bullet{}", id);
        explodeEmitterName_ = std::format("boss_penetrate_bullet_explode{}", id);
        emitterManager_->LoadPreset("boss_penetrate_bullet", bulletEmitterName_);
        emitterManager_->SetEmitterActive(bulletEmitterName_, false);
        emitterManager_->LoadPreset("boss_penetrate_bullet_explode", explodeEmitterName_);
        emitterManager_->SetEmitterActive(explodeEmitterName_, false);
        effectEmitterName_ = std::format("boss_penetrate_bullet_effect{}", id);
        emitterManager_->LoadPreset("boss_penetrate_bullet_effect", effectEmitterName_);
        emitterManager_->SetEmitterActive(effectEmitterName_, false);
    }

    id++;

    if (id > kIdResetThreshold) {
        id = 0;
    }
}

PenetratingBossBullet::~PenetratingBossBullet() = default;

void PenetratingBossBullet::Initialize(const Vector3& position, const Vector3& velocity) {
    Projectile::Initialize(position, velocity);

    Projectile::SetDefaultModel();
    model_->Update();

    transform_.scale = Vector3(kInitialScale, kInitialScale, kInitialScale);

    Projectile::ActivateBulletEmitter(position);

    if (emitterManager_) {
        emitterManager_->SetEmitterActive(effectEmitterName_, true);
        emitterManager_->SetEmitterPosition(effectEmitterName_, position);
    }

    // 貫通弾はプレイヤー弾と相殺しないため kNoCancelType を指定
    if (!collider_) {
        collider_ = std::make_unique<BulletCollider>(this,
            static_cast<uint32_t>(CollisionTypeId::PLAYER),
            BulletCollider::kNoCancelType);
        collider_->SetHitHandler([this](Tako::Collider* other) {
            Player* player = static_cast<Player*>(other->GetOwner());
            if (!player) return;
            if (player->IsParrying()) {
                player->OnParrySuccess();
            } else {
                player->OnHit(GetDamage());
            }
            SetActive(false);
        });
    }
    float colliderRadius = GlobalVariables::GetInstance()->GetValueFloat("PenetratingBossBullet", "ColliderRadius");
    collider_->SetTransform(&transform_);
    collider_->SetRadius(colliderRadius);
    collider_->SetOffset(Vector3(0.0f, 0.0f, 0.0f));
    collider_->SetTypeID(static_cast<uint32_t>(CollisionTypeId::BOSS_ATTACK));
    collider_->SetOwner(this);
    collider_->SetActive(true);
    collider_->Reset();

    CollisionManager::GetInstance()->AddCollider(collider_.get());
}

void PenetratingBossBullet::Finalize() {
    if (collider_) {
        CollisionManager::GetInstance()->RemoveCollider(collider_.get());
    }

    // 追加エフェクトは一時化せず即時破棄（爆発演出は explode 側で完結）
    if (emitterManager_) {
        emitterManager_->RemoveEmitter(effectEmitterName_);
    }

    Projectile::FinalizeEmitters();
}

void PenetratingBossBullet::Update(float deltaTime) {
    if (!isActive_) {
        return;
    }

    Projectile::Update(deltaTime);

    transform_.rotate += rotationSpeed_ * deltaTime;

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

