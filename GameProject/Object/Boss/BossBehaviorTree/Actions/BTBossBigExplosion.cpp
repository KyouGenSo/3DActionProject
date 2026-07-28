#include "BTBossBigExplosion.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"

#include "CollisionManager.h"
#include "EmitterManager.h"
#include "PostEffectManager.h"
#include "PostEffectStruct.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossBigExplosion::BTBossBigExplosion() {
    name_ = "BossBigExplosion";
}

BTBossBigExplosion::~BTBossBigExplosion() {
    OnCleanup();
}

Tako::BTNodeStatus BTBossBigExplosion::OnExecute(Tako::BTBlackboard* /*blackboard*/, Boss* boss, float deltaTime) {
    switch (phase_) {
    case Phase::Warning:
        if (phaseTimer_ >= warningDuration_) {
            AdvancePhase(Phase::Blinking);
        }
        break;

    case Phase::Blinking:
        UpdateBlink(deltaTime);
        if (phaseTimer_ >= blinkDuration_) {
            BeginFalling();
            AdvancePhase(Phase::Falling);
        }
        break;

    case Phase::Falling: {
        UpdateBlink(deltaTime);
        fallHeight_ -= fallSpeed_ * deltaTime;

        // fallSpeed_ が極小でも無限落下しないよう時間で強制着地
        const float timeout = spawnHeight_ / (std::max)(fallSpeed_, 0.01f) + kFallTimeoutMargin;
        if (fallHeight_ <= 0.0f || phaseTimer_ >= timeout) {
            TriggerImpact();
            AdvancePhase(Phase::Impact);
        }
        else if (cachedEmitterManager_) {
            const Vector3 fallPos(targetPos_.x, fallHeight_, targetPos_.z);
            cachedEmitterManager_->SetEmitterPosition(kEmitterFallFlash, fallPos);
            cachedEmitterManager_->SetEmitterPosition(kEmitterFallFire, fallPos);
        }
        break;
    }

    case Phase::Impact:
        if (!burstEnded_ && phaseTimer_ >= burstDuration_) {
            EndBurst();
        }
        if (!colliderClosed_ && phaseTimer_ >= colliderActiveDuration_) {
            CloseCollider();
        }
        if (phaseTimer_ >= impactDuration_) {
            EndBurst();
            CloseCollider();
            AdvancePhase(Phase::Recovery);
        }
        break;

    case Phase::Recovery:
        EnterAttackRecovery(boss);
        if (phaseTimer_ >= recoveryTime_) {
            return FinishAttack();
        }
        break;
    }

    phaseTimer_ += deltaTime;
    elapsedTime_ += deltaTime;

    return Tako::BTNodeStatus::Running;
}

void BTBossBigExplosion::OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) {
    phase_ = Phase::Warning;
    phaseTimer_ = 0.0f;
    blinkClock_ = 0.0f;
    fallHeight_ = spawnHeight_;
    burstEnded_ = false;
    colliderClosed_ = false;

    // 発動時のプレイヤー位置を着弾点として固定
    Player* player = blackboard->GetPtr<Player>("player");
    targetPos_ = player ? player->GetTransform().translate : boss->GetTransform().translate;

    float diameter = attackRadius_ * 2.0f;
    decal_ = std::make_unique<Decal>();
    decal_->Initialize();
    decal_->SetShape(DecalShape::Circle);
    decal_->SetTranslate(Vector3(targetPos_.x, 0.0f, targetPos_.z));
    decal_->SetScale(Vector3(diameter, 1.0f, diameter));
    decal_->SetEdgeSoftness(0.02f);
    decal_->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
    decal_->SetVisible(true);

    colliderTransform_.translate = Vector3(targetPos_.x, colliderY_, targetPos_.z);
    colliderTransform_.rotate = Vector3(0.0f, 0.0f, 0.0f);
    colliderTransform_.scale = Vector3(1.0f, 1.0f, 1.0f);

    collider_ = std::make_unique<MeteorImpactCollider>(boss);
    collider_->SetTransform(&colliderTransform_);
    collider_->SetRadius(attackRadius_);
    collider_->SetDamage(damage_);
    collider_->SetOwner(boss);
    collider_->SetActive(false);
    CollisionManager::GetInstance()->AddCollider(collider_.get());
}

void BTBossBigExplosion::UpdateBlink(float deltaTime) {
    blinkClock_ += deltaTime;
    float sinValue = std::abs(std::sin(blinkClock_ * blinkFrequency_ * std::numbers::pi_v<float>));
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;
    if (decal_) {
        decal_->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
    }
}

void BTBossBigExplosion::BeginFalling() {
    fallHeight_ = spawnHeight_;

    if (!cachedEmitterManager_) {
        return;
    }

    const Vector3 spawnPos(targetPos_.x, spawnHeight_, targetPos_.z);
    cachedEmitterManager_->SetEmitterPosition(kEmitterFallFlash, spawnPos);
    cachedEmitterManager_->SetEmitterPosition(kEmitterFallFire, spawnPos);
    if (syncEmitterRadius_) {
        cachedEmitterManager_->SetEmitterRadius(kEmitterFallFire, attackRadius_);
    }
    ForceImmediateEmit(kEmitterFallFlash);
    ForceImmediateEmit(kEmitterFallFire);
    cachedEmitterManager_->SetEmitterActive(kEmitterFallFlash, true);
    cachedEmitterManager_->SetEmitterActive(kEmitterFallFire, true);
}

void BTBossBigExplosion::TriggerImpact() {
    if (decal_) {
        decal_->SetVisible(false);
    }

    if (cachedEmitterManager_) {
        cachedEmitterManager_->SetEmitterActive(kEmitterFallFlash, false);
        cachedEmitterManager_->SetEmitterActive(kEmitterFallFire, false);

        // プリセット保存値の position 残骸を必ず上書きする
        const Vector3 impactPos(targetPos_.x, impactYOffset_, targetPos_.z);
        cachedEmitterManager_->SetEmitterPosition(kEmitterImpactSpark, impactPos);
        cachedEmitterManager_->SetEmitterPosition(kEmitterImpactSmoke, impactPos);
        if (syncEmitterRadius_) {
            cachedEmitterManager_->SetEmitterRadius(kEmitterImpactSmoke, attackRadius_);
        }
        ForceImmediateEmit(kEmitterImpactSpark);
        ForceImmediateEmit(kEmitterImpactSmoke);
        cachedEmitterManager_->SetEmitterActive(kEmitterImpactSpark, true);
        cachedEmitterManager_->SetEmitterActive(kEmitterImpactSmoke, true);
    }

    if (collider_) {
        collider_->SetActive(true);
    }

    if (bwEnabled_) {
        PostEffectManager::GetInstance()->ApplyTemporaryEffect(
            "BWFilter", bwDuration_, BWFilterParam{ bwThreshold_ });
    }
}

void BTBossBigExplosion::EndBurst() {
    if (burstEnded_) {
        return;
    }
    burstEnded_ = true;

    if (cachedEmitterManager_) {
        cachedEmitterManager_->SetEmitterActive(kEmitterImpactSpark, false);
        cachedEmitterManager_->SetEmitterActive(kEmitterImpactSmoke, false);
    }
}

void BTBossBigExplosion::CloseCollider() {
    if (colliderClosed_) {
        return;
    }
    colliderClosed_ = true;

    if (collider_) {
        collider_->SetActive(false);
    }
}

void BTBossBigExplosion::ForceImmediateEmit(const char* emitterName) {
    if (!cachedEmitterManager_) {
        return;
    }
    if (auto emitter = cachedEmitterManager_->GetEmitterByName(emitterName)) {
        emitter->SetFrequencyTime(emitter->GetFrequency());
    }
}

void BTBossBigExplosion::AdvancePhase(Phase next) {
    phase_ = next;
    phaseTimer_ = 0.0f;
}

void BTBossBigExplosion::OnCleanup() {
    if (cachedEmitterManager_) {
        if (phase_ == Phase::Falling) {
            cachedEmitterManager_->SetEmitterActive(kEmitterFallFlash, false);
            cachedEmitterManager_->SetEmitterActive(kEmitterFallFire, false);
        }
        if (phase_ == Phase::Impact && !burstEnded_) {
            cachedEmitterManager_->SetEmitterActive(kEmitterImpactSpark, false);
            cachedEmitterManager_->SetEmitterActive(kEmitterImpactSmoke, false);
        }
    }

    decal_.reset();

    if (collider_) {
        CollisionManager::GetInstance()->RemoveCollider(collider_.get());
        collider_.reset();
    }

    phase_ = Phase::Warning;
    phaseTimer_ = 0.0f;
    blinkClock_ = 0.0f;
    burstEnded_ = false;
    colliderClosed_ = false;
}

void BTBossBigExplosion::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("warningDuration"))        warningDuration_ = params["warningDuration"];
    if (params.contains("blinkDuration"))          blinkDuration_ = params["blinkDuration"];
    if (params.contains("impactDuration"))         impactDuration_ = params["impactDuration"];
    if (params.contains("recoveryTime"))           recoveryTime_ = params["recoveryTime"];
    if (params.contains("spawnHeight"))            spawnHeight_ = params["spawnHeight"];
    if (params.contains("fallSpeed"))              fallSpeed_ = params["fallSpeed"];
    if (params.contains("attackRadius"))           attackRadius_ = params["attackRadius"];
    if (params.contains("damage"))                 damage_ = params["damage"];
    if (params.contains("colliderY"))              colliderY_ = params["colliderY"];
    if (params.contains("colliderActiveDuration")) colliderActiveDuration_ = params["colliderActiveDuration"];
    if (params.contains("burstDuration"))          burstDuration_ = params["burstDuration"];
    if (params.contains("impactYOffset"))          impactYOffset_ = params["impactYOffset"];
    if (params.contains("syncEmitterRadius"))      syncEmitterRadius_ = params["syncEmitterRadius"];
    if (params.contains("blinkFrequency"))         blinkFrequency_ = params["blinkFrequency"];
    if (params.contains("bwEnabled"))              bwEnabled_ = params["bwEnabled"];
    if (params.contains("bwDuration"))             bwDuration_ = params["bwDuration"];
    if (params.contains("bwThreshold"))            bwThreshold_ = params["bwThreshold"];
}

void BTBossBigExplosion::OnExtractParameters(nlohmann::json& out) const {
    out["warningDuration"]        = warningDuration_;
    out["blinkDuration"]          = blinkDuration_;
    out["impactDuration"]         = impactDuration_;
    out["recoveryTime"]           = recoveryTime_;
    out["spawnHeight"]            = spawnHeight_;
    out["fallSpeed"]              = fallSpeed_;
    out["attackRadius"]           = attackRadius_;
    out["damage"]                 = damage_;
    out["colliderY"]              = colliderY_;
    out["colliderActiveDuration"] = colliderActiveDuration_;
    out["burstDuration"]          = burstDuration_;
    out["impactYOffset"]          = impactYOffset_;
    out["syncEmitterRadius"]      = syncEmitterRadius_;
    out["blinkFrequency"]         = blinkFrequency_;
    out["bwEnabled"]              = bwEnabled_;
    out["bwDuration"]             = bwDuration_;
    out["bwThreshold"]            = bwThreshold_;
}

#ifdef _DEBUG
bool BTBossBigExplosion::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Warning Duration##bigexpl", &warningDuration_, 0.05f, 0.0f, 5.0f))          changed = true;
    if (ImGui::DragFloat("Blink Duration##bigexpl", &blinkDuration_, 0.05f, 0.0f, 3.0f))              changed = true;
    if (ImGui::DragFloat("Impact Duration##bigexpl", &impactDuration_, 0.05f, 0.1f, 3.0f))            changed = true;
    if (ImGui::DragFloat("Recovery Time##bigexpl", &recoveryTime_, 0.05f, 0.0f, 3.0f))                changed = true;

    ImGui::SeparatorText("Fall");
    if (ImGui::DragFloat("Spawn Height##bigexpl", &spawnHeight_, 0.5f, 5.0f, 80.0f))                  changed = true;
    if (ImGui::DragFloat("Fall Speed##bigexpl", &fallSpeed_, 0.5f, 1.0f, 100.0f))                     changed = true;

    ImGui::SeparatorText("Attack Area");
    if (ImGui::DragFloat("Attack Radius##bigexpl", &attackRadius_, 0.5f, 1.0f, 40.0f))                changed = true;
    if (ImGui::DragFloat("Damage##bigexpl", &damage_, 0.5f, 1.0f, 100.0f))                            changed = true;
    if (ImGui::DragFloat("Collider Y##bigexpl", &colliderY_, 0.1f, 0.0f, 5.0f))                       changed = true;
    if (ImGui::DragFloat("Collider Active Time##bigexpl", &colliderActiveDuration_, 0.01f, 0.05f, 1.0f)) changed = true;

    ImGui::SeparatorText("Impact Burst");
    if (ImGui::DragFloat("Burst Duration##bigexpl", &burstDuration_, 0.01f, 0.02f, 1.0f))             changed = true;
    if (ImGui::DragFloat("Impact Y Offset##bigexpl", &impactYOffset_, 0.1f, 0.0f, 5.0f))              changed = true;
    if (ImGui::Checkbox("Sync Emitter Radius##bigexpl", &syncEmitterRadius_))                         changed = true;

    ImGui::SeparatorText("Decal");
    if (ImGui::DragFloat("Blink Frequency##bigexpl", &blinkFrequency_, 0.5f, 1.0f, 30.0f))            changed = true;

    ImGui::SeparatorText("BW Filter");
    if (ImGui::Checkbox("BW Enabled##bigexpl", &bwEnabled_))                                          changed = true;
    if (ImGui::DragFloat("BW Duration##bigexpl", &bwDuration_, 0.01f, 0.05f, 1.0f))                   changed = true;
    if (ImGui::DragFloat("BW Threshold##bigexpl", &bwThreshold_, 0.01f, 0.0f, 1.0f))                  changed = true;

    return changed;
}
#endif
