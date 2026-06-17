#pragma once

#include "Projectile.h"
#include "../../../GameProject/Collision/CollisionTypeIdDef.h"
#include <memory>
#include <string>

namespace  Tako
{
    class EmitterManager;
    class ForceFieldManager;
}

class BulletCollider;

/// <summary>
/// プレイヤーの弾。
/// </summary>
class PlayerBullet : public Projectile {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr uint32_t kIdResetThreshold = 10000;
    static constexpr float kInitialScale = 0.0f;    ///< パーティクル描画のため0
    static constexpr float kMinSpeedSquared = 1.0f; ///< 速度2乗がこれ未満で消滅。Repel攻撃で減速したケース対策

public:
    PlayerBullet(Tako::EmitterManager* emitterManager);

    ~PlayerBullet() override;

    /// <summary>
    /// 弾を初期化し、エフェクトとボス攻撃用コライダーを準備する。
    /// </summary>
    /// <param name="position">発射位置（ワールド座標）</param>
    /// <param name="velocity">速度ベクトル（毎秒の移動量）</param>
    void Initialize(const Tako::Vector3& position, const Tako::Vector3& velocity) override;

    void Finalize();

    void Update(float deltaTime) override;

    CollisionTypeId GetTypeId() const { return CollisionTypeId::PLAYER_ATTACK; }

    BulletCollider* GetCollider() const { return collider_.get(); }

    /// <summary>
    /// 非所有。設定すると毎フレーム ForceField の力を速度へ加算する。nullptr なら直進。
    /// </summary>
    /// <param name="manager">参照する力場マネージャ。非所有。nullptr 可</param>
    void SetForceFieldManager(Tako::ForceFieldManager* manager) { forceFieldManager_ = manager; }

private:
    std::unique_ptr<BulletCollider> collider_;

    static uint32_t id;

    Tako::ForceFieldManager* forceFieldManager_ = nullptr; ///< 非所有 / null 許容

    std::string effectEmitterName_; ///< 弾本体に上乗せする追加エフェクト

    // 調整可能パラメータ
    float yBoundaryMin_ = -10.0f; ///< これを下回ると消滅
    float yBoundaryMax_ = 50.0f;  ///< これを上回ると消滅
};
