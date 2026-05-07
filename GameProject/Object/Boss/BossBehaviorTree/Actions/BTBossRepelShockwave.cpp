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

BTNodeStatus BTBossRepelShockwave::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return status_;
    }

    ForceFieldManager* ffm = boss->GetForceFieldManager();
    if (!ffm) {
        // ForceFieldManager 未注入：DI 配線漏れの兆候。Phase1 攻撃は実行不可。
        status_ = BTNodeStatus::Failure;
        return status_;
    }

    const float deltaTime = blackboard->GetDeltaTime();

    // 初回実行時の初期化
    if (isFirstExecute_) {
        elapsedTime_ = 0.0f;
        ringTriggered_ = false;

        // Reset 用にマネージャをキャッシュ（Reset 中は blackboard が無いため）
        cachedForceFieldManager_ = ffm;
        cachedEmitterManager_ = boss->GetEmitterManager();

        // ForceField 初期登録（強度 0 / 半径ほぼ 0 で確保）
        ForceFieldData field{};
        field.type = static_cast<uint32_t>(ForceFieldType::Repel);
        field.position = boss->GetTransform().translate;
        field.direction = { 0.0f, 0.0f, 0.0f };  // Repel では未使用
        field.strength = 0.0f;
        field.radius = 0.001f;
        field.falloff = falloff_;
        field.affectMask = affectMask_;
        forceFieldId_ = ffm->AddForceField(field);

        // 中心フラッシュ演出を起動（予兆フェーズで光る）
        if (cachedEmitterManager_) {
            cachedEmitterManager_->SetEmitterActive(flashEmitterName_, true);
            cachedEmitterManager_->SetEmitterPosition(flashEmitterName_, boss->GetTransform().translate);
        }

        // Boss 硬直フラグを ON。cachedBoss_ + enteredRecovery_ を保存しておくことで、
        // Reset 経由（BTParallel 中断含む）でも Cleanup 内で確実に解除できる。
        boss->EnterRecovery();
        cachedBoss_ = boss;
        enteredRecovery_ = true;
        isFirstExecute_ = false;
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
        // Phase 0: 予兆 — 弱い力場（プレイヤーへの予告として軽く感じさせる）
        field.strength = strength_ * 0.1f;
        field.radius = maxRadius_ * 0.2f;
    }
    else if (elapsedTime_ < expandEnd) {
        // Phase 1: 展開 — 半径を 0 → max へランプアップ
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
        // Phase 2: 持続 — 最大半径維持
        field.strength = strength_;
        field.radius = maxRadius_;
    }
    else {
        // Phase 3: 終了 — クリーンアップして Success
        // Boss::ExitRecovery は Cleanup 内で実施するためここでの個別呼び出しは不要。
        Cleanup();
        status_ = BTNodeStatus::Success;
        return status_;
    }

    // エミッター位置をボス追従させる（ボスが移動しても演出が中心ズレしない）
    if (cachedEmitterManager_) {
        cachedEmitterManager_->SetEmitterPosition(flashEmitterName_, boss->GetTransform().translate);
        if (ringTriggered_) {
            cachedEmitterManager_->SetEmitterPosition(ringEmitterName_, boss->GetTransform().translate);
        }
    }

    // ForceField を更新
    if (forceFieldId_ >= 0) {
        ffm->UpdateForceField(static_cast<uint32_t>(forceFieldId_), field);
    }

    status_ = BTNodeStatus::Running;
    return status_;
}

void BTBossRepelShockwave::Reset() {
    BTNode::Reset();
    // スタン等による中断時：力場とエミッターを必ず後始末してから状態をリセット
    Cleanup();
}

void BTBossRepelShockwave::Cleanup() {
    // Boss 硬直状態の解除（成功終了 / Reset 経由の両方で確実に実行）。
    // BTParallel が子ノードを中断する場合、Boss::ExitRecovery が呼ばれないと
    // 硬直フラグが残ってボスの行動全体が固まるリスクがあるため、ここに集約する。
    if (cachedBoss_ && enteredRecovery_) {
        cachedBoss_->ExitRecovery();
    }
    enteredRecovery_ = false;

    // ForceField 削除
    if (cachedForceFieldManager_ && forceFieldId_ >= 0) {
        cachedForceFieldManager_->RemoveForceField(static_cast<uint32_t>(forceFieldId_));
    }
    forceFieldId_ = -1;

    // エミッター停止
    if (cachedEmitterManager_) {
        cachedEmitterManager_->SetEmitterActive(ringEmitterName_, false);
        cachedEmitterManager_->SetEmitterActive(flashEmitterName_, false);
    }

    // 状態フラグ初期化
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    ringTriggered_ = false;
}

void BTBossRepelShockwave::ApplyParameters(const nlohmann::json& params) {
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

nlohmann::json BTBossRepelShockwave::ExtractParameters() const {
    return {
        {"warningTime",      warningTime_},
        {"expandTime",       expandTime_},
        {"sustainTime",      sustainTime_},
        {"maxRadius",        maxRadius_},
        {"strength",         strength_},
        {"falloff",          falloff_},
        {"affectMask",       affectMask_},
        {"ringEmitterName",  ringEmitterName_},
        {"flashEmitterName", flashEmitterName_},
    };
}

#ifdef _DEBUG
bool BTBossRepelShockwave::DrawImGui() {
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
