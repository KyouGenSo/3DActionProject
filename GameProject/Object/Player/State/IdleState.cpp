#include "IdleState.h"
#include "PlayerStateMachine.h"
#include "../Player.h"
#include "Input.h"
#include "Input/InputHandler.h"
#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

void IdleState::Enter(Player* player)
{
	// TODO: アニメーション作成後に実装
	// player->GetModel()->PlayAnimation("Idle");
	idleTime_ = 0.0f;
}

void IdleState::Update(Player* player, float deltaTime)
{
	idleTime_ += deltaTime;
}

void IdleState::Exit(Player* player)
{
	idleTime_ = 0.0f;
}

void IdleState::HandleInput(Player* player)
{
	InputHandler* input = player->GetInputHandler();
	if (!input) return;

	PlayerStateMachine* stateMachine = player->GetStateMachine();
	if (!stateMachine) return;

	// 上から優先順に遷移判定

	if (input->IsParrying() && player->CanParry())
	{
		stateMachine->ChangeState("Parry");
		return;
	}

	if (input->IsAttacking())
	{
		stateMachine->ChangeState("Attack");
		return;
	}

    if (input->IsShooting() && player->CanShoot())
	{
		stateMachine->ChangeState("Shoot");
		return;
	}

	if (input->IsDashing() && player->CanDash())
	{
		stateMachine->ChangeState("Dash");
		return;
	}

	if (input->IsMoving())
	{
		stateMachine->ChangeState("Move");
		return;
	}
}

void IdleState::DrawImGui(Player* player)
{
#ifdef _DEBUG
	ImGui::Text("=== Idle State Details ===");
	ImGui::Separator();

	ImGui::Text("Idle Time: %.2f seconds", idleTime_);

	if (ImGui::TreeNode("Animation Info")) {
		ImGui::Text("Animation: Idle");
		ImGui::Text("TODO: Add idle animation variations");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Available Actions")) {
		ImGui::BulletText("Press W/A/S/D to Move");
		ImGui::BulletText("Press Z to Attack");
		ImGui::BulletText("Press Left Ctrl to Shoot");
		ImGui::BulletText("Press Space to Dash");
		ImGui::BulletText("Press F to Parry");
		ImGui::TreePop();
	}
#endif
}