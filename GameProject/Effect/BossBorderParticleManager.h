#pragma once
#include "Vector3.h"

namespace Tako {
    class EmitterManager;
}

/// <summary>
/// 戦闘エリア境界線を表示するエミッター4個を制御（フェーズ2で表示）
/// </summary>
class BossBorderParticleManager
{
public:
    explicit BossBorderParticleManager(Tako::EmitterManager* emitterManager, float areaSize);

    ~BossBorderParticleManager() = default;

    /// <summary>
    /// フェーズ2のときだけ境界線を有効化し、ボス周囲に追従させる
    /// </summary>
    /// <param name="bossPhase">ボスの現在フェーズ。2 のとき境界線を表示</param>
    /// <param name="bossPosition">追従先のボス位置（XZ のみ使用、Y は0固定）</param>
    void Update(int bossPhase, const Tako::Vector3& bossPosition);

    /// <summary>
    /// 4方向の境界線エミッターを一括で表示/非表示する
    /// </summary>
    /// <param name="active">true で表示、false で非表示</param>
    void SetActive(bool active);

    bool IsActive() const { return isActive_; }

    void SetAreaSize(float areaSize) { areaSize_ = areaSize; }

    float GetAreaSize() const { return areaSize_; }

private:
    /// <summary>
    /// 4方向のエミッターをボスの XZ から areaSize_ だけ離した位置に配置する
    /// </summary>
    /// <param name="bossPosition">基準となるボス位置（Y は0に固定して使用）</param>
    void UpdatePositions(const Tako::Vector3& bossPosition);

private:
    Tako::EmitterManager* emitterManager_ = nullptr;
    bool isActive_ = false;
    float areaSize_ = 0.0f;
};
