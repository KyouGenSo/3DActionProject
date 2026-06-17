#include "BTBossWideShoot.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossWideShoot::BTBossWideShoot() {
    name_ = "BossWideShoot";
}

Tako::BTNodeStatus BTBossWideShoot::OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) {
    if (elapsedTime_ < chargeTime_) {
        AimAtPlayer(blackboard, deltaTime);
        bulletSignEffect_.Update(boss, deltaTime);
    }
    else if (currentSweep_ < sweepCount_) {
        if (!hasEndedEffect_) {
            bulletSignEffect_.End(boss);
            hasEndedEffect_ = true;
        }

        timeSinceLastFire_ += deltaTime;
        if (timeSinceLastFire_ >= fireInterval_) {
            FireBullet(boss);
            firedInSweep_++;
            timeSinceLastFire_ = 0.0f;

            if (firedInSweep_ >= bulletsPerSweep_) {
                currentSweep_++;
                firedInSweep_ = 0;

                if (currentSweep_ >= sweepCount_) {
                    EnterAttackRecovery(boss);
                }
            }
        }
    }

    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= totalDuration_) {
        timeSinceLastFire_ = 0.0f;
        currentSweep_ = 0;
        firedInSweep_ = 0;
        hasEndedEffect_ = false;
        return FinishAttack();
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossWideShoot::OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) {
    timeSinceLastFire_ = 0.0f;
    currentSweep_ = 0;
    firedInSweep_ = 0;
    hasEndedEffect_ = false;

    int totalBullets = bulletsPerSweep_ * sweepCount_;
    if (totalBullets > 0) {
        fireInterval_ = firingDuration_ / static_cast<float>(totalBullets);
    }

    totalDuration_ = chargeTime_ + firingDuration_ + recoveryTime_;

    bulletSignEffect_.Start(boss, chargeTime_);

    // 基準方向 = プレイヤー方向（XZ 平面）
    Player* player = blackboard->GetPtr<Player>("player");
    if (player) {
        Vector3 playerPos = player->GetTransform().translate;
        Vector3 bossPos = boss->GetTransform().translate;
        baseDirection_ = playerPos - bossPos;
        baseDirection_.y = 0.0f;
        if (baseDirection_.Length() > kDirectionEpsilon) {
            baseDirection_ = baseDirection_.Normalize();
        } else {
            baseDirection_ = Vector3(0.0f, 0.0f, 1.0f);
        }
    }
}

void BTBossWideShoot::AimAtPlayer(Tako::BTBlackboard* blackboard, float /*deltaTime*/) {
    Vector3 dir = FacePlayerInstant(blackboard, kDirectionEpsilon);
    if (dir.Length() > 0.0f) {
        baseDirection_ = dir;
    }
}

void BTBossWideShoot::FireBullet(Boss* boss) {
    Vector3 firePosition = boss->GetTransform().translate;
    float angleOffset = GetCurrentAngleOffset();
    Vector3 bulletDirection = CalculateBulletDirection(baseDirection_, angleOffset);

    bool isPenetrating = IsPenetratingBullet();
    float speed = isPenetrating ? penetratingBulletSpeed_ : normalBulletSpeed_;
    Vector3 bulletVelocity = bulletDirection * speed;

    if (isPenetrating) {
        boss->RequestPenetratingBulletSpawn(firePosition, bulletVelocity);
    } else {
        boss->RequestBulletSpawn(firePosition, bulletVelocity);
    }
}

float BTBossWideShoot::GetCurrentAngleOffset() const {
    if (bulletsPerSweep_ <= 1) return 0.0f;

    float t = static_cast<float>(firedInSweep_) / static_cast<float>(bulletsPerSweep_ - 1);
    float baseOffset = sweepAngle_ * (2.0f * t - 1.0f);

    // 偶数回目のスイープは逆方向
    if (currentSweep_ % 2 == 1) {
        baseOffset = -baseOffset;
    }
    return baseOffset;
}

bool BTBossWideShoot::IsPenetratingBullet() const {
    if (penetratingCount_ <= 0) return false;
    if (penetratingCount_ >= bulletsPerSweep_) return true;

    int interval = bulletsPerSweep_ / penetratingCount_;
    if (interval <= 0) interval = 1;

    return (firedInSweep_ % interval == 0) && (firedInSweep_ / interval < penetratingCount_);
}

Vector3 BTBossWideShoot::CalculateBulletDirection(const Vector3& baseDirection, float angleOffset) {
    if (std::abs(angleOffset) < kAngleEpsilon) return baseDirection;

    float cos_angle = cosf(angleOffset);
    float sin_angle = sinf(angleOffset);

    Vector3 rotatedDirection;
    rotatedDirection.x = baseDirection.x * cos_angle - baseDirection.z * sin_angle;
    rotatedDirection.y = baseDirection.y;
    rotatedDirection.z = baseDirection.x * sin_angle + baseDirection.z * cos_angle;
    return rotatedDirection.Normalize();
}

void BTBossWideShoot::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("chargeTime"))             chargeTime_ = params["chargeTime"];
    if (params.contains("recoveryTime"))           recoveryTime_ = params["recoveryTime"];
    if (params.contains("firingDuration"))         firingDuration_ = params["firingDuration"];
    if (params.contains("sweepAngle"))             sweepAngle_ = params["sweepAngle"];
    if (params.contains("bulletsPerSweep"))        bulletsPerSweep_ = params["bulletsPerSweep"];
    if (params.contains("sweepCount"))             sweepCount_ = params["sweepCount"];
    if (params.contains("normalBulletSpeed"))      normalBulletSpeed_ = params["normalBulletSpeed"];
    if (params.contains("penetratingBulletSpeed")) penetratingBulletSpeed_ = params["penetratingBulletSpeed"];
    if (params.contains("penetratingCount"))       penetratingCount_ = params["penetratingCount"];
}

void BTBossWideShoot::OnExtractParameters(nlohmann::json& out) const {
    out["chargeTime"]             = chargeTime_;
    out["recoveryTime"]           = recoveryTime_;
    out["firingDuration"]         = firingDuration_;
    out["sweepAngle"]             = sweepAngle_;
    out["bulletsPerSweep"]        = bulletsPerSweep_;
    out["sweepCount"]             = sweepCount_;
    out["normalBulletSpeed"]      = normalBulletSpeed_;
    out["penetratingBulletSpeed"] = penetratingBulletSpeed_;
    out["penetratingCount"]       = penetratingCount_;
}

#ifdef _DEBUG
bool BTBossWideShoot::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Time Control");
    if (ImGui::DragFloat("Charge Time##wide", &chargeTime_, 0.05f, 0.0f, 3.0f))       changed = true;
    if (ImGui::DragFloat("Recovery Time##wide", &recoveryTime_, 0.05f, 0.0f, 3.0f))   changed = true;
    if (ImGui::DragFloat("Firing Duration##wide", &firingDuration_, 0.1f, 0.1f, 5.0f)) changed = true;

    ImGui::SeparatorText("Sweep Control");
    if (ImGui::SliderAngle("Sweep Angle##wide", &sweepAngle_, 0.0f, 90.0f))           changed = true;
    if (ImGui::DragInt("Bullets Per Sweep##wide", &bulletsPerSweep_, 1, 1, 30))       changed = true;
    if (ImGui::DragInt("Sweep Count##wide", &sweepCount_, 1, 1, 5))                   changed = true;

    ImGui::SeparatorText("Bullet Speed");
    if (ImGui::DragFloat("Normal Speed##wide", &normalBulletSpeed_, 1.0f, 5.0f, 100.0f))                changed = true;
    if (ImGui::DragFloat("Penetrating Speed##wide", &penetratingBulletSpeed_, 1.0f, 5.0f, 50.0f))       changed = true;

    ImGui::SeparatorText("Bullet Type");
    if (ImGui::DragInt("Penetrating Count##wide", &penetratingCount_, 1, 0, bulletsPerSweep_))          changed = true;

    return changed;
}
#endif
