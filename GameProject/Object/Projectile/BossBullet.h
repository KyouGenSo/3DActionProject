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

class BossBulletCollider;

/// <summary>
/// ボスの弾クラス
/// </summary>
class BossBullet : public Projectile
{
    //=========================================================================================
    // 定数
    //=========================================================================================
    static constexpr uint32_t kIdResetThreshold = 10000; ///< ID リセット閾値
    static constexpr float kInitialScale = 0.0f; ///< 初期スケール

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BossBullet(Tako::EmitterManager* emittermanager);

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~BossBullet() override;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="position">初期位置</param>
    /// <param name="velocity">初期速度</param>
    void Initialize(const Tako::Vector3& position, const Tako::Vector3& velocity) override;

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// 更新
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過時間</param>
    void Update(float deltaTime) override;

    /// <summary>
    /// コリジョンタイプ ID を取得
    /// </summary>
    CollisionTypeId GetTypeId() const { return CollisionTypeId::BOSS_ATTACK; }

    /// <summary>
    /// コライダーを取得
    /// </summary>
    BossBulletCollider* GetCollider() const { return collider_.get(); }

private:
    // エフェクト用の回転速度
    Tako::Vector3 rotationSpeed_;

    // 専用コライダー
    std::unique_ptr<BossBulletCollider> collider_;

    // id
    static uint32_t id;

    // 追加エフェクトエミッタ (弾本体エミッタと並列に動かす上乗せ演出)
    std::string effectEmitterName_;

    // 調整可能パラメータ
    float rotationSpeedMin_ = -10.0f; ///< 回転速度の最小値
    float rotationSpeedMax_ = 10.0f; ///< 回転速度の最大値
    float yBoundaryMin_ = 0.0f; ///< Y 座標の下限
    float yBoundaryMax_ = 50.0f; ///< Y 座標の上限
};