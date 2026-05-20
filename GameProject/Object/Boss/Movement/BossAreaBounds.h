#pragma once
#include "Vector3.h"

class Boss;

/// <summary>
/// ボス移動関連の共通ユーティリティ
/// </summary>
namespace BossMovement {

    /// <summary>
    /// XZ 平面の活動可能エリア境界
    /// </summary>
    struct AreaBounds {
        float xMin;
        float xMax;
        float zMin;
        float zMax;
    };

    /// <summary>
    /// ステージ全域の境界 (kAreaMargin のみ適用、Phase 無視)
    /// プレイヤー追跡や攻撃突進など、Phase 2 で狭まる戦闘エリアに
    /// 縛られたくない用途で使用する。
    /// </summary>
    /// <returns>XZ の境界</returns>
    AreaBounds CalcStageBounds();

    /// <summary>
    /// ボスの現在 Phase に応じた活動可能領域を計算
    /// Phase 2 では kBossPhase2AreaSize 分だけ全方向に内側へ狭まる。
    /// </summary>
    /// <param name="boss">対象ボス (Phase を参照)</param>
    /// <returns>XZ の境界</returns>
    AreaBounds CalcAreaBounds(const Boss* boss);

    /// <summary>
    /// 指定境界で XZ 座標をクランプ (Y は維持)
    /// </summary>
    /// <param name="position">調整前の位置</param>
    /// <param name="bounds">クランプ対象境界</param>
    /// <returns>境界内の位置</returns>
    Tako::Vector3 ClampToBounds(const Tako::Vector3& position, const AreaBounds& bounds);

}
