#include "BTBossMeleeAttack.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "../../../../Collision/BossMeleeAttackCollider.h"
#include "../../../../Common/GameConst.h"
#include "Object3d.h"
#include "Mat4x4Func.h"
#include "RandomEngine.h"
#include <cmath>
#include <algorithm>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossMeleeAttack::BTBossMeleeAttack() {
    name_ = "BossMeleeAttack";
}

Tako::BTNodeStatus BTBossMeleeAttack::OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) {
    elapsedTime_ += deltaTime;
    phaseTimer_ += deltaTime;

    switch (currentPhase_) {
    case MeleePhase::Prepare:
        ProcessPreparePhase(blackboard, deltaTime);
        if (phaseTimer_ >= prepareTime_) {
            currentPhase_ = MeleePhase::Execute;
            phaseTimer_ = 0.0f;
            boss->SetAttackSignEmitterActive(false);
            if (boss->GetMeleeAttackCollider()) {
                boss->GetMeleeAttackCollider()->SetActive(true);
                boss->GetMeleeAttackCollider()->Reset();
                colliderActivated_ = true;
            }
            InitializeRush(blackboard);
        }
        break;

    case MeleePhase::Execute:
        ProcessExecutePhase(blackboard, deltaTime);
        if (phaseTimer_ >= attackDuration_) {
            if (boss->GetMeleeAttackCollider()) {
                boss->GetMeleeAttackCollider()->SetActive(false);
            }
            boss->SetMeleeAttackBlockVisible(false);

            if (comboIndex_ < comboMaxCount_ - 1) {
                currentPhase_ = MeleePhase::Interval;
                phaseTimer_ = 0.0f;
            } else {
                currentPhase_ = MeleePhase::Recovery;
                phaseTimer_ = 0.0f;
                EnterAttackRecovery(boss);
            }
        }
        break;

    case MeleePhase::Interval:
        ProcessIntervalPhase(blackboard, deltaTime);
        if (phaseTimer_ >= comboInterval_) {
            comboIndex_++;
            InitializeSwingForCurrentCombo();
            boss->SetMeleeAttackBlockVisible(true);
            boss->SetAttackSignEmitterActive(true);
            UpdateBlockPosition(boss);
            currentPhase_ = MeleePhase::Prepare;
            phaseTimer_ = 0.0f;
        }
        break;

    case MeleePhase::Recovery:
        ProcessRecoveryPhase(boss);
        if (phaseTimer_ >= recoveryTime_) {
            phaseTimer_ = 0.0f;
            currentPhase_ = MeleePhase::Prepare;
            colliderActivated_ = false;
            return FinishAttack();
        }
        break;
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossMeleeAttack::OnInitialize(Tako::BTBlackboard* /*blackboard*/, Boss* boss) {
    phaseTimer_ = 0.0f;
    currentPhase_ = MeleePhase::Prepare;
    colliderActivated_ = false;

    isComboMode_ = Tako::RandomEngine::GetInstance()->GetBool(comboProbability_);
    comboMaxCount_ = isComboMode_ ? 3 : 1;
    comboIndex_ = 0;

    InitializeSwingForCurrentCombo();

    if (isComboMode_) {
        totalDuration_ = prepareTime_ + (attackDuration_ * 3) + (comboInterval_ * 2) + recoveryTime_;
    } else {
        totalDuration_ = prepareTime_ + attackDuration_ + recoveryTime_;
    }

    boss->SetMeleeAttackBlockVisible(true);
    boss->SetAttackSignEmitterActive(true);
    UpdateBlockPosition(boss);

    rushInitialized_ = false;
}

void BTBossMeleeAttack::OnCleanup() {
    phaseTimer_ = 0.0f;
    blockAngle_ = 0.0f;
    currentPhase_ = MeleePhase::Prepare;
    colliderActivated_ = false;
    rushInitialized_ = false;

    isComboMode_ = false;
    comboMaxCount_ = 1;
    comboIndex_ = 0;
    currentSwingDirection_ = 1.0f;
}

void BTBossMeleeAttack::AimAtPlayer(Tako::BTBlackboard* blackboard, float deltaTime) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Player* player = blackboard->GetPtr<Player>("player");
    if (!player) return;

    Vector3 playerPos = player->GetTransform().translate;
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 toPlayer = playerPos - bossPos;
    toPlayer.y = 0.0f;

    if (toPlayer.Length() > kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
        float targetAngle = atan2f(toPlayer.x, toPlayer.z);

        float currentAngle = boss->GetRotate().y;
        float angleDiff = targetAngle - currentAngle;

        while (angleDiff >  std::numbers::pi_v<float>) angleDiff -= 2.0f * std::numbers::pi_v<float>;
        while (angleDiff < -std::numbers::pi_v<float>) angleDiff += 2.0f * std::numbers::pi_v<float>;

        float rotationSpeed = 5.0f;
        float newAngle = currentAngle + angleDiff * rotationSpeed * deltaTime;

        boss->SetRotate(Vector3(0.0f, newAngle, 0.0f));
    }
}

void BTBossMeleeAttack::ProcessPreparePhase(Tako::BTBlackboard* blackboard, float deltaTime) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    AimAtPlayer(blackboard, deltaTime);
    UpdateBlockPosition(boss);

    Object3d* block = boss->GetMeleeAttackBlock();
    if (block) {
        boss->SetAttackSignEmitterPosition(block->GetTransform().translate);
    }
}

void BTBossMeleeAttack::ProcessExecutePhase(Tako::BTBlackboard* blackboard, float deltaTime) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    BossMeleeAttackCollider* collider = boss->GetMeleeAttackCollider();
    bool hasHit = collider && collider->HasHitPlayer();

    if (hasHit) {
        Player* player = blackboard->GetPtr<Player>("player");
        if (player) {
            Vector3 playerPos = player->GetTransform().translate;
            Vector3 bossPos = boss->GetTransform().translate;
            Vector3 toPlayer = playerPos - bossPos;
            toPlayer.y = 0.0f;
            float currentDistance = toPlayer.Length();

            if (currentDistance < stopDistance_ && currentDistance > kDirectionEpsilon) {
                Vector3 direction = toPlayer.Normalize();
                Vector3 stopPos = playerPos - direction * stopDistance_;
                stopPos = ClampToArea(stopPos);
                boss->SetTranslate(stopPos);
            }
        }
    } else {
        // ミス時: 通常通り突進
        float t = phaseTimer_ / attackDuration_;
        t = std::clamp(t, 0.0f, 1.0f);
        t = t * t * (3.0f - 2.0f * t);  // Smoothstep

        Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
        boss->SetTranslate(newPosition);
    }

    // ブロック回転
    float rotationSpeed = swingAngle_ / attackDuration_ * currentSwingDirection_;
    blockAngle_ += rotationSpeed * deltaTime;
    UpdateBlockPosition(boss);
}

void BTBossMeleeAttack::ProcessRecoveryPhase(Boss* boss) {
    (void)boss;
}

void BTBossMeleeAttack::ProcessIntervalPhase(Tako::BTBlackboard* blackboard, float deltaTime) {
    AimAtPlayer(blackboard, deltaTime);
}

void BTBossMeleeAttack::InitializeSwingForCurrentCombo() {
    if (comboIndex_ % 2 == 0) {
        blockAngle_ = kBlockStartAngle;
        currentSwingDirection_ = 1.0f;
    } else {
        blockAngle_ = -kBlockStartAngle;
        currentSwingDirection_ = -1.0f;
    }
}

Vector3 BTBossMeleeAttack::ClampToArea(const Vector3& position) {
    Vector3 clampedPos = position;
    clampedPos.x = std::clamp(clampedPos.x,
        GameConst::kStageXMin + GameConst::kAreaMargin,
        GameConst::kStageXMax - GameConst::kAreaMargin);
    clampedPos.z = std::clamp(clampedPos.z,
        GameConst::kStageZMin + GameConst::kAreaMargin,
        GameConst::kStageZMax - GameConst::kAreaMargin);
    clampedPos.y = position.y;
    return clampedPos;
}

void BTBossMeleeAttack::InitializeRush(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    rushInitialized_ = true;
    startPosition_ = boss->GetTransform().translate;

    Player* player = blackboard->GetPtr<Player>("player");
    if (player) {
        Vector3 playerPos = player->GetTransform().translate;
        Vector3 toPlayer = playerPos - startPosition_;
        toPlayer.y = 0.0f;

        if (toPlayer.Length() > kDirectionEpsilon) {
            rushDirection_ = toPlayer.Normalize();
            float angle = atan2f(rushDirection_.x, rushDirection_.z);
            boss->SetRotate(Vector3(0.0f, angle, 0.0f));

            targetPosition_ = startPosition_ + rushDirection_ * rushDistance_;
            targetPosition_ = ClampToArea(targetPosition_);
        } else {
            rushDirection_ = Vector3(0.0f, 0.0f, 1.0f);
            targetPosition_ = startPosition_;
        }
    } else {
        rushDirection_ = Vector3(0.0f, 0.0f, 1.0f);
        targetPosition_ = startPosition_;
    }
}

void BTBossMeleeAttack::UpdateBlockPosition(Boss* boss) {
    Object3d* block = boss->GetMeleeAttackBlock();
    if (!block) return;

    Vector3 bossPos = boss->GetTranslate();
    float bossRotY = boss->GetRotate().y;
    float worldAngle = bossRotY + blockAngle_;

    Matrix4x4 rotationMatrix = Mat4x4::MakeRotateY(worldAngle);
    Vector3 localOffset = { 0.0f, 0.0f, blockRadius_ };
    Vector3 worldOffset = Mat4x4::TransformNormal(rotationMatrix, localOffset);

    Vector3 blockPos = bossPos + worldOffset;

    Transform blockTransform;
    blockTransform.translate = blockPos;
    blockTransform.rotate = { 0.0f, worldAngle, 0.0f };
    blockTransform.scale = { blockScale_, blockScale_, blockScale_ };
    block->SetTransform(blockTransform);
    block->Update();

    BossMeleeAttackCollider* collider = boss->GetMeleeAttackCollider();
    if (collider && colliderActivated_) {
        collider->SetOrientation(Mat4x4::MakeRotateY(bossRotY));
    }
}

void BTBossMeleeAttack::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("prepareTime"))      prepareTime_ = params["prepareTime"];
    if (params.contains("attackDuration"))   attackDuration_ = params["attackDuration"];
    if (params.contains("recoveryTime"))     recoveryTime_ = params["recoveryTime"];
    if (params.contains("blockRadius"))      blockRadius_ = params["blockRadius"];
    if (params.contains("blockScale"))       blockScale_ = params["blockScale"];
    if (params.contains("swingAngle"))       swingAngle_ = params["swingAngle"];
    if (params.contains("rushDistance"))     rushDistance_ = params["rushDistance"];
    if (params.contains("stopDistance"))     stopDistance_ = params["stopDistance"];
    if (params.contains("comboInterval"))    comboInterval_ = params["comboInterval"];
    if (params.contains("comboProbability")) comboProbability_ = params["comboProbability"];
}

void BTBossMeleeAttack::OnExtractParameters(nlohmann::json& out) const {
    out["prepareTime"]      = prepareTime_;
    out["attackDuration"]   = attackDuration_;
    out["recoveryTime"]     = recoveryTime_;
    out["blockRadius"]      = blockRadius_;
    out["blockScale"]       = blockScale_;
    out["swingAngle"]       = swingAngle_;
    out["rushDistance"]     = rushDistance_;
    out["stopDistance"]     = stopDistance_;
    out["comboInterval"]    = comboInterval_;
    out["comboProbability"] = comboProbability_;
}

#ifdef _DEBUG
bool BTBossMeleeAttack::OnDrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Prepare Time##melee", &prepareTime_, 0.05f, 0.0f, 3.0f))     changed = true;
    if (ImGui::DragFloat("Attack Duration##melee", &attackDuration_, 0.05f, 0.0f, 3.0f)) changed = true;
    if (ImGui::DragFloat("Recovery Time##melee", &recoveryTime_, 0.05f, 0.0f, 3.0f))   changed = true;
    if (ImGui::DragFloat("Block Radius##melee", &blockRadius_, 0.1f, 0.5f, 10.0f))     changed = true;
    if (ImGui::DragFloat("Block Scale##melee", &blockScale_, 0.1f, 0.1f, 5.0f))        changed = true;
    if (ImGui::SliderAngle("Swing Angle##melee", &swingAngle_, 0.0f, 360.0f))          changed = true;
    if (ImGui::DragFloat("Rush Distance##melee", &rushDistance_, 1.0f, 0.0f, 50.0f))   changed = true;
    if (ImGui::DragFloat("Stop Distance##melee", &stopDistance_, 0.5f, 1.0f, 20.0f))   changed = true;

    ImGui::Separator();
    ImGui::Text("Combo Parameters:");
    if (ImGui::SliderFloat("Combo Probability##melee", &comboProbability_, 0.0f, 1.0f)) changed = true;
    if (ImGui::DragFloat("Combo Interval##melee", &comboInterval_, 0.05f, 0.1f, 2.0f))  changed = true;

    ImGui::Separator();
    ImGui::Text("Debug Info:");
    ImGui::Text("Mode: %s", isComboMode_ ? "COMBO (3 hits)" : "SINGLE (1 hit)");
    ImGui::Text("Combo: %d / %d", comboIndex_ + 1, comboMaxCount_);
    ImGui::Text("Swing Direction: %s", currentSwingDirection_ > 0 ? "Right->Left" : "Left->Right");

    return changed;
}
#endif
