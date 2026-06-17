#include "BTBossSlashAttack.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"

#include "CollisionManager.h"
#include "EmitterManager.h"

#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossSlashAttack::BTBossSlashAttack() {
    name_ = "BossSlashAttack";
}

BTBossSlashAttack::~BTBossSlashAttack() {
    OnCleanup();
}

Tako::BTNodeStatus BTBossSlashAttack::OnExecute(Tako::BTBlackboard* /*blackboard*/, Boss* boss, float deltaTime) {
    const float warningEnd = warningDuration_;
    const float blinkEnd = warningEnd + blinkDuration_;
    const float attackEnd = blinkEnd + attackDuration_;

    if (elapsedTime_ < warningEnd) {
        // Warning: Decal を固定アルファで表示（OnInitialize で設定済み）
    }
    else if (elapsedTime_ < blinkEnd) {
        UpdateBlinkingPhase(elapsedTime_ - warningEnd);
    }
    else if (elapsedTime_ < attackEnd) {
        if (!hasBegunAttack_) {
            BeginAttackPhase(boss);
            hasBegunAttack_ = true;
        }
    }
    else {
        if (!hasEndedAttack_) {
            EndAttackPhase(boss);
            hasEndedAttack_ = true;
        }
        // 多重呼び出しは Helper がガード
        EnterAttackRecovery(boss);
    }

    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= totalDuration_) {
        hasBegunAttack_ = false;
        hasEndedAttack_ = false;
        return FinishAttack();
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossSlashAttack::OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) {
    hasBegunAttack_ = false;
    hasEndedAttack_ = false;

    totalDuration_ = warningDuration_ + blinkDuration_ + attackDuration_ + recoveryTime_;

    // 発動時のプレイヤー位置を着弾点として固定
    Vector3 targetPos = blackboard->GetPtr<Player>("player")->GetTransform().translate;

    float diameter = attackRadius_ * 2.0f;
    slashDecal_ = std::make_unique<Decal>();
    slashDecal_->Initialize();
    slashDecal_->SetShape(DecalShape::Circle);
    slashDecal_->SetTranslate(Vector3(targetPos.x, 0.0f, targetPos.z));
    slashDecal_->SetScale(Vector3(diameter, 1.0f, diameter));
    slashDecal_->SetEdgeSoftness(0.02f);
    slashDecal_->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
    slashDecal_->SetVisible(true);

    colliderTransform_.translate = targetPos;
    colliderTransform_.rotate = Vector3(0.0f, 0.0f, 0.0f);
    colliderTransform_.scale = Vector3(1.0f, 1.0f, 1.0f);

    slashCollider_ = std::make_unique<MeteorImpactCollider>(boss);
    slashCollider_->SetTransform(&colliderTransform_);
    slashCollider_->SetRadius(attackRadius_);
    slashCollider_->SetDamage(damage_);
    slashCollider_->SetOwner(boss);
    slashCollider_->SetActive(false);
    CollisionManager::GetInstance()->AddCollider(slashCollider_.get());

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && !particleInitialized_) {
        emitterName_ = "slash_attack_0";
        emitterMgr->LoadPreset("sphere_attack_slash", emitterName_);
        emitterMgr->SetEmitterActive(emitterName_, false);
        particleInitialized_ = true;
    }
}

void BTBossSlashAttack::UpdateBlinkingPhase(float phaseElapsed) {
    float sinValue = std::abs(std::sin(phaseElapsed * blinkFrequency_ * 3.14159265f));
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;
    if (slashDecal_) {
        slashDecal_->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
    }
}

void BTBossSlashAttack::BeginAttackPhase(Boss* boss) {
    if (slashDecal_)    slashDecal_->SetVisible(false);
    if (slashCollider_) slashCollider_->SetActive(true);

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particleInitialized_) {
        emitterMgr->SetEmitterPosition(emitterName_, colliderTransform_.translate);
        emitterMgr->SetEmitterRadius(emitterName_, attackRadius_);
        emitterMgr->SetEmitterActive(emitterName_, true);
    }
}

void BTBossSlashAttack::EndAttackPhase(Boss* boss) {
    if (slashCollider_) slashCollider_->SetActive(false);

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particleInitialized_) {
        emitterMgr->SetEmitterActive(emitterName_, false);
    }
}

void BTBossSlashAttack::OnCleanup() {
    // 攻撃フェーズに入ったまま中断された場合のみエミッタを停止する
    // （Warning/Blinking では未起動、EndAttackPhase 後は停止済み）
    if (cachedEmitterManager_ && particleInitialized_ && hasBegunAttack_ && !hasEndedAttack_) {
        cachedEmitterManager_->SetEmitterActive(emitterName_, false);
    }

    slashDecal_.reset();

    if (slashCollider_) {
        CollisionManager::GetInstance()->RemoveCollider(slashCollider_.get());
        slashCollider_.reset();
    }

    hasBegunAttack_ = false;
    hasEndedAttack_ = false;
}

void BTBossSlashAttack::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("warningDuration")) warningDuration_ = params["warningDuration"];
    if (params.contains("blinkDuration"))   blinkDuration_ = params["blinkDuration"];
    if (params.contains("attackDuration"))  attackDuration_ = params["attackDuration"];
    if (params.contains("recoveryTime"))    recoveryTime_ = params["recoveryTime"];
    if (params.contains("attackRadius"))    attackRadius_ = params["attackRadius"];
    if (params.contains("damage"))          damage_ = params["damage"];
    if (params.contains("blinkFrequency"))  blinkFrequency_ = params["blinkFrequency"];
}

void BTBossSlashAttack::OnExtractParameters(nlohmann::json& out) const {
    out["warningDuration"] = warningDuration_;
    out["blinkDuration"]   = blinkDuration_;
    out["attackDuration"]  = attackDuration_;
    out["recoveryTime"]    = recoveryTime_;
    out["attackRadius"]    = attackRadius_;
    out["damage"]          = damage_;
    out["blinkFrequency"]  = blinkFrequency_;
}

#ifdef _DEBUG
bool BTBossSlashAttack::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Warning Duration##slash", &warningDuration_, 0.05f, 0.1f, 5.0f))   changed = true;
    if (ImGui::DragFloat("Blink Duration##slash", &blinkDuration_, 0.05f, 0.1f, 3.0f))       changed = true;
    if (ImGui::DragFloat("Attack Duration##slash", &attackDuration_, 0.05f, 0.1f, 3.0f))     changed = true;
    if (ImGui::DragFloat("Recovery Time##slash", &recoveryTime_, 0.05f, 0.0f, 3.0f))         changed = true;

    ImGui::SeparatorText("Attack Parameters");
    if (ImGui::DragFloat("Attack Radius##slash", &attackRadius_, 0.5f, 1.0f, 30.0f))         changed = true;
    if (ImGui::DragFloat("Damage##slash", &damage_, 0.5f, 1.0f, 50.0f))                      changed = true;
    if (ImGui::DragFloat("Blink Frequency##slash", &blinkFrequency_, 0.5f, 1.0f, 30.0f))     changed = true;

    return changed;
}
#endif
