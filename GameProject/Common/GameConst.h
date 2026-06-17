#pragma once

/// <summary>
/// 固定値の定数。調整が必要なパラメータは GlobalVariables を使う
/// </summary>
namespace GameConst {

    // ステージ範囲
    inline constexpr float kStageXMin = -100.0f;
    inline constexpr float kStageXMax = 100.0f;
    inline constexpr float kStageZMin = -140.0f;
    inline constexpr float kStageZMax = 60.0f;

    /// <summary>
    /// これより短いベクトルは方向なしとみなす閾値
    /// </summary>
    inline constexpr float kDirectionEpsilon = 0.01f;

    /// <summary>
    /// ボスがステージ端に寄りすぎないためのエリア境界マージン
    /// </summary>
    inline constexpr float kAreaMargin = 10.0f;

    /// <summary>
    /// ボスフェーズ2の戦闘エリアの範囲
    /// </summary>
    inline constexpr float kBossPhase2AreaSize = 30.0f;
}
