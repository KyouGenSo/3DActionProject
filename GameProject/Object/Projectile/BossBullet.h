#pragma once

#include "Projectile.h"
#include "../../../GameProject/Collision/CollisionTypeIdDef.h"
#include <memory>
#include <string>

namespace Tako
{
    class EmitterManager;
    class ModelManager;
}

class BulletCollider;

/// <summary>
/// ボスの弾。プレイヤー弾と衝突すると相殺される。
/// </summary>
class BossBullet : public Projectile
{
private: //定数
    static constexpr uint32_t kIdResetThreshold = 10000;
    static constexpr float kInitialScale = 0.0f;

public: //メンバー関数
    BossBullet(Tako::EmitterManager* emittermanager);
    ~BossBullet() override;

    /// <summary>
    /// 弾を初期化し、エフェクトとプレイヤー相殺用コライダーを準備する。
    /// </summary>
    /// <param name="position">発射位置（ワールド座標）</param>
    /// <param name="velocity">速度ベクトル（毎秒の移動量）</param>
    void Initialize(const Tako::Vector3& position, const Tako::Vector3& velocity) override;

    void Finalize();
    void Update(float deltaTime) override;

    //=================================
    //Getter
    //=================================
    CollisionTypeId GetTypeId() const { return CollisionTypeId::BOSS_ATTACK; }
    BulletCollider* GetCollider() const { return collider_.get(); }

private: //メンバー変数
    Tako::Vector3                   rotationSpeed_;
    std::unique_ptr<BulletCollider> collider_;
    static uint32_t                 id;
    std::string                     effectEmitterName_;  ///< 弾本体に上乗せする追加エフェクト

    //調整可能パラメータ
    float rotationSpeedMin_ = -10.0f;
    float rotationSpeedMax_ = 10.0f;
    float yBoundaryMin_     = 0.0f;    ///< これを下回ると消滅
    float yBoundaryMax_     = 50.0f;   ///< これを上回ると消滅
};
