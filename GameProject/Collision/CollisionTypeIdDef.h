#pragma once
#include <cstdint>

/// <summary>
/// 衝突カテゴリ識別用タイプ ID
/// </summary>
enum class CollisionTypeId : uint32_t {
    DEFAULT,
    PLAYER,
    PLAYER_ATTACK,
    PLAYER_PROJECTILE,
    BOSS,
    BOSS_ATTACK,
    BOSS_PROJECTILE,
    ENVIRONMENT,
};