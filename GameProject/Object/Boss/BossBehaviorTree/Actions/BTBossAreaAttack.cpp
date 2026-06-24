#include "BTBossAreaAttack.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "../../../../Common/GameConst.h"

#include "RandomEngine.h"
#include "CollisionManager.h"
#include "EmitterManager.h"

#include <cmath>
#include <algorithm>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossAreaAttack::BTBossAreaAttack() {
    name_ = "BossAreaAttack";
}

BTBossAreaAttack::~BTBossAreaAttack() {
    OnCleanup();
}

Tako::BTNodeStatus BTBossAreaAttack::OnExecute(Tako::BTBlackboard* /*blackboard*/, Boss* boss, float deltaTime) {
    // フェーズ管理: Warning → Blinking → Attack → Recovery
    const float warningEnd = warningDuration_;
    const float blinkEnd = warningEnd + blinkDuration_;
    const float attackEnd = blinkEnd + attackDuration_;

    if (elapsedTime_ < warningEnd) {

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

void BTBossAreaAttack::OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) {
    hasBegunAttack_ = false;
    hasEndedAttack_ = false;

    totalDuration_ = warningDuration_ + blinkDuration_ + attackDuration_ + recoveryTime_;

    SelectRandomQuadrants(blackboard);

    Vector3 bossPos = boss->GetTransform().translate;
    float halfArea = GameConst::kBossPhase2AreaSize * 0.5f;

    for (int i = 0; i < kQuadrantCount; ++i) {
        Vector3 center = GetQuadrantCenter(i, bossPos);

        quadrantDecals_[i] = std::make_unique<Decal>();
        quadrantDecals_[i]->Initialize();
        quadrantDecals_[i]->SetShape(DecalShape::Rectangle);
        quadrantDecals_[i]->SetTranslate(Vector3(center.x, 0.0f, center.z));
        quadrantDecals_[i]->SetScale(Vector3(halfArea * 2.0f, 1.0f, halfArea * 2.0f));
        quadrantDecals_[i]->SetEdgeSoftness(0.02f);

        if (activeQuadrants_[i]) {
            quadrantDecals_[i]->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
            quadrantDecals_[i]->SetVisible(true);
        } else {
            quadrantDecals_[i]->SetVisible(false);
        }

        colliderTransforms_[i].translate = center;
        colliderTransforms_[i].rotate = Vector3(0.0f, 0.0f, 0.0f);
        colliderTransforms_[i].scale = Vector3(1.0f, 1.0f, 1.0f);

        quadrantColliders_[i] = std::make_unique<BossAreaAttackCollider>(boss);
        quadrantColliders_[i]->SetTransform(&colliderTransforms_[i]);
        quadrantColliders_[i]->SetSize(Vector3(halfArea * 2.0f, kColliderHeight, halfArea * 2.0f));
        quadrantColliders_[i]->SetDamage(damage_);
        quadrantColliders_[i]->SetOwner(boss);
        quadrantColliders_[i]->SetActive(false);
        CollisionManager::GetInstance()->AddCollider(quadrantColliders_[i].get());
    }

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && !particlesInitialized_) {
        for (int i = 0; i < kQuadrantCount; ++i) {
            emitterNames_[i] = "area_attack_q" + std::to_string(i);
            emitterMgr->LoadPreset("attack_slash", emitterNames_[i]);
            emitterMgr->SetEmitterActive(emitterNames_[i], false);
        }
        particlesInitialized_ = true;
    }
}

void BTBossAreaAttack::OnCleanup() {
    // 攻撃中に中断された場合のみエミッタを停止する
    if (cachedEmitterManager_ && particlesInitialized_ && hasBegunAttack_ && !hasEndedAttack_) {
        for (int i = 0; i < kQuadrantCount; ++i) {
            if (activeQuadrants_[i]) {
                cachedEmitterManager_->SetEmitterActive(emitterNames_[i], false);
            }
        }
    }

    for (int i = 0; i < kQuadrantCount; ++i) {
        quadrantDecals_[i].reset();
    }

    for (int i = 0; i < kQuadrantCount; ++i) {
        if (quadrantColliders_[i]) {
            CollisionManager::GetInstance()->RemoveCollider(quadrantColliders_[i].get());
            quadrantColliders_[i].reset();
        }
    }

    activeQuadrants_.fill(false);
    hasBegunAttack_ = false;
    hasEndedAttack_ = false;
}

void BTBossAreaAttack::SelectRandomQuadrants(Tako::BTBlackboard* blackboard) {
    RandomEngine* rng = RandomEngine::GetInstance();

    int count = rng->GetInt(minQuadrants_, maxQuadrants_);
    count = std::clamp(count, 1, kQuadrantCount);

    activeQuadrants_.fill(false);

    int playerQuadrant = GetPlayerQuadrant(blackboard);
    activeQuadrants_[playerQuadrant] = true;

    if (count > 1) {
        std::array<int, kQuadrantCount - 1> remaining;
        int idx = 0;
        for (int i = 0; i < kQuadrantCount; ++i) {
            if (i != playerQuadrant) {
                remaining[idx++] = i;
            }
        }

        // Fisher-Yates シャッフル
        for (int i = static_cast<int>(remaining.size()) - 1; i > 0; --i) {
            int j = rng->GetInt(0, i);
            std::swap(remaining[i], remaining[j]);
        }

        for (int i = 0; i < count - 1; ++i) {
            activeQuadrants_[remaining[i]] = true;
        }
    }
}

int BTBossAreaAttack::GetPlayerQuadrant(Tako::BTBlackboard* blackboard) const {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Player* player = blackboard->GetPtr<Player>("player");
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 playerPos = player->GetTransform().translate;

    int xIndex = (playerPos.x >= bossPos.x) ? 1 : 0;
    int zIndex = (playerPos.z >= bossPos.z) ? 1 : 0;
    return zIndex * 2 + xIndex;
}

Vector3 BTBossAreaAttack::GetQuadrantCenter(int quadrantIndex, const Vector3& bossPos) const {
    float halfArea = GameConst::kBossPhase2AreaSize * 0.5f;

    float xSign = (quadrantIndex % 2 == 0) ? -1.0f : 1.0f;
    float zSign = (quadrantIndex < 2) ? -1.0f : 1.0f;

    return Vector3(bossPos.x + xSign * halfArea, bossPos.y, bossPos.z + zSign * halfArea);
}

void BTBossAreaAttack::UpdateBlinkingPhase(float phaseElapsed) {
    float sinValue = std::abs(std::sin(phaseElapsed * blinkFrequency_ * std::numbers::pi_v<float>));
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;

    for (int i = 0; i < kQuadrantCount; ++i) {
        if (activeQuadrants_[i] && quadrantDecals_[i]) {
            quadrantDecals_[i]->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
        }
    }
}

void BTBossAreaAttack::BeginAttackPhase(Boss* boss) {
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (activeQuadrants_[i] && quadrantDecals_[i]) {
            quadrantDecals_[i]->SetVisible(false);
        }
    }
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (activeQuadrants_[i] && quadrantColliders_[i]) {
            quadrantColliders_[i]->SetActive(true);
        }
    }

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particlesInitialized_) {
        Vector3 bossPos = boss->GetTransform().translate;
        for (int i = 0; i < kQuadrantCount; ++i) {
            if (activeQuadrants_[i]) {
                Vector3 center = GetQuadrantCenter(i, bossPos);
                emitterMgr->SetEmitterPosition(emitterNames_[i], center);
                emitterMgr->SetEmitterActive(emitterNames_[i], true);
            }
        }
    }
}

void BTBossAreaAttack::EndAttackPhase(Boss* boss) {
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (quadrantDecals_[i]) quadrantDecals_[i]->SetVisible(false);
    }
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (quadrantColliders_[i]) quadrantColliders_[i]->SetActive(false);
    }

    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particlesInitialized_) {
        for (int i = 0; i < kQuadrantCount; ++i) {
            emitterMgr->SetEmitterActive(emitterNames_[i], false);
        }
    }
}

void BTBossAreaAttack::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("warningDuration")) warningDuration_ = params["warningDuration"];
    if (params.contains("blinkDuration"))   blinkDuration_ = params["blinkDuration"];
    if (params.contains("attackDuration"))  attackDuration_ = params["attackDuration"];
    if (params.contains("recoveryTime"))    recoveryTime_ = params["recoveryTime"];
    if (params.contains("minQuadrants"))    minQuadrants_ = params["minQuadrants"];
    if (params.contains("maxQuadrants"))    maxQuadrants_ = params["maxQuadrants"];
    if (params.contains("damage"))          damage_ = params["damage"];
    if (params.contains("blinkFrequency"))  blinkFrequency_ = params["blinkFrequency"];
}

void BTBossAreaAttack::OnExtractParameters(nlohmann::json& out) const {
    out["warningDuration"] = warningDuration_;
    out["blinkDuration"]   = blinkDuration_;
    out["attackDuration"]  = attackDuration_;
    out["recoveryTime"]    = recoveryTime_;
    out["minQuadrants"]    = minQuadrants_;
    out["maxQuadrants"]    = maxQuadrants_;
    out["damage"]          = damage_;
    out["blinkFrequency"]  = blinkFrequency_;
}

#ifdef _DEBUG
bool BTBossAreaAttack::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Warning Duration##area", &warningDuration_, 0.05f, 0.1f, 5.0f)) changed = true;
    if (ImGui::DragFloat("Blink Duration##area", &blinkDuration_, 0.05f, 0.1f, 3.0f))     changed = true;
    if (ImGui::DragFloat("Attack Duration##area", &attackDuration_, 0.05f, 0.1f, 3.0f))   changed = true;
    if (ImGui::DragFloat("Recovery Time##area", &recoveryTime_, 0.05f, 0.0f, 3.0f))       changed = true;

    ImGui::SeparatorText("Attack Parameters");
    if (ImGui::DragInt("Min Quadrants##area", &minQuadrants_, 1, 1, kQuadrantCount))      changed = true;
    if (ImGui::DragInt("Max Quadrants##area", &maxQuadrants_, 1, 1, kQuadrantCount))      changed = true;
    if (ImGui::DragFloat("Damage##area", &damage_, 0.5f, 1.0f, 50.0f))                    changed = true;
    if (ImGui::DragFloat("Blink Frequency##area", &blinkFrequency_, 0.5f, 1.0f, 30.0f))   changed = true;

    if (minQuadrants_ > maxQuadrants_) {
        maxQuadrants_ = minQuadrants_;
        changed = true;
    }

    return changed;
}
#endif
