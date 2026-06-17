#pragma once

#include "SphereCollider.h"
#include <cstdint>
#include <functional>
#include <unordered_set>

class Projectile;

/// <summary>
/// 弾共通コライダー。対象ヒットはハンドラへ委譲し、打ち消し typeID との衝突で所有弾を非アクティブ化
/// </summary>
class BulletCollider : public Tako::SphereCollider {
public: //定数
    /// 打ち消し判定なし（貫通弾用）を表す cancelTypeId のセンチネル値
    static constexpr uint32_t kNoCancelType = 0xFFFFFFFFu;

public: //メンバー関数
    /// <summary>
    /// 所有弾・命中対象 typeID・打ち消し typeID を指定して生成
    /// </summary>
    /// <param name="owner">この弾を所有する Projectile</param>
    /// <param name="targetTypeId">ヒットハンドラを呼ぶ命中対象の typeID</param>
    /// <param name="cancelTypeId">衝突で弾を消す typeID（無い場合は kNoCancelType）</param>
    BulletCollider(Projectile* owner, uint32_t targetTypeId, uint32_t cancelTypeId);
    ~BulletCollider() override = default;

    /// <summary>
    /// 対象 typeID なら多重ヒットを除外しハンドラ呼び出し、打ち消し typeID なら所有弾を非アクティブ化
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnCollisionEnter(Tako::Collider* other) override;
    void OnCollisionStay(Tako::Collider* other) override {}
    void OnCollisionExit(Tako::Collider* other) override {}

    /// <summary>
    /// 多重ヒット履歴をクリア
    /// </summary>
    void Reset() { hitTargets_.clear(); }

    //=====================================================================
    //Setter
    //=====================================================================
    /// <summary>
    /// 対象ヒット時の処理（パリィ / ダメージ等）を設定
    /// </summary>
    void SetHitHandler(std::function<void(Tako::Collider* target)> handler) { hitHandler_ = std::move(handler); }

private: //メンバー変数
    Projectile*                          owner_        = nullptr;
    uint32_t                             targetTypeId_ = 0;
    uint32_t                             cancelTypeId_ = kNoCancelType;
    std::function<void(Tako::Collider*)> hitHandler_;
    std::unordered_set<void*>            hitTargets_;
};
