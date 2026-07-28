#include "BTBossRingShockwave.h"
#include "../../Boss.h"

#include "Object3d.h"

#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossRingShockwave::BTBossRingShockwave() {
    name_ = "BossRingShockwave";
}

BTBossRingShockwave::~BTBossRingShockwave() {
    OnCleanup();
}

Tako::BTNodeStatus BTBossRingShockwave::OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) {
    const float warningEnd = warningTime_;
    const float travelEnd = warningEnd + travelDuration_;
    const float fadeEnd = travelEnd + fadeTime_;

    if (elapsedTime_ < warningEnd) {
        // Warning Phase: プレイヤーへ追従しながら点滅
        const Vector3 dir = FacePlayerInstant(blackboard);
        if (dir.Length() > 0.0f) {
            lockedDir_ = dir;
            lockedYaw_ = atan2f(dir.x, dir.z);
        }
        originPos_ = boss->GetTranslate() + lockedDir_ * spawnOffsetForward_;

        const float sinValue = std::abs(std::sin(elapsedTime_ * blinkFrequency_ * std::numbers::pi_v<float>));
        UpdateRingVisual(boss, kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue);
    }
    else if (elapsedTime_ < travelEnd) {
        if (!hasLaunched_) {
            ringColliders_.Activate(ComputeCurrentScale() * (1.0f - Boss::kRingShockwaveWidthFraction * 0.5f));
            hasLaunched_ = true;
        }

        traveled_ += moveSpeed_ * deltaTime;
        UpdateRingVisual(boss, kRingBaseAlpha);

        const float scale = ComputeCurrentScale();
        const float widthFraction = Boss::kRingShockwaveWidthFraction;
        Vector3 center = originPos_ + lockedDir_ * traveled_;
        center.y = colliderY_;
        ringColliders_.UpdateArc(center, lockedYaw_, kSweepRad,
                                 scale * (1.0f - widthFraction * 0.5f), scale * widthFraction * colliderScale_,
                                 moveSpeed_ * deltaTime);
    }
    else {
        if (!hasEnded_) {
            ringColliders_.Deactivate();
            hasEnded_ = true;
        }

        if (elapsedTime_ < fadeEnd) {
            const float t = (fadeTime_ > 0.0f) ? (elapsedTime_ - travelEnd) / fadeTime_ : 1.0f;
            UpdateRingVisual(boss, kRingBaseAlpha * (1.0f - t));
        }
        else {
            boss->SetRingShockwaveVisible(false);
        }
        EnterAttackRecovery(boss);
    }

    elapsedTime_ += deltaTime;

    if (elapsedTime_ >= totalDuration_) {
        return FinishAttack();
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossRingShockwave::OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) {
    (void)blackboard;

    hasLaunched_ = false;
    hasEnded_ = false;
    traveled_ = 0.0f;

    travelDuration_ = (moveSpeed_ > 0.0f) ? maxDistance_ / moveSpeed_ : 0.0f;
    totalDuration_ = warningTime_ + travelDuration_ + fadeTime_ + recoveryTime_;

    ringColliders_.Initialize(boss, segmentCount_);
    ringColliders_.SetDamage(damage_);

    lockedYaw_ = boss->GetRotate().y;
    lockedDir_ = Vector3(sinf(lockedYaw_), 0.0f, cosf(lockedYaw_));
    originPos_ = boss->GetTranslate() + lockedDir_ * spawnOffsetForward_;

    boss->SetRingShockwaveVisible(true);
    UpdateRingVisual(boss, kRingBaseAlpha);
}

float BTBossRingShockwave::ComputeCurrentScale() const {
    if (maxDistance_ <= 0.0f) return maxScale_;
    const float t = std::clamp(traveled_ / maxDistance_, 0.0f, 1.0f);
    return initialScale_ + (maxScale_ - initialScale_) * t;
}

void BTBossRingShockwave::UpdateRingVisual(Boss* boss, float alpha) {
    Object3d* ring = boss ? boss->GetRingShockwaveModel() : nullptr;
    if (!ring) return;

    const float scale = ComputeCurrentScale();
    const Vector3 pos = originPos_ + lockedDir_ * traveled_;
    ring->SetTranslate(Vector3(pos.x, spawnHeight_, pos.z));
    ring->SetRotate(Vector3(0.0f, lockedYaw_, 0.0f));
    ring->SetScale(Vector3(scale, 1.0f, scale));
    ring->SetMaterialColor(Vector4(1.0f, 1.0f, 1.0f, alpha));
    // WRAP サンプラーなので剰余を取らず加算のみでシームレスにループする
    ring->SetUvTransform(Transform{ Vector3(1.0f, 1.0f, 1.0f),
                                    Vector3(0.0f, 0.0f, 0.0f),
                                    Vector3(elapsedTime_ * uvScrollU_, elapsedTime_ * uvScrollV_, 0.0f) });
    ring->Update();
}

void BTBossRingShockwave::OnCleanup() {
    ringColliders_.Finalize();

    if (cachedBoss_) {
        cachedBoss_->SetRingShockwaveVisible(false);
    }

    hasLaunched_ = false;
    hasEnded_ = false;
    traveled_ = 0.0f;
}

void BTBossRingShockwave::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("warningTime"))        warningTime_ = params["warningTime"];
    if (params.contains("moveSpeed"))          moveSpeed_ = params["moveSpeed"];
    if (params.contains("initialScale"))       initialScale_ = params["initialScale"];
    if (params.contains("maxScale"))           maxScale_ = params["maxScale"];
    if (params.contains("maxDistance"))        maxDistance_ = params["maxDistance"];
    if (params.contains("damage"))             damage_ = params["damage"];
    if (params.contains("fadeTime"))           fadeTime_ = params["fadeTime"];
    if (params.contains("recoveryTime"))       recoveryTime_ = params["recoveryTime"];
    if (params.contains("colliderY"))          colliderY_ = params["colliderY"];
    if (params.contains("colliderScale"))      colliderScale_ = params["colliderScale"];
    if (params.contains("spawnHeight"))        spawnHeight_ = params["spawnHeight"];
    if (params.contains("spawnOffsetForward")) spawnOffsetForward_ = params["spawnOffsetForward"];
    if (params.contains("segmentCount"))       segmentCount_ = params["segmentCount"];
    if (params.contains("blinkFrequency"))     blinkFrequency_ = params["blinkFrequency"];
    if (params.contains("uvScrollU"))          uvScrollU_ = params["uvScrollU"];
    if (params.contains("uvScrollV"))          uvScrollV_ = params["uvScrollV"];
}

void BTBossRingShockwave::OnExtractParameters(nlohmann::json& out) const {
    out["warningTime"]        = warningTime_;
    out["moveSpeed"]          = moveSpeed_;
    out["initialScale"]       = initialScale_;
    out["maxScale"]           = maxScale_;
    out["maxDistance"]        = maxDistance_;
    out["damage"]             = damage_;
    out["fadeTime"]           = fadeTime_;
    out["recoveryTime"]       = recoveryTime_;
    out["colliderY"]          = colliderY_;
    out["colliderScale"]      = colliderScale_;
    out["spawnHeight"]        = spawnHeight_;
    out["spawnOffsetForward"] = spawnOffsetForward_;
    out["segmentCount"]       = segmentCount_;
    out["blinkFrequency"]     = blinkFrequency_;
    out["uvScrollU"]          = uvScrollU_;
    out["uvScrollV"]          = uvScrollV_;
}

#ifdef _DEBUG
bool BTBossRingShockwave::OnDrawImGui() {
    bool changed = false;

    ImGui::SeparatorText("Phase Timing");
    if (ImGui::DragFloat("Warning Time##ringshock", &warningTime_, 0.05f, 0.1f, 5.0f))          changed = true;
    if (ImGui::DragFloat("Fade Time##ringshock", &fadeTime_, 0.05f, 0.0f, 2.0f))                changed = true;
    if (ImGui::DragFloat("Recovery Time##ringshock", &recoveryTime_, 0.05f, 0.0f, 3.0f))        changed = true;

    ImGui::SeparatorText("Shockwave Motion");
    if (ImGui::DragFloat("Move Speed##ringshock", &moveSpeed_, 0.5f, 1.0f, 60.0f))              changed = true;
    if (ImGui::DragFloat("Max Distance##ringshock", &maxDistance_, 0.5f, 5.0f, 100.0f))         changed = true;
    if (ImGui::DragFloat("Spawn Offset Forward##ringshock", &spawnOffsetForward_, 0.1f, 0.0f, 10.0f)) changed = true;

    ImGui::SeparatorText("Shockwave Scale");
    if (ImGui::DragFloat("Initial Scale##ringshock", &initialScale_, 0.1f, 0.5f, 20.0f))        changed = true;
    if (ImGui::DragFloat("Max Scale##ringshock", &maxScale_, 0.1f, 1.0f, 50.0f))                changed = true;

    ImGui::SeparatorText("UV Scroll");
    if (ImGui::DragFloat("UV Scroll U##ringshock", &uvScrollU_, 0.05f, -5.0f, 5.0f))            changed = true;
    if (ImGui::DragFloat("UV Scroll V##ringshock", &uvScrollV_, 0.05f, -5.0f, 5.0f))            changed = true;

    ImGui::SeparatorText("Collider");
    if (ImGui::DragInt("Segment Count##ringshock", &segmentCount_, 1,
                       RingColliderGroup::kMinSegments, RingColliderGroup::kMaxSegments))       changed = true;
    if (ImGui::DragFloat("Collider Y##ringshock", &colliderY_, 0.1f, 0.0f, 5.0f))               changed = true;
    if (ImGui::DragFloat("Collider Scale##ringshock", &colliderScale_, 0.05f, 0.1f, 3.0f))      changed = true;
    if (ImGui::DragFloat("Spawn Height##ringshock", &spawnHeight_, 0.05f, 0.0f, 3.0f))          changed = true;

    ImGui::SeparatorText("Attack Parameters");
    if (ImGui::DragFloat("Damage##ringshock", &damage_, 0.5f, 1.0f, 50.0f))                     changed = true;
    if (ImGui::DragFloat("Blink Frequency##ringshock", &blinkFrequency_, 0.5f, 1.0f, 30.0f))    changed = true;

    return changed;
}
#endif
