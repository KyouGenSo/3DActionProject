#include "AttackNode.h"
#include "../../Boss.h"
#include "EmitterManager.h"

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

AttackNode::AttackNode() {
    name_ = "AttackNode";  // 派生クラスのコンストラクタで上書きされる
}

BTNodeStatus AttackNode::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return status_;
    }

    if (isFirstExecute_) {
        elapsedTime_ = 0.0f;
        cachedBoss_ = boss;
        cachedEmitterManager_ = boss->GetEmitterManager();
        // isBypassRecoveryGuard ON 時は攻撃開始時から常時スタン可能化
        // （Recovery 状態と無関係にプレイヤー近接で Stunned へ遷移するようになる）
        if (isBypassRecoveryGuard_) {
            boss->SetForceVulnerable(true);
        }
        OnInitialize(blackboard, boss);
        isFirstExecute_ = false;
    }

    status_ = OnExecute(blackboard, boss, blackboard->GetDeltaTime());
    return status_;
}

void AttackNode::Reset() {
    BTNode::Reset();
    // BTParallel 等が中断する場合でも Boss のフラグを必ず解除する。
    if (cachedBoss_) {
        if (enteredRecovery_) {
            cachedBoss_->ExitRecovery();
        }
        if (isBypassRecoveryGuard_) {
            cachedBoss_->SetForceVulnerable(false);
        }
    }
    enteredRecovery_ = false;

    // 派生クラスの後始末
    OnCleanup();

    // 共通フラグ初期化
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    cachedBoss_ = nullptr;
    cachedEmitterManager_ = nullptr;
}

void AttackNode::EnterAttackRecovery(Boss* boss) {
    if (enteredRecovery_ || !boss) return;
    boss->EnterRecovery();
    enteredRecovery_ = true;
}

BTNodeStatus AttackNode::FinishAttack() {
    // Recovery / ForceVulnerable 解除
    if (cachedBoss_) {
        if (enteredRecovery_) {
            cachedBoss_->ExitRecovery();
        }
        if (isBypassRecoveryGuard_) {
            cachedBoss_->SetForceVulnerable(false);
        }
    }
    enteredRecovery_ = false;

    OnCleanup();

    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    cachedBoss_ = nullptr;
    cachedEmitterManager_ = nullptr;

    status_ = BTNodeStatus::Success;
    return status_;
}

void AttackNode::ApplyParameters(const nlohmann::json& params) {
    if (params.contains("isBypassRecoveryGuard")) {
        isBypassRecoveryGuard_ = params["isBypassRecoveryGuard"];
    }
    OnApplyParameters(params);
}

nlohmann::json AttackNode::ExtractParameters() const {
    nlohmann::json out = {
        {"isBypassRecoveryGuard", isBypassRecoveryGuard_},
    };
    OnExtractParameters(out);
    return out;
}

#ifdef _DEBUG
bool AttackNode::DrawImGui() {
    bool changed = false;
    if (ImGui::Checkbox("Bypass Recovery Guard##attackNode", &isBypassRecoveryGuard_)) {
        changed = true;
    }
    ImGui::TextDisabled("ON: Recovery 状態でなくても近接でスタン誘発");
    ImGui::Separator();
    if (OnDrawImGui()) changed = true;
    return changed;
}
#endif
