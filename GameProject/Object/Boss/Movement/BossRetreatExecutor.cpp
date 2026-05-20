#include "BossRetreatExecutor.h"

#include "BossAreaBounds.h"
#include "../Boss.h"
#include "../../Player/Player.h"
#include "Mat4x4Func.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

void BossRetreatExecutor::Begin(Boss* boss, const Player* player) {
    elapsedTime_     = 0.0f;
    retreatDuration_ = 0.0f;
    startPosition_   = boss->GetTransform().translate;
    targetPosition_  = startPosition_;

    if (!player) {
        return;
    }

    const Vector3 playerPos = player->GetTransform().translate;

    Vector3 toRetreat = startPosition_ - playerPos;
    toRetreat.y = 0.0f;
    const float currentDistance = toRetreat.Length();

    if (currentDistance <= kDirectionEpsilon) {
        return;
    }

    const Vector3 primaryDirection = toRetreat.Normalize();

    const float retreatDistance = params_.targetDistance - currentDistance;
    if (retreatDistance <= 0.0f) {
        return;
    }

    const Vector3 bestDirection = FindBestRetreatDirection(boss, primaryDirection, retreatDistance);

    const float angle = atan2f(-bestDirection.x, -bestDirection.z);
    boss->SetRotate(Vector3(0.0f, angle, 0.0f));

    targetPosition_ = startPosition_ + bestDirection * retreatDistance;
    targetPosition_ = BossMovement::ClampToBounds(targetPosition_, BossMovement::CalcAreaBounds(boss));

    Vector3 actualMove = targetPosition_ - startPosition_;
    actualMove.y = 0.0f;
    const float actualDistance = actualMove.Length();
    retreatDuration_ = actualDistance / params_.retreatSpeed;
}

void BossRetreatExecutor::Tick(Boss* boss, float deltaTime) {
    if (retreatDuration_ <= 0.0f) {
        return;
    }

    elapsedTime_ += deltaTime;

    float t = elapsedTime_ / retreatDuration_;
    t = std::clamp(t, 0.0f, 1.0f);
    t = t * t * (kEasingCoeffA - kEasingCoeffB * t);

    const Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
    boss->SetTranslate(newPosition);
}

bool BossRetreatExecutor::IsFinished(const Boss* boss) const {
    if (retreatDuration_ <= 0.0f) {
        return true;
    }

    Vector3 diff = boss->GetTransform().translate - targetPosition_;
    diff.y = 0.0f;
    return diff.Length() < kArrivalThreshold;
}

void BossRetreatExecutor::SnapToTarget(Boss* boss) const {
    boss->SetTranslate(targetPosition_);
}

void BossRetreatExecutor::Reset() {
    startPosition_   = {};
    targetPosition_  = {};
    elapsedTime_     = 0.0f;
    retreatDuration_ = 0.0f;
}

void BossRetreatExecutor::ApplyJson(const nlohmann::json& j) {
    if (j.contains("retreatSpeed")) {
        params_.retreatSpeed = j["retreatSpeed"];
    }
    if (j.contains("targetDistance")) {
        params_.targetDistance = j["targetDistance"];
    }
}

nlohmann::json BossRetreatExecutor::ToJson() const {
    return {
        {"retreatSpeed",   params_.retreatSpeed},
        {"targetDistance", params_.targetDistance}
    };
}

Vector3 BossRetreatExecutor::FindBestRetreatDirection(const Boss* boss,
                                                      const Vector3& primaryDirection,
                                                      float retreatDistance) const {
    const float primaryScore = EvaluateDirection(boss, primaryDirection, retreatDistance);

    if (primaryScore >= kMinRetreatDistance) {
        return primaryDirection;
    }

    constexpr float kHalfPi = std::numbers::pi_v<float> / 2.0f;
    constexpr float kPi     = std::numbers::pi_v<float>;

    const Matrix4x4 rotLeft90  = Mat4x4::MakeRotateY(kHalfPi);
    const Matrix4x4 rotRight90 = Mat4x4::MakeRotateY(-kHalfPi);
    const Matrix4x4 rot180     = Mat4x4::MakeRotateY(kPi);

    struct DirectionCandidate {
        Vector3 direction;
        float   score;
    };

    std::array<DirectionCandidate, 4> candidates = { {
        { primaryDirection, primaryScore },
        { Mat4x4::TransformNormal(rotLeft90,  primaryDirection), 0.0f },
        { Mat4x4::TransformNormal(rotRight90, primaryDirection), 0.0f },
        { Mat4x4::TransformNormal(rot180,     primaryDirection), 0.0f }
    } };

    for (size_t i = 1; i < candidates.size(); ++i) {
        candidates[i].score = EvaluateDirection(boss, candidates[i].direction, retreatDistance);
    }

    auto best = std::ranges::max_element(candidates,
        [](const DirectionCandidate& a, const DirectionCandidate& b) {
            return a.score < b.score;
        });

    return best->direction;
}

float BossRetreatExecutor::EvaluateDirection(const Boss* boss,
                                             const Vector3& direction,
                                             float retreatDistance) const {
    Vector3 targetPos = startPosition_ + direction * retreatDistance;
    targetPos = BossMovement::ClampToBounds(targetPos, BossMovement::CalcAreaBounds(boss));

    Vector3 actualMove = targetPos - startPosition_;
    actualMove.y = 0.0f;
    return actualMove.Length();
}

#ifdef _DEBUG
bool BossRetreatExecutor::DrawImGui(const char* idSuffix) {
    bool changed = false;

    const std::string speedLabel  = std::string("Retreat Speed")   + idSuffix;
    const std::string targetLabel = std::string("Target Distance") + idSuffix;

    if (ImGui::DragFloat(speedLabel.c_str(), &params_.retreatSpeed, 1.0f, 10.0f, 200.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat(targetLabel.c_str(), &params_.targetDistance, 0.5f, 1.0f, 100.0f)) {
        changed = true;
    }

    ImGui::Separator();
    ImGui::Text("Runtime Info:");
    ImGui::Text("Duration: %.2f sec", retreatDuration_);
    ImGui::Text("Elapsed: %.2f sec", elapsedTime_);

    return changed;
}
#endif
