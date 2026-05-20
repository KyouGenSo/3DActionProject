#include "BTBossRepelShockwave.h"
#include "../../Boss.h"
#include "ForceFieldManager.h"
#include "EmitterManager.h"
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

        // Phase 1 突入時にスフィア表示開始（一度だけ）
        if (!ringTriggered_) {
            boss->SetRepelShockwaveSphereVisible(true);
            ringTriggered_ = true;
        }
        // スフィアのサイズを ForceField の半径と同期
        boss->SetRepelShockwaveSphereScale(field.radius);
    }
    else if (elapsedTime_ < sustainEnd) {
        // Phase 2: 持続 — 最大半径維持 + Recovery 突入（プレイヤーがスタンを取れる隙）
        field.strength = strength_;
        field.radius = maxRadius_;
        boss->SetRepelShockwaveSphereScale(field.radius);
        EnterAttackRecovery(boss);
    }
    else {
        // Phase 3: 終了
        return FinishAttack();
    }

    // ForceField 更新
    if (forceFieldId_ >= 0) {
        ffm->UpdateForceField(static_cast<uint32_t>(forceFieldId_), field);
    }

    // ===== Phase 1 (expand) 以降: バリアパーティクル & 渦力場をボスに追従 =====
    // Phase 3 は上で早期 return 済みのため、ここに来た時点で Phase 0/1/2 のいずれか。
    // warningEnd を境に Phase 1+ のみ実行（Y は 0 固定、半径は field.radius と完全同期）。
    if (elapsedTime_ >= warningEnd) {
        const Vector3 bossPosFlat = {
            boss->GetTransform().translate.x,
            0.0f,
            boss->GetTransform().translate.z,
        };

        // Phase 1 突入時の一度きり活性化
        if (!barrierActivated_ && cachedEmitterManager_) {
            cachedEmitterManager_->SetEmitterActive(barrierEmitterInstance_, true);
            barrierActivated_ = true;
        }

        // 渦力場（ParticlesOnly）: 位置/半径を毎フレーム更新、その他は preset 値維持
        if (vortexFieldId_ >= 0) {
            ForceFieldData vf = vortexFieldBase_;
            vf.position = bossPosFlat;
            vf.radius = field.radius + 10.0f;
            ffm->UpdateForceField(static_cast<uint32_t>(vortexFieldId_), vf);
        }

        // バリアエミッタ: 位置/半径をバリアスフィアと同期
        if (cachedEmitterManager_) {
            cachedEmitterManager_->SetEmitterPosition(barrierEmitterInstance_, bossPosFlat);
            cachedEmitterManager_->SetEmitterRadius(barrierEmitterInstance_, field.radius);
        }
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossRepelShockwave::OnInitialize(Tako::BTBlackboard* /*blackboard*/, Boss* boss) {
    ringTriggered_ = false;
    barrierActivated_ = false;

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

    // ===== バリアパーティクルエミッタ初期化 =====
    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr) {
        cachedEmitterManager_ = emitterMgr;

        // 初回のみ preset をロード（インスタンス寿命を跨いで再利用）
        if (!barrierEmitterLoaded_) {
            emitterMgr->LoadPreset(barrierEmitterPreset_, barrierEmitterInstance_);
            barrierEmitterLoaded_ = true;
        }
        // OnInitialize 時点では非アクティブ。Phase 1 突入で起動する。
        emitterMgr->SetEmitterActive(barrierEmitterInstance_, false);
    }

    // ===== 渦力場（ParticlesOnly）初期登録 =====
    // preset 値（strength=1000 / direction=(0,-1,0) / affectMask=0 = ParticlesOnly）を読み込んで保持。
    // 既存 Repel 力場とは独立した別エントリとして AddForceField。
    if (ffm->LoadPresetToData(vortexFieldPreset_, vortexFieldBase_)) {
        ForceFieldData initial = vortexFieldBase_;
        initial.position = { boss->GetTransform().translate.x, 0.0f, boss->GetTransform().translate.z };
        initial.strength = 0.0f;
        initial.radius = 0.001f;
        vortexFieldId_ = ffm->AddForceField(initial);
    }
}

void BTBossRepelShockwave::OnCleanup() {
    // バリアエミッタ停止（preset は emitterMap_ に残し次回再利用）
    if (cachedEmitterManager_) {
        cachedEmitterManager_->SetEmitterActive(barrierEmitterInstance_, false);
    }
    cachedEmitterManager_ = nullptr;

    // ForceField 削除: erase ベースの実装に合わせて「後から登録した vortex を先に削除」
    if (cachedForceFieldManager_) {
        if (vortexFieldId_ >= 0) {
            cachedForceFieldManager_->RemoveForceField(static_cast<uint32_t>(vortexFieldId_));
        }
        if (forceFieldId_ >= 0) {
            cachedForceFieldManager_->RemoveForceField(static_cast<uint32_t>(forceFieldId_));
        }
    }
    vortexFieldId_ = -1;
    forceFieldId_ = -1;
    cachedForceFieldManager_ = nullptr;

    // 衝撃波スフィアを非表示化（cachedBoss_ は親 AttackNode で管理）
    if (cachedBoss_) {
        cachedBoss_->SetRepelShockwaveSphereVisible(false);
    }

    ringTriggered_ = false;
    barrierActivated_ = false;
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
}

void BTBossRepelShockwave::OnExtractParameters(nlohmann::json& out) const {
    out["warningTime"]      = warningTime_;
    out["expandTime"]       = expandTime_;
    out["sustainTime"]      = sustainTime_;
    out["maxRadius"]        = maxRadius_;
    out["strength"]         = strength_;
    out["falloff"]          = falloff_;
    out["affectMask"]       = affectMask_;
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
