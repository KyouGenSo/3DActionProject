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
    // 位置と速度を設定
    transform_.translate = position;
    velocity_ = velocity;

    // モデルの初期化
    if (!model_) {
        model_ = std::make_unique<Object3d>();
        model_->Initialize();
    }

    // モデルに Transform を設定
    model_->SetTransform(transform_);

    // タイマーリセット
    elapsedTime_ = 0.0f;

    // アクティブ化
    isActive_ = true;
}

void Projectile::Update(float deltaTime) {
    if (!isActive_) {
        return;
    }

    // 生存時間を更新
    UpdateLifetime(deltaTime);

    // 移動処理
    Move(deltaTime);

    // モデルの更新
    if (model_) {
        model_->SetTransform(transform_);
        model_->Update();
    }
}

void Projectile::Draw() {
    if (!isActive_ || !model_) {
        return;
    }

    // モデルを描画
    model_->Draw();
}

void Projectile::UpdateLifetime(float deltaTime) {
    elapsedTime_ += deltaTime;

    // 生存時間を超えたら非アクティブ化
    if (elapsedTime_ >= lifeTime_) {
        isActive_ = false;
    }
}

void Projectile::Move(float deltaTime) {
    // 速度に基づいて移動
    transform_.translate += velocity_ * deltaTime;
}

void Projectile::SetDefaultModel() {
    if (model_) {
        model_->SetModel(EnginePaths::ModelPath("sphere.gltf"));
        if (!model_->GetModel()) {
            // sphere モデルがない場合は代替モデルを使用
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
        // 爆発エフェクトを一時的に生成
        emitterManager_->CreateTemporaryEmitterFrom(
            explodeEmitterName_,
            explodeEmitterName_ + "temp",
            0.5f);
        // エミッターを削除
        emitterManager_->RemoveEmitter(bulletEmitterName_);
        emitterManager_->RemoveEmitter(explodeEmitterName_);
    }
}