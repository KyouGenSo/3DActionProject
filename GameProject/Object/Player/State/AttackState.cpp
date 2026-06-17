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

    maxSearchTime_ = gv->GetValueFloat("AttackState", "SearchTime");
    maxMoveTime_ = gv->GetValueFloat("AttackState", "MoveTime");
    blockRadius_ = gv->GetValueFloat("AttackState", "BlockRadius");
    blockScale_ = gv->GetValueFloat("AttackState", "BlockScale");
    recoveryTime_ = gv->GetValueFloat("AttackState", "RecoveryTime");
    maxCombo_ = gv->GetValueInt("AttackState", "MaxCombo");

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
    LoadComboData();

    phaseTimer_ = 0.0f;
    hasBufferedInput_ = false;
    phase_ = SearchTarget;
    targetEnemy_ = nullptr;

    InitializeComboAttack(player);

    player->ResetMoveToTarget();
}

void AttackState::InitializeComboAttack(Player* player)
{
    // TODO: アニメーション作成後に実装
    // player->GetModel()->PlayAnimation("Attack" + std::to_string(comboIndex_));

    const ComboData& currentCombo = combos_[comboIndex_];

    if (player->GetMeleeAttackCollider()) {
        player->GetMeleeAttackCollider()->SetActive(true);
        player->GetMeleeAttackCollider()->Reset();
        // 最終コンボのみノックバック
        player->GetMeleeAttackCollider()->SetKnockbackEnabled(comboIndex_ == maxCombo_ - 1);
        player->GetMeleeAttackCollider()->SetAttackDamage(currentCombo.damage);
    }

    if (player->GetAttackBlock()) {
        player->SetAttackBlockVisible(true);

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
    if (player->GetMeleeAttackCollider()) {
        player->GetMeleeAttackCollider()->SetActive(false);
    }

    player->SetAttackBlockVisible(false);

    // 途中離脱でもコンボをリセット
    comboIndex_ = 0;
    hasBufferedInput_ = false;

    targetEnemy_ = nullptr;
}

//=========================================================================================
// 更新処理
//=========================================================================================

void AttackState::Update(Player* player, float deltaTime)
{
    // ホットリロード対応で毎フレーム同期
    LoadComboData();

    switch (phase_) {
    case SearchTarget:
        phaseTimer_ += deltaTime;

        SearchForTarget(player);

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

    player->MoveToTarget(targetEnemy_, deltaTime);

    Vector3 toTarget = targetEnemy_->GetTransform().translate - player->GetTransform().translate;
    toTarget.y = 0.0f;  // 水平距離のみ
    float currentDistance = toTarget.Length();

    // ターゲットが攻撃範囲外へ離れたら追わずその場で攻撃
    if (currentDistance > player->GetAttackMinDistance()) {
        player->ResetMoveToTarget();
        TransitionToPhase(ExecuteAttack);
        return;
    }

    // 到達 or 最大移動時間超過で攻撃へ
    if (player->HasReachedTarget() || phaseTimer_ >= maxMoveTime_) {
        TransitionToPhase(ExecuteAttack);
    }
}

void AttackState::ProcessExecuteAttack(Player* player, float deltaTime)
{
    phaseTimer_ += deltaTime;

    const ComboData& currentCombo = combos_[comboIndex_];

    float angularVelocity = currentCombo.swingAngle / currentCombo.attackDuration;
    blockAngle_ += angularVelocity * deltaTime * currentCombo.swingDirection;
    UpdateBlockPosition(player);

    if (player->GetMeleeAttackCollider()) {
        player->GetMeleeAttackCollider()->Damage();
    }

    if (phaseTimer_ >= currentCombo.attackDuration) {
        OnExecuteAttackComplete(player);
    }
}

void AttackState::OnExecuteAttackComplete(Player* player)
{
    // 先行入力があり次コンボが残っていれば継続
    if (hasBufferedInput_ && comboIndex_ < maxCombo_ - 1) {
        comboIndex_++;
        hasBufferedInput_ = false;
        TransitionToPhase(SearchTarget);
        InitializeComboAttack(player);
        return;
    }

    // 最終コンボまで到達したら硬直へ
    if (comboIndex_ >= maxCombo_ - 1) {
        TransitionToPhase(Recovery);
        return;
    }

    // 途中離脱は硬直なしで即 Idle
    PlayerStateMachine* stateMachine = player->GetStateMachine();
    if (stateMachine) {
        stateMachine->ChangeState("Idle");
    }
}

void AttackState::ProcessRecovery(Player* player, float deltaTime)
{
    phaseTimer_ += deltaTime;

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
        // 水平回転（XZ 平面）
        float worldAngle = playerRotY + blockAngle_;
        blockPos = {
            playerPos.x + sinf(worldAngle) * blockRadius_,
            playerPos.y,
            playerPos.z + cosf(worldAngle) * blockRadius_
        };
    }
    else {
        // 垂直回転（縦切り）。blockAngle_ は垂直角度（π/2 が真上、-π/2 が真下）
        float horizontalDist = cosf(blockAngle_) * blockRadius_;
        float verticalDist = sinf(blockAngle_) * blockRadius_;

        blockPos = {
            playerPos.x + sinf(playerRotY) * horizontalDist,
            playerPos.y + verticalDist,
            playerPos.z + cosf(playerRotY) * horizontalDist
        };
    }

    Transform blockTransform;
    blockTransform.translate = blockPos;

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

    const char* phaseNames[] = { "SearchTarget", "MoveToTarget", "ExecuteAttack", "Recovery" };
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Phase: %s", phaseNames[GetPhase()]);

    if (ImGui::TreeNode("Combo System")) {
        ImGui::Text("Combo Index: %d / %d", GetComboIndex() + 1, GetMaxCombo());

        float comboProgress = static_cast<float>(GetComboIndex() + 1) / static_cast<float>(GetMaxCombo());
        ImGui::ProgressBar(comboProgress, ImVec2(-1, 0), "Combo Progress");

        if (HasBufferedInput()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Buffered Input: YES");
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Buffered Input: NO");
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Current Combo Data")) {
        const ComboData& currentCombo = GetCurrentComboData();

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

        if (GetPhase() == ExecuteAttack) {
            float attackProgress = GetPhaseTimer() / currentCombo.attackDuration;
            ImGui::ProgressBar(attackProgress, ImVec2(-1, 0), "Attack Progress");
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Timers")) {
        switch (GetPhase()) {
        case SearchTarget:
            ImGui::Text("Search Timer: %.2f / %.2f", GetPhaseTimer(), maxSearchTime_);
            ImGui::ProgressBar(GetPhaseTimer() / maxSearchTime_);
            break;
        case MoveToTarget:
            ImGui::Text("Move Timer: %.2f / %.2f", GetPhaseTimer(), GetMaxMoveTime());
            ImGui::ProgressBar(GetPhaseTimer() / GetMaxMoveTime());
            break;
        case ExecuteAttack:
            ImGui::Text("Attack Timer: %.2f / %.2f", GetPhaseTimer(), GetCurrentComboData().attackDuration);
            ImGui::ProgressBar(GetPhaseTimer() / GetCurrentComboData().attackDuration);
            break;
        case Recovery:
            ImGui::Text("Recovery Timer: %.2f / %.2f", GetPhaseTimer(), GetRecoveryTime());
            ImGui::ProgressBar(GetPhaseTimer() / GetRecoveryTime());
            break;
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Target Info")) {
        if (GetTargetEnemy()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Target: DETECTED");
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Target: NONE");
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("All Combo Data")) {
        for (int i = 0; i < kMaxComboCount; ++i) {
            const ComboData& combo = combos_[i];
            bool isCurrentCombo = (i == GetComboIndex());

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
