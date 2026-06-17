#pragma once
#include "Vector3.h"

/// <summary>
/// 弾1発分の生成パラメータ（発射位置と初速度）
/// </summary>
struct BulletSpawnRequest {
    Tako::Vector3 position;
    Tako::Vector3 velocity;
};
