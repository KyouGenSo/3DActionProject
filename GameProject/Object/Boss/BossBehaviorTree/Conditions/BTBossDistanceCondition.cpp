#include "BTBossDistanceCondition.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "Vector3.h"

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossDistanceCondition::BTBossDistanceCondition() {
    name_ = "DistanceCondition";
}

Tako::BTNodeStatus BTBossDistanceCondition::Execute(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Player* player = blackboard->GetPtr<Player>("player");

    if (!boss || !player) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 playerPos = player->GetTransform().translate;

    // 水平距離（Y 軸を無視）
    Vector3 diff = playerPos - bossPos;
    diff.y = 0.0f;
    float distance = diff.Length();

    if (distance >= minDistance_ && distance <= maxDistance_) {
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    status_ = Tako::BTNodeStatus::Failure;
    return Tako::BTNodeStatus::Failure;
}

void BTBossDistanceCondition::Reset() {
    Tako::BTNode::Reset();
}

void BTBossDistanceCondition::ApplyParameters(const nlohmann::json& params) {
    if (params.contains("minDistance")) {
        minDistance_ = params["minDistance"].get<float>();
    }
    if (params.contains("maxDistance")) {
        maxDistance_ = params["maxDistance"].get<float>();
    }
}

nlohmann::json BTBossDistanceCondition::ExtractParameters() const {
    return {
        {"minDistance", minDistance_},
        {"maxDistance", maxDistance_}
    };
}

#ifdef _DEBUG
bool BTBossDistanceCondition::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Min Distance##dist", &minDistance_, 0.5f, 0.0f, 100.0f, "%.1f")) {
        // min が max を超えないように
        if (minDistance_ > maxDistance_) {
            minDistance_ = maxDistance_;
        }
        changed = true;
    }

    if (ImGui::DragFloat("Max Distance##dist", &maxDistance_, 0.5f, 0.0f, 100.0f, "%.1f")) {
        // max が min を下回らないように
        if (maxDistance_ < minDistance_) {
            maxDistance_ = minDistance_;
        }
        changed = true;
    }

    ImGui::TextDisabled("Success if: %.1f <= distance <= %.1f", minDistance_, maxDistance_);

    return changed;
}
#endif
