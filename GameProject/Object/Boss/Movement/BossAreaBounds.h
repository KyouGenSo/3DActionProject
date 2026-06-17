#pragma once
#include "Vector3.h"

class Boss;

/// <summary>
/// ボス移動関連の共通ユーティリティ
/// </summary>
namespace BossMovement {

    struct AreaBounds {
        float xMin;
        float xMax;
        float zMin;
        float zMax;
    };

    /// <summary>
    /// ステージ全域の境界 (Phase 無視)。Phase 2 で狭まる戦闘エリアに縛られたくない用途で使う
    /// </summary>
    /// <returns>kAreaMargin 分だけ内側に縮めた XZ 境界</returns>
    AreaBounds CalcStageBounds();

    /// <summary>
    /// Phase 2 では kBossPhase2AreaSize 分だけ全方向に内側へ狭めた活動領域を返す
    /// </summary>
    /// <param name="boss">フェーズ判定に使う。nullptr または Phase 2 以外ならステージ全域と同じ境界を返す</param>
    /// <returns>現在のフェーズに応じた XZ 境界</returns>
    AreaBounds CalcAreaBounds(const Boss* boss);

    /// <summary>
    /// XZ をクランプ（Y は維持）
    /// </summary>
    /// <param name="position">クランプ対象の座標</param>
    /// <param name="bounds">XZ の許容範囲</param>
    /// <returns>XZ を bounds 内に収め、Y は入力のままの座標</returns>
    Tako::Vector3 ClampToBounds(const Tako::Vector3& position, const AreaBounds& bounds);

}
