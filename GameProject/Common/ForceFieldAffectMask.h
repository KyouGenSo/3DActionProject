#pragma once
#include <cstdint>

/// <summary>
/// このゲーム固有の ForceField affectMask 意味付け
/// </summary>
/// <remarks>
/// エンジンの ForceFieldData::affectMask は単なる汎用ビットフラグで意味を持たない。
/// このヘッダがゲームでのビット意味付けの単一情報源（single source of truth）。
///
/// 用途:
///   1. ForceField 登録時 ForceFieldData::affectMask に設定する値（影響対象を制限）
///   2. ForceFieldManager::EvaluateForceAt(pos, mask) のクエリ時に渡す値
///      （呼び出し側が「自分は何か」を mask で表明する）
///
/// 拡張時: 新しい影響対象（例: 敵キャラ、アイテム）を追加する場合はビットを追加。
/// </remarks>
namespace GameForceField {

    /// <summary>
    /// 影響対象を識別するビットフラグ
    /// </summary>
    enum AffectMask : uint32_t {
        /// <summary>パーティクル粒子のみに作用（CPU 駆動オブジェクトには作用しない）</summary>
        ParticlesOnly = 0u,
        /// <summary>プレイヤーの弾</summary>
        AffectBullets = 1u << 0,
        /// <summary>プレイヤーキャラクター本体</summary>
        AffectPlayer  = 1u << 1,
        /// <summary>ボス本体</summary>
        AffectBoss    = 1u << 2,
        /// <summary>全ての CPU 駆動対象</summary>
        AffectAll     = AffectBullets | AffectPlayer | AffectBoss,
    };

} // namespace GameForceField
