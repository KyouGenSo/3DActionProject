#pragma once

#include "PlayerBullet.h"
#include "BossBullet.h"
#include "PenetratingBossBullet.h"
#include "../../Common/BulletSpawnRequest.h"

#include <memory>
#include <vector>

namespace Tako
{
    class EmitterManager;
    class ForceFieldManager;
}

/// <summary>
/// プレイヤー弾・ボス弾・貫通弾の生成・更新・削除を管理する。
/// </summary>
class ProjectileManager {
public:
    explicit ProjectileManager(Tako::EmitterManager* emitterManager);

    ~ProjectileManager();

    /// <summary>
    /// 全弾を更新し、非アクティブ弾を削除する。
    /// </summary>
    void Update(float deltaTime);

    /// <summary>
    /// 全弾を Finalize してコンテナを空にする（シーン終了／リセット用）。
    /// </summary>
    void Clear();

    /// <summary>
    /// リクエスト1件につきプレイヤー弾を1発生成する。生成時に力場マネージャを注入する。
    /// </summary>
    /// <param name="requests">弾ごとの発射位置と速度のリスト</param>
    void SpawnPlayerBullets(const std::vector<BulletSpawnRequest>& requests);

    /// <summary>
    /// リクエスト1件につきボス弾を1発生成する。
    /// </summary>
    /// <param name="requests">弾ごとの発射位置と速度のリスト</param>
    void SpawnBossBullets(const std::vector<BulletSpawnRequest>& requests);

    /// <summary>
    /// リクエスト1件につき貫通ボス弾を1発生成する。
    /// </summary>
    /// <param name="requests">弾ごとの発射位置と速度のリスト</param>
    void SpawnPenetratingBossBullets(const std::vector<BulletSpawnRequest>& requests);

    /// <summary>
    /// 非所有。設定後に生成されるプレイヤー弾のみ力場の影響を受ける。ボス弾は対象外。
    /// </summary>
    /// <param name="manager">プレイヤー弾へ注入する力場マネージャ。非所有。nullptr 可</param>
    void SetForceFieldManager(Tako::ForceFieldManager* manager) { forceFieldManager_ = manager; }

private:
    Tako::EmitterManager* emitterManager_ = nullptr;

    Tako::ForceFieldManager* forceFieldManager_ = nullptr; ///< 非所有 / null 許容

    std::vector<std::unique_ptr<PlayerBullet>>           playerBullets_;
    std::vector<std::unique_ptr<BossBullet>>             bossBullets_;
    std::vector<std::unique_ptr<PenetratingBossBullet>>  penetratingBossBullets_;
};
