#pragma once
#include "Vector3.h"
#include <algorithm>

/// <summary>
/// XZ 平面上の移動可能範囲。マーカー値で無効状態を表す
/// </summary>
struct DynamicBoundary
{
    /// <summary>
    /// 未設定（無効）を表す番兵値
    /// </summary>
    static constexpr float kDisabledMarker = 9999.0f;

    float xMin = -kDisabledMarker;
    float xMax = kDisabledMarker;
    float zMin = -kDisabledMarker;
    float zMax = kDisabledMarker;

    /// <summary>
    /// XZ 各軸の最小・最大値を直接指定して境界を設定
    /// </summary>
    /// <param name="minX">X 軸の最小値</param>
    /// <param name="maxX">X 軸の最大値</param>
    /// <param name="minZ">Z 軸の最小値</param>
    /// <param name="maxZ">Z 軸の最大値</param>
    void Set(float minX, float maxX, float minZ, float maxZ)
    {
        xMin = minX;
        xMax = maxX;
        zMin = minZ;
        zMax = maxZ;
    }

    /// <summary>
    /// 中心と片側範囲(xRange/zRange)から境界を設定
    /// </summary>
    /// <param name="center">境界の中心位置（Y は未使用）</param>
    /// <param name="xRange">中心から X 方向への片側幅</param>
    /// <param name="zRange">中心から Z 方向への片側幅</param>
    void SetFromCenter(const Tako::Vector3& center, float xRange, float zRange)
    {
        xMin = center.x - xRange;
        xMax = center.x + xRange;
        zMin = center.z - zRange;
        zMax = center.z + zRange;
    }

    void Clear()
    {
        xMin = -kDisabledMarker;
        xMax = kDisabledMarker;
        zMin = -kDisabledMarker;
        zMax = kDisabledMarker;
    }

    /// <summary>
    /// いずれかの値がマーカー値でなければ有効
    /// </summary>
    bool IsEnabled() const
    {
        return xMin > -kDisabledMarker || xMax < kDisabledMarker ||
            zMin > -kDisabledMarker || zMax < kDisabledMarker;
    }

    /// <summary>
    /// 位置を境界内に収める（X・Z のみclamp、Y は元の値を維持）
    /// </summary>
    /// <param name="position">対象位置</param>
    /// <returns>境界内に収めた位置</returns>
    Tako::Vector3 Clamp(const Tako::Vector3& position) const
    {
        Tako::Vector3 result = position;
        result.x = std::clamp(result.x, xMin, xMax);
        result.z = std::clamp(result.z, zMin, zMax);
        return result;
    }

    /// <summary>
    /// 位置が境界内（XZ）にあるかを判定
    /// </summary>
    /// <param name="position">対象位置（Y は未使用）</param>
    /// <returns>境界内なら true</returns>
    bool Contains(const Tako::Vector3& position) const
    {
        return position.x >= xMin && position.x <= xMax &&
            position.z >= zMin && position.z <= zMax;
    }
};
