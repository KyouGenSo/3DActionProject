#include "BTBossIdle.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "Vector3.h"
#include <cmath>
#include <numbers>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossIdle::BTBossIdle() {
    name_ = "BossIdle";
}

Tako::BTNodeStatus BTBossIdle::Execute(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    if (!boss) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    if (isFirstExecute_) {
        elapsedTime_ = 0.0f;
        isFirstExecute_ = false;
    }

    LookAtPlayer(blackboard, deltaTime);

    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= idleDuration_) {
        isFirstExecute_ = true;
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    status_ = Tako::BTNodeStatus::Running;
    return Tako::BTNodeStatus::Running;
}

void BTBossIdle::Reset() {
    Tako::BTNode::Reset();
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
}

void BTBossIdle::LookAtPlayer(Tako::BTBlackboard* blackboard, float deltaTime) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    Player* player = blackboard->GetPtr<Player>("player");
    if (!player) {
        return;
    }

    Vector3 playerPos = player->GetTransform().translate;
    Vector3 bossPos = boss->GetTransform().translate;
    Vector3 toPlayer = playerPos - bossPos;
    toPlayer.y = 0.0f;  // Y 軸は無視

    if (toPlayer.Length() > kDirectionEpsilon) {
        toPlayer.Normalize();

        float targetAngle = atan2f(toPlayer.x, toPlayer.z);

        float currentAngle = boss->GetTransform().rotate.y;

        // 角度差を -π～π に正規化
        constexpr float kPi = static_cast<float>(std::numbers::pi);
        constexpr float kTwoPi = 2.0f * kPi;
        float angleDiff = targetAngle - currentAngle;
        while (angleDiff > kPi) angleDiff -= kTwoPi;
        while (angleDiff < -kPi) angleDiff += kTwoPi;

        float rotationAmount = angleDiff * rotationSpeed_ * deltaTime;

        // 急激な回転を防ぐため1フレームの回転量を制限
        float maxRotationPerFrame = rotationSpeed_ * deltaTime;
        if (fabs(rotationAmount) > maxRotationPerFrame) {
            rotationAmount = (rotationAmount > 0) ? maxRotationPerFrame : -maxRotationPerFrame;
        }

        Transform transform = boss->GetTransform();
        transform.rotate.y += rotationAmount;
        boss->SetTransform(transform);
    }
}

nlohmann::json BTBossIdle::ExtractParameters() const {
    return {{"idleDuration", idleDuration_}};
}

#ifdef _DEBUG
bool BTBossIdle::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Idle Duration##idle", &idleDuration_, 0.1f, 0.0f, 10.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Rotation Speed##idle", &rotationSpeed_, 0.1f, 0.1f, 20.0f)) {
        changed = true;
    }

    return changed;
}
#endif
