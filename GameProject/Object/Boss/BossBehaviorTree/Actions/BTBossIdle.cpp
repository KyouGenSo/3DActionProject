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

    // 初回実行時の初期化
    if (isFirstExecute_) {
        elapsedTime_ = 0.0f;
        isFirstExecute_ = false;

        // 次のアクションカウンターをインクリメント
        int actionCounter = blackboard->GetInt("ActionCounter", 0);
        blackboard->SetInt("ActionCounter", actionCounter + 1);
    }

    // プレイヤーの方向を向く
    LookAtPlayer(blackboard, deltaTime);

    // 経過時間を更新
    elapsedTime_ += deltaTime;

    // 待機時間が経過したら成功を返す
    if (elapsedTime_ >= idleDuration_) {
        isFirstExecute_ = true;  // 次回実行時のためにリセット
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    // まだ待機中
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

        // 向きたい角度を計算
        float targetAngle = atan2f(toPlayer.x, toPlayer.z);

        // 現在の角度
        float currentAngle = boss->GetTransform().rotate.y;

        // 角度差を計算（-π～πの範囲に正規化）
        constexpr float kPi = static_cast<float>(std::numbers::pi);
        constexpr float kTwoPi = 2.0f * kPi;
        float angleDiff = targetAngle - currentAngle;
        while (angleDiff > kPi) angleDiff -= kTwoPi;
        while (angleDiff < -kPi) angleDiff += kTwoPi;

        // スムーズに回転（回転速度を調整）
        float rotationAmount = angleDiff * rotationSpeed_ * deltaTime;

        // 回転量を制限（急激な回転を防ぐ）
        float maxRotationPerFrame = rotationSpeed_ * deltaTime;
        if (fabs(rotationAmount) > maxRotationPerFrame) {
            rotationAmount = (rotationAmount > 0) ? maxRotationPerFrame : -maxRotationPerFrame;
        }

        // 回転を適用
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
