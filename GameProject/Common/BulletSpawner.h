#pragma once
#include <vector>
#include "Vector3.h"
#include "BulletSpawnRequest.h"

/// <summary>
/// 弾生成リクエストを蓄積し、GameScene 側でまとめて消費するためのキュー
/// </summary>
class BulletSpawner
{
public: //メンバー関数
    BulletSpawner() = default;
    ~BulletSpawner() = default;

    /// <summary>
    /// 弾生成リクエストをキューに追加する
    /// </summary>
    /// <param name="position">生成位置（ワールド座標）</param>
    /// <param name="velocity">初速度ベクトル</param>
    void RequestSpawn(const Tako::Vector3& position, const Tako::Vector3& velocity);

    /// <summary>
    /// 保留中リクエストを move で返し、内部を空にする
    /// </summary>
    /// <returns>蓄積されていた全リクエスト</returns>
    std::vector<BulletSpawnRequest> Consume();

    void Clear() { pendingBullets_.clear(); }

    //============================
    //Getter
    //============================
    bool HasPending() const { return !pendingBullets_.empty(); }
    size_t GetPendingCount() const { return pendingBullets_.size(); }

private: //メンバー変数
    std::vector<BulletSpawnRequest> pendingBullets_;
};
