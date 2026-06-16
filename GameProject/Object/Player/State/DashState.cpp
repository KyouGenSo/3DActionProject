#include "DashState.h"
#include "PlayerStateMachine.h"
#include "../Player.h"
#include "Input/InputHandler.h"
#include "GlobalVariables.h"
#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

void DashState::Enter(Player* player)
{
	// ダッシュアニメーションを再生
	// TODO: アニメーション作成後に実装
	// player->GetModel()->PlayAnimation("Dash");

	timer_ = 0.0f;
	player->SetInvincible(true);
}

void DashState::Update(Player* player, float deltaTime)
{
	// GlobalVariables から値を同期
	GlobalVariables* gv = GlobalVariables::GetInstance();
	duration_ = gv->GetValueFloat("DashState", "Duration");
	speed_ = gv->GetValueFloat("DashState", "Speed");

	timer_ += deltaTime;

	// ダッシュ移動処理
	player->Move(speed_);
	
	// ダッシュ時間が終了したら元の状態に戻る
	if (timer_ >= duration_)
	{
		PlayerStateMachine* stateMachine = player->GetStateMachine();
		if (stateMachine)
		{
			// 移動入力があれば Walk、なければ Idle に遷移
			InputHandler* input = player->GetInputHandler();
			if (input && input->IsMoving())
			{
				stateMachine->ChangeState("Move");
			}
			else
			{
				stateMachine->ChangeState("Idle");
			}
		}
	}
}

void DashState::Exit(Player* player)
{
	player->SetInvincible(false);
	timer_ = 0.0f;

	// クールダウン開始
	player->StartDashCooldown();
}

void DashState::HandleInput(Player* player)
{
	// ダッシュ中は入力を受け付けない
}

void DashState::DrawImGui(Player* player)
{
#ifdef _DEBUG
	ImGui::Text("=== Dash State Details ===");
	ImGui::Separator();

	// ダッシュ進行状況
	float progress = (GetDuration() > 0.0f) ? (GetTimer() / GetDuration()) : 0.0f;
	ImGui::Text("Dash Progress: %.1f%%", progress * 100.0f);
	ImGui::ProgressBar(progress);

	// タイマー情報
	ImGui::Text("Timer: %.3f / %.3f", GetTimer(), GetDuration());

	// パラメータ調整
	if (ImGui::TreeNode("Dash Parameters")) {
		float duration = GetDuration();
		if (ImGui::SliderFloat("Duration", &duration, 0.01f, 0.5f, "%.3f")) SetDuration(duration);
		float speed = GetSpeed();
		if (ImGui::SliderFloat("Speed", &speed, 1.0f, 50.0f, "%.1f")) SetSpeed(speed);

		// 推奨値のプリセット
		if (ImGui::Button("Fast Dash")) {
			SetDuration(0.05f);
			SetSpeed(10.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Long Dash")) {
			SetDuration(0.2f);
			SetSpeed(5.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Teleport")) {
			SetDuration(0.01f);
			SetSpeed(50.0f);
		}

		ImGui::TreePop();
	}

	// デバッグ情報
	if (ImGui::TreeNode("Debug Info")) {
		ImGui::Text("Actual Distance: %.2f", GetTimer() * GetSpeed());
		ImGui::Text("Frame Count: %d", (int)(GetTimer() * 60.0f));
		ImGui::TreePop();
	}
#endif
}