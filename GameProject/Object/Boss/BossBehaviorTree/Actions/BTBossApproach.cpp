#include "BTBossApproach.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "../../../../Common/GameConst.h"

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

    // 初回実行時の初期化
    if (isFirstExecute_) {
        InitializeApproach(boss, player);
        isFirstExecute_ = false;

        // 既に目標距離内にいる場合は即座に成功
        if (approachDuration_ <= 0.0f) {
            isFirstExecute_ = true;
            status_ = Tako::BTNodeStatus::Success;
            return Tako::BTNodeStatus::Success;
        }
    }

    // 接近移動の更新
    UpdateApproachMovement(boss, deltaTime);

    // 経過時間を更新
    elapsedTime_ += deltaTime;

    // 終了判定（位置ベース）
    Vector3 currentPos = boss->GetTransform().translate;
    Vector3 diff = currentPos - targetPosition_;
    diff.y = 0.0f;  // 水平距離のみ
    float distanceToTarget = diff.Length();

    if (distanceToTarget < kArrivalThreshold) {
        // 目標位置に到達
        boss->SetTranslate(targetPosition_);

        // リセットして成功を返す
        isFirstExecute_ = true;
        elapsedTime_ = 0.0f;
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    // まだ接近中
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
    // タイマーリセット
    elapsedTime_ = 0.0f;

    // 開始位置を記録
    startPosition_ = boss->GetTransform().translate;

    // プレイヤー位置を取得
    Vector3 playerPos = player->GetTransform().translate;

    // プレイヤーへの方向ベクトル
    Vector3 toPlayer = playerPos - startPosition_;
    toPlayer.y = 0.0f;  // 水平面のみ
    float distance = toPlayer.Length();

    if (distance > kDirectionEpsilon) {
        Vector3 direction = toPlayer.Normalize();

        // プレイヤー方向を向く
        float angle = atan2f(direction.x, direction.z);
        boss->SetRotate(Vector3(0.0f, angle, 0.0f));

        // 目標位置 = プレイヤー位置から targetDistance_ 手前
        float approachDistance = distance - targetDistance_;
        if (approachDistance > 0.0f) {
            targetPosition_ = startPosition_ + direction * approachDistance;
            targetPosition_ = ClampToArea(targetPosition_);

            // 実際の移動距離から所要時間を計算
            Vector3 actualMove = targetPosition_ - startPosition_;
            actualMove.y = 0.0f;
            float actualDistance = actualMove.Length();
            approachDuration_ = actualDistance / approachSpeed_;
        }
        else {
            // 既に目標距離内にいる
            targetPosition_ = startPosition_;
            approachDuration_ = 0.0f;
        }
    }
    else {
        // プレイヤーとほぼ同じ位置
        targetPosition_ = startPosition_;
        approachDuration_ = 0.0f;
    }
}

void BTBossApproach::UpdateApproachMovement(Boss* boss, float deltaTime) {
    deltaTime; // 未使用警告抑制

    if (approachDuration_ > 0.0f) {
        // 接近中の移動
        float t = elapsedTime_ / approachDuration_;

        // clamp to [0, 1] - これにより elapsedTime_ > approachDuration_ でも t=1.0 となる
        t = std::clamp(t, 0.0f, 1.0f);

        // イージング（加速→減速）: smoothstep
        t = t * t * (kEasingCoeffA - kEasingCoeffB * t);

        Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
        boss->SetTranslate(newPosition);
    }
}

Vector3 BTBossApproach::ClampToArea(const Vector3& position) {
    Vector3 clampedPos = position;

    // GameConstants のステージ境界を使用
    // X 座標の制限
    clampedPos.x = std::clamp(clampedPos.x, GameConst::kStageXMin + GameConst::kAreaMargin, GameConst::kStageXMax - GameConst::kAreaMargin);

    // Z 座標の制限
    clampedPos.z = std::clamp(clampedPos.z, GameConst::kStageZMin + GameConst::kAreaMargin, GameConst::kStageZMax - GameConst::kAreaMargin);

    // Y 座標は元の値を保持
    clampedPos.y = position.y;

    return clampedPos;
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
