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

BTNodeStatus BTBossWideShoot::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    // 初回実行時の初期化
    if (isFirstExecute_) {
        InitializeWideShoot(blackboard);
        isFirstExecute_ = false;
    }

    // チャージフェーズ：プレイヤーの方向を向く
    if (elapsedTime_ < chargeTime_) {
        AimAtPlayer(blackboard, deltaTime);
        bulletSignEffect_.Update(boss, deltaTime);
    }
    // 発射フェーズ
    else if (currentSweep_ < sweepCount_) {
        // 最初の発射前にエフェクトを終了
        if (!hasEndedEffect_) {
            bulletSignEffect_.End(boss);
            hasEndedEffect_ = true;
        }

        // 発射間隔チェック
        timeSinceLastFire_ += deltaTime;
        if (timeSinceLastFire_ >= fireInterval_) {
            FireBullet(boss);
            firedInSweep_++;
            timeSinceLastFire_ = 0.0f;

            // 1スイープ完了チェック
            if (firedInSweep_ >= bulletsPerSweep_) {
                currentSweep_++;
                firedInSweep_ = 0;

                // 全スイープ完了で硬直フェーズ開始
                if (currentSweep_ >= sweepCount_) {
                    boss->EnterRecovery();
                }
            }
        }
    }

    // 経過時間を更新
    elapsedTime_ += deltaTime;

    // 状態終了チェック
    if (elapsedTime_ >= totalDuration_) {
        // 硬直フェーズ終了
        boss->ExitRecovery();

        // リセットして成功を返す
        isFirstExecute_ = true;
        elapsedTime_ = 0.0f;
        timeSinceLastFire_ = 0.0f;
        currentSweep_ = 0;
        firedInSweep_ = 0;
        hasEndedEffect_ = false;
        status_ = BTNodeStatus::Success;
        return BTNodeStatus::Success;
    }

    // まだ射撃処理中
    status_ = BTNodeStatus::Running;
    return BTNodeStatus::Running;
}

void BTBossWideShoot::Reset() {
    BTNode::Reset();
    elapsedTime_ = 0.0f;
    timeSinceLastFire_ = 0.0f;
    currentSweep_ = 0;
    firedInSweep_ = 0;
    isFirstExecute_ = true;
    hasEndedEffect_ = false;
    // 注意: Reset 時は boss 参照がないため、ExitRecovery()は呼べない
}

void BTBossWideShoot::InitializeWideShoot(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    // タイマーリセット
    elapsedTime_ = 0.0f;
    timeSinceLastFire_ = 0.0f;
    currentSweep_ = 0;
    firedInSweep_ = 0;
    hasEndedEffect_ = false;

    // firingDuration から fireInterval を計算
    int totalBullets = bulletsPerSweep_ * sweepCount_;
    if (totalBullets > 0) {
        fireInterval_ = firingDuration_ / static_cast<float>(totalBullets);
    }

    // totalDuration を計算
    totalDuration_ = chargeTime_ + firingDuration_ + recoveryTime_;

    // 射撃予兆エフェクト開始
    bulletSignEffect_.Start(boss, chargeTime_);

    // 基準方向を計算（プレイヤー方向）
    Player* player = blackboard->GetPlayer();
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

void BTBossWideShoot::AimAtPlayer(BTBlackboard* blackboard, float deltaTime) {
    Boss* boss = blackboard->GetBoss();
    Player* player = blackboard->GetPlayer();
    if (!player) {
        return;
    }

    Vector3 playerPos = player->GetTransform().translate;
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 toPlayer = playerPos - bossPos;
    toPlayer.y = 0.0f;

    if (toPlayer.Length() > kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
        float angle = atan2f(toPlayer.x, toPlayer.z);
        boss->SetRotate(Vector3(0.0f, angle, 0.0f));

        // 基準方向を更新
        baseDirection_ = toPlayer;
    }
}

void BTBossWideShoot::FireBullet(Boss* boss) {
    // 発射位置
    Vector3 firePosition = boss->GetTransform().translate;

    // 角度オフセットを取得
    float angleOffset = GetCurrentAngleOffset();

    // 発射方向を計算
    Vector3 bulletDirection = CalculateBulletDirection(baseDirection_, angleOffset);

    // 弾種と速度を決定
    bool isPenetrating = IsPenetratingBullet();
    float speed = isPenetrating ? penetratingBulletSpeed_ : normalBulletSpeed_;

    // 弾の速度ベクトルを計算
    Vector3 bulletVelocity = bulletDirection * speed;

    // 弾を生成リクエスト
    if (isPenetrating) {
        boss->RequestPenetratingBulletSpawn(firePosition, bulletVelocity);
    } else {
        boss->RequestBulletSpawn(firePosition, bulletVelocity);
    }
}

float BTBossWideShoot::GetCurrentAngleOffset() const {
    if (bulletsPerSweep_ <= 1) {
        return 0.0f;
    }

    // 0番目: -sweepAngle_, 最後: +sweepAngle_
    float t = static_cast<float>(firedInSweep_) / static_cast<float>(bulletsPerSweep_ - 1);
    float baseOffset = sweepAngle_ * (2.0f * t - 1.0f);

    // 偶数回目のスイープは逆方向
    if (currentSweep_ % 2 == 1) {
        baseOffset = -baseOffset;
    }

    return baseOffset;
}

bool BTBossWideShoot::IsPenetratingBullet() const {
    if (penetratingCount_ <= 0) {
        return false;
    }
    if (penetratingCount_ >= bulletsPerSweep_) {
        return true;
    }

    // 均等に分散配置
    int interval = bulletsPerSweep_ / penetratingCount_;
    if (interval <= 0) {
        interval = 1;
    }

    return (firedInSweep_ % interval == 0) &&
           (firedInSweep_ / interval < penetratingCount_);
}

Vector3 BTBossWideShoot::CalculateBulletDirection(const Vector3& baseDirection, float angleOffset) {
    if (std::abs(angleOffset) < kAngleEpsilon) {
        return baseDirection;
    }

    // Y 軸回転行列を作成
    float cos_angle = cosf(angleOffset);
    float sin_angle = sinf(angleOffset);

    // 回転を適用（Y 軸回転）
    Vector3 rotatedDirection;
    rotatedDirection.x = baseDirection.x * cos_angle - baseDirection.z * sin_angle;
    rotatedDirection.y = baseDirection.y;
    rotatedDirection.z = baseDirection.x * sin_angle + baseDirection.z * cos_angle;

    rotatedDirection = rotatedDirection.Normalize();
    return rotatedDirection;
}

nlohmann::json BTBossWideShoot::ExtractParameters() const {
    return {
        {"chargeTime", chargeTime_},
        {"recoveryTime", recoveryTime_},
        {"firingDuration", firingDuration_},
        {"sweepAngle", sweepAngle_},
        {"bulletsPerSweep", bulletsPerSweep_},
        {"sweepCount", sweepCount_},
        {"normalBulletSpeed", normalBulletSpeed_},
        {"penetratingBulletSpeed", penetratingBulletSpeed_},
        {"penetratingCount", penetratingCount_}
    };
}

#ifdef _DEBUG
bool BTBossWideShoot::DrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Time Control");
    if (ImGui::DragFloat("Charge Time##wide", &chargeTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Recovery Time##wide", &recoveryTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Firing Duration##wide", &firingDuration_, 0.1f, 0.1f, 5.0f)) {
        changed = true;
    }

    ImGui::SeparatorText("Sweep Control");
    if (ImGui::SliderAngle("Sweep Angle##wide", &sweepAngle_, 0.0f, 90.0f)) {
        changed = true;
    }
    if (ImGui::DragInt("Bullets Per Sweep##wide", &bulletsPerSweep_, 1, 1, 30)) {
        changed = true;
    }
    if (ImGui::DragInt("Sweep Count##wide", &sweepCount_, 1, 1, 5)) {
        changed = true;
    }

    ImGui::SeparatorText("Bullet Speed");
    if (ImGui::DragFloat("Normal Speed##wide", &normalBulletSpeed_, 1.0f, 5.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Penetrating Speed##wide", &penetratingBulletSpeed_, 1.0f, 5.0f, 50.0f)) {
        changed = true;
    }

    ImGui::SeparatorText("Bullet Type");
    if (ImGui::DragInt("Penetrating Count##wide", &penetratingCount_, 1, 0, bulletsPerSweep_)) {
        changed = true;
    }

    return changed;
}
#endif
