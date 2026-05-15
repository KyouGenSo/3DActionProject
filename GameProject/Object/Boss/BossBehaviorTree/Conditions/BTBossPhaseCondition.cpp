#include "BTBossPhaseCondition.h"
#include "../../Boss.h"

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

BTBossPhaseCondition::BTBossPhaseCondition() {
    name_ = "PhaseCondition";
}

Tako::BTNodeStatus BTBossPhaseCondition::Execute(Tako::BTBlackboard* blackboard) {
    // ボスを取得
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    if (!boss) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    // 現在のフェーズを取得して比較
    uint32_t currentPhase = boss->GetPhase();

    if (EvaluateCondition(currentPhase)) {
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    status_ = Tako::BTNodeStatus::Failure;
    return Tako::BTNodeStatus::Failure;
}

void BTBossPhaseCondition::Reset() {
    Tako::BTNode::Reset();
}

bool BTBossPhaseCondition::EvaluateCondition(uint32_t currentPhase) const {
    switch (comparison_) {
    case Comparison::Equal:
        return currentPhase == targetPhase_;
    case Comparison::NotEqual:
        return currentPhase != targetPhase_;
    case Comparison::GreaterOrEqual:
        return currentPhase >= targetPhase_;
    case Comparison::LessOrEqual:
        return currentPhase <= targetPhase_;
    default:
        return false;
    }
}

void BTBossPhaseCondition::ApplyParameters(const nlohmann::json& params) {
    if (params.contains("targetPhase")) {
        targetPhase_ = params["targetPhase"].get<uint32_t>();
    }
    if (params.contains("comparison")) {
        comparison_ = static_cast<Comparison>(params["comparison"].get<int>());
    }
}

nlohmann::json BTBossPhaseCondition::ExtractParameters() const {
    return {
        {"targetPhase", targetPhase_},
        {"comparison", static_cast<int>(comparison_)}
    };
}

#ifdef _DEBUG
bool BTBossPhaseCondition::DrawImGui() {
    bool changed = false;

    // フェーズ値の編集
    int phase = static_cast<int>(targetPhase_);
    if (ImGui::DragInt("Target Phase##phase", &phase, 1, 1, 2)) {
        targetPhase_ = static_cast<uint32_t>(phase);
        changed = true;
    }

    // 比較タイプの選択
    int comp = static_cast<int>(comparison_);
    const char* compItems[] = { "Equal (==)", "Not Equal (!=)", "Greater Or Equal (>=)", "Less Or Equal (<=)" };
    if (ImGui::Combo("Comparison##phase", &comp, compItems, IM_ARRAYSIZE(compItems))) {
        comparison_ = static_cast<Comparison>(comp);
        changed = true;
    }

    return changed;
}
#endif
