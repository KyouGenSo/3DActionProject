#pragma once
#include <cstdint>

#include "Collider.h"
#include "CollisionTypeIdDef.h"
#include "../Object/Player/Player.h"

/// <summary>
/// プレイヤーへのヒット解決。相手が PLAYER のとき、パリィ成功なら OnParrySuccess、否なら OnHit(damage)。
/// </summary>
/// <param name="other">衝突相手のコライダー</param>
/// <param name="damage">与えるダメージ量</param>
inline void ResolvePlayerHit(Tako::Collider* other, float damage) {
    if (!other) return;
    if (other->GetTypeID() != static_cast<uint32_t>(CollisionTypeId::PLAYER)) return;

    Player* player = static_cast<Player*>(other->GetOwner());
    if (player->IsParrying()) {
        player->OnParrySuccess();
        return;
    }
    player->OnHit(damage);
}
