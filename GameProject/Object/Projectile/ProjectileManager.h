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
/// プロジェクタイル（弾）集約管理クラス
/// プレイヤー弾・ボス弾・貫通弾の生成・更新・削除を一手に引き受ける
/// </summary>
class ProjectileManager {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="emitterManager">エミッターマネージャー（非所有ポインタ）</param>
    explicit ProjectileManager(Tako::EmitterManager* emitterManager);

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~ProjectileManager();

    /// <summary>
    /// 全弾の更新と非アクティブ弾の削除
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// 全弾を Finalize してコンテナを空にする（シーン終了／リセット用）
    /// </summary>
    void Clear();

    /// <summary>
    /// プレイヤー弾の生成リクエストを処理
    /// </summary>
    void SpawnPlayerBullets(const std::vector<BulletSpawnRequest>& requests);

    /// <summary>
    /// ボス通常弾の生成リクエストを処理
    /// </summary>
    void SpawnBossBullets(const std::vector<BulletSpawnRequest>& requests);

    /// <summary>
    /// ボス貫通弾の生成リクエストを処理
    /// </summary>
    void SpawnPenetratingBossBullets(const std::vector<BulletSpawnRequest>& requests);

    /// <summary>
    /// ForceFieldManager を注入する（非所有）。
    /// 設定後に生成されるプレイヤー弾は力場の影響を受けるようになる。
    /// ボス弾は force field の影響を受けない設計のため、注入しない。
    /// </summary>
    void SetForceFieldManager(Tako::ForceFieldManager* manager) { forceFieldManager_ = manager; }

private:
    // エミッターマネージャー（非所有）
    Tako::EmitterManager* emitterManager_ = nullptr;

    // ForceFieldManager（非所有 / null 許容）— プレイヤー弾の生成時に注入
    Tako::ForceFieldManager* forceFieldManager_ = nullptr;

    // 弾コンテナ
    std::vector<std::unique_ptr<PlayerBullet>>           playerBullets_;
    std::vector<std::unique_ptr<BossBullet>>             bossBullets_;
    std::vector<std::unique_ptr<PenetratingBossBullet>>  penetratingBossBullets_;
};
