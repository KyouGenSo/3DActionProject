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
    Cleanup();
}

BTNodeStatus BTBossMeteorRain::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    // 初回実行時の初期化
    if (isFirstExecute_) {
        InitializeMeteorRain(boss);
        isFirstExecute_ = false;
    }

    // フェーズ管理: Charge → Launch → Warning → Blink → Impact → Recovery
    float chargeEnd = chargeTime_;
    float launchEnd = chargeEnd + launchDuration_;
    float warningEnd = launchEnd + warningDuration_;
    float blinkEnd = warningEnd + blinkDuration_;
    float impactEnd = blinkEnd + impactDuration_;

    // Phase: Charge（予備動作エフェクト）
    if (elapsedTime_ < chargeEnd) {
        bulletSignEffect_.Update(boss, deltaTime);
    }
    // Phase: Launch（弾発射演出）
    else if (elapsedTime_ < launchEnd) {
        // Charge 完了時にエフェクトを終了（1回のみ）
        if (bulletSignEffect_.IsActive()) {
            bulletSignEffect_.End(boss);
        }

        // Launch フェーズ内の経過時間（chargeEnd 基準）
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
    // Phase: Warning（Decal予兆表示）
    else if (elapsedTime_ < warningEnd) {
        // 残りの上方向弾を全て発射（Launch フェーズで撃ちきれなかった場合の保険）
        while (bulletsLaunched_ < impactCount_) {
            LaunchBullet(boss, bulletsLaunched_);
            bulletsLaunched_++;
        }
        // 残りの水平弾を全て発射（Launch フェーズで撃ちきれなかった場合の保険）
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
    // Phase: Blink（点滅警告）
    else if (elapsedTime_ < blinkEnd) {
        float phaseElapsed = elapsedTime_ - warningEnd;
        UpdateBlinkingPhase(phaseElapsed);
    }
    // Phase: Impact（着弾ダメージ判定）
    else if (elapsedTime_ < impactEnd) {
        // 着弾開始処理（1回のみ）
        if (!hasBegunImpact_) {
            BeginImpactPhase(boss);
            hasBegunImpact_ = true;
        }
    }
    // Phase: Recovery（硬直）
    else {
        // 着弾終了処理（1回のみ）
        if (!hasEndedImpact_) {
            EndImpactPhase(boss);
            hasEndedImpact_ = true;
        }

        if (!enteredRecovery_) {
            boss->EnterRecovery();
            enteredRecovery_ = true;
        }
    }

    // 経過時間を更新
    elapsedTime_ += deltaTime;

    // 状態終了チェック
    if (elapsedTime_ >= totalDuration_) {
        // 硬直フェーズ終了
        boss->ExitRecovery();

        // クリーンアップとリセット
        Cleanup();
        isFirstExecute_ = true;
        elapsedTime_ = 0.0f;
        hasBegunImpact_ = false;
        hasEndedImpact_ = false;
        enteredRecovery_ = false;
        decalsShown_ = false;
        bulletsLaunched_ = 0;
        horizontalBulletsLaunched_ = 0;
        status_ = BTNodeStatus::Success;
        return BTNodeStatus::Success;
    }

    // まだ処理中
    status_ = BTNodeStatus::Running;
    return BTNodeStatus::Running;
}

void BTBossMeteorRain::Reset() {
    BTNode::Reset();
    Cleanup();
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    hasBegunImpact_ = false;
    hasEndedImpact_ = false;
    enteredRecovery_ = false;
    decalsShown_ = false;
    bulletsLaunched_ = 0;
    horizontalBulletsLaunched_ = 0;
}

void BTBossMeteorRain::InitializeMeteorRain(Boss* boss) {
    // タイマーリセット
    elapsedTime_ = 0.0f;
    hasBegunImpact_ = false;
    hasEndedImpact_ = false;
    enteredRecovery_ = false;
    decalsShown_ = false;
    bulletsLaunched_ = 0;
    horizontalBulletsLaunched_ = 0;

    // 総時間を計算（Charge フェーズを含む）
    totalDuration_ = chargeTime_ + launchDuration_ + warningDuration_ + blinkDuration_ + impactDuration_ + recoveryTime_;

    // 予備動作エフェクト開始
    bulletSignEffect_.Start(boss, chargeTime_);

    // 着弾数を決定
    RandomEngine* rng = RandomEngine::GetInstance();
    impactCount_ = rng->GetInt(minImpacts_, maxImpacts_);
    impactCount_ = std::clamp(impactCount_, 1, kMaxImpacts);

    // ランダムな着弾位置を生成
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

    // Decal とコライダーの初期化
    impactDecals_.clear();
    impactDecals_.reserve(impactCount_);
    impactColliders_.clear();
    impactColliders_.reserve(impactCount_);

    float diameter = impactRadius_ * 2.0f;

    for (int i = 0; i < impactCount_; ++i) {
        // Decal の生成（Circle 形状）
        auto decal = std::make_unique<Decal>();
        decal->Initialize();
        decal->SetShape(DecalShape::Circle);
        decal->SetTranslate(Vector3(impactPositions_[i].x, 0.0f, impactPositions_[i].z));
        decal->SetScale(Vector3(diameter, 1.0f, diameter));
        decal->SetEdgeSoftness(0.02f);
        decal->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
        // Launch フェーズ中はまだ非表示、Warning フェーズで表示
        decal->SetVisible(false);
        impactDecals_.push_back(std::move(decal));

        // スフィアコライダーの生成（全 Transform 追加済みなのでポインタは安定）
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

void BTBossMeteorRain::GenerateImpactPositions(const Vector3& bossPos) {
    impactPositions_.clear();
    impactPositions_.reserve(impactCount_);

    RandomEngine* rng = RandomEngine::GetInstance();
    float areaSize = GameConst::kBossPhase2AreaSize;

    // 最小分離距離: 2つの着弾円が接する距離（直径分）
    float minSeparation = 2.0f * impactRadius_;
    float minSepSq = minSeparation * minSeparation;

    for (int i = 0; i < impactCount_; ++i) {
        bool placed = false;

        for (int retry = 0; retry < kMaxPlacementRetries; ++retry) {
            float x = bossPos.x + rng->GetFloat(-areaSize, areaSize);
            float z = bossPos.z + rng->GetFloat(-areaSize, areaSize);
            Vector3 candidate(x, bossPos.y, z);

            // 既存の着弾位置との距離チェック（二乗距離で比較、sqrt回避）
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

        // リトライ上限内に配置できなかった場合、残りを中止
        if (!placed) {
            impactCount_ = static_cast<int>(impactPositions_.size());
            break;
        }
    }
}

void BTBossMeteorRain::LaunchBullet(Boss* boss, int index) {
    Vector3 bossPos = boss->GetTransform().translate;
    RandomEngine* rng = RandomEngine::GetInstance();

    // 上方向に弾を発射（既存処理：yBoundaryMax_=50 を超えて自動消滅、ダメージなし）
    float spreadX = rng->GetFloat(-launchSpreadXZ_, launchSpreadXZ_);
    float spreadZ = rng->GetFloat(-launchSpreadXZ_, launchSpreadXZ_);
    boss->RequestBulletSpawn(bossPos, Vector3(spreadX, launchSpeed_, spreadZ));
}

void BTBossMeteorRain::UpdateBlinkingPhase(float phaseElapsed) {
    // sin 波で点滅（BTBossAreaAttack と同じパターン）
    float sinValue = std::abs(std::sin(phaseElapsed * blinkFrequency_ * 3.14159265f));
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;

    for (int i = 0; i < impactCount_; ++i) {
        if (impactDecals_[i]) {
            impactDecals_[i]->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
        }
    }
}

void BTBossMeteorRain::BeginImpactPhase(Boss* boss) {
    // Decal を即非表示
    for (int i = 0; i < impactCount_; ++i) {
        if (impactDecals_[i]) {
            impactDecals_[i]->SetVisible(false);
        }
    }

    // コライダーを有効化
    for (int i = 0; i < impactCount_; ++i) {
        if (impactColliders_[i]) {
            impactColliders_[i]->SetActive(true);
        }
    }

    // 各着弾位置の上空から弾を落下スポーン
    for (int i = 0; i < impactCount_; ++i) {
        Vector3 spawnPos(impactPositions_[i].x, impactPositions_[i].y + fallHeight_, impactPositions_[i].z);
        boss->RequestBulletSpawn(spawnPos, Vector3(0.0f, -fallSpeed_, 0.0f));
    }
}

void BTBossMeteorRain::EndImpactPhase(Boss* boss) {
    // Decal を非表示（既に非表示のはずだが安全のため）
    for (int i = 0; i < impactCount_; ++i) {
        if (impactDecals_[i]) {
            impactDecals_[i]->SetVisible(false);
        }
    }

    // コライダーを無効化
    for (int i = 0; i < impactCount_; ++i) {
        if (impactColliders_[i]) {
            impactColliders_[i]->SetActive(false);
        }
    }
}

void BTBossMeteorRain::Cleanup() {
    // Decal のクリーンアップ（デストラクタが DecalManager から自動削除）
    impactDecals_.clear();

    // コライダーのクリーンアップ
    for (auto& collider : impactColliders_) {
        if (collider) {
            CollisionManager::GetInstance()->RemoveCollider(collider.get());
        }
    }
    impactColliders_.clear();

    // その他のリソースをクリア
    colliderTransforms_.clear();
    impactPositions_.clear();
    impactCount_ = 0;
}

nlohmann::json BTBossMeteorRain::ExtractParameters() const {
    return {
        {"chargeTime", chargeTime_},
        {"launchDuration", launchDuration_},
        {"warningDuration", warningDuration_},
        {"blinkDuration", blinkDuration_},
        {"impactDuration", impactDuration_},
        {"recoveryTime", recoveryTime_},
        {"minImpacts", minImpacts_},
        {"maxImpacts", maxImpacts_},
        {"impactRadius", impactRadius_},
        {"damage", damage_},
        {"blinkFrequency", blinkFrequency_},
        {"launchSpeed", launchSpeed_},
        {"launchSpreadXZ", launchSpreadXZ_},
        {"fallSpeed", fallSpeed_},
        {"fallHeight", fallHeight_},
        {"horizontalSpeed", horizontalSpeed_},
        {"horizontalBulletCount", horizontalBulletCount_}
    };
}

#ifdef _DEBUG
bool BTBossMeteorRain::DrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Charge Time##meteor", &chargeTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Launch Duration##meteor", &launchDuration_, 0.05f, 0.1f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Warning Duration##meteor", &warningDuration_, 0.05f, 0.1f, 5.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Blink Duration##meteor", &blinkDuration_, 0.05f, 0.1f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Impact Duration##meteor", &impactDuration_, 0.05f, 0.1f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Recovery Time##meteor", &recoveryTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }

    ImGui::SeparatorText("Attack Parameters");
    if (ImGui::DragInt("Min Impacts##meteor", &minImpacts_, 1, 1, kMaxImpacts)) {
        changed = true;
    }
    if (ImGui::DragInt("Max Impacts##meteor", &maxImpacts_, 1, 1, kMaxImpacts)) {
        changed = true;
    }
    if (ImGui::DragFloat("Impact Radius##meteor", &impactRadius_, 0.5f, 1.0f, 20.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Damage##meteor", &damage_, 0.5f, 1.0f, 50.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Blink Frequency##meteor", &blinkFrequency_, 0.5f, 1.0f, 30.0f)) {
        changed = true;
    }

    ImGui::SeparatorText("Launch Parameters");
    if (ImGui::DragFloat("Launch Speed##meteor", &launchSpeed_, 1.0f, 5.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Launch Spread XZ##meteor", &launchSpreadXZ_, 0.5f, 0.0f, 20.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Fall Speed##meteor", &fallSpeed_, 1.0f, 5.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Fall Height##meteor", &fallHeight_, 1.0f, 5.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Horizontal Speed##meteor", &horizontalSpeed_, 1.0f, 5.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::DragInt("Horizontal Bullet Count##meteor", &horizontalBulletCount_, 1, 1, 20)) {
        changed = true;
    }

    // 制約の自動修正
    if (minImpacts_ > maxImpacts_) {
        maxImpacts_ = minImpacts_;
        changed = true;
    }

    return changed;
}
#endif
