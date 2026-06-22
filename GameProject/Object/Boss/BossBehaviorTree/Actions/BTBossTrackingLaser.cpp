#include "BTBossTrackingLaser.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"

#include "CollisionManager.h"
#include "EmitterManager.h"
#include "BoxEmitter.h"

#include <cmath>
#include <numbers>
#include <DirectXMath.h>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossTrackingLaser::BTBossTrackingLaser() {
    name_ = "BossTrackingLaser";
}

BTBossTrackingLaser::~BTBossTrackingLaser() {
    OnCleanup();
}

Tako::BTNodeStatus BTBossTrackingLaser::OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) {
    const float aimEnd = aimDuration_;
    const float blinkEnd = aimEnd + blinkDuration_;
    const float attackEnd = blinkEnd + attackDuration_;

    if (elapsedTime_ < blinkEnd) {
        // Aim / Blink Phase: プレイヤーへ追従
        FacePlayerInstant(blackboard);

        Vector3 center;
        float yawRad = 0.0f;
        float length = 0.0f;
        if (ComputeBeam(boss, blackboard, center, yawRad, length)) {
            firedCenter_ = center;
            firedYawRad_ = yawRad;
            firedLength_ = length;
            ApplyBeamToDecal(center, yawRad, length);
        }

        if (elapsedTime_ >= aimEnd) {
            UpdateBlinkingPhase(elapsedTime_ - aimEnd);
        }
    }
    else if (elapsedTime_ < attackEnd) {
        if (!hasBegunAttack_) {
            BeginAttackPhase(boss);
            hasBegunAttack_ = true;
        }
        // 発生の一瞬だけエミッターを有効化し、その後トレイルの残光に任せる
        else if (!hasStoppedEmit_ && (elapsedTime_ - blinkEnd) >= laserEmitDuration_) {
            if (cachedEmitterManager_ && particleInitialized_) {
                cachedEmitterManager_->SetEmitterActive(emitterName_, false);
            }
            hasStoppedEmit_ = true;
        }
    }
    else {
        if (!hasEndedAttack_) {
            EndAttackPhase(boss);
            hasEndedAttack_ = true;
        }
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

void BTBossTrackingLaser::OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) {
    hasBegunAttack_ = false;
    hasEndedAttack_ = false;
    hasStoppedEmit_ = false;

    totalDuration_ = aimDuration_ + blinkDuration_ + attackDuration_ + recoveryTime_;

    beamDecal_ = std::make_unique<Decal>();
    beamDecal_->Initialize();
    beamDecal_->SetShape(DecalShape::Rectangle);
    beamDecal_->SetEdgeSoftness(0.02f);
    beamDecal_->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
    beamDecal_->SetVisible(true);

    Vector3 center;
    float yawRad = 0.0f;
    float length = 0.0f;
    if (ComputeBeam(boss, blackboard, center, yawRad, length)) {
        firedCenter_ = center;
        firedYawRad_ = yawRad;
        firedLength_ = length;
        ApplyBeamToDecal(center, yawRad, length);
    }

    colliderTransform_.translate = Vector3(firedCenter_.x, beamHeight_, firedCenter_.z);
    colliderTransform_.rotate = Vector3(0.0f, firedYawRad_, 0.0f);
    colliderTransform_.scale = Vector3(1.0f, 1.0f, 1.0f);

    beamCollider_ = std::make_unique<BossAreaAttackCollider>(boss);
    beamCollider_->SetTransform(&colliderTransform_);
    beamCollider_->SetSize(Vector3(beamWidth_, colliderHeight_, firedLength_));
    beamCollider_->SetDamage(damage_);
    beamCollider_->SetOwner(boss);
    beamCollider_->SetActive(false);
    CollisionManager::GetInstance()->AddCollider(beamCollider_.get());

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && !particleInitialized_) {
        emitterName_ = "boss_tracking_laser";
        emitterMgr->LoadPreset("boss_razer_trail", emitterName_);
        emitterMgr->SetEmitterActive(emitterName_, false);
        if (auto emitter = emitterMgr->GetEmitterByName(emitterName_)) {
            if (auto box = dynamic_cast<BoxEmitter*>(emitter.get())) {
                presetBoxSize_ = box->GetSize();
            }
        }
        particleInitialized_ = true;
    }
}

bool BTBossTrackingLaser::ComputeBeam(Boss* boss, Tako::BTBlackboard* blackboard,
                                      Vector3& outCenter, float& outYawRad, float& outLength) const {
    outCenter = Vector3(0.0f, 0.0f, 0.0f);
    outYawRad = 0.0f;
    outLength = 0.0f;

    Player* player = blackboard->GetPtr<Player>("player");
    if (!boss || !player) return false;

    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 toPlayer = player->GetTransform().translate - bossPos;
    toPlayer.y = 0.0f;

    float distance = toPlayer.Length();
    if (distance <= GameConst::kDirectionEpsilon) return false;

    Vector3 dir = toPlayer.Normalize();
    float length = distance + endOffset_;

    outCenter = bossPos + dir * (length * 0.5f);
    outYawRad = atan2f(dir.x, dir.z);
    outLength = length;
    return true;
}

void BTBossTrackingLaser::ApplyBeamToDecal(const Vector3& center, float yawRad, float length) {
    if (!beamDecal_) return;
    beamDecal_->SetTranslate(Vector3(center.x, 0.0f, center.z));
    beamDecal_->SetRotate(Vector3(0.0f, yawRad, 0.0f));
    beamDecal_->SetScale(Vector3(beamWidth_, 1.0f, length));
}

void BTBossTrackingLaser::UpdateBlinkingPhase(float phaseElapsed) {
    float sinValue = std::abs(std::sin(phaseElapsed * blinkFrequency_ * std::numbers::pi_v<float>));
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;
    if (beamDecal_) {
        beamDecal_->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
    }
}

void BTBossTrackingLaser::BeginAttackPhase(Boss* boss) {
    if (beamDecal_) beamDecal_->SetVisible(false);

    colliderTransform_.translate = Vector3(firedCenter_.x, beamHeight_, firedCenter_.z);
    colliderTransform_.rotate = Vector3(0.0f, firedYawRad_, 0.0f);
    if (beamCollider_) {
        beamCollider_->SetSize(Vector3(beamWidth_, colliderHeight_, firedLength_));
        beamCollider_->SetActive(true);
    }

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particleInitialized_) {
        if (auto emitter = emitterMgr->GetEmitterByName(emitterName_)) {
            if (auto box = dynamic_cast<BoxEmitter*>(emitter.get())) {
                box->SetPosition(Vector3(firedCenter_.x, beamHeight_, firedCenter_.z));
                box->SetRotation(Vector3(0.0f, DirectX::XMConvertToDegrees(firedYawRad_), 0.0f));
                box->SetSize(Vector3(presetBoxSize_.x, presetBoxSize_.y, firedLength_));
            }
        }
        emitterMgr->SetEmitterActive(emitterName_, true);
    }
}

void BTBossTrackingLaser::EndAttackPhase(Boss* boss) {
    if (beamCollider_) beamCollider_->SetActive(false);

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particleInitialized_) {
        emitterMgr->SetEmitterActive(emitterName_, false);
    }
}

void BTBossTrackingLaser::OnCleanup() {
    // 発生中に中断された場合のみエミッタを停止する
    if (cachedEmitterManager_ && particleInitialized_ && hasBegunAttack_ && !hasEndedAttack_) {
        cachedEmitterManager_->SetEmitterActive(emitterName_, false);
    }

    beamDecal_.reset();

    if (beamCollider_) {
        CollisionManager::GetInstance()->RemoveCollider(beamCollider_.get());
        beamCollider_.reset();
    }

    hasBegunAttack_ = false;
    hasEndedAttack_ = false;
    hasStoppedEmit_ = false;
}

void BTBossTrackingLaser::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("aimDuration"))       aimDuration_ = params["aimDuration"];
    if (params.contains("blinkDuration"))     blinkDuration_ = params["blinkDuration"];
    if (params.contains("attackDuration"))    attackDuration_ = params["attackDuration"];
    if (params.contains("recoveryTime"))      recoveryTime_ = params["recoveryTime"];
    if (params.contains("beamWidth"))         beamWidth_ = params["beamWidth"];
    if (params.contains("endOffset"))         endOffset_ = params["endOffset"];
    if (params.contains("beamHeight"))        beamHeight_ = params["beamHeight"];
    if (params.contains("colliderHeight"))    colliderHeight_ = params["colliderHeight"];
    if (params.contains("laserEmitDuration")) laserEmitDuration_ = params["laserEmitDuration"];
    if (params.contains("damage"))            damage_ = params["damage"];
    if (params.contains("blinkFrequency"))    blinkFrequency_ = params["blinkFrequency"];
}

void BTBossTrackingLaser::OnExtractParameters(nlohmann::json& out) const {
    out["aimDuration"]       = aimDuration_;
    out["blinkDuration"]     = blinkDuration_;
    out["attackDuration"]    = attackDuration_;
    out["recoveryTime"]      = recoveryTime_;
    out["beamWidth"]         = beamWidth_;
    out["endOffset"]         = endOffset_;
    out["beamHeight"]        = beamHeight_;
    out["colliderHeight"]    = colliderHeight_;
    out["laserEmitDuration"] = laserEmitDuration_;
    out["damage"]            = damage_;
    out["blinkFrequency"]    = blinkFrequency_;
}

#ifdef _DEBUG
bool BTBossTrackingLaser::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Aim Duration##laser", &aimDuration_, 0.05f, 0.1f, 5.0f))         changed = true;
    if (ImGui::DragFloat("Blink Duration##laser", &blinkDuration_, 0.05f, 0.1f, 3.0f))     changed = true;
    if (ImGui::DragFloat("Attack Duration##laser", &attackDuration_, 0.05f, 0.05f, 3.0f))  changed = true;
    if (ImGui::DragFloat("Recovery Time##laser", &recoveryTime_, 0.05f, 0.0f, 3.0f))       changed = true;
    if (ImGui::DragFloat("Laser Emit Duration##laser", &laserEmitDuration_, 0.01f, 0.0f, 1.0f)) changed = true;

    ImGui::SeparatorText("Beam Shape");
    if (ImGui::DragFloat("Beam Width##laser", &beamWidth_, 0.1f, 0.2f, 10.0f))             changed = true;
    if (ImGui::DragFloat("End Offset##laser", &endOffset_, 0.1f, 0.0f, 20.0f))             changed = true;
    if (ImGui::DragFloat("Beam Height##laser", &beamHeight_, 0.1f, 0.0f, 10.0f))           changed = true;
    if (ImGui::DragFloat("Collider Height##laser", &colliderHeight_, 0.1f, 0.5f, 10.0f))   changed = true;

    ImGui::SeparatorText("Attack Parameters");
    if (ImGui::DragFloat("Damage##laser", &damage_, 0.5f, 1.0f, 50.0f))                    changed = true;
    if (ImGui::DragFloat("Blink Frequency##laser", &blinkFrequency_, 0.5f, 1.0f, 30.0f))   changed = true;

    return changed;
}
#endif
