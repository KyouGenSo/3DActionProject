#include "BTBossApproach.h"
#include "../../Boss.h"
#include "../../Movement/BossAreaBounds.h"
#include "../../../Player/Player.h"
#include "EaseFunc.h"

#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossApproach::BTBossApproach() {
    name_ = "BossApproach";
}

Tako::BTNodeStatus BTBossApproach::Execute(Tako::BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetPtr<Boss>("boss");
    if (!boss) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    Player* player = blackboard->GetPtr<Player>("player");
    if (!player) {
        status_ = Tako::BTNodeStatus::Failure;
        return Tako::BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    if (isFirstExecute_) {
        InitializeApproach(boss, player);
        isFirstExecute_ = false;

        // 既に目標距離内なら即成功
        if (approachDuration_ <= 0.0f) {
            isFirstExecute_ = true;
            status_ = Tako::BTNodeStatus::Success;
            return Tako::BTNodeStatus::Success;
        }
    }

    UpdateApproachMovement(boss, deltaTime);

    elapsedTime_ += deltaTime;

    Vector3 currentPos = boss->GetTransform().translate;
    Vector3 diff = currentPos - targetPosition_;
    diff.y = 0.0f;  // 水平距離のみ
    float distanceToTarget = diff.Length();

    if (distanceToTarget < kArrivalThreshold) {
        boss->SetTranslate(targetPosition_);

        isFirstExecute_ = true;
        elapsedTime_ = 0.0f;
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    status_ = Tako::BTNodeStatus::Running;
    return Tako::BTNodeStatus::Running;
}

void BTBossApproach::Reset() {
    Tako::BTNode::Reset();
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    approachDuration_ = 0.0f;
}

void BTBossApproach::InitializeApproach(Boss* boss, Player* player) {
    elapsedTime_ = 0.0f;

    startPosition_ = boss->GetTransform().translate;

    Vector3 playerPos = player->GetTransform().translate;

    Vector3 toPlayer = playerPos - startPosition_;
    toPlayer.y = 0.0f;  // 水平面のみ
    float distance = toPlayer.Length();

    if (distance > kDirectionEpsilon) {
        Vector3 direction = toPlayer.Normalize();

        float angle = atan2f(direction.x, direction.z);
        boss->SetRotate(Vector3(0.0f, angle, 0.0f));

        // 目標位置 = プレイヤーから targetDistance_ 手前
        float approachDistance = distance - targetDistance_;
        if (approachDistance > 0.0f) {
            targetPosition_ = startPosition_ + direction * approachDistance;
            targetPosition_ = BossMovement::ClampToBounds(targetPosition_, BossMovement::CalcStageBounds());

            // Clamp 後の実移動距離から所要時間を算出
            Vector3 actualMove = targetPosition_ - startPosition_;
            actualMove.y = 0.0f;
            float actualDistance = actualMove.Length();
            approachDuration_ = actualDistance / approachSpeed_;
        }
        else {
            // 既に目標距離内
            targetPosition_ = startPosition_;
            approachDuration_ = 0.0f;
        }
    }
    else {
        // プレイヤーとほぼ同位置
        targetPosition_ = startPosition_;
        approachDuration_ = 0.0f;
    }
}

void BTBossApproach::UpdateApproachMovement(Boss* boss, float deltaTime) {
    deltaTime; // 未使用警告抑制

    if (approachDuration_ > 0.0f) {
        float t = elapsedTime_ / approachDuration_;

        // 超過しても t=1.0 で頭打ち
        t = std::clamp(t, 0.0f, 1.0f);

        t = Ease::SmoothStep(t);

        Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
        boss->SetTranslate(newPosition);
    }
}

nlohmann::json BTBossApproach::ExtractParameters() const {
    return {
        {"approachSpeed", approachSpeed_},
        {"targetDistance", targetDistance_}
    };
}

#ifdef _DEBUG
bool BTBossApproach::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Approach Speed##approach", &approachSpeed_, 1.0f, 10.0f, 200.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Target Distance##approach", &targetDistance_, 0.5f, 1.0f, 50.0f)) {
        changed = true;
    }

    ImGui::Separator();
    ImGui::Text("Runtime Info:");
    ImGui::Text("Duration: %.2f sec", approachDuration_);
    ImGui::Text("Elapsed: %.2f sec", elapsedTime_);

    return changed;
}
#endif
