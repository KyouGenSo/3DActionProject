#include "AttackState.h"
#include "PlayerStateMachine.h"
#include "../Player.h"
#include "Input/InputHandler.h"
#include "../../../Collision/MeleeAttackCollider.h"
#include "../../Boss/Boss.h"
#include "CollisionManager.h"
#include "Object3d.h"
#include "GlobalVariables.h"

#include <format>
#include <numbers>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

//=========================================================================================
// データ読み込み
//=========================================================================================

void AttackState::LoadComboData()
{
    GlobalVariables* gv = GlobalVariables::GetInstance();

    // 基本パラメータの読み込み
    maxSearchTime_ = gv->GetValueFloat("AttackState", "SearchTime");
    maxMoveTime_ = gv->GetValueFloat("AttackState", "MoveTime");
    blockRadius_ = gv->GetValueFloat("AttackState", "BlockRadius");
    blockScale_ = gv->GetValueFloat("AttackState", "BlockScale");
    recoveryTime_ = gv->GetValueFloat("AttackState", "RecoveryTime");
    maxCombo_ = gv->GetValueInt("AttackState", "MaxCombo");

    // 各コンボデータの読み込み
    for (int i = 0; i < kMaxComboCount; ++i) {
        std::string prefix = std::format("Combo{}_", i);

        combos_[i].startAngle = gv->GetValueFloat("AttackState", prefix + "StartAngle");
        combos_[i].swingAngle = gv->GetValueFloat("AttackState", prefix + "SwingAngle");
        combos_[i].swingDirection = gv->GetValueFloat("AttackState", prefix + "SwingDirection");
        combos_[i].attackDuration = gv->GetValueFloat("AttackState", prefix + "AttackDuration");
        combos_[i].damage = gv->GetValueFloat("AttackState", prefix + "Damage");

        int axisValue = gv->GetValueInt("AttackState", prefix + "Axis");
        combos_[i].axis = (axisValue == 0) ? SwingAxis::Horizontal : SwingAxis::Vertical;
    }
}

//=========================================================================================
// 状態遷移
//=========================================================================================

void AttackState::Enter(Player* player)
{
    // GlobalVariables からコンボデータを読み込み
    LoadComboData();

    // 状態を初期化
    phaseTimer_ = 0.0f;
    hasBufferedInput_ = false;
    phase_ = SearchTarget;
    targetEnemy_ = nullptr;

    // コンボ攻撃の初期化
    InitializeComboAttack(player);

    // MoveToTarget 状態をリセット
    player->ResetMoveToTarget();
}

void AttackState::InitializeComboAttack(Player* player)
{
    // 攻撃アニメーションを再生
    // TODO: アニメーション作成後に実装
    // player->GetModel()->PlayAnimation("Attack" + std::to_string(comboIndex_));

    const ComboData& currentCombo = combos_[comboIndex_];

    // 攻撃範囲 Collider を有効化
    if (player->GetMeleeAttackCollider()) {
        player->GetMeleeAttackCollider()->SetActive(true);
        player->GetMeleeAttackCollider()->Reset();
        player->GetMeleeAttackCollider()->SetKnockbackEnabled(comboIndex_ == maxCombo_ - 1);
        player->GetMeleeAttackCollider()->SetAttackDamage(currentCombo.damage);
    }

    // 攻撃ブロックを表示して初期位置設定
    if (player->GetAttackBlock()) {
        player->SetAttackBlockVisible(true);

        // 現在のコンボの開始角度を設定
        blockAngle_ = currentCombo.startAngle;

        UpdateBlockPosition(player);
    }
}

void AttackState::TransitionToPhase(AttackPhase newPhase)
{
    phase_ = newPhase;
    phaseTimer_ = 0.0f;
}

void AttackState::Exit(Player* player)
{
    // 攻撃範囲 Collider を無効化
    if (player->GetMeleeAttackCollider()) {
        player->GetMeleeAttackCollider()->SetActive(false);
    }

    // 攻撃ブロックを非表示
    player->SetAttackBlockVisible(false);

    // コンボインデックスをリセット（途中離脱でもリセット）
    comboIndex_ = 0;
    hasBufferedInput_ = false;

    targetEnemy_ = nullptr;
}

//=========================================================================================
// 更新処理
//=========================================================================================

void AttackState::Update(Player* player, float deltaTime)
{
    // GlobalVariables から値を同期（ホットリロード対応）
    LoadComboData();

    switch (phase_) {
    case SearchTarget:
        phaseTimer_ += deltaTime;

        // 毎フレーム敵を検索
        SearchForTarget(player);

        // 一定時間待機後、次のフェーズへ
        if (phaseTimer_ >= maxSearchTime_) {
            if (targetEnemy_) {
                TransitionToPhase(MoveToTarget);
            }
            else {
                TransitionToPhase(ExecuteAttack);
            }
        }
        break;

    case MoveToTarget:
        ProcessMoveToTarget(player, deltaTime);
        break;

    case ExecuteAttack:
        ProcessExecuteAttack(player, deltaTime);
        break;

    case Recovery:
        ProcessRecovery(player, deltaTime);
        break;
    }
}

void AttackState::HandleInput(Player* player)
{
    InputHandler* input = player->GetInputHandler();
    if (!input) return;

    // 攻撃実行中にプリインプットを受け付け
    // 次のコンボへ進めるかどうかは OnExecuteAttackComplete で判定
    if (phase_ == ExecuteAttack && input->IsAttacking() && comboIndex_ < maxCombo_ - 1) {
        hasBufferedInput_ = true;
    }
}

//=========================================================================================
// フェーズ処理
//=========================================================================================

void AttackState::SearchForTarget(Player* player)
{
    if (!player->GetMeleeAttackCollider()) return;

    targetEnemy_ = player->GetMeleeAttackCollider()->GetDetectedEnemy();
}

void AttackState::ProcessMoveToTarget(Player* player, float deltaTime)
{
    if (!targetEnemy_) {
        TransitionToPhase(ExecuteAttack);
        return;
    }

    phaseTimer_ += deltaTime;

    // 移動実行
    player->MoveToTarget(targetEnemy_, deltaTime);

    // ターゲットとの現在距離をチェック
    Vector3 toTarget = targetEnemy_->GetTransform().translate - player->GetTransform().translate;
    toTarget.y = 0.0f;  // 水平距離のみ
    float currentDistance = toTarget.Length();

    // ターゲットが攻撃範囲外に離れた場合、移動を終止してその場で攻撃
    if (currentDistance > player->GetAttackMinDistance()) {
        player->ResetMoveToTarget();
        TransitionToPhase(ExecuteAttack);
        return;
    }

    // 位置ベースの終了判定または最大移動時間超過
    if (player->HasReachedTarget() || phaseTimer_ >= maxMoveTime_) {
        TransitionToPhase(ExecuteAttack);
    }
}

void AttackState::ProcessExecuteAttack(Player* player, float deltaTime)
{
    phaseTimer_ += deltaTime;

    const ComboData& currentCombo = combos_[comboIndex_];

    // ブロックを回転させる
    float angularVelocity = currentCombo.swingAngle / currentCombo.attackDuration;
    blockAngle_ += angularVelocity * deltaTime * currentCombo.swingDirection;
    UpdateBlockPosition(player);

    if (player->GetMeleeAttackCollider()) {
        // ダメージ判定（攻撃中は常に有効）
        player->GetMeleeAttackCollider()->Damage();
    }

    // 攻撃時間が終了したら次の処理へ
    if (phaseTimer_ >= currentCombo.attackDuration) {
        OnExecuteAttackComplete(player);
    }
}

void AttackState::OnExecuteAttackComplete(Player* player)
{
    // プリインプットがあり、まだ次のコンボがある場合
    if (hasBufferedInput_ && comboIndex_ < maxCombo_ - 1) {
        // 次のコンボへ
        comboIndex_++;
        hasBufferedInput_ = false;
        TransitionToPhase(SearchTarget);
        InitializeComboAttack(player);
        return;
    }

    // 4コンボ完走時（comboIndex_ == 3）は硬直へ
    if (comboIndex_ >= maxCombo_ - 1) {
        TransitionToPhase(Recovery);
        return;
    }

    // それ以外（途中離脱）は即座に Idle へ（硬直なし）
    PlayerStateMachine* stateMachine = player->GetStateMachine();
    if (stateMachine) {
        stateMachine->ChangeState("Idle");
    }
}

void AttackState::ProcessRecovery(Player* player, float deltaTime)
{
    phaseTimer_ += deltaTime;

    // 硬直時間が終了したら Idle へ
    if (phaseTimer_ >= recoveryTime_) {
        PlayerStateMachine* stateMachine = player->GetStateMachine();
        if (stateMachine) {
            stateMachine->ChangeState("Idle");
        }
    }
}

//=========================================================================================
// ブロック位置更新
//=========================================================================================

void AttackState::UpdateBlockPosition(Player* player)
{
    Object3d* block = player->GetAttackBlock();
    if (!block) return;

    const ComboData& currentCombo = combos_[comboIndex_];
    Vector3 playerPos = player->GetTranslate();
    float playerRotY = player->GetRotate().y;

    Vector3 blockPos;

    if (currentCombo.axis == SwingAxis::Horizontal) {
        // 水平回転（XZ 平面）：コンボ0, 1, 3
        float worldAngle = playerRotY + blockAngle_;
        blockPos = {
            playerPos.x + sinf(worldAngle) * blockRadius_,
            playerPos.y,
            playerPos.z + cosf(worldAngle) * blockRadius_
        };
    }
    else {
        // 垂直回転（プレイヤー向き基準の YZ 平面）：コンボ2（縦切り）
        // blockAngle_は垂直角度として使用（π/2が真上、-π/2が真下）
        float horizontalDist = cosf(blockAngle_) * blockRadius_;
        float verticalDist = sinf(blockAngle_) * blockRadius_;

        blockPos = {
            playerPos.x + sinf(playerRotY) * horizontalDist,
            playerPos.y + verticalDist,
            playerPos.z + cosf(playerRotY) * horizontalDist
        };
    }

    // ブロックのトランスフォーム設定
    Transform blockTransform;
    blockTransform.translate = blockPos;

    // 回転はプレイヤーの向きに合わせる
    if (currentCombo.axis == SwingAxis::Horizontal) {
        blockTransform.rotate = { 0.0f, playerRotY + blockAngle_, 0.0f };
    }
    else {
        // 垂直回転時は X 軸回転も加える
        blockTransform.rotate = { blockAngle_, playerRotY, 0.0f };
    }

    blockTransform.scale = { blockScale_, blockScale_, blockScale_ };

    block->SetTransform(blockTransform);
}

//=========================================================================================
// デバッグ表示
//=========================================================================================

void AttackState::DrawImGui(Player* player)
{
#ifdef _DEBUG
    ImGui::Text("=== Attack State Details ===");
    ImGui::Separator();

    // フェーズ情報
    const char* phaseNames[] = { "SearchTarget", "MoveToTarget", "ExecuteAttack", "Recovery" };
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Phase: %s", phaseNames[phase_]);

    // コンボ情報
    if (ImGui::TreeNode("Combo System")) {
        ImGui::Text("Combo Index: %d / %d", comboIndex_ + 1, maxCombo_);

        // コンボ進行バー
        float comboProgress = static_cast<float>(comboIndex_ + 1) / static_cast<float>(maxCombo_);
        ImGui::ProgressBar(comboProgress, ImVec2(-1, 0), "Combo Progress");

        // プリインプット状態
        if (hasBufferedInput_) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Buffered Input: YES");
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Buffered Input: NO");
        }

        ImGui::TreePop();
    }

    // 現在のコンボデータ
    if (ImGui::TreeNode("Current Combo Data")) {
        const ComboData& currentCombo = combos_[comboIndex_];

        const char* axisNames[] = { "Horizontal", "Vertical" };
        ImGui::Text("Axis: %s", axisNames[static_cast<int>(currentCombo.axis)]);
        ImGui::Text("Start Angle: %.2f rad (%.1f deg)",
            currentCombo.startAngle,
            currentCombo.startAngle * 180.0f / std::numbers::pi_v<float>);
        ImGui::Text("Swing Angle: %.2f rad (%.1f deg)",
            currentCombo.swingAngle,
            currentCombo.swingAngle * 180.0f / std::numbers::pi_v<float>);
        ImGui::Text("Direction: %s", currentCombo.swingDirection > 0 ? "+" : "-");
        ImGui::Text("Duration: %.2f s", currentCombo.attackDuration);
        ImGui::Text("Damage: %.1f", currentCombo.damage);

        // 攻撃進行バー
        if (phase_ == ExecuteAttack) {
            float attackProgress = phaseTimer_ / currentCombo.attackDuration;
            ImGui::ProgressBar(attackProgress, ImVec2(-1, 0), "Attack Progress");
        }

        ImGui::TreePop();
    }

    // タイマー情報
    if (ImGui::TreeNode("Timers")) {
        switch (phase_) {
        case SearchTarget:
            ImGui::Text("Search Timer: %.2f / %.2f", phaseTimer_, maxSearchTime_);
            ImGui::ProgressBar(phaseTimer_ / maxSearchTime_);
            break;
        case MoveToTarget:
            ImGui::Text("Move Timer: %.2f / %.2f", phaseTimer_, maxMoveTime_);
            ImGui::ProgressBar(phaseTimer_ / maxMoveTime_);
            break;
        case ExecuteAttack:
            ImGui::Text("Attack Timer: %.2f / %.2f", phaseTimer_, combos_[comboIndex_].attackDuration);
            ImGui::ProgressBar(phaseTimer_ / combos_[comboIndex_].attackDuration);
            break;
        case Recovery:
            ImGui::Text("Recovery Timer: %.2f / %.2f", phaseTimer_, recoveryTime_);
            ImGui::ProgressBar(phaseTimer_ / recoveryTime_);
            break;
        }

        ImGui::TreePop();
    }

    // ターゲット情報
    if (ImGui::TreeNode("Target Info")) {
        if (targetEnemy_) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Target: DETECTED");
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Target: NONE");
        }

        ImGui::TreePop();
    }

    // 全コンボデータ一覧
    if (ImGui::TreeNode("All Combo Data")) {
        for (int i = 0; i < kMaxComboCount; ++i) {
            const ComboData& combo = combos_[i];
            bool isCurrentCombo = (i == comboIndex_);

            if (isCurrentCombo) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            }

            if (ImGui::TreeNode((void*)(intptr_t)i, "Combo %d%s", i + 1, isCurrentCombo ? " (Current)" : "")) {
                const char* axisNames[] = { "Horizontal", "Vertical" };
                ImGui::Text("Axis: %s", axisNames[static_cast<int>(combo.axis)]);
                ImGui::Text("Start: %.1f deg", combo.startAngle * 180.0f / std::numbers::pi_v<float>);
                ImGui::Text("Swing: %.1f deg", combo.swingAngle * 180.0f / std::numbers::pi_v<float>);
                ImGui::Text("Dir: %s, Duration: %.2fs",
                    combo.swingDirection > 0 ? "+" : "-",
                    combo.attackDuration);
                ImGui::Text("Damage: %.1f", combo.damage);
                ImGui::TreePop();
            }

            if (isCurrentCombo) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::TreePop();
    }

    // ブロックパラメータ
    if (ImGui::TreeNode("Block Parameters")) {
        ImGui::Text("Block Radius: %.2f", blockRadius_);
        ImGui::Text("Block Scale: %.2f", blockScale_);
        ImGui::Text("Current Block Angle: %.2f rad (%.1f deg)",
            blockAngle_,
            blockAngle_ * 180.0f / std::numbers::pi_v<float>);
        ImGui::Text("Recovery Time: %.2f s", recoveryTime_);

        ImGui::TreePop();
    }
#endif
}
