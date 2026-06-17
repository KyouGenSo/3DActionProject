#pragma once

#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <string>

namespace Tako
{
    class Object3d;
    class Model;
    class EmitterManager;
}


/// <summary>
/// 弾の基底クラス。移動と生存時間のみ管理し、衝突判定は派生クラスで実装する。
/// </summary>
class Projectile {
public:
    Projectile();

    virtual ~Projectile();

    /// <summary>
    /// 弾を発射位置と速度で初期化し、アクティブ化する。
    /// </summary>
    /// <param name="position">発射位置（ワールド座標）</param>
    /// <param name="velocity">速度ベクトル（毎秒の移動量）</param>
    virtual void Initialize(const Tako::Vector3& position, const Tako::Vector3& velocity);

    virtual void Update(float deltaTime);

    virtual void Draw();

    bool IsActive() const { return isActive_; }

    void SetActive(bool active) { isActive_ = active; }

    float GetDamage() const { return damage_; }

    void SetDamage(float damage) { damage_ = damage; }

    const Tako::Vector3& GetVelocity() const { return velocity_; }

    void SetVelocity(const Tako::Vector3& velocity) { velocity_ = velocity; }

    const Tako::Transform& GetTransform() const { return transform_; }

    Tako::Transform* GetTransformPtr() { return &transform_; }

    Tako::Object3d* GetModel() const { return model_.get(); }

protected:
    void UpdateLifetime(float deltaTime);

    virtual void Move(float deltaTime);

    /// <summary>
    /// sphere.gltf を試し、なければ white_cube.gltf を使う。
    /// </summary>
    void SetDefaultModel();

    /// <summary>
    /// 弾本体のエミッターを有効化し、指定位置へ移動する。
    /// </summary>
    /// <param name="position">エミッターを配置するワールド座標</param>
    void ActivateBulletEmitter(const Tako::Vector3& position);

    /// <summary>
    /// 爆発エフェクトを一時生成し、弾と爆発のエミッターを削除する。
    /// </summary>
    void FinalizeEmitters();

protected:
    std::unique_ptr<Tako::Object3d> model_;

    Tako::Transform transform_{};

    bool isActive_ = false;

    Tako::Vector3 velocity_;

    float damage_ = 10.0f;

    float lifeTime_ = 5.0f;

    float elapsedTime_ = 0.0f;

    Tako::EmitterManager* emitterManager_ = nullptr;

    std::string bulletEmitterName_;

    std::string explodeEmitterName_;
};