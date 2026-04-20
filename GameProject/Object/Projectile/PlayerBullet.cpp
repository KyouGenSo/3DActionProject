#include "PlayerBullet.h"
#include "../../Collision/PlayerBulletCollider.h"
#include "../../Common/GameConst.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "CollisionManager.h"
#include "EmitterManager.h"
#include "GlobalVariables.h"
#include <format>

using namespace Tako;

uint32_t PlayerBullet::id = 0;

PlayerBullet::PlayerBullet(EmitterManager* emitterManager) {
    // GlobalVariables から値を取得
    GlobalVariables* gv = GlobalVariables::GetInstance();

    // 弾のパラメータ設定（デフォルト値を使用、GlobalVariables に未登録の場合に備える）
    damage_ = gv->GetValueFloat("PlayerBullet", "Damage");
    lifeTime_ = gv->GetValueFloat("PlayerBullet", "Lifetime");

    // デフォルト値の設定（GlobalVariables に登録されていない場合）
    if (damage_ <= 0.0f) {
        damage_ = 10.0f;
    }
    if (lifeTime_ <= 0.0f) {
        lifeTime_ = 3.0f;
    }

    // エミッターマネージャーの設定
    emitterManager_ = emitterManager;

    // エフェクトプリセットをロード
    if (emitterManager_) {
        bulletEmitterName_ = std::format("player_bullet{}", id);
        explodeEmitterName_ = std::format("player_bullet_explode{}", id);
        emitterManager_->LoadPreset("player_bullet", bulletEmitterName_);
        emitterManager_->SetEmitterActive(bulletEmitterName_, false);
        emitterManager_->LoadPreset("player_bullet_explode", explodeEmitterName_);
        emitterManager_->SetEmitterActive(explodeEmitterName_, false);
    }

    id++;

    if (id > kIdResetThreshold) {
        id = 0; // ID のリセット
    }
}

PlayerBullet::~PlayerBullet() = default;

void PlayerBullet::Initialize(const Vector3& position, const Vector3& velocity) {
    // 親クラスの初期化
    Projectile::Initialize(position, velocity);

    // モデルをロード
    Projectile::SetDefaultModel();
    model_->Update();

    // スケールを設定（パーティクル描画のため0に設定）
    transform_.scale = Vector3(kInitialScale, kInitialScale, kInitialScale);

    // エミッターを有効化
    Projectile::ActivateBulletEmitter(position);

    // コライダーの設定
    if (!collider_) {
        collider_ = std::make_unique<PlayerBulletCollider>(this);
    }

    GlobalVariables* gv = GlobalVariables::GetInstance();
    float colliderRadius = gv->GetValueFloat("PlayerBullet", "ColliderRadius");
    if (colliderRadius <= 0.0f) {
        colliderRadius = 0.5f; // デフォルト値
    }

    collider_->SetTransform(&transform_);
    collider_->SetRadius(colliderRadius);
    collider_->SetOffset(Vector3(0.0f, 0.0f, 0.0f));
    collider_->SetTypeID(static_cast<uint32_t>(CollisionTypeId::PLAYER_PROJECTILE));
    collider_->SetOwner(this);
    collider_->SetActive(true);
    collider_->Reset();

    // CollisionManager に登録
    CollisionManager::GetInstance()->AddCollider(collider_.get());
}

void PlayerBullet::Finalize() {
    // CollisionManager から削除
    if (collider_) {
        CollisionManager::GetInstance()->RemoveCollider(collider_.get());
    }

    // エミッター終了処理
    Projectile::FinalizeEmitters();
}

void PlayerBullet::Update(float deltaTime) {
    if (!isActive_) {
        return;
    }

    // 親クラスの更新処理
    Projectile::Update(deltaTime);

    // 軌跡エフェクト位置を同期
    if (emitterManager_) {
        emitterManager_->SetEmitterPosition(bulletEmitterName_, transform_.translate);
        emitterManager_->SetEmitterPosition(explodeEmitterName_, transform_.translate);
    }

    // エリア外に出たら非アクティブ化
    Vector3 pos = transform_.translate;
    if (pos.x < GameConst::kStageXMin || pos.x > GameConst::kStageXMax ||
        pos.z < GameConst::kStageZMin || pos.z > GameConst::kStageZMax ||
        pos.y < yBoundaryMin_ || pos.y > yBoundaryMax_) {
        isActive_ = false;
    }
}

