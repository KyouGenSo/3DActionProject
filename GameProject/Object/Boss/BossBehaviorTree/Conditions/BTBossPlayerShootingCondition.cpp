#include "BTBossPlayerShootingCondition.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "Vector3.h"

#include <cmath>
#include <numbers>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossPlayerShootingCondition::BTBossPlayerShootingCondition() {
    name_ = "PlayerShootingCondition";
}

Tako::BTNodeStatus BTBossPlayerShootingCondition::Execute(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Player* player = blackboard->GetPtr<Player>("player");

    if (!boss || !player || !player->IsShooting()) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    // XZ 平面上で照準方向とボスへの方向を比較（Y 軸を無視）
    Vector3 toBoss = boss->GetTransform().translate - player->GetTransform().translate;
    toBoss.y = 0.0f;
    Vector3 aim = player->GetAimDirection();
    aim.y = 0.0f;

    if (toBoss.LengthSquared() <= 0.0001f || aim.LengthSquared() <= 0.0001f) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    float cosThreshold = std::cos(angleThreshold_ * std::numbers::pi_v<float> / 180.0f);
    if (aim.Normalize().Dot(toBoss.Normalize()) >= cosThreshold) {
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    status_ = Tako::BTNodeStatus::Failure;
    return Tako::BTNodeStatus::Failure;
}

void BTBossPlayerShootingCondition::Reset() {
    Tako::BTNode::Reset();
}

void BTBossPlayerShootingCondition::ApplyParameters(const nlohmann::json& params) {
    if (params.contains("angleThreshold")) {
        angleThreshold_ = params["angleThreshold"].get<float>();
    }
}

nlohmann::json BTBossPlayerShootingCondition::ExtractParameters() const {
    return {
        {"angleThreshold", angleThreshold_}
    };
}

#ifdef _DEBUG
bool BTBossPlayerShootingCondition::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Angle Threshold##shoot", &angleThreshold_, 1.0f, 0.0f, 180.0f, "%.1f deg")) {
        changed = true;
    }

    ImGui::TextDisabled("Success if: shooting && aim angle <= %.1f deg", angleThreshold_);

    return changed;
}
#endif
