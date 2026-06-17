#include "MoveState.h"
#include "PlayerStateMachine.h"
#include "../Player.h"
#include "Input/InputHandler.h"
#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

void MoveState::Enter(Player* player)
{
	// TODO: アニメーション作成後に実装
	// player->GetModel()->PlayAnimation("Walk");
	moveTime_ = 0.0f;
}

void MoveState::Update(Player* player, float deltaTime)
{
	player->Move(1.0f);
	moveTime_ += deltaTime;
}

void MoveState::Exit(Player* player)
{
	moveTime_ = 0.0f;
}

void MoveState::HandleInput(Player* player)
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

	if (!input->IsMoving())
	{
		stateMachine->ChangeState("Idle");
		return;
	}
}

void MoveState::DrawImGui(Player* player)
{
#ifdef _DEBUG
	ImGui::Text("=== Move State Details ===");
	ImGui::Separator();

	ImGui::Text("Move Time: %.2f seconds", moveTime_);

	if (player) {
		ImGui::Text("Current Speed: %.2f", player->GetSpeed());
		const Vector3& velocity = player->GetVelocity();
		ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity.x, velocity.y, velocity.z);
		ImGui::Text("Velocity Magnitude: %.2f", velocity.Length());
	}

	if (ImGui::TreeNode("Movement Direction")) {
		InputHandler* input = player->GetInputHandler();
		if (input && input->IsMoving()) {
			ImGui::Text("Input Active: YES");
			// TODO: Display actual input direction vector
		} else {
			ImGui::Text("Input Active: NO");
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Animation Info")) {
		ImGui::Text("Animation: Walk");
		ImGui::Text("TODO: Add walk/run animation blending");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Available Actions")) {
		ImGui::BulletText("Press Space to Dash");
		ImGui::BulletText("Press Z to Attack");
		ImGui::BulletText("Press Left Ctrl to Shoot");
		ImGui::BulletText("Press F to Parry");
		ImGui::BulletText("Release movement keys to stop");
		ImGui::TreePop();
	}
#endif
}