#include "BTBossRetreat.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "../../../../Common/GameConst.h"
#include "Mat4x4Func.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossRetreat::BTBossRetreat() {
    name_ = "BossRetreat";
}

Tako::BTNodeStatus BTBossRetreat::Execute(Tako::BTBlackboard* blackboard) {
    boss_ = blackboard->GetPtr<Boss>("boss");
    if (!boss_) {
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
        InitializeRetreat(player);
        isFirstExecute_ = false;

        // 既に目標距離以上離れている場合は即座に成功
        if (retreatDuration_ <= 0.0f) {
            isFirstExecute_ = true;
            status_ = Tako::BTNodeStatus::Success;
            return Tako::BTNodeStatus::Success;
        }
    }

    // 離脱移動の更新
    UpdateRetreatMovement(deltaTime);

    // 経過時間を更新
    elapsedTime_ += deltaTime;

    // 終了判定（位置ベース）
    Vector3 currentPos = boss_->GetTransform().translate;
    Vector3 diff = currentPos - targetPosition_;
    diff.y = 0.0f;  // 水平距離のみ
    float distanceToTarget = diff.Length();

    if (distanceToTarget < kArrivalThreshold) {
        // 目標位置に到達
        boss_->SetTranslate(targetPosition_);

        // リセットして成功を返す
        isFirstExecute_ = true;
        elapsedTime_ = 0.0f;
        status_ = Tako::BTNodeStatus::Success;
        return Tako::BTNodeStatus::Success;
    }

    // まだ離脱中
    status_ = Tako::BTNodeStatus::Running;
    return Tako::BTNodeStatus::Running;
}

void BTBossRetreat::Reset() {
    Tako::BTNode::Reset();
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;
    retreatDuration_ = 0.0f;
}

void BTBossRetreat::InitializeRetreat(const Player* player) {
    // タイマーリセット
    elapsedTime_ = 0.0f;

    // 開始位置を記録
    startPosition_ = boss_->GetTransform().translate;

    // プレイヤー位置を取得
    Vector3 playerPos = player->GetTransform().translate;

    // プレイヤーからボスへの方向ベクトル（離れる方向）
    Vector3 toRetreat = startPosition_ - playerPos;
    toRetreat.y = 0.0f;  // 水平面のみ
    float currentDistance = toRetreat.Length();

    if (currentDistance > kDirectionEpsilon) {
        Vector3 primaryDirection = toRetreat.Normalize();

        // 目標位置 = 現在位置から targetDistance_ まで離れた位置
        float retreatDistance = targetDistance_ - currentDistance;
        if (retreatDistance > 0.0f) {
            // 壁回避: 最適な離脱方向を探索
            Vector3 bestDirection = FindBestRetreatDirection(primaryDirection, retreatDistance);

            // プレイヤーを向いたまま（bestDirection の逆方向を向く）
            float angle = atan2f(-bestDirection.x, -bestDirection.z);
            boss_->SetRotate(Vector3(0.0f, angle, 0.0f));

            targetPosition_ = startPosition_ + bestDirection * retreatDistance;
            targetPosition_ = ClampToArea(targetPosition_);

            // 実際の移動距離から所要時間を計算
            Vector3 actualMove = targetPosition_ - startPosition_;
            actualMove.y = 0.0f;
            float actualDistance = actualMove.Length();
            retreatDuration_ = actualDistance / retreatSpeed_;
        }
        else {
            // 既に目標距離以上離れている
            targetPosition_ = startPosition_;
            retreatDuration_ = 0.0f;
        }
    }
    else {
        // プレイヤーとほぼ同じ位置
        targetPosition_ = startPosition_;
        retreatDuration_ = 0.0f;
    }
}

void BTBossRetreat::UpdateRetreatMovement(float deltaTime) {
    deltaTime; // 未使用警告抑制

    if (retreatDuration_ > 0.0f) {
        // 離脱中の移動
        float t = elapsedTime_ / retreatDuration_;

        // clamp to [0, 1]
        t = std::clamp(t, 0.0f, 1.0f);

        // イージング（加速→減速）: smoothstep
        t = t * t * (kEasingCoeffA - kEasingCoeffB * t);

        Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
        boss_->SetTranslate(newPosition);
    }
}

Vector3 BTBossRetreat::ClampToArea(const Vector3& position) {
    Vector3 clampedPos = position;

    uint8_t phase = boss_->GetPhase();

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

Vector3 BTBossRetreat::FindBestRetreatDirection(const Vector3& primaryDirection, float retreatDistance) {
    // 元の方向での移動距離を評価
    float primaryScore = EvaluateDirection(primaryDirection, retreatDistance);

    // 閾値以上なら元の方向を使用
    if (primaryScore >= kMinRetreatDistance) {
        return primaryDirection;
    }

    // 代替方向を評価（Mat4x4::MakeRotateY + TransformNormal で回転）
    constexpr float kHalfPi = std::numbers::pi_v<float> / 2.0f;  // 90度
    constexpr float kPi = std::numbers::pi_v<float>;              // 180度

    Matrix4x4 rotLeft90 = Mat4x4::MakeRotateY(kHalfPi);
    Matrix4x4 rotRight90 = Mat4x4::MakeRotateY(-kHalfPi);
    Matrix4x4 rot180 = Mat4x4::MakeRotateY(kPi);

    struct DirectionCandidate {
        Vector3 direction;
        float score;
    };

    std::array<DirectionCandidate, 4> candidates = { {
        { primaryDirection, primaryScore },
        { Mat4x4::TransformNormal(rotLeft90, primaryDirection), 0.0f },   // 左90度
        { Mat4x4::TransformNormal(rotRight90, primaryDirection), 0.0f },  // 右90度
        { Mat4x4::TransformNormal(rot180, primaryDirection), 0.0f }       // 180度（逆方向）
    } };

    // 各方向のスコアを計算
    for (size_t i = 1; i < candidates.size(); ++i) {
        candidates[i].score = EvaluateDirection(candidates[i].direction, retreatDistance);
    }

    // 最高スコアの方向を選択
    auto best = std::ranges::max_element(candidates,
                                         [](const DirectionCandidate& a, const DirectionCandidate& b) {
                                             return a.score < b.score;
                                         });

    return best->direction;
}

float BTBossRetreat::EvaluateDirection(const Vector3& direction, float retreatDistance) {
    Vector3 targetPos = startPosition_ + direction * retreatDistance;
    targetPos = ClampToArea(targetPos);

    Vector3 actualMove = targetPos - startPosition_;
    actualMove.y = 0.0f;
    return actualMove.Length();
}

nlohmann::json BTBossRetreat::ExtractParameters() const {
    return {
        {"retreatSpeed", retreatSpeed_},
        {"targetDistance", targetDistance_}
    };
}

#ifdef _DEBUG
bool BTBossRetreat::DrawImGui() {
    bool changed = false;

    if (ImGui::DragFloat("Retreat Speed##retreat", &retreatSpeed_, 1.0f, 10.0f, 200.0f)) {
        changed = true;
    }
    if (ImGui::DragFloat("Target Distance##retreat", &targetDistance_, 0.5f, 1.0f, 100.0f)) {
        changed = true;
    }

    ImGui::Separator();
    ImGui::Text("Runtime Info:");
    ImGui::Text("Duration: %.2f sec", retreatDuration_);
    ImGui::Text("Elapsed: %.2f sec", elapsedTime_);

    return changed;
}
#endif
