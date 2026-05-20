#include "BossAreaBounds.h"
#include "../Boss.h"
#include "../../../Common/GameConst.h"

#include <algorithm>

namespace BossMovement {

    AreaBounds CalcStageBounds() {
        return {
            GameConst::kStageXMin + GameConst::kAreaMargin,
            GameConst::kStageXMax - GameConst::kAreaMargin,
            GameConst::kStageZMin + GameConst::kAreaMargin,
            GameConst::kStageZMax - GameConst::kAreaMargin,
        };
    }

    AreaBounds CalcAreaBounds(const Boss* boss) {
        AreaBounds bounds = CalcStageBounds();

        if (boss && boss->GetPhase() == 2) {
            bounds.xMin += GameConst::kBossPhase2AreaSize;
            bounds.xMax -= GameConst::kBossPhase2AreaSize;
            bounds.zMin += GameConst::kBossPhase2AreaSize;
            bounds.zMax -= GameConst::kBossPhase2AreaSize;
        }

        return bounds;
    }

    Tako::Vector3 ClampToBounds(const Tako::Vector3& position, const AreaBounds& bounds) {
        Tako::Vector3 result = position;
        result.x = std::clamp(result.x, bounds.xMin, bounds.xMax);
        result.z = std::clamp(result.z, bounds.zMin, bounds.zMax);
        result.y = position.y;
        return result;
    }

}
