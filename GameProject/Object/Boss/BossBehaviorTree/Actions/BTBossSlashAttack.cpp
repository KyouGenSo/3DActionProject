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
    Cleanup();
}

BTNodeStatus BTBossSlashAttack::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    // 初回実行時の初期化
    if (isFirstExecute_) {
        InitializeSlashAttack(boss, blackboard);
        isFirstExecute_ = false;
    }

    // フェーズ管理: Warning → Blinking → Attack → Recovery
    float warningEnd = warningDuration_;
    float blinkEnd = warningEnd + blinkDuration_;
    float attackEnd = blinkEnd + attackDuration_;

    // Phase: Warning（予兆表示）
    if (elapsedTime_ < warningEnd) {
        // Warning フェーズでは Decal を固定アルファで表示（初期化時に設定済み）
    }
    // Phase: Blinking（点滅警告）
    else if (elapsedTime_ < blinkEnd) {
        float phaseElapsed = elapsedTime_ - warningEnd;
        UpdateBlinkingPhase(phaseElapsed);
    }
    // Phase: Attack（斬撃発動）
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

void BTBossSlashAttack::Reset() {
    BTNode::Reset();
    Cleanup();
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    hasBegunAttack_ = false;
    hasEndedAttack_ = false;
    enteredRecovery_ = false;
}

void BTBossSlashAttack::InitializeSlashAttack(Boss* boss, BTBlackboard* blackboard) {
    // タイマーリセット
    elapsedTime_ = 0.0f;
    hasBegunAttack_ = false;
    hasEndedAttack_ = false;
    enteredRecovery_ = false;

    // 総時間を計算
    totalDuration_ = warningDuration_ + blinkDuration_ + attackDuration_ + recoveryTime_;

    // プレイヤー位置を取得・確定（発動時の位置を使い続ける）
    Vector3 targetPos = blackboard->GetPlayer()->GetTransform().translate;

    // Decal の生成（Circle 形状）
    float diameter = attackRadius_ * 2.0f;
    slashDecal_ = std::make_unique<Decal>();
    slashDecal_->Initialize();
    slashDecal_->SetShape(DecalShape::Circle);
    slashDecal_->SetTranslate(Vector3(targetPos.x, 0.0f, targetPos.z));
    slashDecal_->SetScale(Vector3(diameter, 1.0f, diameter));
    slashDecal_->SetEdgeSoftness(0.02f);
    slashDecal_->SetColor(Vector4(1.0f, 0.2f, 0.1f, kDecalBaseAlpha));
    slashDecal_->SetVisible(true);

    // コライダー用の Transform を設定
    colliderTransform_.translate = targetPos;
    colliderTransform_.rotate = Vector3(0.0f, 0.0f, 0.0f);
    colliderTransform_.scale = Vector3(1.0f, 1.0f, 1.0f);

    // スフィアコライダーの生成（MeteorImpactCollider を再利用、パリィ対応済み）
    slashCollider_ = std::make_unique<MeteorImpactCollider>(boss);
    slashCollider_->SetTransform(&colliderTransform_);
    slashCollider_->SetRadius(attackRadius_);
    slashCollider_->SetDamage(damage_);
    slashCollider_->SetOwner(boss);
    slashCollider_->SetActive(false);
    CollisionManager::GetInstance()->AddCollider(slashCollider_.get());

    // パーティクルエミッターの初期化
    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && !particleInitialized_) {
        emitterName_ = "slash_attack_0";
        emitterMgr->LoadPreset("sphere_attack_slash", emitterName_);
        emitterMgr->SetEmitterActive(emitterName_, false);
        particleInitialized_ = true;
    }
}

void BTBossSlashAttack::UpdateBlinkingPhase(float phaseElapsed) {
    // sin 波で点滅（BTBossAreaAttack と同じアルゴリズム）
    float sinValue = std::abs(std::sin(phaseElapsed * blinkFrequency_ * 3.14159265f));
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;

    if (slashDecal_) {
        slashDecal_->SetColor(Vector4(1.0f, 0.2f, 0.1f, alpha));
    }
}

void BTBossSlashAttack::BeginAttackPhase(Boss* boss) {
    // Decal を非表示
    if (slashDecal_) {
        slashDecal_->SetVisible(false);
    }

    // コライダーを有効化
    if (slashCollider_) {
        slashCollider_->SetActive(true);
    }

    // パーティクルを発動
    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particleInitialized_) {
        emitterMgr->SetEmitterPosition(emitterName_, colliderTransform_.translate);
        emitterMgr->SetEmitterRadius(emitterName_, attackRadius_);
        emitterMgr->SetEmitterActive(emitterName_, true);
    }
}

void BTBossSlashAttack::EndAttackPhase(Boss* boss) {
    // コライダーを無効化
    if (slashCollider_) {
        slashCollider_->SetActive(false);
    }

    // パーティクルを停止
    EmitterManager* emitterMgr = boss->GetEmitterManager();
    if (emitterMgr && particleInitialized_) {
        emitterMgr->SetEmitterActive(emitterName_, false);
    }
}

void BTBossSlashAttack::Cleanup() {
    // Decal のクリーンアップ（デストラクタが DecalManager から自動削除）
    slashDecal_.reset();

    // コライダーのクリーンアップ
    if (slashCollider_) {
        CollisionManager::GetInstance()->RemoveCollider(slashCollider_.get());
        slashCollider_.reset();
    }
}

nlohmann::json BTBossSlashAttack::ExtractParameters() const {
    return {
        {"warningDuration", warningDuration_},
        {"blinkDuration", blinkDuration_},
        {"attackDuration", attackDuration_},
        {"recoveryTime", recoveryTime_},
        {"attackRadius", attackRadius_},
        {"damage", damage_},
        {"blinkFrequency", blinkFrequency_}
    };
}

#ifdef _DEBUG
bool BTBossSlashAttack::DrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Warning Duration##slash", &warningDuration_, 0.05f, 0.1f, 5.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Blink Duration##slash", &blinkDuration_, 0.05f, 0.1f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Attack Duration##slash", &attackDuration_, 0.05f, 0.1f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Recovery Time##slash", &recoveryTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }

    ImGui::SeparatorText("Attack Parameters");
    if (ImGui::DragFloat("Attack Radius##slash", &attackRadius_, 0.5f, 1.0f, 30.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Damage##slash", &damage_, 0.5f, 1.0f, 50.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Blink Frequency##slash", &blinkFrequency_, 0.5f, 1.0f, 30.0f)) {
        changed = true;
    }

    return changed;
}
#endif
