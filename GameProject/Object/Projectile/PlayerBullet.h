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

class PlayerBulletCollider;

/// <summary>
/// プレイヤーの弾クラス
/// BossBullet と対称の設計パターン
/// </summary>
class PlayerBullet : public Projectile {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr uint32_t kIdResetThreshold = 10000; ///< ID リセット閾値
    static constexpr float kInitialScale = 0.0f;         ///< 初期スケール（パーティクル描画のため0）
    static constexpr float kMinSpeedSquared = 1.0f;      ///< 速度低下消滅閾値の二乗（1 m/s 以下で消滅・Repel攻撃対策）

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="emitterManager">エミッターマネージャー</param>
    PlayerBullet(Tako::EmitterManager* emitterManager);

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~PlayerBullet() override;

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
    CollisionTypeId GetTypeId() const { return CollisionTypeId::PLAYER_ATTACK; }

    /// <summary>
    /// コライダーを取得
    /// </summary>
    PlayerBulletCollider* GetCollider() const { return collider_.get(); }

    /// <summary>
    /// ForceFieldManager を注入する（非所有）。
    /// 設定されていれば毎フレーム ForceField の合計力を速度に加算し、
    /// パーティクル粒子と同じ力場効果を CPU 駆動の弾にも反映する。
    /// nullptr のままなら従来通り直進する。
    /// </summary>
    void SetForceFieldManager(Tako::ForceFieldManager* manager) { forceFieldManager_ = manager; }

private:
    // 専用コライダー
    std::unique_ptr<PlayerBulletCollider> collider_;

    // id（複数弾の識別用）
    static uint32_t id;

    // ForceFieldManager（非所有 / null 許容）— ForceField の力を弾の挙動に反映するためのクエリ先
    Tako::ForceFieldManager* forceFieldManager_ = nullptr;

    // 追加エフェクトエミッタ (弾本体エミッタと並列に動かす上乗せ演出)
    std::string effectEmitterName_;

    // 調整可能パラメータ
    float yBoundaryMin_ = -10.0f;  ///< Y 座標の下限
    float yBoundaryMax_ = 50.0f;   ///< Y 座標の上限
};
