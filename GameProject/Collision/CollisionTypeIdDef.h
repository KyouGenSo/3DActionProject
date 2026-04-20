#pragma once
#include <cstdint>

/// <summary>
/// 衝突判定タイプ ID 定義
/// オブジェクトのカテゴリを識別して衝突処理を制御
/// </summary>
enum class CollisionTypeId : uint32_t {
    DEFAULT,                /// デフォルトタイプ
    PLAYER,                 /// プレイヤー本体
    PLAYER_ATTACK,          /// プレイヤーの近接攻撃
    PLAYER_PROJECTILE,      /// プレイヤーの遠距離攻撃
    BOSS,                   /// 敵キャラクター
    BOSS_ATTACK,            /// 敵の攻撃
    BOSS_PROJECTILE,        /// 敵の遠距離攻撃
    ENVIRONMENT,            /// 環境オブジェクト
};