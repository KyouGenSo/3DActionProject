#pragma once
#include <cstdint>

/// <summary>
/// ForceFieldData::affectMask のゲーム側ビット意味付け（単一情報源）。
/// エンジン側 affectMask は意味を持たない汎用ビットフラグ。
/// 用途: ForceField 登録値、および EvaluateForceAt(pos, mask) のクエリ値
///       （呼び出し側が自分の種別を mask で表明する）。
/// </summary>
namespace GameForceField {

    enum AffectMask : uint32_t {
        ParticlesOnly = 0u,        ///< 粒子のみ。CPU 駆動オブジェクトには作用しない
        AffectBullets = 1u << 0,
        AffectPlayer  = 1u << 1,
        AffectBoss    = 1u << 2,
        AffectAll     = AffectBullets | AffectPlayer | AffectBoss,
    };

} // namespace GameForceField
