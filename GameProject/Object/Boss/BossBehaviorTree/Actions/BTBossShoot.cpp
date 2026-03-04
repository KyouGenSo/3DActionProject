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

BTNodeStatus BTBossShoot::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    // 初回実行時の初期化
    if (isFirstExecute_) {
        InitializeShoot(boss);
        isFirstExecute_ = false;
        hasFired_ = false;
    }

    // プレイヤーの方向を向く（射撃準備中）
    if (elapsedTime_ < chargeTime_) {
        AimAtPlayer(boss, deltaTime);
        bulletSignEffect_.Update(boss, deltaTime);
    }

    // 弾を発射
    if (elapsedTime_ >= chargeTime_ && !hasFired_) {
        bulletSignEffect_.End(boss);
        FireBullets(boss);
        hasFired_ = true;
        boss->EnterRecovery();  // 硬直フェーズ開始
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
        hasFired_ = false;
        status_ = BTNodeStatus::Success;
        return BTNodeStatus::Success;
    }

    // まだ射撃処理中
    status_ = BTNodeStatus::Running;
    return BTNodeStatus::Running;
}

void BTBossShoot::Reset() {
    BTNode::Reset();
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    hasFired_ = false;
    // 注意: Reset 時は boss 参照がないため、ExitRecovery()は呼べない
    // スタン等によるリセット時は、BossBehaviorTree 側で状態クリアが必要
}

void BTBossShoot::InitializeShoot(Boss* boss) {
    // タイマーリセット
    elapsedTime_ = 0.0f;

    // totalDuration を計算
    totalDuration_ = chargeTime_ + recoveryTime_;

    // 射撃予兆エフェクト開始
    bulletSignEffect_.Start(boss, chargeTime_);
}

void BTBossShoot::AimAtPlayer(Boss* boss, float deltaTime) {
    Player* player = boss->GetPlayer();
    if (!player) {
        return;
    }

    Vector3 playerPos = player->GetTransform().translate;
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 toPlayer = playerPos - bossPos;
    toPlayer.y = 0.0f; // Y 軸は無視

    if (toPlayer.Length() > kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
        float angle = atan2f(toPlayer.x, toPlayer.z);
        boss->SetRotate(Vector3(0.0f, angle, 0.0f));
    }
}

void BTBossShoot::FireBullets(Boss* boss) {
    Player* player = boss->GetPlayer();
    if (!player) {
        return;
    }

    // 発射位置（ボスの前方少し上）
    Vector3 firePosition = boss->GetTransform().translate;

    // プレイヤーへの方向を計算
    Vector3 playerPos = player->GetTransform().translate;
    Vector3 toPlayer = playerPos - firePosition;

    // 水平方向のみで計算（Y 成分は小さく保つ）
    float distance = sqrtf(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    if (distance > kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
    }

    // 弾を生成リクエスト
    for (int i = 0; i < bulletCount_; i++) {
        // 発射角度を計算（扇状に分散）
        float angleOffset = 0.0f;
        if (bulletCount_ > 1) {
            // -spreadAngle_から+spreadAngle_の範囲に均等に分散
            float t = static_cast<float>(i) / static_cast<float>(bulletCount_ - 1);
            angleOffset = spreadAngle_ * (2.0f * t - 1.0f);
        }

        // 発射方向を計算
        Vector3 bulletDirection = CalculateBulletDirection(toPlayer, angleOffset);

        // 弾の速度ベクトルを計算
        Vector3 bulletVelocity = bulletDirection * bulletSpeed_;

        // ボスに弾生成をリクエスト
        boss->RequestBulletSpawn(firePosition, bulletVelocity);
    }
}

Vector3 BTBossShoot::CalculateBulletDirection(const Vector3& baseDirection, float angleOffset) {
    if (std::abs(angleOffset) < kAngleEpsilon) {
        // オフセットがない場合はそのまま返す
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

nlohmann::json BTBossShoot::ExtractParameters() const {
    return {
        {"chargeTime", chargeTime_},
        {"recoveryTime", recoveryTime_},
        {"bulletSpeed", bulletSpeed_},
        {"spreadAngle", spreadAngle_},
        {"bulletCount",bulletCount_}
    };
}

#ifdef _DEBUG
bool BTBossShoot::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Charge Time##shoot", &chargeTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Recovery Time##shoot", &recoveryTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Bullet Speed##shoot", &bulletSpeed_, 1.0f, 5.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::SliderAngle("Spread Angle##shoot", &spreadAngle_, 0.0f, 45.0f)) {
        changed = true;
    }
    if (ImGui::DragInt("Bullet Count##shoot", &bulletCount_, 1, 1, 10)) {
        changed = true;
    }

    return changed;
}
#endif