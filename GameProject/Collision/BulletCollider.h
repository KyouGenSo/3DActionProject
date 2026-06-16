#pragma once

#include "SphereCollider.h"
#include <cstdint>
#include <functional>
#include <unordered_set>

class Projectile;

/// <summary>
/// 弾共通コライダー。対象 typeID へのヒット処理をハンドラに委譲し、
/// 打ち消し typeID（あれば）との衝突で所有弾を非アクティブ化する。
/// </summary>
class BulletCollider : public Tako::SphereCollider {
public:
    /// 打ち消し判定を行わない場合に cancelTypeId へ渡す値（貫通弾用）
    static constexpr uint32_t kNoCancelType = 0xFFFFFFFFu;

    /// <param name="owner">所有弾</param>
    /// <param name="targetTypeId">ダメージ対象の typeID</param>
    /// <param name="cancelTypeId">衝突で弾を消す typeID（無い場合は kNoCancelType）</param>
    BulletCollider(Projectile* owner, uint32_t targetTypeId, uint32_t cancelTypeId);
    ~BulletCollider() override = default;

    /// <summary>
    /// 対象ヒット時の処理（パリィ / ダメージ等）を設定。引数は衝突相手。
    /// </summary>
    void SetHitHandler(std::function<void(Tako::Collider* target)> handler) { hitHandler_ = std::move(handler); }

    void OnCollisionEnter(Tako::Collider* other) override;
    void OnCollisionStay(Tako::Collider* other) override {}
    void OnCollisionExit(Tako::Collider* other) override {}

    /// <summary>
    /// 多重ヒット履歴をクリア
    /// </summary>
    void Reset() { hitTargets_.clear(); }

private:
    Projectile* owner_ = nullptr;
    uint32_t targetTypeId_ = 0;
    uint32_t cancelTypeId_ = kNoCancelType;
    std::function<void(Tako::Collider*)> hitHandler_;
    std::unordered_set<void*> hitTargets_;
};
