#include "BTBossRepelShockwave.h"
#include "../../Boss.h"
#include "EmitterManager.h"
#include "ForceFieldManager.h"
#include "ParticleStruct.h"
#include <algorithm>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossRepelShockwave::BTBossRepelShockwave() {
    name_ = "BossRepelShockwave";
}

Tako::BTNodeStatus BTBossRepelShockwave::OnExecute(Tako::BTBlackboard* /*blackboard*/, Boss* boss, float deltaTime) {
    ForceFieldManager* ffm = boss->GetForceFieldManager();
    if (!ffm) {
        return Tako::BTNodeStatus::Failure;
    }

    elapsedTime_ += deltaTime;

    // フェーズ境界
    const float warningEnd = warningTime_;
    const float expandEnd = warningTime_ + expandTime_;
    const float sustainEnd = expandEnd + sustainTime_;

    // 力場の毎フレーム再構成（位置はボスに追従）
    ForceFieldData field{};
    field.type = static_cast<uint32_t>(ForceFieldType::Repel);
    field.position = boss->GetTransform().translate;
    field.direction = { 0.0f, 0.0f, 0.0f };
    field.falloff = falloff_;
    field.affectMask = affectMask_;

    if (elapsedTime_ < warningEnd) {
        // Phase 0: 予兆 — 弱い力場
        field.strength = strength_ * 0.1f;
        field.radius = maxRadius_ * 0.2f;
    }
    else if (elapsedTime_ < expandEnd) {
        // Phase 1: 展開 — 半径ランプアップ
        const float t = (elapsedTime_ - warningEnd) / std::max<float>(expandTime_, 0.0001f);
        field.strength = strength_;
        field.radius = maxRadius_ * std::clamp(t, 0.0f, 1.0f);

        // 衝撃波リング演出を一度だけ ON
        if (!ringTriggered_ && cachedEmitterManager_) {
            cachedEmitterManager_->SetEmitterActive(ringEmitterName_, true);
            cachedEmitterManager_->SetEmitterPosition(ringEmitterName_, boss->GetTransform().translate);
            ringTriggered_ = true;
        }
    }
    else if (elapsedTime_ < sustainEnd) {
        // Phase 2: 持続 — 最大半径維持 + Recovery 突入（プレイヤーがスタンを取れる隙）
        field.strength = strength_;
        field.radius = maxRadius_;
        EnterAttackRecovery(boss);
    }
    else {
        // Phase 3: 終了
        return FinishAttack();
    }

    // エミッター位置をボス追従
    if (cachedEmitterManager_) {
        cachedEmitterManager_->SetEmitterPosition(flashEmitterName_, boss->GetTransform().translate);
        if (ringTriggered_) {
            cachedEmitterManager_->SetEmitterPosition(ringEmitterName_, boss->GetTransform().translate);
        }
    }

    // ForceField 更新
    if (forceFieldId_ >= 0) {
        ffm->UpdateForceField(static_cast<uint32_t>(forceFieldId_), field);
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossRepelShockwave::OnInitialize(Tako::BTBlackboard* /*blackboard*/, Boss* boss) {
    ringTriggered_ = false;

    ForceFieldManager* ffm = boss->GetForceFieldManager();
    if (!ffm) return;

    cachedForceFieldManager_ = ffm;

    // ForceField 初期登録（強度 0 / 半径ほぼ 0 で確保）
    ForceFieldData field{};
    field.type = static_cast<uint32_t>(ForceFieldType::Repel);
    field.position = boss->GetTransform().translate;
    field.direction = { 0.0f, 0.0f, 0.0f };
    field.strength = 0.0f;
    field.radius = 0.001f;
    field.falloff = falloff_;
    field.affectMask = affectMask_;
    forceFieldId_ = ffm->AddForceField(field);

    // 中心フラッシュ演出を起動
    if (cachedEmitterManager_) {
        cachedEmitterManager_->SetEmitterActive(flashEmitterName_, true);
        cachedEmitterManager_->SetEmitterPosition(flashEmitterName_, boss->GetTransform().translate);
    }
}

void BTBossRepelShockwave::OnCleanup() {
    // ForceField 削除
    if (cachedForceFieldManager_ && forceFieldId_ >= 0) {
        cachedForceFieldManager_->RemoveForceField(static_cast<uint32_t>(forceFieldId_));
    }
    forceFieldId_ = -1;
    cachedForceFieldManager_ = nullptr;

    // エミッター停止
    if (cachedEmitterManager_) {
        cachedEmitterManager_->SetEmitterActive(ringEmitterName_, false);
        cachedEmitterManager_->SetEmitterActive(flashEmitterName_, false);
    }

    ringTriggered_ = false;
}

void BTBossRepelShockwave::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("warningTime"))  warningTime_ = params["warningTime"];
    if (params.contains("expandTime"))   expandTime_ = params["expandTime"];
    if (params.contains("sustainTime"))  sustainTime_ = params["sustainTime"];
    if (params.contains("maxRadius"))    maxRadius_ = params["maxRadius"];
    if (params.contains("strength"))     strength_ = params["strength"];
    if (params.contains("falloff"))      falloff_ = params["falloff"];
    if (params.contains("affectMask") && params["affectMask"].is_number_unsigned()) {
        affectMask_ = params["affectMask"].get<uint32_t>();
    }
    if (params.contains("ringEmitterName") && params["ringEmitterName"].is_string()) {
        ringEmitterName_ = params["ringEmitterName"].get<std::string>();
    }
    if (params.contains("flashEmitterName") && params["flashEmitterName"].is_string()) {
        flashEmitterName_ = params["flashEmitterName"].get<std::string>();
    }
}

void BTBossRepelShockwave::OnExtractParameters(nlohmann::json& out) const {
    out["warningTime"]      = warningTime_;
    out["expandTime"]       = expandTime_;
    out["sustainTime"]      = sustainTime_;
    out["maxRadius"]        = maxRadius_;
    out["strength"]         = strength_;
    out["falloff"]          = falloff_;
    out["affectMask"]       = affectMask_;
    out["ringEmitterName"]  = ringEmitterName_;
    out["flashEmitterName"] = flashEmitterName_;
}

#ifdef _DEBUG
bool BTBossRepelShockwave::OnDrawImGui() {
    bool changed = false;
    if (ImGui::DragFloat("Warning Time##repel",  &warningTime_,  0.05f, 0.0f, 3.0f))  changed = true;
    if (ImGui::DragFloat("Expand Time##repel",   &expandTime_,   0.05f, 0.0f, 3.0f))  changed = true;
    if (ImGui::DragFloat("Sustain Time##repel",  &sustainTime_,  0.05f, 0.0f, 3.0f))  changed = true;
    if (ImGui::DragFloat("Max Radius##repel",    &maxRadius_,    0.5f,  0.0f, 50.0f)) changed = true;
    if (ImGui::DragFloat("Strength##repel",      &strength_,     1.0f,  0.0f, 200.0f))changed = true;
    if (ImGui::DragFloat("Falloff##repel",       &falloff_,      0.05f, 0.0f, 5.0f))  changed = true;

    int maskInt = static_cast<int>(affectMask_);
    if (ImGui::CheckboxFlags("Affect Bullets##repel", &maskInt, GameForceField::AffectBullets)) changed = true;
    if (ImGui::CheckboxFlags("Affect Player##repel",  &maskInt, GameForceField::AffectPlayer))  changed = true;
    if (ImGui::CheckboxFlags("Affect Boss##repel",    &maskInt, GameForceField::AffectBoss))    changed = true;
    affectMask_ = static_cast<uint32_t>(maskInt);

    return changed;
}
#endif
