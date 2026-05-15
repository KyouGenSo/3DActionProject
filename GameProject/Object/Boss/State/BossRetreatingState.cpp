#include "BossRetreatingState.h"
#include "../Boss.h"
#include "BossStateMachine.h"
#include "../../Player/Player.h"
#include "../../../Common/GameConst.h"
#include "BehaviorTree.h"
#include "BTBlackboard.h"
#include "Mat4x4Func.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

using namespace Tako;

BossRetreatingState::BossRetreatingState()
	: BossState("Retreating")
{
}

void BossRetreatingState::Enter(Boss* boss)
{
	elapsedTime_ = 0.0f;

	startPosition_ = boss->GetTransform().translate;

	Player* player = boss->GetBehaviorTree()->GetBlackboard()->GetPtr<Player>("player");
	if (!player) {
		// プレイヤーがいない場合はすぐ Normal に戻る
		retreatDuration_ = 0.0f;
		targetPosition_ = startPosition_;
		return;
	}

	Vector3 playerPos = player->GetTransform().translate;

	// プレイヤーからボスへの方向ベクトル（離れる方向）
	Vector3 toRetreat = startPosition_ - playerPos;
	toRetreat.y = 0.0f;
	float currentDistance = toRetreat.Length();

	if (currentDistance > kDirectionEpsilon) {
		Vector3 primaryDirection = toRetreat.Normalize();

		float retreatDistance = targetDistance_ - currentDistance;
		if (retreatDistance > 0.0f) {
			// 壁回避: 最適な離脱方向を探索
			Vector3 bestDirection = FindBestRetreatDirection(primaryDirection, retreatDistance, boss);

			// プレイヤーを向いたまま
			float angle = atan2f(-bestDirection.x, -bestDirection.z);
			boss->SetRotate(Vector3(0.0f, angle, 0.0f));

			targetPosition_ = startPosition_ + bestDirection * retreatDistance;
			targetPosition_ = ClampToArea(targetPosition_, boss);

			// 実際の移動距離から所要時間を計算
			Vector3 actualMove = targetPosition_ - startPosition_;
			actualMove.y = 0.0f;
			float actualDistance = actualMove.Length();
			retreatDuration_ = actualDistance / retreatSpeed_;
		}
		else {
			targetPosition_ = startPosition_;
			retreatDuration_ = 0.0f;
		}
	}
	else {
		targetPosition_ = startPosition_;
		retreatDuration_ = 0.0f;
	}
}

void BossRetreatingState::Update(Boss* boss, float deltaTime)
{
	// 即座に完了するケース
	if (retreatDuration_ <= 0.0f) {
		boss->GetStateMachine()->ChangeState("Normal");
		return;
	}

	elapsedTime_ += deltaTime;

	// 離脱移動の更新
	UpdateRetreatMovement(boss);

	// 終了判定（位置ベース）
	Vector3 currentPos = boss->GetTransform().translate;
	Vector3 diff = currentPos - targetPosition_;
	diff.y = 0.0f;
	float distanceToTarget = diff.Length();

	if (distanceToTarget < kArrivalThreshold) {
		boss->SetTranslate(targetPosition_);
		boss->GetStateMachine()->ChangeState("Normal");
	}
}

void BossRetreatingState::Exit(Boss* boss)
{
	(void)boss;
	elapsedTime_ = 0.0f;
	retreatDuration_ = 0.0f;
}

void BossRetreatingState::UpdateRetreatMovement(Boss* boss)
{
	if (retreatDuration_ > 0.0f) {
		float t = elapsedTime_ / retreatDuration_;
		t = std::clamp(t, 0.0f, 1.0f);

		// イージング（加速→減速）: smoothstep
		t = t * t * (kEasingCoeffA - kEasingCoeffB * t);

		Vector3 newPosition = Vector3::Lerp(startPosition_, targetPosition_, t);
		boss->SetTranslate(newPosition);
	}
}

Vector3 BossRetreatingState::ClampToArea(const Vector3& position, Boss* boss)
{
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

Vector3 BossRetreatingState::FindBestRetreatDirection(const Vector3& primaryDirection, float retreatDistance, Boss* boss)
{
	float primaryScore = EvaluateDirection(primaryDirection, retreatDistance, boss);

	if (primaryScore >= kMinRetreatDistance) {
		return primaryDirection;
	}

	// 代替方向を評価
	constexpr float kHalfPi = std::numbers::pi_v<float> / 2.0f;
	constexpr float kPi = std::numbers::pi_v<float>;

	Matrix4x4 rotLeft90 = Mat4x4::MakeRotateY(kHalfPi);
	Matrix4x4 rotRight90 = Mat4x4::MakeRotateY(-kHalfPi);
	Matrix4x4 rot180 = Mat4x4::MakeRotateY(kPi);

	struct DirectionCandidate {
		Vector3 direction;
		float score;
	};

	std::array<DirectionCandidate, 4> candidates = { {
		{ primaryDirection, primaryScore },
		{ Mat4x4::TransformNormal(rotLeft90, primaryDirection), 0.0f },
		{ Mat4x4::TransformNormal(rotRight90, primaryDirection), 0.0f },
		{ Mat4x4::TransformNormal(rot180, primaryDirection), 0.0f }
	} };

	for (size_t i = 1; i < candidates.size(); ++i) {
		candidates[i].score = EvaluateDirection(candidates[i].direction, retreatDistance, boss);
	}

	auto best = std::max_element(candidates.begin(), candidates.end(),
		[](const DirectionCandidate& a, const DirectionCandidate& b) {
			return a.score < b.score;
		});

	return best->direction;
}

float BossRetreatingState::EvaluateDirection(const Vector3& direction, float retreatDistance, Boss* boss)
{
	Vector3 targetPos = startPosition_ + direction * retreatDistance;
	targetPos = ClampToArea(targetPos, boss);

	Vector3 actualMove = targetPos - startPosition_;
	actualMove.y = 0.0f;
	return actualMove.Length();
}
