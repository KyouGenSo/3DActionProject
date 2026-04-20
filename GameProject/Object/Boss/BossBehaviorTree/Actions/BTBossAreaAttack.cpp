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
    Cleanup();
}

BTNodeStatus BTBossAreaAttack::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    // 初回実行時の初期化
    if (isFirstExecute_) {
        InitializeAreaAttack(blackboard);
        isFirstExecute_ = false;
    }

    // フェーズ管理: Warning → Blinking → Attack → Recovery
    float warningEnd = warningDuration_;
    float blinkEnd = warningEnd + blinkDuration_;
    float attackEnd = blinkEnd + attackDuration_;

    // Phase: Warning（予兆表示）
    if (elapsedTime_ < warningEnd) {
        UpdateWarningPhase();
    }
    // Phase: Blinking（点滅警告）
    else if (elapsedTime_ < blinkEnd) {
        float phaseElapsed = elapsedTime_ - warningEnd;
        UpdateBlinkingPhase(phaseElapsed);
    }
    // Phase: Attack（攻撃発動）
    else if (elapsedTime_ < attackEnd) {
        // 攻撃開始処理（1回のみ）
        if (!hasBegunAttack_) {
            BeginAttackPhase(boss);
            hasBegunAttack_ = true;
        }
    }
    // Phase: Recovery（硬直）
    else {
        // 攻撃終了処理（1回のみ）
        if (!hasEndedAttack_) {
            EndAttackPhase(boss);
            hasEndedAttack_ = true;
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
        hasBegunAttack_ = false;
        hasEndedAttack_ = false;
        enteredRecovery_ = false;
        status_ = BTNodeStatus::Success;
        return BTNodeStatus::Success;
    }

    // まだ処理中
    status_ = BTNodeStatus::Running;
    return BTNodeStatus::Running;
}

void BTBossAreaAttack::Reset() {
    BTNode::Reset();

    // パーティクルエミッターを停止（攻撃中に状態がリセットされた場合の安全策）
    if (cachedEmitterMgr_ && particlesInitialized_ && hasBegunAttack_ && !hasEndedAttack_) {
        for (int i = 0; i < kQuadrantCount; ++i) {
            if (activeQuadrants_[i]) {
                cachedEmitterMgr_->SetEmitterActive(emitterNames_[i], false);
            }
        }
    }

    Cleanup();
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    hasBegunAttack_ = false;
    hasEndedAttack_ = false;
    enteredRecovery_ = false;
}

void BTBossAreaAttack::InitializeAreaAttack(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();

    // タイマーリセット
    elapsedTime_ = 0.0f;
    hasBegunAttack_ = false;
    hasEndedAttack_ = false;
    enteredRecovery_ = false;

    // 総時間を計算
    totalDuration_ = warningDuration_ + blinkDuration_ + attackDuration_ + recoveryTime_;

    // ランダムに攻撃象限を選択（プレイヤー象限を確定枠として含める）
    SelectRandomQuadrants(blackboard);

    // ボスの位置を取得
    Vector3 bossPos = boss->GetTransform().translate;
    float halfArea = GameConst::kBossPhase2AreaSize * 0.5f;

    // 各象限の Decal・コライダー・パーティクルを初期化
    for (int i = 0; i < kQuadrantCount; ++i) {
        Vector3 center = GetQuadrantCenter(i, bossPos);

        // Decal の生成
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

        // コライダー用の Transform を設定
        colliderTransforms_[i].translate = center;
        colliderTransforms_[i].rotate = Vector3(0.0f, 0.0f, 0.0f);
        colliderTransforms_[i].scale = Vector3(1.0f, 1.0f, 1.0f);

        // コライダーの生成
        quadrantColliders_[i] = std::make_unique<BossAreaAttackCollider>(boss);
        quadrantColliders_[i]->SetTransform(&colliderTransforms_[i]);
        quadrantColliders_[i]->SetSize(Vector3(halfArea * 2.0f, kColliderHeight, halfArea * 2.0f));
        quadrantColliders_[i]->SetDamage(damage_);
        quadrantColliders_[i]->SetOwner(boss);
        quadrantColliders_[i]->SetActive(false);
        CollisionManager::GetInstance()->AddCollider(quadrantColliders_[i].get());
    }

    // パーティクルエミッターの初期化
    EmitterManager* emitterMgr = boss->GetEmitterManager();
    cachedEmitterMgr_ = emitterMgr;  // Reset時にboss参照を取れないためキャッシュする
    if (emitterMgr && !particlesInitialized_) {
        for (int i = 0; i < kQuadrantCount; ++i) {
            emitterNames_[i] = "area_attack_q" + std::to_string(i);
            emitterMgr->LoadPreset("attack_slash", emitterNames_[i]);
            emitterMgr->SetEmitterActive(emitterNames_[i], false);
        }
        particlesInitialized_ = true;
    }
}

void BTBossAreaAttack::Cleanup() {
    // Decal のクリーンアップ（デストラクタが DecalManager から自動削除）
    for (int i = 0; i < kQuadrantCount; ++i) {
        quadrantDecals_[i].reset();
    }

    // コライダーのクリーンアップ
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (quadrantColliders_[i]) {
            CollisionManager::GetInstance()->RemoveCollider(quadrantColliders_[i].get());
            quadrantColliders_[i].reset();
        }
    }

    // 象限フラグをリセット
    activeQuadrants_.fill(false);
}

void BTBossAreaAttack::SelectRandomQuadrants(BTBlackboard* blackboard) {
    RandomEngine* rng = RandomEngine::GetInstance();

    // 攻撃する象限数を決定（minQuadrants_ ~ maxQuadrants_）
    int count = rng->GetInt(minQuadrants_, maxQuadrants_);
    count = std::clamp(count, 1, kQuadrantCount);

    // 全象限をリセット
    activeQuadrants_.fill(false);

    // プレイヤー象限を確定枠として設定
    int playerQuadrant = GetPlayerQuadrant(blackboard);
    activeQuadrants_[playerQuadrant] = true;

    // 残り象限から (count - 1) 個を追加選択
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

        // 先頭から (count - 1) 個を有効化
        for (int i = 0; i < count - 1; ++i) {
            activeQuadrants_[remaining[i]] = true;
        }
    }
}

int BTBossAreaAttack::GetPlayerQuadrant(BTBlackboard* blackboard) const {
    Boss* boss = blackboard->GetBoss();
    Player* player = blackboard->GetPlayer();
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 playerPos = player->GetTransform().translate;

    // 象限マッピング（GetQuadrantCenterの符号ロジックの逆算）:
    //   Q0(-X,-Z) Q1(+X,-Z)
    //   Q2(-X,+Z) Q3(+X,+Z)
    int xIndex = (playerPos.x >= bossPos.x) ? 1 : 0;
    int zIndex = (playerPos.z >= bossPos.z) ? 1 : 0;
    return zIndex * 2 + xIndex;
}

Vector3 BTBossAreaAttack::GetQuadrantCenter(int quadrantIndex, const Vector3& bossPos) const {
    float halfArea = GameConst::kBossPhase2AreaSize * 0.5f;

    // Q0: 左奥 (-X, -Z), Q1: 右奥 (+X, -Z), Q2: 左手前 (-X, +Z), Q3: 右手前 (+X, +Z)
    float xSign = (quadrantIndex % 2 == 0) ? -1.0f : 1.0f;
    float zSign = (quadrantIndex < 2) ? -1.0f : 1.0f;

    return Vector3(
        bossPos.x + xSign * halfArea,
        bossPos.y,
        bossPos.z + zSign * halfArea
    );
}

void BTBossAreaAttack::UpdateWarningPhase() {
    // Warning フェーズでは Decal を固定アルファで表示（初期化時に設定済み）
}

void BTBossAreaAttack::UpdateBlinkingPhase(float phaseElapsed) {
    // sin 波で点滅（0 ～ 1 の範囲で振動）
    float sinValue = std::abs(std::sin(phaseElapsed * blinkFrequency_ * 3.14159265f));
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;

    for (int i = 0; i < kQuadrantCount; ++i) {
        if (activeQuadrants_[i] && quadrantDecals_[i]) {
            quadrantDecals_[i]->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
        }
    }
}

void BTBossAreaAttack::BeginAttackPhase(Boss* boss) {
    // Decal を非表示
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (activeQuadrants_[i] && quadrantDecals_[i]) {
            quadrantDecals_[i]->SetVisible(false);
        }
    }

    // コライダーを有効化
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (activeQuadrants_[i] && quadrantColliders_[i]) {
            quadrantColliders_[i]->SetActive(true);
        }
    }

    // パーティクルを発動
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
    // Decal を非表示
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (quadrantDecals_[i]) {
            quadrantDecals_[i]->SetVisible(false);
        }
    }

    // コライダーを無効化
    for (int i = 0; i < kQuadrantCount; ++i) {
        if (quadrantColliders_[i]) {
            quadrantColliders_[i]->SetActive(false);
        }
    }

    // パーティクルを停止
    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particlesInitialized_) {
        for (int i = 0; i < kQuadrantCount; ++i) {
            emitterMgr->SetEmitterActive(emitterNames_[i], false);
        }
    }
}

nlohmann::json BTBossAreaAttack::ExtractParameters() const {
    return {
        {"warningDuration", warningDuration_},
        {"blinkDuration", blinkDuration_},
        {"attackDuration", attackDuration_},
        {"recoveryTime", recoveryTime_},
        {"minQuadrants", minQuadrants_},
        {"maxQuadrants", maxQuadrants_},
        {"damage", damage_},
        {"blinkFrequency", blinkFrequency_}
    };
}

#ifdef _DEBUG
bool BTBossAreaAttack::DrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Warning Duration##area", &warningDuration_, 0.05f, 0.1f, 5.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Blink Duration##area", &blinkDuration_, 0.05f, 0.1f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Attack Duration##area", &attackDuration_, 0.05f, 0.1f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Recovery Time##area", &recoveryTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }

    ImGui::SeparatorText("Attack Parameters");
    if (ImGui::DragInt("Min Quadrants##area", &minQuadrants_, 1, 1, kQuadrantCount)) {
        changed = true;
    }
    if (ImGui::DragInt("Max Quadrants##area", &maxQuadrants_, 1, 1, kQuadrantCount)) {
        changed = true;
    }
    if (ImGui::DragFloat("Damage##area", &damage_, 0.5f, 1.0f, 50.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Blink Frequency##area", &blinkFrequency_, 0.5f, 1.0f, 30.0f)) {
        changed = true;
    }

    // 制約の自動修正
    if (minQuadrants_ > maxQuadrants_) {
        maxQuadrants_ = minQuadrants_;
        changed = true;
    }

    return changed;
}
#endif
