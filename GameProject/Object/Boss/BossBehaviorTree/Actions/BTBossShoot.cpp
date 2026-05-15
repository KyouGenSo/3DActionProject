#include "BTBossShoot.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossShoot::BTBossShoot() {
    name_ = "BossShoot";
}

Tako::BTNodeStatus BTBossShoot::OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) {
    // プレイヤーの方向を向く（射撃準備中）
    if (elapsedTime_ < chargeTime_) {
        AimAtPlayer(blackboard, deltaTime);
        bulletSignEffect_.Update(boss, deltaTime);
    }

    // 弾を発射
    if (elapsedTime_ >= chargeTime_ && !hasFired_) {
        bulletSignEffect_.End(boss);
        FireBullets(blackboard);
        hasFired_ = true;
        EnterAttackRecovery(boss);  // 硬直フェーズ開始
    }

    elapsedTime_ += deltaTime;

    // 状態終了チェック
    if (elapsedTime_ >= totalDuration_) {
        return FinishAttack();
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossShoot::OnInitialize(Tako::BTBlackboard* /*blackboard*/, Boss* boss) {
    totalDuration_ = chargeTime_ + recoveryTime_;
    hasFired_ = false;
    bulletSignEffect_.Start(boss, chargeTime_);
}

void BTBossShoot::AimAtPlayer(Tako::BTBlackboard* blackboard, float /*deltaTime*/) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Player* player = blackboard->GetPtr<Player>("player");
    if (!player) return;

    Vector3 playerPos = player->GetTransform().translate;
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 toPlayer = playerPos - bossPos;
    toPlayer.y = 0.0f;

    if (toPlayer.Length() > kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
        float angle = atan2f(toPlayer.x, toPlayer.z);
        boss->SetRotate(Vector3(0.0f, angle, 0.0f));
    }
}

void BTBossShoot::FireBullets(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Player* player = blackboard->GetPtr<Player>("player");
    if (!player) return;

    Vector3 firePosition = boss->GetTransform().translate;
    Vector3 playerPos = player->GetTransform().translate;
    Vector3 toPlayer = playerPos - firePosition;

    float distance = sqrtf(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    if (distance > kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
    }

    for (int i = 0; i < bulletCount_; i++) {
        float angleOffset = 0.0f;
        if (bulletCount_ > 1) {
            float t = static_cast<float>(i) / static_cast<float>(bulletCount_ - 1);
            angleOffset = spreadAngle_ * (2.0f * t - 1.0f);
        }
        Vector3 bulletDirection = CalculateBulletDirection(toPlayer, angleOffset);
        Vector3 bulletVelocity = bulletDirection * bulletSpeed_;
        boss->RequestBulletSpawn(firePosition, bulletVelocity);
    }
}

Vector3 BTBossShoot::CalculateBulletDirection(const Vector3& baseDirection, float angleOffset) {
    if (std::abs(angleOffset) < kAngleEpsilon) return baseDirection;

    float cos_angle = cosf(angleOffset);
    float sin_angle = sinf(angleOffset);

    Vector3 rotatedDirection;
    rotatedDirection.x = baseDirection.x * cos_angle - baseDirection.z * sin_angle;
    rotatedDirection.y = baseDirection.y;
    rotatedDirection.z = baseDirection.x * sin_angle + baseDirection.z * cos_angle;
    return rotatedDirection.Normalize();
}

void BTBossShoot::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("chargeTime"))   chargeTime_ = params["chargeTime"];
    if (params.contains("bulletSpeed"))  bulletSpeed_ = params["bulletSpeed"];
    if (params.contains("spreadAngle"))  spreadAngle_ = params["spreadAngle"];
    if (params.contains("recoveryTime")) recoveryTime_ = params["recoveryTime"];
    if (params.contains("bulletCount"))  bulletCount_ = params["bulletCount"];
}

void BTBossShoot::OnExtractParameters(nlohmann::json& out) const {
    out["chargeTime"]   = chargeTime_;
    out["recoveryTime"] = recoveryTime_;
    out["bulletSpeed"]  = bulletSpeed_;
    out["spreadAngle"]  = spreadAngle_;
    out["bulletCount"]  = bulletCount_;
}

#ifdef _DEBUG
bool BTBossShoot::OnDrawImGui() {
    bool changed = false;
    if (ImGui::DragFloat("Charge Time##shoot", &chargeTime_, 0.05f, 0.0f, 3.0f))     changed = true;
    if (ImGui::DragFloat("Recovery Time##shoot", &recoveryTime_, 0.05f, 0.0f, 3.0f)) changed = true;
    if (ImGui::DragFloat("Bullet Speed##shoot", &bulletSpeed_, 1.0f, 5.0f, 100.0f))  changed = true;
    if (ImGui::SliderAngle("Spread Angle##shoot", &spreadAngle_, 0.0f, 45.0f))       changed = true;
    if (ImGui::DragInt("Bullet Count##shoot", &bulletCount_, 1, 1, 10))              changed = true;
    return changed;
}
#endif
