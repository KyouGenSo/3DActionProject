#include "ShootState.h"
#include "PlayerStateMachine.h"
#include "../Player.h"
#include "Input/InputHandler.h"
#include "Camera.h"
#include "Matrix4x4.h"
#include "Mat4x4Func.h"
#include "Vec3Func.h"
#include "GlobalVariables.h"
#include <algorithm>  // for std::max
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

void ShootState::Enter(Player* player)
{
	// TODO: アニメーション作成後に実装
	// player->GetModel()->PlayAnimation("Shoot");

	fireRateTimer_ = 0.0f;
}

void ShootState::Update(Player* player, float deltaTime)
{
	GlobalVariables* gv = GlobalVariables::GetInstance();
	fireRate_ = gv->GetValueFloat("ShootState", "FireRate");
	moveSpeedMultiplier_ = gv->GetValueFloat("ShootState", "MoveSpeedMultiplier");

	// 射撃無効時（フェーズ2など）は撃たずに離脱
	if (!player->IsShootingEnabled()) {
		PlayerStateMachine* stateMachine = player->GetStateMachine();
		InputHandler* input = player->GetInputHandler();
		if (stateMachine && input) {
			if (input->IsMoving()) {
				stateMachine->ChangeState("Move");
			} else {
				stateMachine->ChangeState("Idle");
			}
		}
		return;
	}

	if (fireRateTimer_ > 0.0f)
	{
		fireRateTimer_ -= deltaTime;
	}

	CalculateAimDirection(player);

	if (fireRateTimer_ <= 0.0f)
	{
		Fire(player);
		fireRateTimer_ = fireRate_;
	}

    player->Move(moveSpeedMultiplier_, false);
}

void ShootState::Exit(Player* player)
{
	fireRateTimer_ = 0.0f;
}

void ShootState::HandleInput(Player* player)
{
	InputHandler* input = player->GetInputHandler();
	if (!input) return;

	// 射撃ボタンが離されたら元の状態に戻る
	if (!input->IsShooting())
	{
		PlayerStateMachine* stateMachine = player->GetStateMachine();
		if (stateMachine)
		{
			if (input->IsMoving())
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

void ShootState::CalculateAimDirection(Player* player)
{
	InputHandler* input = player->GetInputHandler();
	if (!input || !input->IsShooting()) {
		// スティック入力なし → プレイヤー前方
		float yaw = player->GetRotate().y;
		aimDirection_ = Vector3(std::sin(yaw), 0.0f, std::cos(yaw));
		return;
	}

	Vector2 stick = input->GetAimDirection();

	// スティック入力を3D 化（X=左右, Y=前後 → X=X, Z=Y）
	Vector3 localDirection = Vector3(stick.x, 0.0f, stick.y);

	// カメラ相対座標系（カメラの Y 回転基準）
	Camera* camera = player->GetCamera();
	float cameraYaw = camera ? camera->GetRotateY() : player->GetRotate().y;
	Matrix4x4 rotationMatrix = Mat4x4::MakeRotateY(cameraYaw);

	aimDirection_ = Mat4x4::TransformNormal(rotationMatrix, localDirection);

    aimDirection_ = aimDirection_.Normalize();

    // 発射方向にプレイヤーを向ける
    if (aimDirection_.Length() > 0.01f) {
        float targetAngle = std::atan2(aimDirection_.x, aimDirection_.z);
        float aimRotationLerp = GlobalVariables::GetInstance()->GetValueFloat("ShootState", "AimRotationLerp");
        if (aimRotationLerp <= 0.0f) {
            aimRotationLerp = 0.3f;  // デフォルト値
        }
        Transform* transform = player->GetTransformPtr();
        transform->rotate.y = Vec3::LerpShortAngle(transform->rotate.y, targetAngle, aimRotationLerp);
    }
}

void ShootState::Fire(Player* player)
{
	Vector3 position = player->GetTranslate();

	GlobalVariables* gv = GlobalVariables::GetInstance();
	float bulletSpeed = gv->GetValueFloat("PlayerBullet", "Speed");
	if (bulletSpeed <= 0.0f) {
		bulletSpeed = 30.0f;  // デフォルト値
	}
	Vector3 velocity = aimDirection_ * bulletSpeed;

	player->RequestBulletSpawn(position, velocity);
}

void ShootState::DrawImGui(Player* player)
{
#ifdef _DEBUG
	ImGui::Text("=== Shoot State Details ===");
	ImGui::Separator();

	ImGui::Text("Fire Rate: %.2f (%.1f shots/sec)", GetFireRate(), 1.0f / GetFireRate());
	ImGui::Text("Next Shot In: %.2f", std::max<float>(0.0f, GetFireRate() - GetFireRateTimer()));

	float progress = (GetFireRate() > 0.0f) ? (GetFireRateTimer() / GetFireRate()) : 0.0f;
	ImGui::ProgressBar(progress, ImVec2(-1, 0), "Reload");

	if (ImGui::TreeNode("Aim Direction")) {
		const Tako::Vector3& aim = GetAimDirection();
		ImGui::Text("X: %.3f", aim.x);
		ImGui::Text("Y: %.3f", aim.y);
		ImGui::Text("Z: %.3f", aim.z);
		ImGui::Text("Length: %.3f", aim.Length());
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Shooting Parameters")) {
		float fireRate = GetFireRate();
		if (ImGui::SliderFloat("Fire Rate", &fireRate, 0.05f, 2.0f, "%.2f sec")) SetFireRate(fireRate);

		if (ImGui::Button("Rapid Fire")) {
			SetFireRate(0.05f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Normal")) {
			SetFireRate(0.2f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Slow")) {
			SetFireRate(0.5f);
		}

		ImGui::TreePop();
	}
#endif
}