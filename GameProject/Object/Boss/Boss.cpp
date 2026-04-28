#include "Boss.h"
#include <algorithm>
#include "Object3d.h"
#include "OBBCollider.h"
#include "CollisionManager.h"
#include "../../Collision/CollisionTypeIdDef.h"
#include "../../Collision/BossMeleeAttackCollider.h"
#include "WinApp.h"
#include "EnginePaths.h"
#include "BossBehaviorTree/BossBehaviorTree.h"
#include "GlobalVariables.h"
#include "EmitterManager.h"
#include "State/BossStateMachine.h"
#include "State/BossNormalState.h"
#include "State/BossStunnedState.h"
#include "State/BossRetreatingState.h"
#include "State/BossPhaseTransitionStunState.h"
#include "State/BossDeadState.h"
#include "../../CameraSystem/CameraManager.h"
#include "../Player/Player.h"

#ifdef _DEBUG
#include "BossNodeEditor/BossNodeEditor.h"
#endif

using namespace Tako;

Boss::Boss()
{
}

Boss::~Boss()
{
}

void Boss::Initialize()
{
    InitializeModel();
    InitializeHealth();
    InitializeColliders();
    InitializeEffects();
    InitializeAI();
    InitializeStateMachine();
}

void Boss::InitializeStateMachine()
{
    stateMachine_ = std::make_unique<BossStateMachine>(this);
    stateMachine_->RegisterState("Normal", std::make_unique<BossNormalState>());
    stateMachine_->RegisterState("Stunned", std::make_unique<BossStunnedState>());
    stateMachine_->RegisterState("Retreating", std::make_unique<BossRetreatingState>());
    stateMachine_->RegisterState("PhaseTransitionStun", std::make_unique<BossPhaseTransitionStunState>());
    stateMachine_->RegisterState("Dead", std::make_unique<BossDeadState>());
    stateMachine_->ChangeState("Normal");
}

void Boss::InitializeModel()
{
    model_ = std::make_unique<Object3d>();
    model_->Initialize();
    model_->SetModel(EnginePaths::ModelPath("white_cube.gltf"));
    model_->SetMaterialColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));

    transform_.translate = Vector3(0.0f, initialY_, initialZ_);
    transform_.rotate = Vector3(0.0f, 0.0f, 0.0f);
    transform_.scale = Vector3(1.0f, 1.0f, 1.0f);

    model_->SetTransform(transform_);
}

void Boss::InitializeHealth()
{
    // HP バー UI の初期化（2段バー：フェーズ1=青、フェーズ2=赤）
    hpBar_.InitializeDual(
        EnginePaths::TexturePath("white.dds"),
        Vector2(500.0f, 30.0f),
        0.65f,  // 画面 X 比率
        0.05f,  // 画面 Y 比率
        Vector4{ 0.5f, 0.5f, 1.0f, 1.0f },  // フェーズ1: 青
        Vector4{ 1.0f, 0.3f, 0.3f, 1.0f }   // フェーズ2: 赤
    );

    // フェーズマネージャーの初期化
    phaseManager_.Initialize(kMaxHp, kPhase2Threshold, kPhase2InitialHp);
}

void Boss::InitializeColliders()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    // 本体コライダーの初期化
    float bodySize = gv->GetValueFloat("Boss", "BodyColliderSize");
    bodyCollider_ = std::make_unique<OBBCollider>();
    bodyCollider_->SetTransform(&transform_);
    bodyCollider_->SetSize(Vector3(bodySize, bodySize, bodySize));
    bodyCollider_->SetOffset(Vector3(0.0f, 0.0f, 0.0f));
    bodyCollider_->SetTypeID(static_cast<uint32_t>(CollisionTypeId::BOSS));
    bodyCollider_->SetOwner(this);
    CollisionManager::GetInstance()->AddCollider(bodyCollider_.get());

    // 近接攻撃用ブロックの初期化
    meleeAttackBlock_ = std::make_unique<Object3d>();
    meleeAttackBlock_->Initialize();
    meleeAttackBlock_->SetModel(EnginePaths::ModelPath("white_cube.gltf"));
    meleeAttackBlock_->SetMaterialColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));

    // 近接攻撃コライダーの初期化
    float meleeColliderSizeX = gv->GetValueFloat("BossMeleeAttackCollider", "ColliderSizeX");
    float meleeColliderSizeY = gv->GetValueFloat("BossMeleeAttackCollider", "ColliderSizeY");
    float meleeColliderSizeZ = gv->GetValueFloat("BossMeleeAttackCollider", "ColliderSizeZ");
    float meleeOffsetZ = gv->GetValueFloat("BossMeleeAttackCollider", "OffsetZ");
    meleeAttackCollider_ = std::make_unique<BossMeleeAttackCollider>(this);
    meleeAttackCollider_->SetTransform(&transform_);
    meleeAttackCollider_->SetSize(Vector3(meleeColliderSizeX, meleeColliderSizeY, meleeColliderSizeZ));
    meleeAttackCollider_->SetOffset(Vector3(0.0f, 0.0f, meleeOffsetZ));
    meleeAttackCollider_->SetOwner(this);
    CollisionManager::GetInstance()->AddCollider(meleeAttackCollider_.get());
}

void Boss::InitializeEffects()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();
    shakeEffect_.SetDefaultDuration(gv->GetValueFloat("Boss", "ShakeDuration"));
    shakeEffect_.SetDefaultIntensity(gv->GetValueFloat("Boss", "ShakeIntensity"));
}

void Boss::InitializeAI()
{
    behaviorTree_ = std::make_unique<BossBehaviorTree>(this, player_);

#ifdef _DEBUG
    nodeEditor_ = std::make_unique<BossNodeEditor>();
    nodeEditor_->Initialize();
    BTNodePtr runtimeTree = nodeEditor_->BuildRuntimeTree();
    if (runtimeTree && behaviorTree_) {
        behaviorTree_->SetRootNode(runtimeTree);
    }
#endif
}

void Boss::Finalize()
{
    // Collider を削除
    if (bodyCollider_) {
        CollisionManager::GetInstance()->RemoveCollider(bodyCollider_.get());
    }
    if (meleeAttackCollider_) {
        CollisionManager::GetInstance()->RemoveCollider(meleeAttackCollider_.get());
    }

#ifdef _DEBUG
    // ノードエディタを明示的にクリーンアップ
    // ImGui コンテキストが有効なうちに破棄する必要がある
    // （Boss デストラクタ時点では ImGui が既に終了している可能性があるため）
    if (nodeEditor_) {
        nodeEditor_->Finalize();
        nodeEditor_.reset();
    }
#endif
}

void Boss::Update(float deltaTime)
{
    // HP バーの更新（2段バー）
    hpBar_.UpdateDual(hp_, kMaxHp, kPhase2Threshold, phaseManager_.GetPhase());

    // フェーズとライフの更新
    phaseManager_.Update(hp_);

    // 死亡判定 → Dead 状態へ遷移
    if (phaseManager_.IsDead() && stateMachine_->GetCurrentStateName() != "Dead") {
        stateMachine_->ChangeState("Dead");
    }

    // フェーズに応じてプレイヤーの射撃有効/無効を切り替え
    if (phaseManager_.GetPhase() == 1) {
        if (player_) { player_->SetShootingEnabled(true); }
    }
    else if (phaseManager_.GetPhase() == 2) {
        if (player_) { player_->SetShootingEnabled(false); }
    }

    // ステートマシンが全てを駆動（Normal 内で BT が動く）
    if (!isPause_) {
        stateMachine_->Update(deltaTime);

#ifdef _DEBUG
        // エディタが有効な場合、実行中のノードをハイライト
        if (stateMachine_->GetCurrentStateName() == "Normal" &&
            nodeEditor_ && showNodeEditor_ && behaviorTree_) {
            BTNodePtr currentNode = behaviorTree_->GetCurrentRunningNode();
            if (currentNode) {
                nodeEditor_->HighlightRunningNode(currentNode);
            }
        }
#endif
    }

    // ヒットエフェクトの更新
    static const Vector4 kOriginalColor = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
    hitFlashEffect_.Update(deltaTime, model_.get(), kOriginalColor);

    // シェイクエフェクトの更新
    shakeEffect_.Update(deltaTime);

    // モデルの更新（シェイクオフセットを適用）
    Transform renderTransform = transform_;
    renderTransform.translate += shakeEffect_.GetOffset();
    model_->SetTransform(renderTransform);
    model_->Update();
}

void Boss::Draw()
{
    model_->Draw();

    // 攻撃ブロックの描画（表示フラグが true の時のみ）
    if (meleeAttackBlockVisible_ && meleeAttackBlock_) {
        meleeAttackBlock_->Draw();
    }
}

void Boss::DrawSprite()
{
    hpBar_.Draw();
}

void Boss::OnHit(float damage, float shakeIntensityOverride)
{
    // フェーズ変更処理（準備完了なら変更）
    phaseManager_.ConsumePhaseChangeRequest();

    hp_ -= damage;
    hp_ = std::max<float>(hp_, 0.0f);

    // フェーズ移行スタン条件チェック
    // 条件: フェーズ1 && HP <= 110 && 未トリガー && スタン中でない
    if (phaseManager_.GetPhase() == 1 &&
        hp_ <= kPhaseTransitionStunThreshold &&
        !hasTriggeredPhaseTransitionStun_) {
        // HP を固定
        hp_ = kPhaseTransitionStunThreshold;
        // フェーズ移行スタンを発動
        TriggerPhaseTransitionStun();
    }

    // ヒットフラッシュエフェクト開始（白く光る）
    float hitEffectDuration = GlobalVariables::GetInstance()->GetValueFloat("Boss", "HitEffectDuration");
    hitFlashEffect_.Start(Vector4(1.0f, 1.0f, 1.0f, 1.0f), hitEffectDuration);

    // シェイクエフェクト開始
    StartShake(shakeIntensityOverride);
}

void Boss::StartShake(float intensity)
{
    shakeEffect_.Start(intensity);
}

void Boss::DrawImGui()
{
#ifdef _DEBUG

    // ===== 基本ステータス =====
    ImGui::SeparatorText("Basic Status");

    // HP 表示（数値 + プログレスバー）
    ImGui::Text("HP: %.1f / %.1f (%.1f%%)", hp_, kMaxHp, (hp_ / kMaxHp) * 100.0f);
    ImGui::ProgressBar(hp_ / kMaxHp, ImVec2(-1.0f, 0.0f), "");

    // ライフ、フェーズ
    ImGui::Text("Life: %d", phaseManager_.GetLife());
    ImGui::Text("Phase: %d", phaseManager_.GetPhase());

    // 状態フラグ（警告は赤色でハイライト）
    if (phaseManager_.IsDead()) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Dead: YES");
    }
    else {
        ImGui::Text("Dead: NO");
    }
    ImGui::SameLine();
    if (isPause_) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Paused: YES");
    }
    else {
        ImGui::Text("Paused: NO");
    }

    // ステートマシン状態
    if (stateMachine_) {
        ImGui::Text("State: %s", stateMachine_->GetCurrentStateName().c_str());
    }

    ImGui::Text("Ready to Change Phase: %s", phaseManager_.IsReadyToChangePhase() ? "YES" : "NO");

    // ===== 座標情報（折りたたみ可能） =====
    if (ImGui::CollapsingHeader("Transform")) {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)",
            transform_.translate.x, transform_.translate.y, transform_.translate.z);
        ImGui::Text("Rotation: (%.2f, %.2f, %.2f)",
            transform_.rotate.x, transform_.rotate.y, transform_.rotate.z);
        ImGui::Text("Scale: (%.2f, %.2f, %.2f)",
            transform_.scale.x, transform_.scale.y, transform_.scale.z);

        // 初期位置の調整
        ImGui::Separator();
        ImGui::Text("Initial Position (for respawn):");
        ImGui::DragFloat("Initial Y", &initialY_, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Initial Z", &initialZ_, 1.0f, -50.0f, 50.0f);
    }

    // ===== シェイクエフェクト（折りたたみ可能） =====
    if (ImGui::CollapsingHeader("Shake Effect")) {
        ImGui::Text("Is Shaking: %s", shakeEffect_.IsActive() ? "YES" : "NO");
        ImGui::Text("Timer: %.3f / %.3f", shakeEffect_.GetTimer(), shakeEffect_.GetDuration());
        Vector3 offset = shakeEffect_.GetOffset();
        ImGui::Text("Offset: (%.3f, %.3f, %.3f)", offset.x, offset.y, offset.z);

        ImGui::Separator();
        float duration = shakeEffect_.GetDefaultDuration();
        if (ImGui::DragFloat("Duration", &duration, 0.01f, 0.0f, 2.0f)) {
            shakeEffect_.SetDefaultDuration(duration);
            GlobalVariables::GetInstance()->SetValue("Boss", "ShakeDuration", duration);
        }
        float intensity = shakeEffect_.GetDefaultIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 1.0f)) {
            shakeEffect_.SetDefaultIntensity(intensity);
            GlobalVariables::GetInstance()->SetValue("Boss", "ShakeIntensity", intensity);
        }

        if (ImGui::Button("Test Shake")) {
            StartShake();
        }
    }

    // ===== セクション6: コライダー（折りたたみ可能） =====
    if (ImGui::CollapsingHeader("Collider")) {
        if (bodyCollider_) {
            ImGui::Text("Active: %s", bodyCollider_->IsActive() ? "Yes" : "No");
            ImGui::Text("Type ID: %d (Enemy)", bodyCollider_->GetTypeID());

            Vector3 offset = bodyCollider_->GetOffset();
            ImGui::Text("Offset: (%.2f, %.2f, %.2f)", offset.x, offset.y, offset.z);

            Vector3 size = bodyCollider_->GetSize();
            ImGui::Text("Size: (%.2f, %.2f, %.2f)", size.x, size.y, size.z);

            Vector3 center = bodyCollider_->GetCenter();
            ImGui::Text("Center: (%.2f, %.2f, %.2f)", center.x, center.y, center.z);
        }
    }

    // ===== ビヘイビアツリー =====
    ImGui::SeparatorText("Behavior Tree");

    // ビヘイビアツリーの制御
    if (behaviorTree_) {
        // JSON から直接ビヘイビアツリーに読み込み（デバッグ・リリース共通）
        ImGui::SameLine();
        if (ImGui::Button("Load Tree from JSON")) {
            if (behaviorTree_->LoadFromJSON("resources/Json/BossTree.json")) {
                ImGui::Text("Tree loaded successfully!");
            }
        }

        // デバッグビルド専用：ノードエディタ機能
        if (nodeEditor_) {
            ImGui::SameLine();
            if (ImGui::Button("Node Editor")) {
                showNodeEditor_ = !showNodeEditor_;
                nodeEditor_->SetVisible(showNodeEditor_);
            }

            // エディタのツリーを BehaviorTree に適用
            ImGui::SameLine();
            if (ImGui::Button("Apply Editor Tree")) {
                BTNodePtr runtimeTree = nodeEditor_->BuildRuntimeTree();
                if (runtimeTree && behaviorTree_) {
                    behaviorTree_->SetRootNode(runtimeTree);
                }
            }
        }
    }

    // HP 操作
    float tempHp = hp_;
    if (ImGui::SliderFloat("Set HP", &tempHp, 0.0f, kMaxHp)) {
        hp_ = std::clamp(tempHp, 0.0f, kMaxHp);
    }

    // フェーズ切り替え
    ImVec2 buttonSize(ImGui::GetContentRegionAvail().x * 0.48f, 0);
    if (ImGui::Button("Set Phase 1", buttonSize)) {
        SetPhase(1);
        hp_ = kMaxHp;
        phaseManager_.Reset();
    }
    ImGui::SameLine();
    if (ImGui::Button("Set Phase 2", buttonSize)) {
        SetPhase(2);
        hp_ = kPhase2InitialHp;
    }

    // 一時停止トグル
    ImGui::Spacing();
    ImGui::Checkbox("Pause Boss", &isPause_);

    // 死亡/復活コントロール
    ImGui::Spacing();
    ImVec2 fullButtonSize(ImGui::GetContentRegionAvail().x * 0.48f, 0);

    // Kill ボタン（赤色）
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("Kill Boss", fullButtonSize)) {
        phaseManager_.SetDead(true);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Revive ボタン（緑色）
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
    if (ImGui::Button("Revive Boss", fullButtonSize)) {
        phaseManager_.Reset();
        hp_ = kMaxHp;
    }
    ImGui::PopStyleColor(3);

    // ノードエディタの更新
    if (nodeEditor_ && showNodeEditor_) {
        nodeEditor_->Update();
    }

#endif
}

void Boss::RequestBulletSpawn(const Vector3& position, const Vector3& velocity) {
    bulletSpawner_.RequestSpawn(position, velocity);
}

std::vector<BulletSpawnRequest> Boss::ConsumePendingBullets() {
    return bulletSpawner_.Consume();
}

void Boss::RequestPenetratingBulletSpawn(const Vector3& position, const Vector3& velocity) {
    penetratingBulletSpawner_.RequestSpawn(position, velocity);
}

std::vector<BulletSpawnRequest> Boss::ConsumePendingPenetratingBullets() {
    return penetratingBulletSpawner_.Consume();
}

void Boss::SetPlayer(Player* player) {
    player_ = player;
    if (behaviorTree_) {
        behaviorTree_->SetPlayer(player);
    }
}

void Boss::SetAttackSignEmitterActive(bool active) {
    if (emitterManager_) {
        emitterManager_->SetEmitterActive(attackSignEmitterName_, active);
    }
}

void Boss::SetAttackSignEmitterPosition(const Vector3& position) {
    if (emitterManager_) {
        emitterManager_->SetEmitterPosition(attackSignEmitterName_, position);
    }
}

void Boss::SetBulletSignEmitterActive(bool active) {
    if (emitterManager_) {
        emitterManager_->SetEmitterActive(bulletSignEmitterName_, active);
    }
}

void Boss::SetBulletSignEmitterPosition(const Vector3& position) {
    if (emitterManager_) {
        emitterManager_->SetEmitterPosition(bulletSignEmitterName_, position);
    }
}

void Boss::SetBulletSignEmitterScaleRangeX(float value) {
    if (emitterManager_) {
        emitterManager_->SetEmitterScaleRange(
            bulletSignEmitterName_,
            Vector2(value, value),
            Vector2(1.0f, 1.0f)
        );
    }
}

void Boss::StartStunFlash(const Vector4& color, float duration) {
    hitFlashEffect_.Start(color, duration);
}

void Boss::EnterRecovery() {
    isInRecovery_ = true;
}

void Boss::ExitRecovery() {
    isInRecovery_ = false;
}

void Boss::SetDashing(bool dashing) {
    isDashing_ = dashing;
}

bool Boss::IsStunned() const {
    if (!stateMachine_) return false;
    const std::string& state = stateMachine_->GetCurrentStateName();
    return state == "Stunned" || state == "PhaseTransitionStun";
}

bool Boss::IsInPhaseTransitionStun() const {
    if (!stateMachine_) return false;
    return stateMachine_->GetCurrentStateName() == "PhaseTransitionStun";
}

void Boss::ResetActionState() {
    isInRecovery_ = false;
    isDashing_ = false;
    meleeAttackBlockVisible_ = false;
    SetAttackSignEmitterActive(false);
    SetBulletSignEmitterActive(false);
    if (meleeAttackCollider_) {
        meleeAttackCollider_->SetActive(false);
    }
}

void Boss::OnMeleeAttackHit(float damage, const Vector3& knockbackDir, bool isKnockbackCombo) {
    // ダッシュ中は無視
    if (isDashing_) return;

    const std::string& currentState = stateMachine_->GetCurrentStateName();

    if (currentState == "Stunned") {
        // スタン中: ダメージのみ（+ 4コンボ目ならノックバック有効化）
        OnHit(damage, 1.0f);
        if (isKnockbackCombo) {
            auto* stunnedState = static_cast<BossStunnedState*>(
                stateMachine_->GetState("Stunned"));
            stunnedState->EnableKnockback(knockbackDir);
        }
        CameraManager::GetInstance()->StartShake(0.3f);
        return;
    }

    if (currentState == "PhaseTransitionStun") {
        // フェーズ移行スタン中: フェーズ2へ移行
        CompletePhaseTransition();
        CameraManager::GetInstance()->StartShake(0.3f);
        return;
    }

    if (currentState == "Normal") {
        if (isInRecovery_) {
            // 硬直中: ダメージ + スタンへ遷移
            OnHit(damage, 1.0f);
            CameraManager::GetInstance()->StartShake(0.3f);
            pendingStunDirection_ = knockbackDir;
            pendingStunWithKnockback_ = isKnockbackCombo;
            stateMachine_->ChangeState("Stunned");
        }
        else {
            // 非硬直: 離脱へ遷移
            stateMachine_->ChangeState("Retreating");
        }
        return;
    }
}

void Boss::TriggerPhaseTransitionStun() {
    if (hasTriggeredPhaseTransitionStun_) {
        return;
    }
    hasTriggeredPhaseTransitionStun_ = true;

    // ステートマシンで PhaseTransitionStun 状態へ遷移
    stateMachine_->ChangeState("PhaseTransitionStun");
}

void Boss::CompletePhaseTransition() {
    // HP を100に設定
    hp_ = kPhase2InitialHp;

    // フェーズ2に移行
    phaseManager_.SetPhase(2);

    // Normal へ復帰（PhaseTransitionStunState::Exit()でパーティクル無効化）
    stateMachine_->ChangeState("Normal");
}

void Boss::SetCanAttackSignEmitterActive(bool active) {
    if (emitterManager_) {
        emitterManager_->SetEmitterActive(canAttackSignEmitterName_, active);
    }
}

void Boss::SetCanAttackSignEmitterPosition(const Vector3& position) {
    if (emitterManager_) {
        emitterManager_->SetEmitterPosition(canAttackSignEmitterName_, position);
    }
}