#include "BTBossBulletRain.h"
#include "../../Boss.h"
#include "../../Movement/BossAreaBounds.h"
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

BTBossBulletRain::BTBossBulletRain() {
    name_ = "BossBulletRain";
}

BTBossBulletRain::~BTBossBulletRain() {
    OnCleanup();
}

Tako::BTNodeStatus BTBossBulletRain::OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) {
    if (elapsedTime_ < chargeTime_) {
        bulletSignEffect_.Update(boss, deltaTime);
    }
    else {
        if (bulletSignEffect_.IsActive()) {
            bulletSignEffect_.End(boss);
        }

        // spawnInterval_ ごとに次の弾のライフサイクルを開始（処理落ち時はまとめて追い付く）
        float rainElapsed = elapsedTime_ - chargeTime_;
        while (nextSpawnIndex_ < totalSpawnCount_ &&
               rainElapsed >= static_cast<float>(nextSpawnIndex_) * spawnInterval_) {
            int slot = FindFreeSlot();
            if (slot < 0) {
                // プール枯渇時はこの1発を諦め、硬直開始時刻の事前計算との整合を優先
                nextSpawnIndex_++;
                continue;
            }
            ActivateBullet(slot, blackboard);
            nextSpawnIndex_++;
        }
    }

    for (int i = 0; i < poolSize_; ++i) {
        UpdateBullet(i, boss, deltaTime);
    }

    if (elapsedTime_ >= recoveryStart_) {
        EnterAttackRecovery(boss);
    }

    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= totalDuration_) {
        return FinishAttack();
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossBulletRain::OnInitialize(Tako::BTBlackboard* /*blackboard*/, Boss* boss) {
    nextSpawnIndex_ = 0;
    fallTime_ = fallHeight_ / (std::max)(fallSpeed_, 0.01f);

    // t=0 に1発 + spawnInterval_ ごとに1発を rainDuration_ の間続ける
    float interval = (std::max)(spawnInterval_, 0.01f);
    totalSpawnCount_ = static_cast<int>(rainDuration_ / interval) + 1;

    // スロットは同時進行の最大数 + 余裕2 だけ確保し、Done を再利用する
    float bulletLifecycle = warningDuration_ + blinkDuration_ + fallTime_ + impactActiveDuration_;
    poolSize_ = std::clamp(static_cast<int>(std::ceil(bulletLifecycle / interval)) + 2, 1, kMaxPoolSize);
    poolSize_ = (std::min)(poolSize_, totalSpawnCount_);

    // 最終弾のライフサイクル終了 = 硬直開始
    recoveryStart_ = chargeTime_ + static_cast<float>(totalSpawnCount_ - 1) * interval + bulletLifecycle;
    totalDuration_ = recoveryStart_ + recoveryTime_;

    bulletSignEffect_.Start(boss, chargeTime_);

    bullets_.assign(poolSize_, Bullet{});

    // コライダーがポインタ参照するため再確保で無効化されないよう事前確保
    colliderTransforms_.clear();
    colliderTransforms_.reserve(poolSize_);
    for (int i = 0; i < poolSize_; ++i) {
        Transform t;
        t.translate = Vector3(0.0f, 0.0f, 0.0f);
        t.rotate = Vector3(0.0f, 0.0f, 0.0f);
        t.scale = Vector3(1.0f, 1.0f, 1.0f);
        colliderTransforms_.push_back(t);
    }

    decals_.clear();
    decals_.reserve(poolSize_);
    colliders_.clear();
    colliders_.reserve(poolSize_);

    float diameter = impactRadius_ * 2.0f;

    for (int i = 0; i < poolSize_; ++i) {
        auto decal = std::make_unique<Decal>();
        decal->Initialize();
        decal->SetShape(DecalShape::Circle);
        decal->SetScale(Vector3(diameter, 1.0f, diameter));
        decal->SetEdgeSoftness(0.02f);
        decal->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
        decal->SetVisible(false);
        decals_.push_back(std::move(decal));

        auto collider = std::make_unique<MeteorImpactCollider>(boss);
        collider->SetTransform(&colliderTransforms_[i]);
        collider->SetRadius(impactRadius_);
        collider->SetDamage(damage_);
        collider->SetOwner(boss);
        collider->SetActive(false);
        CollisionManager::GetInstance()->AddCollider(collider.get());
        colliders_.push_back(std::move(collider));
    }
}

void BTBossBulletRain::OnCleanup() {
    // チャージ中に中断された場合も予兆エミッターを残さない
    if (bulletSignEffect_.IsActive() && cachedBoss_) {
        bulletSignEffect_.End(cachedBoss_);
    }

    decals_.clear();

    for (auto& collider : colliders_) {
        if (collider) {
            CollisionManager::GetInstance()->RemoveCollider(collider.get());
        }
    }
    colliders_.clear();

    colliderTransforms_.clear();
    bullets_.clear();
    poolSize_ = 0;
    totalSpawnCount_ = 0;
    nextSpawnIndex_ = 0;
}

int BTBossBulletRain::FindFreeSlot() const {
    for (int i = 0; i < poolSize_; ++i) {
        if (bullets_[i].state == BulletState::Waiting || bullets_[i].state == BulletState::Done) {
            return i;
        }
    }
    return -1;
}

void BTBossBulletRain::ActivateBullet(int index, Tako::BTBlackboard* blackboard) {
    Player* player = blackboard ? blackboard->GetPtr<Player>("player") : nullptr;
    Vector3 pos = SampleImpactPosition(player);

    bullets_[index].state = BulletState::Warning;
    bullets_[index].stateTimer = 0.0f;
    bullets_[index].impactPos = pos;

    colliderTransforms_[index].translate = Vector3(pos.x, colliderY_, pos.z);

    if (decals_[index]) {
        decals_[index]->SetTranslate(Vector3(pos.x, 0.0f, pos.z));
        decals_[index]->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
        decals_[index]->SetVisible(true);
    }
}

Tako::Vector3 BTBossBulletRain::SampleImpactPosition(Player* player) {
    RandomEngine* rng = RandomEngine::GetInstance();

    BossMovement::AreaBounds bounds{
        GameConst::kStageXMin + stageMargin_, GameConst::kStageXMax - stageMargin_,
        GameConst::kStageZMin + stageMargin_, GameConst::kStageZMax - stageMargin_
    };

    const bool aimAtPlayer = player && rng->GetNormalized() < playerAimProbability_;
    const float minSepSq = minSeparation_ * minSeparation_;

    Vector3 candidate(0.0f, 0.0f, 0.0f);
    for (int retry = 0; retry < kMaxPlacementRetries; ++retry) {
        if (aimAtPlayer) {
            Vector3 playerPos = player->GetTransform().translate;
            // sqrt で半径方向の偏りを補正した円内一様分布
            float r = playerAimRadius_ * std::sqrt(rng->GetNormalized());
            float angle = rng->GetAngle();
            candidate = BossMovement::ClampToBounds(
                Vector3(playerPos.x + std::cos(angle) * r, 0.0f, playerPos.z + std::sin(angle) * r), bounds);
        }
        else {
            candidate = Vector3(rng->GetFloat(bounds.xMin, bounds.xMax), 0.0f,
                                rng->GetFloat(bounds.zMin, bounds.zMax));
        }

        if (minSeparation_ <= 0.0f) {
            return candidate;
        }

        // 進行中の弾とだけ間隔チェック（Done 済みの場所への再着弾は許容）
        bool tooClose = false;
        for (int i = 0; i < poolSize_; ++i) {
            const Bullet& b = bullets_[i];
            if (b.state == BulletState::Waiting || b.state == BulletState::Done) {
                continue;
            }
            if (candidate.DistanceSquared(b.impactPos) < minSepSq) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            return candidate;
        }
    }

    // リトライ回数制限オーバーした場合は重なりを許容して採用（弾数は減らさない）
    return candidate;
}

void BTBossBulletRain::UpdateBullet(int index, Boss* boss, float deltaTime) {
    Bullet& b = bullets_[index];

    switch (b.state) {
    case BulletState::Waiting:
    case BulletState::Done:
        return;

    case BulletState::Warning:
        if (b.stateTimer >= warningDuration_) {
            b.state = BulletState::Blinking;
            b.stateTimer = 0.0f;
        }
        break;

    case BulletState::Blinking: {
        float sinValue = std::abs(std::sin(b.stateTimer * blinkFrequency_ * 3.14159265f));
        float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;
        if (decals_[index]) {
            decals_[index]->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
        }
        if (b.stateTimer >= blinkDuration_) {
            b.state = BulletState::Falling;
            b.stateTimer = 0.0f;
            if (decals_[index]) {
                decals_[index]->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalFallingAlpha));
            }
            boss->RequestBulletSpawn(Vector3(b.impactPos.x, fallHeight_, b.impactPos.z),
                                     Vector3(0.0f, -fallSpeed_, 0.0f));
        }
        break;
    }

    case BulletState::Falling:
        // 見た目の弾が地面へ到達する瞬間 (fallTime_ 経過) に判定を有効化
        if (b.stateTimer >= fallTime_) {
            b.state = BulletState::Impact;
            b.stateTimer = 0.0f;
            if (decals_[index]) {
                decals_[index]->SetVisible(false);
            }
            if (colliders_[index]) {
                colliders_[index]->SetActive(true);
            }
        }
        break;

    case BulletState::Impact:
        if (b.stateTimer >= impactActiveDuration_) {
            b.state = BulletState::Done;
            if (colliders_[index]) {
                colliders_[index]->SetActive(false);
            }
        }
        break;
    }

    b.stateTimer += deltaTime;
}

void BTBossBulletRain::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("chargeTime"))           chargeTime_ = params["chargeTime"];
    if (params.contains("rainDuration"))         rainDuration_ = params["rainDuration"];
    if (params.contains("spawnInterval"))        spawnInterval_ = params["spawnInterval"];
    if (params.contains("warningDuration"))      warningDuration_ = params["warningDuration"];
    if (params.contains("blinkDuration"))        blinkDuration_ = params["blinkDuration"];
    if (params.contains("blinkFrequency"))       blinkFrequency_ = params["blinkFrequency"];
    if (params.contains("fallHeight"))           fallHeight_ = params["fallHeight"];
    if (params.contains("fallSpeed"))            fallSpeed_ = params["fallSpeed"];
    if (params.contains("impactRadius"))         impactRadius_ = params["impactRadius"];
    if (params.contains("impactActiveDuration")) impactActiveDuration_ = params["impactActiveDuration"];
    if (params.contains("colliderY"))            colliderY_ = params["colliderY"];
    if (params.contains("damage"))               damage_ = params["damage"];
    if (params.contains("recoveryTime"))         recoveryTime_ = params["recoveryTime"];
    if (params.contains("playerAimProbability")) playerAimProbability_ = params["playerAimProbability"];
    if (params.contains("playerAimRadius"))      playerAimRadius_ = params["playerAimRadius"];
    if (params.contains("stageMargin"))          stageMargin_ = params["stageMargin"];
    if (params.contains("minSeparation"))        minSeparation_ = params["minSeparation"];
}

void BTBossBulletRain::OnExtractParameters(nlohmann::json& out) const {
    out["chargeTime"]           = chargeTime_;
    out["rainDuration"]         = rainDuration_;
    out["spawnInterval"]        = spawnInterval_;
    out["warningDuration"]      = warningDuration_;
    out["blinkDuration"]        = blinkDuration_;
    out["blinkFrequency"]       = blinkFrequency_;
    out["fallHeight"]           = fallHeight_;
    out["fallSpeed"]            = fallSpeed_;
    out["impactRadius"]         = impactRadius_;
    out["impactActiveDuration"] = impactActiveDuration_;
    out["colliderY"]            = colliderY_;
    out["damage"]               = damage_;
    out["recoveryTime"]         = recoveryTime_;
    out["playerAimProbability"] = playerAimProbability_;
    out["playerAimRadius"]      = playerAimRadius_;
    out["stageMargin"]          = stageMargin_;
    out["minSeparation"]        = minSeparation_;
}

#ifdef _DEBUG
bool BTBossBulletRain::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Charge Time##bulletRain", &chargeTime_, 0.05f, 0.0f, 3.0f))           changed = true;
    if (ImGui::DragFloat("Warning Duration##bulletRain", &warningDuration_, 0.05f, 0.0f, 3.0f)) changed = true;
    if (ImGui::DragFloat("Blink Duration##bulletRain", &blinkDuration_, 0.05f, 0.0f, 3.0f))     changed = true;
    if (ImGui::DragFloat("Blink Frequency##bulletRain", &blinkFrequency_, 0.5f, 1.0f, 30.0f))   changed = true;
    if (ImGui::DragFloat("Recovery Time##bulletRain", &recoveryTime_, 0.05f, 0.0f, 3.0f))       changed = true;

    ImGui::SeparatorText("Spawn");
    if (ImGui::DragFloat("Rain Duration##bulletRain", &rainDuration_, 0.1f, 0.1f, 60.0f))       changed = true;
    if (ImGui::DragFloat("Spawn Interval##bulletRain", &spawnInterval_, 0.01f, 0.02f, 2.0f))    changed = true;
    float interval = (std::max)(spawnInterval_, 0.01f);
    int totalBullets = static_cast<int>(rainDuration_ / interval) + 1;
    float lifecycle = warningDuration_ + blinkDuration_ + fallHeight_ / (std::max)(fallSpeed_, 0.01f) + impactActiveDuration_;
    int pool = std::clamp(static_cast<int>(std::ceil(lifecycle / interval)) + 2, 1, kMaxPoolSize);
    ImGui::TextDisabled("Bullets: %d | Pool: %d/%d", totalBullets, (std::min)(pool, totalBullets), kMaxPoolSize);

    ImGui::SeparatorText("Fall & Impact");
    if (ImGui::DragFloat("Fall Height##bulletRain", &fallHeight_, 1.0f, 5.0f, 100.0f))          changed = true;
    if (ImGui::DragFloat("Fall Speed##bulletRain", &fallSpeed_, 1.0f, 5.0f, 200.0f))            changed = true;
    if (ImGui::DragFloat("Impact Radius##bulletRain", &impactRadius_, 0.5f, 0.5f, 20.0f))       changed = true;
    if (ImGui::DragFloat("Impact Active Duration##bulletRain", &impactActiveDuration_, 0.01f, 0.05f, 1.0f)) changed = true;
    if (ImGui::DragFloat("Collider Y##bulletRain", &colliderY_, 0.1f, 0.0f, 5.0f))              changed = true;
    if (ImGui::DragFloat("Damage##bulletRain", &damage_, 0.5f, 0.0f, 50.0f))                    changed = true;

    ImGui::SeparatorText("Targeting");
    if (ImGui::DragFloat("Player Aim Probability##bulletRain", &playerAimProbability_, 0.01f, 0.0f, 1.0f)) changed = true;
    if (ImGui::DragFloat("Player Aim Radius##bulletRain", &playerAimRadius_, 0.5f, 0.0f, 20.0f)) changed = true;
    if (ImGui::DragFloat("Stage Margin##bulletRain", &stageMargin_, 0.5f, 0.0f, 30.0f))         changed = true;
    if (ImGui::DragFloat("Min Separation##bulletRain", &minSeparation_, 0.5f, 0.0f, 20.0f))     changed = true;

    return changed;
}
#endif
