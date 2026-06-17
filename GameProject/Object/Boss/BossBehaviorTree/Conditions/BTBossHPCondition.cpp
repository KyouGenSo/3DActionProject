#include "BTBossHPCondition.h"
#include "../../Boss.h"

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

BTBossHPCondition::BTBossHPCondition() {
    name_ = "HPCondition";
}

Tako::BTNodeStatus BTBossHPCondition::Execute(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    if (!boss) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    float currentHp = boss->GetHp();
    float currentPercent = (currentHp / Boss::GetMaxHp()) * 100.0f;

    if (EvaluateCondition(currentPercent)) {
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    status_ = Tako::BTNodeStatus::Failure;
    return Tako::BTNodeStatus::Failure;
}

void BTBossHPCondition::Reset() {
    Tako::BTNode::Reset();
}

bool BTBossHPCondition::EvaluateCondition(float currentPercent) const {
    switch (comparison_) {
    case Comparison::Less:
        return currentPercent < thresholdPercent_;
    case Comparison::LessOrEqual:
        return currentPercent <= thresholdPercent_;
    case Comparison::Greater:
        return currentPercent > thresholdPercent_;
    case Comparison::GreaterOrEqual:
        return currentPercent >= thresholdPercent_;
    default:
        return false;
    }
}

void BTBossHPCondition::ApplyParameters(const nlohmann::json& params) {
    if (params.contains("thresholdPercent")) {
        thresholdPercent_ = params["thresholdPercent"].get<float>();
    }
    if (params.contains("comparison")) {
        comparison_ = static_cast<Comparison>(params["comparison"].get<int>());
    }
}

nlohmann::json BTBossHPCondition::ExtractParameters() const {
    return {
        {"thresholdPercent", thresholdPercent_},
        {"comparison", static_cast<int>(comparison_)}
    };
}

#ifdef _DEBUG
bool BTBossHPCondition::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("HP Threshold %##hp", &thresholdPercent_, 1.0f, 0.0f, 100.0f, "%.1f%%")) {
        changed = true;
    }

    int comp = static_cast<int>(comparison_);
    const char* compItems[] = { "Less (<)", "Less Or Equal (<=)", "Greater (>)", "Greater Or Equal (>=)" };
    if (ImGui::Combo("Comparison##hp", &comp, compItems, IM_ARRAYSIZE(compItems))) {
        comparison_ = static_cast<Comparison>(comp);
        changed = true;
    }

    return changed;
}
#endif
