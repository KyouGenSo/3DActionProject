#include "BTBossRapidFire.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "../../../../Common/GameConst.h"
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossRapidFire::BTBossRapidFire() {
    name_ = "BossRapidFire";
}

BTNodeStatus BTBossRapidFire::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    // 初回実行時の初期化
    if (isFirstExecute_) {
        InitializeRapidFire(boss);
        isFirstExecute_ = false;
    }

    // フェーズ1: チャージ中（プレイヤーに照準）
    if (elapsedTime_ < chargeTime_) {
        AimAtPlayer(blackboard, deltaTime);
        bulletSignEffect_.Update(boss, deltaTime);
    }
    // フェーズ2: 連続発射中（追尾しながら発射）
    else if (firedCount_ < bulletCount_) {
        // 最初の発射開始時にエフェクト終了
        if (bulletSignEffect_.IsActive()) {
            bulletSignEffect_.End(boss);
        }

        // 発射中もプレイヤー方向を追尾
        AimAtPlayer(blackboard, deltaTime);

        // 発射間隔チェック
        timeSinceLastFire_ += deltaTime;
        if (timeSinceLastFire_ >= fireInterval_) {
            FireBullet(blackboard);
            firedCount_++;
            timeSinceLastFire_ = 0.0f;

            // 最後の弾を発射したら硬直フェーズ開始
            if (firedCount_ >= bulletCount_) {
                boss->EnterRecovery();
            }
        }
    }
    // フェーズ3: 硬直中（何もしない）

    // 経過時間を更新
    elapsedTime_ += deltaTime;

    // 状態終了チェック
    if (elapsedTime_ >= totalDuration_) {
        // 硬直フェーズ終了
        boss->ExitRecovery();

        // リセットして成功を返す
        isFirstExecute_ = true;
        elapsedTime_ = 0.0f;
        firedCount_ = 0;
        timeSinceLastFire_ = 0.0f;
        status_ = BTNodeStatus::Success;
        return BTNodeStatus::Success;
    }

    // まだ射撃処理中
    status_ = BTNodeStatus::Running;
    return BTNodeStatus::Running;
}

void BTBossRapidFire::Reset() {
    BTNode::Reset();
    elapsedTime_ = 0.0f;
    firedCount_ = 0;
    timeSinceLastFire_ = 0.0f;
    isFirstExecute_ = true;
    // 注意: Reset 時は boss 参照がないため、ExitRecovery()は呼べない
}

void BTBossRapidFire::InitializeRapidFire(Boss* boss) {
    // タイマーリセット
    elapsedTime_ = 0.0f;
    firedCount_ = 0;
    // 即座に1発目を撃てるように
    timeSinceLastFire_ = fireInterval_;

    // totalDuration を計算
    // チャージ時間 + (発射間隔 × 弾数) + 硬直時間
    totalDuration_ = chargeTime_ + (fireInterval_ * static_cast<float>(bulletCount_)) + recoveryTime_;

    // 射撃予兆エフェクト開始
    bulletSignEffect_.Start(boss, chargeTime_);
}

void BTBossRapidFire::AimAtPlayer(BTBlackboard* blackboard, float deltaTime) {
    Boss* boss = blackboard->GetBoss();
    Player* player = blackboard->GetPlayer();
    if (!player) {
        return;
    }

    Vector3 playerPos = player->GetTransform().translate;
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 toPlayer = playerPos - bossPos;
    toPlayer.y = 0.0f; // Y 軸は無視

    if (toPlayer.Length() > GameConst::kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
        float angle = atan2f(toPlayer.x, toPlayer.z);
        boss->SetRotate(Vector3(0.0f, angle, 0.0f));
    }
}

void BTBossRapidFire::FireBullet(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    // 発射位置（ボスの座標）
    Vector3 firePosition = boss->GetTransform().translate;

    // プレイヤーへの方向を計算
    Vector3 direction = CalculateDirectionToPlayer(blackboard);

    // 弾の速度ベクトルを計算
    Vector3 bulletVelocity = direction * bulletSpeed_;

    // ボスに弾生成をリクエスト
    boss->RequestBulletSpawn(firePosition, bulletVelocity);
}

Vector3 BTBossRapidFire::CalculateDirectionToPlayer(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    Player* player = blackboard->GetPlayer();
    if (!player) {
        // プレイヤーがいない場合は前方向を返す
        return Vector3(0.0f, 0.0f, 1.0f);
    }

    Vector3 firePosition = boss->GetTransform().translate;
    Vector3 playerPos = player->GetTransform().translate;
    Vector3 toPlayer = playerPos - firePosition;

    // 水平方向のみで計算
    float distance = sqrtf(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    if (distance > GameConst::kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
    } else {
        // プレイヤーが真上にいる場合など
        toPlayer = Vector3(0.0f, 0.0f, 1.0f);
    }

    return toPlayer;
}

nlohmann::json BTBossRapidFire::ExtractParameters() const {
    return {
        {"chargeTime", chargeTime_},
        {"bulletCount", bulletCount_},
        {"fireInterval", fireInterval_},
        {"bulletSpeed", bulletSpeed_},
        {"recoveryTime", recoveryTime_}
    };
}

#ifdef _DEBUG
bool BTBossRapidFire::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Charge Time##rapidfire", &chargeTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragInt("Bullet Count##rapidfire", &bulletCount_, 1, 1, 20)) {
        changed = true;
    }
    if (ImGui::DragFloat("Fire Interval##rapidfire", &fireInterval_, 0.01f, 0.05f, 1.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Bullet Speed##rapidfire", &bulletSpeed_, 1.0f, 5.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Recovery Time##rapidfire", &recoveryTime_, 0.05f, 0.0f, 3.0f)) {
        changed = true;
    }

    return changed;
}
#endif
