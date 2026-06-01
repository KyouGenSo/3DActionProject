#include "BTBossBarrage.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "../../../../Common/GameConst.h"
#include "RandomEngine.h"
#include "EaseFunc.h"

#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossBarrage::BTBossBarrage() {
    name_ = "BossBarrage";
}

Tako::BTNodeStatus BTBossBarrage::OnExecute(Tako::BTBlackboard* /*blackboard*/, Boss* boss, float deltaTime) {
    // フェーズ管理: Move → Charge → Firing → Recovery
    const float moveEnd = moveDuration_;
    const float chargeEnd = moveEnd + chargeTime_;
    const float firingEnd = chargeEnd + firingDuration_;

    // Phase: Move（ステージ中央への移動）
    if (elapsedTime_ < moveEnd) {
        UpdateMove(boss, deltaTime);
    }
    // Phase: Charge（射撃予兆）
    else if (elapsedTime_ < chargeEnd) {
        bulletSignEffect_.Update(boss, deltaTime);
    }
    // Phase: Firing（弾幕発射）
    else if (elapsedTime_ < firingEnd) {
        if (!hasEndedEffect_) {
            bulletSignEffect_.End(boss);
            hasEndedEffect_ = true;
        }

        timeSinceLastFire_ += deltaTime;
        if (timeSinceLastFire_ >= fireInterval_) {
            FireRandomBullet(boss);
            timeSinceLastFire_ = 0.0f;
        }
    }
    // Phase: Recovery（硬直）— 突入時に EnterAttackRecovery を 1 度だけ実行（多重呼び出しは Helper がガード）
    else {
        EnterAttackRecovery(boss);
    }

    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= totalDuration_) {
        timeSinceLastFire_ = 0.0f;
        hasEndedEffect_ = false;
        return FinishAttack();
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossBarrage::OnInitialize(Tako::BTBlackboard* /*blackboard*/, Boss* boss) {
    timeSinceLastFire_ = 0.0f;
    hasEndedEffect_ = false;

    totalDuration_ = moveDuration_ + chargeTime_ + firingDuration_ + recoveryTime_;

    startPosition_ = boss->GetTransform().translate;
    float targetX = (GameConst::kStageXMin + GameConst::kStageXMax) / 2.0f;
    float targetZ = (GameConst::kStageZMin + GameConst::kStageZMax) / 2.0f;
    targetPosition_ = Vector3(targetX, startPosition_.y, targetZ);

    bulletSignEffect_.Start(boss, chargeTime_);
}

void BTBossBarrage::UpdateMove(Boss* boss, float /*deltaTime*/) {
    if (elapsedTime_ < moveDuration_) {
        float t = elapsedTime_ / moveDuration_;
        // イージング（加速→減速）
        t = Ease::SmoothStep(t);

        Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
        boss->SetTranslate(newPosition);

        Vector3 direction = targetPosition_ - startPosition_;
        direction.y = 0.0f;
        if (direction.Length() > kDirectionEpsilon) {
            direction = direction.Normalize();
            float angle = atan2f(direction.x, direction.z);
            boss->SetRotate(Vector3(0.0f, angle, 0.0f));
        }
    } else {
        boss->SetTranslate(targetPosition_);
    }
}

void BTBossBarrage::FireRandomBullet(Boss* boss) {
    Vector3 firePosition = boss->GetTransform().translate;

    RandomEngine* rng = RandomEngine::GetInstance();
    Vector3 direction = rng->GetRandomDirectionXZ();

    bool isPenetrating = rng->GetBool(penetratingRatio_);

    float speed;
    if (isPenetrating) {
        speed = rng->GetFloat(penetratingBulletSpeedMin_, penetratingBulletSpeedMax_);
        boss->RequestPenetratingBulletSpawn(firePosition, direction * speed);
    } else {
        speed = rng->GetFloat(normalBulletSpeedMin_, normalBulletSpeedMax_);
        boss->RequestBulletSpawn(firePosition, direction * speed);
    }
}

void BTBossBarrage::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("moveDuration"))              moveDuration_ = params["moveDuration"];
    if (params.contains("chargeTime"))                chargeTime_ = params["chargeTime"];
    if (params.contains("firingDuration"))            firingDuration_ = params["firingDuration"];
    if (params.contains("recoveryTime"))              recoveryTime_ = params["recoveryTime"];
    if (params.contains("fireInterval"))              fireInterval_ = params["fireInterval"];
    if (params.contains("normalBulletSpeedMin"))      normalBulletSpeedMin_ = params["normalBulletSpeedMin"];
    if (params.contains("normalBulletSpeedMax"))      normalBulletSpeedMax_ = params["normalBulletSpeedMax"];
    if (params.contains("penetratingBulletSpeedMin")) penetratingBulletSpeedMin_ = params["penetratingBulletSpeedMin"];
    if (params.contains("penetratingBulletSpeedMax")) penetratingBulletSpeedMax_ = params["penetratingBulletSpeedMax"];
    if (params.contains("penetratingRatio"))          penetratingRatio_ = params["penetratingRatio"];
}

void BTBossBarrage::OnExtractParameters(nlohmann::json& out) const {
    out["moveDuration"]              = moveDuration_;
    out["chargeTime"]                = chargeTime_;
    out["firingDuration"]            = firingDuration_;
    out["recoveryTime"]              = recoveryTime_;
    out["fireInterval"]              = fireInterval_;
    out["normalBulletSpeedMin"]      = normalBulletSpeedMin_;
    out["normalBulletSpeedMax"]      = normalBulletSpeedMax_;
    out["penetratingBulletSpeedMin"] = penetratingBulletSpeedMin_;
    out["penetratingBulletSpeedMax"] = penetratingBulletSpeedMax_;
    out["penetratingRatio"]          = penetratingRatio_;
}

#ifdef _DEBUG
bool BTBossBarrage::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Move Duration##barrage", &moveDuration_, 0.05f, 0.1f, 2.0f))   changed = true;
    if (ImGui::DragFloat("Charge Time##barrage", &chargeTime_, 0.05f, 0.0f, 3.0f))       changed = true;
    if (ImGui::DragFloat("Firing Duration##barrage", &firingDuration_, 0.1f, 0.5f, 10.0f)) changed = true;
    if (ImGui::DragFloat("Recovery Time##barrage", &recoveryTime_, 0.05f, 0.0f, 3.0f))   changed = true;

    ImGui::SeparatorText("Fire Control");
    if (ImGui::DragFloat("Fire Interval##barrage", &fireInterval_, 0.01f, 0.02f, 0.5f))  changed = true;
    float fireRate = 1.0f / fireInterval_;
    ImGui::Text("Fire Rate: %.1f shots/sec", fireRate);

    ImGui::SeparatorText("Normal Bullet Speed");
    if (ImGui::DragFloat("Speed Min##normal", &normalBulletSpeedMin_, 1.0f, 5.0f, 100.0f))      changed = true;
    if (ImGui::DragFloat("Speed Max##normal", &normalBulletSpeedMax_, 1.0f, 5.0f, 100.0f))      changed = true;

    ImGui::SeparatorText("Penetrating Bullet Speed");
    if (ImGui::DragFloat("Speed Min##penetrating", &penetratingBulletSpeedMin_, 1.0f, 5.0f, 50.0f)) changed = true;
    if (ImGui::DragFloat("Speed Max##penetrating", &penetratingBulletSpeedMax_, 1.0f, 5.0f, 50.0f)) changed = true;

    ImGui::SeparatorText("Bullet Type Ratio");
    if (ImGui::SliderFloat("Penetrating Ratio##barrage", &penetratingRatio_, 0.0f, 1.0f))           changed = true;

    return changed;
}
#endif
