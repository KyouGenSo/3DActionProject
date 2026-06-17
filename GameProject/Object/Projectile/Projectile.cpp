#include "Projectile.h"
#include "Object3d.h"
#include "Model.h"
#include "EmitterManager.h"
#include "EnginePaths.h"

using namespace Tako;

Projectile::Projectile() {
}

Projectile::~Projectile() {
}

void Projectile::Initialize(const Vector3& position, const Vector3& velocity) {
    transform_.translate = position;
    velocity_ = velocity;

    if (!model_) {
        model_ = std::make_unique<Object3d>();
        model_->Initialize();
    }

    model_->SetTransform(transform_);

    elapsedTime_ = 0.0f;

    isActive_ = true;
}

void Projectile::Update(float deltaTime) {
    if (!isActive_) {
        return;
    }

    UpdateLifetime(deltaTime);

    Move(deltaTime);

    if (model_) {
        model_->SetTransform(transform_);
        model_->Update();
    }
}

void Projectile::Draw() {
    if (!isActive_ || !model_) {
        return;
    }

    model_->Draw();
}

void Projectile::UpdateLifetime(float deltaTime) {
    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= lifeTime_) {
        isActive_ = false;
    }
}

void Projectile::Move(float deltaTime) {
    transform_.translate += velocity_ * deltaTime;
}

void Projectile::SetDefaultModel() {
    if (model_) {
        model_->SetModel(EnginePaths::ModelPath("sphere.gltf"));
        if (!model_->GetModel()) {
            model_->SetModel(EnginePaths::ModelPath("white_cube.gltf"));
        }
    }
}

void Projectile::ActivateBulletEmitter(const Vector3& position) {
    if (emitterManager_) {
        emitterManager_->SetEmitterActive(bulletEmitterName_, true);
        emitterManager_->SetEmitterPosition(bulletEmitterName_, position);
    }
}

void Projectile::FinalizeEmitters() {
    if (emitterManager_) {
        emitterManager_->CreateTemporaryEmitterFrom(
            explodeEmitterName_,
            explodeEmitterName_ + "temp",
            0.5f);
        emitterManager_->RemoveEmitter(bulletEmitterName_);
        emitterManager_->RemoveEmitter(explodeEmitterName_);
    }
}