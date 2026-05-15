#include "BTBossMeteorRain.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "../../../../Common/GameConst.h"
#include "RandomEngine.h"
#include "CollisionManager.h"

#include <cmath>
#include <algorithm>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossMeteorRain::BTBossMeteorRain() {
    name_ = "BossMeteorRain";
}

BTBossMeteorRain::~BTBossMeteorRain() {
    OnCleanup();
}

Tako::BTNodeStatus BTBossMeteorRain::OnExecute(Tako::BTBlackboard* /*blackboard*/, Boss* boss, float deltaTime) {
    // フェーズ管理: Charge → Launch → Warning → Blink → Impact → Recovery
    const float chargeEnd = chargeTime_;
    const float launchEnd = chargeEnd + launchDuration_;
    const float warningEnd = launchEnd + warningDuration_;
    const float blinkEnd = warningEnd + blinkDuration_;
    const float impactEnd = blinkEnd + impactDuration_;

    if (elapsedTime_ < chargeEnd) {
        bulletSignEffect_.Update(boss, deltaTime);
    }
    else if (elapsedTime_ < launchEnd) {
        if (bulletSignEffect_.IsActive()) {
            bulletSignEffect_.End(boss);
        }

        float launchElapsed = elapsedTime_ - chargeEnd;

        // 上方向弾（impactCount_ 発、既存処理）
        float launchInterval = (impactCount_ > 0) ? launchDuration_ / static_cast<float>(impactCount_) : launchDuration_;
        int expectedLaunched = static_cast<int>(launchElapsed / launchInterval);
        expectedLaunched = std::min<int>(expectedLaunched, impactCount_);

        while (bulletsLaunched_ < expectedLaunched) {
            LaunchBullet(boss, bulletsLaunched_);
            bulletsLaunched_++;
        }

        // 水平弾（horizontalBulletCount_ 発、独立タイミング）
        if (horizontalBulletCount_ > 0) {
            Vector3 bossPos = boss->GetTransform().translate;
            float hInterval = launchDuration_ / static_cast<float>(horizontalBulletCount_);
            int expectedH = std::min<int>(static_cast<int>(launchElapsed / hInterval), horizontalBulletCount_);
            while (horizontalBulletsLaunched_ < expectedH) {
                float angle = (2.0f * 3.14159265f / static_cast<float>(horizontalBulletCount_)) * static_cast<float>(horizontalBulletsLaunched_);
                float vx = std::cos(angle) * horizontalSpeed_;
                float vz = std::sin(angle) * horizontalSpeed_;
                boss->RequestBulletSpawn(bossPos, Vector3(vx, 0.0f, vz));
                horizontalBulletsLaunched_++;
            }
        }
    }
    else if (elapsedTime_ < warningEnd) {
        // 残りの上方向弾を全て発射（保険）
        while (bulletsLaunched_ < impactCount_) {
            LaunchBullet(boss, bulletsLaunched_);
            bulletsLaunched_++;
        }
        // 残りの水平弾を全て発射（保険）
        if (horizontalBulletCount_ > 0) {
            Vector3 bossPos = boss->GetTransform().translate;
            while (horizontalBulletsLaunched_ < horizontalBulletCount_) {
                float angle = (2.0f * 3.14159265f / static_cast<float>(horizontalBulletCount_)) * static_cast<float>(horizontalBulletsLaunched_);
                float vx = std::cos(angle) * horizontalSpeed_;
                float vz = std::sin(angle) * horizontalSpeed_;
                boss->RequestBulletSpawn(bossPos, Vector3(vx, 0.0f, vz));
                horizontalBulletsLaunched_++;
            }
        }
        // Warning フェーズ開始時に Decal を表示（1回のみ）
        if (!decalsShown_) {
            for (int i = 0; i < impactCount_; ++i) {
                if (impactDecals_[i]) {
                    impactDecals_[i]->SetVisible(true);
                }
            }
            decalsShown_ = true;
        }
    }
    else if (elapsedTime_ < blinkEnd) {
        UpdateBlinkingPhase(elapsedTime_ - warningEnd);
    }
    else if (elapsedTime_ < impactEnd) {
        if (!hasBegunImpact_) {
            BeginImpactPhase(boss);
            hasBegunImpact_ = true;
        }
    }
    else {
        if (!hasEndedImpact_) {
            EndImpactPhase(boss);
            hasEndedImpact_ = true;
        }
        EnterAttackRecovery(boss);
    }

    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= totalDuration_) {
        hasBegunImpact_ = false;
        hasEndedImpact_ = false;
        decalsShown_ = false;
        bulletsLaunched_ = 0;
        horizontalBulletsLaunched_ = 0;
        return FinishAttack();
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossMeteorRain::OnInitialize(Tako::BTBlackboard* /*blackboard*/, Boss* boss) {
    hasBegunImpact_ = false;
    hasEndedImpact_ = false;
    decalsShown_ = false;
    bulletsLaunched_ = 0;
    horizontalBulletsLaunched_ = 0;

    totalDuration_ = chargeTime_ + launchDuration_ + warningDuration_ + blinkDuration_ + impactDuration_ + recoveryTime_;

    bulletSignEffect_.Start(boss, chargeTime_);

    RandomEngine* rng = RandomEngine::GetInstance();
    impactCount_ = rng->GetInt(minImpacts_, maxImpacts_);
    impactCount_ = std::clamp(impactCount_, 1, kMaxImpacts);

    Vector3 bossPos = boss->GetTransform().translate;
    GenerateImpactPositions(bossPos);

    // コライダー用 Transform の事前確保（再配置対策）
    colliderTransforms_.clear();
    colliderTransforms_.reserve(impactCount_);
    for (int i = 0; i < impactCount_; ++i) {
        Transform t;
        t.translate = impactPositions_[i];
        t.rotate = Vector3(0.0f, 0.0f, 0.0f);
        t.scale = Vector3(1.0f, 1.0f, 1.0f);
        colliderTransforms_.push_back(t);
    }

    // Decal とコライダー初期化
    impactDecals_.clear();
    impactDecals_.reserve(impactCount_);
    impactColliders_.clear();
    impactColliders_.reserve(impactCount_);

    float diameter = impactRadius_ * 2.0f;

    for (int i = 0; i < impactCount_; ++i) {
        auto decal = std::make_unique<Decal>();
        decal->Initialize();
        decal->SetShape(DecalShape::Circle);
        decal->SetTranslate(Vector3(impactPositions_[i].x, 0.0f, impactPositions_[i].z));
        decal->SetScale(Vector3(diameter, 1.0f, diameter));
        decal->SetEdgeSoftness(0.02f);
        decal->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
        decal->SetVisible(false);
        impactDecals_.push_back(std::move(decal));

        auto collider = std::make_unique<MeteorImpactCollider>(boss);
        collider->SetTransform(&colliderTransforms_[i]);
        collider->SetRadius(impactRadius_);
        collider->SetDamage(damage_);
        collider->SetOwner(boss);
        collider->SetActive(false);
        CollisionManager::GetInstance()->AddCollider(collider.get());
        impactColliders_.push_back(std::move(collider));
    }
}

void BTBossMeteorRain::OnCleanup() {
    impactDecals_.clear();

    for (auto& collider : impactColliders_) {
        if (collider) {
            CollisionManager::GetInstance()->RemoveCollider(collider.get());
        }
    }
    impactColliders_.clear();

    colliderTransforms_.clear();
    impactPositions_.clear();
    impactCount_ = 0;
    hasBegunImpact_ = false;
    hasEndedImpact_ = false;
    decalsShown_ = false;
    bulletsLaunched_ = 0;
    horizontalBulletsLaunched_ = 0;
}

void BTBossMeteorRain::GenerateImpactPositions(const Vector3& bossPos) {
    impactPositions_.clear();
    impactPositions_.reserve(impactCount_);

    RandomEngine* rng = RandomEngine::GetInstance();
    float areaSize = GameConst::kBossPhase2AreaSize;

    float minSeparation = 2.0f * impactRadius_;
    float minSepSq = minSeparation * minSeparation;

    for (int i = 0; i < impactCount_; ++i) {
        bool placed = false;

        for (int retry = 0; retry < kMaxPlacementRetries; ++retry) {
            float x = bossPos.x + rng->GetFloat(-areaSize, areaSize);
            float z = bossPos.z + rng->GetFloat(-areaSize, areaSize);
            Vector3 candidate(x, bossPos.y, z);

            bool tooClose = false;
            for (const auto& existing : impactPositions_) {
                if (candidate.DistanceSquared(existing) < minSepSq) {
                    tooClose = true;
                    break;
                }
            }

            if (!tooClose) {
                impactPositions_.push_back(candidate);
                placed = true;
                break;
            }
        }

        if (!placed) {
            impactCount_ = static_cast<int>(impactPositions_.size());
            break;
        }
    }
}

void BTBossMeteorRain::LaunchBullet(Boss* boss, int /*index*/) {
    Vector3 bossPos = boss->GetTransform().translate;
    RandomEngine* rng = RandomEngine::GetInstance();

    float spreadX = rng->GetFloat(-launchSpreadXZ_, launchSpreadXZ_);
    float spreadZ = rng->GetFloat(-launchSpreadXZ_, launchSpreadXZ_);
    boss->RequestBulletSpawn(bossPos, Vector3(spreadX, launchSpeed_, spreadZ));
}

void BTBossMeteorRain::UpdateBlinkingPhase(float phaseElapsed) {
    float sinValue = std::abs(std::sin(phaseElapsed * blinkFrequency_ * 3.14159265f));
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;

    for (int i = 0; i < impactCount_; ++i) {
        if (impactDecals_[i]) {
            impactDecals_[i]->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
        }
    }
}

void BTBossMeteorRain::BeginImpactPhase(Boss* boss) {
    for (int i = 0; i < impactCount_; ++i) {
        if (impactDecals_[i]) impactDecals_[i]->SetVisible(false);
    }
    for (int i = 0; i < impactCount_; ++i) {
        if (impactColliders_[i]) impactColliders_[i]->SetActive(true);
    }

    // 各着弾位置の上空から弾を落下スポーン
    for (int i = 0; i < impactCount_; ++i) {
        Vector3 spawnPos(impactPositions_[i].x, impactPositions_[i].y + fallHeight_, impactPositions_[i].z);
        boss->RequestBulletSpawn(spawnPos, Vector3(0.0f, -fallSpeed_, 0.0f));
    }
}

void BTBossMeteorRain::EndImpactPhase(Boss* /*boss*/) {
    for (int i = 0; i < impactCount_; ++i) {
        if (impactDecals_[i])    impactDecals_[i]->SetVisible(false);
        if (impactColliders_[i]) impactColliders_[i]->SetActive(false);
    }
}

void BTBossMeteorRain::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("chargeTime"))            chargeTime_ = params["chargeTime"];
    if (params.contains("launchDuration"))        launchDuration_ = params["launchDuration"];
    if (params.contains("warningDuration"))       warningDuration_ = params["warningDuration"];
    if (params.contains("blinkDuration"))         blinkDuration_ = params["blinkDuration"];
    if (params.contains("impactDuration"))        impactDuration_ = params["impactDuration"];
    if (params.contains("recoveryTime"))          recoveryTime_ = params["recoveryTime"];
    if (params.contains("minImpacts"))            minImpacts_ = params["minImpacts"];
    if (params.contains("maxImpacts"))            maxImpacts_ = params["maxImpacts"];
    if (params.contains("impactRadius"))          impactRadius_ = params["impactRadius"];
    if (params.contains("damage"))                damage_ = params["damage"];
    if (params.contains("blinkFrequency"))        blinkFrequency_ = params["blinkFrequency"];
    if (params.contains("launchSpeed"))           launchSpeed_ = params["launchSpeed"];
    if (params.contains("launchSpreadXZ"))        launchSpreadXZ_ = params["launchSpreadXZ"];
    if (params.contains("fallSpeed"))             fallSpeed_ = params["fallSpeed"];
    if (params.contains("fallHeight"))            fallHeight_ = params["fallHeight"];
    if (params.contains("horizontalSpeed"))       horizontalSpeed_ = params["horizontalSpeed"];
    if (params.contains("horizontalBulletCount")) horizontalBulletCount_ = params["horizontalBulletCount"];
}

void BTBossMeteorRain::OnExtractParameters(nlohmann::json& out) const {
    out["chargeTime"]            = chargeTime_;
    out["launchDuration"]        = launchDuration_;
    out["warningDuration"]       = warningDuration_;
    out["blinkDuration"]         = blinkDuration_;
    out["impactDuration"]        = impactDuration_;
    out["recoveryTime"]          = recoveryTime_;
    out["minImpacts"]            = minImpacts_;
    out["maxImpacts"]            = maxImpacts_;
    out["impactRadius"]          = impactRadius_;
    out["damage"]                = damage_;
    out["blinkFrequency"]        = blinkFrequency_;
    out["launchSpeed"]           = launchSpeed_;
    out["launchSpreadXZ"]        = launchSpreadXZ_;
    out["fallSpeed"]             = fallSpeed_;
    out["fallHeight"]            = fallHeight_;
    out["horizontalSpeed"]       = horizontalSpeed_;
    out["horizontalBulletCount"] = horizontalBulletCount_;
}

#ifdef _DEBUG
bool BTBossMeteorRain::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Charge Time##meteor", &chargeTime_, 0.05f, 0.0f, 3.0f))         changed = true;
    if (ImGui::DragFloat("Launch Duration##meteor", &launchDuration_, 0.05f, 0.1f, 3.0f)) changed = true;
    if (ImGui::DragFloat("Warning Duration##meteor", &warningDuration_, 0.05f, 0.1f, 5.0f)) changed = true;
    if (ImGui::DragFloat("Blink Duration##meteor", &blinkDuration_, 0.05f, 0.1f, 3.0f))   changed = true;
    if (ImGui::DragFloat("Impact Duration##meteor", &impactDuration_, 0.05f, 0.1f, 3.0f)) changed = true;
    if (ImGui::DragFloat("Recovery Time##meteor", &recoveryTime_, 0.05f, 0.0f, 3.0f))     changed = true;

    ImGui::SeparatorText("Attack Parameters");
    if (ImGui::DragInt("Min Impacts##meteor", &minImpacts_, 1, 1, kMaxImpacts))           changed = true;
    if (ImGui::DragInt("Max Impacts##meteor", &maxImpacts_, 1, 1, kMaxImpacts))           changed = true;
    if (ImGui::DragFloat("Impact Radius##meteor", &impactRadius_, 0.5f, 1.0f, 20.0f))     changed = true;
    if (ImGui::DragFloat("Damage##meteor", &damage_, 0.5f, 1.0f, 50.0f))                  changed = true;
    if (ImGui::DragFloat("Blink Frequency##meteor", &blinkFrequency_, 0.5f, 1.0f, 30.0f)) changed = true;

    ImGui::SeparatorText("Launch Parameters");
    if (ImGui::DragFloat("Launch Speed##meteor", &launchSpeed_, 1.0f, 5.0f, 100.0f))      changed = true;
    if (ImGui::DragFloat("Launch Spread XZ##meteor", &launchSpreadXZ_, 0.5f, 0.0f, 20.0f)) changed = true;
    if (ImGui::DragFloat("Fall Speed##meteor", &fallSpeed_, 1.0f, 5.0f, 100.0f))          changed = true;
    if (ImGui::DragFloat("Fall Height##meteor", &fallHeight_, 1.0f, 5.0f, 100.0f))        changed = true;
    if (ImGui::DragFloat("Horizontal Speed##meteor", &horizontalSpeed_, 1.0f, 5.0f, 100.0f))                 changed = true;
    if (ImGui::DragInt("Horizontal Bullet Count##meteor", &horizontalBulletCount_, 1, 1, 20))                changed = true;

    if (minImpacts_ > maxImpacts_) {
        maxImpacts_ = minImpacts_;
        changed = true;
    }

    return changed;
}
#endif
