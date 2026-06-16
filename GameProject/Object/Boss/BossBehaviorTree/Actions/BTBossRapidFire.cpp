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

Tako::BTNodeStatus BTBossRapidFire::OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) {
    // フェーズ1: チャージ中（プレイヤーに照準）
    if (elapsedTime_ < chargeTime_) {
        FacePlayerInstant(blackboard);
        bulletSignEffect_.Update(boss, deltaTime);
    }
    // フェーズ2: 連続発射中（追尾しながら発射）
    else if (firedCount_ < bulletCount_) {
        // 最初の発射開始時にエフェクト終了
        if (bulletSignEffect_.IsActive()) {
            bulletSignEffect_.End(boss);
        }

        // 発射中もプレイヤー方向を追尾
        FacePlayerInstant(blackboard);

        // 発射間隔チェック
        timeSinceLastFire_ += deltaTime;
        if (timeSinceLastFire_ >= fireInterval_) {
            FireBullet(blackboard);
            firedCount_++;
            timeSinceLastFire_ = 0.0f;

            // 最後の弾を発射したら硬直フェーズ開始
            if (firedCount_ >= bulletCount_) {
                EnterAttackRecovery(boss);
            }
        }
    }
    // フェーズ3: 硬直中（何もしない）

    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= totalDuration_) {
        firedCount_ = 0;
        timeSinceLastFire_ = 0.0f;
        return FinishAttack();
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossRapidFire::OnInitialize(Tako::BTBlackboard* /*blackboard*/, Boss* boss) {
    firedCount_ = 0;
    // 即座に 1 発目を撃てるよう発射タイマを満タンに
    timeSinceLastFire_ = fireInterval_;
    // totalDuration: チャージ + 発射全体 + 硬直
    totalDuration_ = chargeTime_ + (fireInterval_ * static_cast<float>(bulletCount_)) + recoveryTime_;
    bulletSignEffect_.Start(boss, chargeTime_);
}

void BTBossRapidFire::FireBullet(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Vector3 firePosition = boss->GetTransform().translate;
    Vector3 direction = CalculateDirectionToPlayer(blackboard);
    Vector3 bulletVelocity = direction * bulletSpeed_;
    boss->RequestBulletSpawn(firePosition, bulletVelocity);
}

Vector3 BTBossRapidFire::CalculateDirectionToPlayer(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Player* player = blackboard->GetPtr<Player>("player");
    if (!player) return Vector3(0.0f, 0.0f, 1.0f);

    Vector3 firePosition = boss->GetTransform().translate;
    Vector3 playerPos = player->GetTransform().translate;
    Vector3 toPlayer = playerPos - firePosition;

    float distance = sqrtf(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    if (distance > GameConst::kDirectionEpsilon) {
        toPlayer = toPlayer.Normalize();
    } else {
        toPlayer = Vector3(0.0f, 0.0f, 1.0f);
    }
    return toPlayer;
}

void BTBossRapidFire::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("chargeTime"))   chargeTime_ = params["chargeTime"];
    if (params.contains("bulletCount"))  bulletCount_ = params["bulletCount"];
    if (params.contains("fireInterval")) fireInterval_ = params["fireInterval"];
    if (params.contains("bulletSpeed"))  bulletSpeed_ = params["bulletSpeed"];
    if (params.contains("recoveryTime")) recoveryTime_ = params["recoveryTime"];
}

void BTBossRapidFire::OnExtractParameters(nlohmann::json& out) const {
    out["chargeTime"]   = chargeTime_;
    out["bulletCount"]  = bulletCount_;
    out["fireInterval"] = fireInterval_;
    out["bulletSpeed"]  = bulletSpeed_;
    out["recoveryTime"] = recoveryTime_;
}

#ifdef _DEBUG
bool BTBossRapidFire::OnDrawImGui() {
    bool changed = false;
    if (ImGui::DragFloat("Charge Time##rapidfire", &chargeTime_, 0.05f, 0.0f, 3.0f))      changed = true;
    if (ImGui::DragInt("Bullet Count##rapidfire", &bulletCount_, 1, 1, 20))               changed = true;
    if (ImGui::DragFloat("Fire Interval##rapidfire", &fireInterval_, 0.01f, 0.05f, 1.0f)) changed = true;
    if (ImGui::DragFloat("Bullet Speed##rapidfire", &bulletSpeed_, 1.0f, 5.0f, 100.0f))   changed = true;
    if (ImGui::DragFloat("Recovery Time##rapidfire", &recoveryTime_, 0.05f, 0.0f, 3.0f))  changed = true;
    return changed;
}
#endif
