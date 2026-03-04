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
	// 射撃アニメーションを再生
	// TODO: アニメーション作成後に実装
	// player->GetModel()->PlayAnimation("Shoot");

	fireRateTimer_ = 0.0f;
}

void ShootState::Update(Player* player, float deltaTime)
{
	// GlobalVariables から値を同期
	GlobalVariables* gv = GlobalVariables::GetInstance();
	fireRate_ = gv->GetValueFloat("ShootState", "FireRate");
	moveSpeedMultiplier_ = gv->GetValueFloat("ShootState", "MoveSpeedMultiplier");

	// 射撃が無効化されている場合（フェーズ2など）
	if (!player->IsShootingEnabled()) {
		// 射撃せずに Idle/Move に戻る
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

	// 発射レートタイマーの更新
	if (fireRateTimer_ > 0.0f)
	{
		fireRateTimer_ -= deltaTime;
	}

	// エイム方向の計算
	CalculateAimDirection(player);

	// 射撃可能なら発射
	if (fireRateTimer_ <= 0.0f)
	{
		Fire(player);
		fireRateTimer_ = fireRate_;
	}

	// 移動処理
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
		// 右スティック入力なし → プレイヤー前方
		float yaw = player->GetRotate().y;
		aimDirection_ = Vector3(std::sin(yaw), 0.0f, std::cos(yaw));
		return;
	}

	// 右スティック方向を取得
	Vector2 stick = input->GetAimDirection();

	// スティック入力を3D ベクトルに変換（X=左右, Y=前後 → X=X, Z=Y）
	Vector3 localDirection = Vector3(stick.x, 0.0f, stick.y);

	// カメラの Y 回転を基準にする（カメラ相対座標系）
	Camera* camera = player->GetCamera();
	float cameraYaw = camera ? camera->GetRotateY() : player->GetRotate().y;
	Matrix4x4 rotationMatrix = Mat4x4::MakeRotateY(cameraYaw);

	// ローカル方向をワールド方向に変換
	aimDirection_ = Mat4x4::TransformNormal(rotationMatrix, localDirection);

	// 正規化
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
	// 発射位置（プレイヤー中心）
	Vector3 position = player->GetTranslate();

	// 弾速度を計算
	GlobalVariables* gv = GlobalVariables::GetInstance();
	float bulletSpeed = gv->GetValueFloat("PlayerBullet", "Speed");
	if (bulletSpeed <= 0.0f) {
		bulletSpeed = 30.0f;  // デフォルト値
	}
	Vector3 velocity = aimDirection_ * bulletSpeed;

	// 弾生成リクエストを追加
	player->RequestBulletSpawn(position, velocity);
}

void ShootState::DrawImGui(Player* player)
{
#ifdef _DEBUG
	ImGui::Text("=== Shoot State Details ===");
	ImGui::Separator();

	// 発射レート情報
	ImGui::Text("Fire Rate: %.2f (%.1f shots/sec)", fireRate_, 1.0f / fireRate_);
	ImGui::Text("Next Shot In: %.2f", std::max<float>(0.0f, fireRate_ - fireRateTimer_));

	// プログレスバー
	float progress = (fireRate_ > 0.0f) ? (fireRateTimer_ / fireRate_) : 0.0f;
	ImGui::ProgressBar(progress, ImVec2(-1, 0), "Reload");

	// 照準方向
	if (ImGui::TreeNode("Aim Direction")) {
		ImGui::Text("X: %.3f", aimDirection_.x);
		ImGui::Text("Y: %.3f", aimDirection_.y);
		ImGui::Text("Z: %.3f", aimDirection_.z);
		ImGui::Text("Length: %.3f", aimDirection_.Length());
		ImGui::TreePop();
	}

	// パラメータ調整
	if (ImGui::TreeNode("Shooting Parameters")) {
		ImGui::SliderFloat("Fire Rate", &fireRate_, 0.05f, 2.0f, "%.2f sec");

		// プリセット
		if (ImGui::Button("Rapid Fire")) {
			fireRate_ = 0.05f;
		}
		ImGui::SameLine();
		if (ImGui::Button("Normal")) {
			fireRate_ = 0.2f;
		}
		ImGui::SameLine();
		if (ImGui::Button("Slow")) {
			fireRate_ = 0.5f;
		}

		ImGui::TreePop();
	}
#endif
}