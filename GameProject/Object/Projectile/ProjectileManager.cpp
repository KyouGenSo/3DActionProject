#include "ProjectileManager.h"

#include <algorithm>
#include <cassert>

namespace {
    /// <summary>
    /// 弾コンテナを更新し、非アクティブ弾を Finalize してから削除する共通ヘルパ
    /// </summary>
    template <typename T>
    void UpdateAndCleanup(std::vector<std::unique_ptr<T>>& bullets, float dt)
    {
        // アクティブ弾のみ Update
        for (auto& b : bullets) {
            if (b && b->IsActive())
            {
                b->Update(dt);
            }
        }

        // 非アクティブ弾を除去（Finalize() で専用コライダーも自動解除される）
        std::erase_if(bullets,
            [](const std::unique_ptr<T>& b) {
                if (b && !b->IsActive())
                {
                    b->Finalize();
                    return true;
                }
                return false;
            });
    }

    /// <summary>
    /// SpawnRequest 群から弾を生成してベクタに追加する共通ヘルパ
    /// </summary>
    template <typename BulletT>
    void SpawnBullets(
        std::vector<std::unique_ptr<BulletT>>& dst,
        const std::vector<BulletSpawnRequest>& requests,
        Tako::EmitterManager* emitterManager)
    {
        for (const auto& req : requests) {
            auto bullet = std::make_unique<BulletT>(emitterManager);
            bullet->Initialize(req.position, req.velocity);
            dst.push_back(std::move(bullet));
        }
    }
} // namespace

ProjectileManager::ProjectileManager(Tako::EmitterManager* emitterManager)
    : emitterManager_(emitterManager)
{
}

ProjectileManager::~ProjectileManager()
{
    for (auto& b : playerBullets_) {
        assert(!b);
    }
    for (auto& b : bossBullets_) {
        assert(!b);
    }
    for (auto& b : penetratingBossBullets_) {
        assert(!b);
    }
}

void ProjectileManager::Update(float deltaTime)
{
    UpdateAndCleanup(playerBullets_,          deltaTime);
    UpdateAndCleanup(bossBullets_,            deltaTime);
    UpdateAndCleanup(penetratingBossBullets_, deltaTime);
}

void ProjectileManager::Clear()
{
    // 専用コライダー解除のため、すべて Finalize してからコンテナを空にする
    for (auto& b : playerBullets_)          { if (b) b->Finalize(); }
    for (auto& b : bossBullets_)            { if (b) b->Finalize(); }
    for (auto& b : penetratingBossBullets_) { if (b) b->Finalize(); }

    playerBullets_.clear();
    bossBullets_.clear();
    penetratingBossBullets_.clear();
}

void ProjectileManager::SpawnPlayerBullets(const std::vector<BulletSpawnRequest>& requests)
{
    // PlayerBullet はテンプレートを使わず特殊化：生成直後に ForceFieldManager を注入する。
    // ボス弾は force field の影響を受けない設計のため、共通テンプレートのまま注入しない。
    for (const auto& req : requests) {
        auto bullet = std::make_unique<PlayerBullet>(emitterManager_);
        bullet->SetForceFieldManager(forceFieldManager_);
        bullet->Initialize(req.position, req.velocity);
        playerBullets_.push_back(std::move(bullet));
    }
}

void ProjectileManager::SpawnBossBullets(const std::vector<BulletSpawnRequest>& requests)
{
    SpawnBullets(bossBullets_, requests, emitterManager_);
}

void ProjectileManager::SpawnPenetratingBossBullets(const std::vector<BulletSpawnRequest>& requests)
{
    SpawnBullets(penetratingBossBullets_, requests, emitterManager_);
}
