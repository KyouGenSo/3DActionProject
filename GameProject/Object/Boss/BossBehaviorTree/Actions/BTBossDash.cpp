#include "BTBossDash.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "../../../../Common/GameConst.h"
#include "RandomEngine.h"

#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossDash::BTBossDash() {
    name_ = "BossDash";
}

BTNodeStatus BTBossDash::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return BTNodeStatus::Failure;
    }

    float deltaTime = blackboard->GetDeltaTime();

    // 初回実行時の初期化
    if (isFirstExecute_) {
        InitializeDash(boss);
        isFirstExecute_ = false;
        boss->SetDashing(true);  // ダッシュ開始
    }

    // ダッシュ移動の更新
    UpdateDashMovement(boss, deltaTime);

    // 経過時間を更新
    elapsedTime_ += deltaTime;

    // ダッシュが完了したか確認
    if (elapsedTime_ >= dashDuration_) {
        // 最終位置に設定
        boss->SetTranslate(targetPosition_);

        // ダッシュ終了
        boss->SetDashing(false);

        // リセットして成功を返す
        isFirstExecute_ = true;
        elapsedTime_ = 0.0f;
        status_ = BTNodeStatus::Success;
        return BTNodeStatus::Success;
    }

    // まだダッシュ中
    status_ = BTNodeStatus::Running;
    return BTNodeStatus::Running;
}

void BTBossDash::Reset() {
    BTNode::Reset();
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    // 注意: Reset 時は boss 参照がないため、SetDashing(false)は呼べない
    // BossBehaviorTree::Update()でのリセット時にも安全に動作するよう
    // BTBossDash 自体は状態を保持しない設計とする
}

void BTBossDash::InitializeDash(Boss* boss) {
    // タイマーリセット
    elapsedTime_ = 0.0f;

    // 開始位置を記録
    startPosition_ = boss->GetTransform().translate;

    // ランダムな方向を生成
    RandomEngine* rng = RandomEngine::GetInstance();

    // XZ 平面上のランダムな方向を取得（Y=0で正規化済み）
    dashDirection_ = rng->GetRandomDirectionXZ();

    // ランダムなダッシュ距離を取得
    float dashDistance = rng->GetFloat(minDistance_, maxDistance_);

    // 目標位置を計算
    targetPosition_ = startPosition_ + dashDirection_ * dashDistance;

    // エリア内に収まるよう調整
    targetPosition_ = ClampToArea(targetPosition_, boss);

    // ダッシュ方向を再計算（エリア制限後）
    dashDirection_ = targetPosition_ - startPosition_;
    float actualDistance = dashDirection_.Length();
    if (actualDistance > kDirectionEpsilon) {
        dashDirection_ = dashDirection_.Normalize();
        // ダッシュ時間を調整（距離に応じて）
        dashDuration_ = actualDistance / dashSpeed_;
    }

    // ダッシュ方向を向く
    if (dashDirection_.Length() > kDirectionEpsilon) {
        float angle = atan2f(dashDirection_.x, dashDirection_.z);
        boss->SetRotate(Vector3(0.0f, angle, 0.0f));
    }
}

void BTBossDash::UpdateDashMovement(Boss* boss, float deltaTime) {
    deltaTime; // 未使用警告抑制
    if (elapsedTime_ < dashDuration_) {
        // ダッシュ中の移動
        float t = elapsedTime_ / dashDuration_;

        // イージング（加速→減速）
        t = t * t * (kEasingCoeffA - kEasingCoeffB * t);

        Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
        boss->SetTranslate(newPosition);

        // ダッシュエフェクト的な表現（少し振動させる）
        float vibration = sinf(elapsedTime_ * vibrationFreq_) * vibrationAmp_;
        Vector3 currentPos = boss->GetTransform().translate;
        currentPos.y += vibration;
        boss->SetTranslate(currentPos);
    }
}

Vector3 BTBossDash::ClampToArea(const Vector3& position, const Boss* boss) {
    Vector3 clampedPos = position;

    uint8_t phase = boss->GetPhase();

    float Xmin = GameConst::kStageXMin + GameConst::kAreaMargin;
    float Xmax = GameConst::kStageXMax - GameConst::kAreaMargin;
    float Zmin = GameConst::kStageZMin + GameConst::kAreaMargin;
    float Zmax = GameConst::kStageZMax - GameConst::kAreaMargin;

    if (phase == 2) {
        Xmin += GameConst::kBossPhase2AreaSize;
        Xmax -= GameConst::kBossPhase2AreaSize;
        Zmin += GameConst::kBossPhase2AreaSize;
        Zmax -= GameConst::kBossPhase2AreaSize;
    }

    // GameConstants のステージ境界を使用
    // X 座標の制限
    clampedPos.x = std::clamp(clampedPos.x, Xmin, Xmax);

    // Z 座標の制限
    clampedPos.z = std::clamp(clampedPos.z, Zmin, Zmax);

    // Y 座標は元の値を保持
    clampedPos.y = position.y;

    return clampedPos;
}

nlohmann::json BTBossDash::ExtractParameters() const {
    return {
        {"dashSpeed", dashSpeed_},
        {"dashDuration", dashDuration_}
    };
}

#ifdef _DEBUG
bool BTBossDash::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Speed##dash", &dashSpeed_, 1.0f, 10.0f, 200.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Duration##dash", &dashDuration_, 0.05f, 0.1f, 3.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Min Distance##dash", &minDistance_, 0.1f, 0.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Max Distance##dash", &maxDistance_, 0.1f, 0.0f, 100.0f)) {
        changed = true;
    }

    ImGui::Separator();
    ImGui::Text("Effect Parameters:");
    if (ImGui::DragFloat("Vibration Freq##dash", &vibrationFreq_, 1.0f, 1.0f, 100.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Vibration Amp##dash", &vibrationAmp_, 0.01f, 0.0f, 0.5f)) {
        changed = true;
    }

    return changed;
}
#endif