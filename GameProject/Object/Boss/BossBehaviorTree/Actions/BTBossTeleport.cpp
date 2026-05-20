#include "BTBossTeleport.h"
#include "../../Boss.h"
#include "../../Movement/BossAreaBounds.h"
#include "../../../Player/Player.h"
#include "Object3d.h"
#include "RandomEngine.h"
#include <algorithm>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

BTBossTeleport::BTBossTeleport() {
    name_ = "BossTeleport";
}

void BTBossTeleport::OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) {
    teleportFired_ = false;

    // テレポート中フラグを立てる: Boss::Update 内で aura エミッタが自動無効化される
    boss->SetTeleporting(true);

    // テレポート目標座標を決定 (3 モード)
    auto* rng = Tako::RandomEngine::GetInstance();
    const auto stageBounds = BossMovement::CalcStageBounds();

    switch (mode_) {
    case TeleportMode::Fixed: {
        targetTeleportPosition_ = Vector3(
            targetPositionX_, targetPositionY_, targetPositionZ_);
        break;
    }
    case TeleportMode::RandomFromBoss: {
        Vector3 dir = rng->GetRandomDirectionXZ();
        float dist = rng->GetFloat(randomMinDistance_, randomMaxDistance_);
        Vector3 candidate = boss->GetTranslate() + dir * dist;
        targetTeleportPosition_ = BossMovement::ClampToBounds(candidate, stageBounds);
        break;
    }
    case TeleportMode::RandomFromPlayer: {
        // Blackboard からプレイヤーを取得。null ならボス中心にフォールバック。
        Player* player = blackboard ? blackboard->GetPtr<Player>("player") : nullptr;
        const Vector3 center = player ? player->GetTranslate() : boss->GetTranslate();
        Vector3 dir = rng->GetRandomDirectionXZ();
        float dist = rng->GetFloat(playerRandomMinDistance_, playerRandomMaxDistance_);
        Vector3 candidate = center + dir * dist;
        targetTeleportPosition_ = BossMovement::ClampToBounds(candidate, stageBounds);
        break;
    }
    }

    // 半透明描画モードへ切替 + 元のマテリアル状態をキャッシュ
    Object3d* model = boss->GetModel();
    if (model) {
        originalMaterialColor_ = model->GetMaterialColor();
        originalTransparentState_ = model->IsTransparent();
        model->SetTransparent(true);
        // フェードアウト開始時は完全不透明 (alpha = 1)
        UpdateModelAlpha(model, 1.0f);
    }
}

Tako::BTNodeStatus BTBossTeleport::OnExecute(Tako::BTBlackboard* /*blackboard*/, Boss* boss, float deltaTime) {
    elapsedTime_ += deltaTime;

    Object3d* model = boss->GetModel();

    const float fadeOutEnd = fadeOutDuration_;
    const float fadeInEnd  = fadeOutDuration_ + fadeInDuration_;

    if (elapsedTime_ < fadeOutEnd) {
        // Phase 0: フェードアウト (alpha 1 → 0)
        const float t = (fadeOutDuration_ > 0.0001f)
            ? std::clamp(elapsedTime_ / fadeOutDuration_, 0.0f, 1.0f)
            : 1.0f;
        const float alpha = 1.0f - t;
        if (model) UpdateModelAlpha(model, alpha);
        boss->SetBodyParticleEmitterActive(true);
    }
    else if (!teleportFired_) {
        // Phase 1: 瞬間移動 (alpha = 0、body emitter OFF、座標瞬時更新)
        if (model) UpdateModelAlpha(model, 0.0f);
        boss->SetBodyParticleEmitterActive(false);
        boss->SetTranslate(targetTeleportPosition_);
        teleportFired_ = true;
    }
    else if (elapsedTime_ < fadeInEnd) {
        // Phase 2: フェードイン (alpha 0 → 1)
        const float t = (fadeInDuration_ > 0.0001f)
            ? std::clamp((elapsedTime_ - fadeOutDuration_) / fadeInDuration_, 0.0f, 1.0f)
            : 1.0f;
        const float alpha = t;
        if (model) UpdateModelAlpha(model, alpha);
        boss->SetBodyParticleEmitterActive(true);
    }
    else {
        // Phase 3: 完了 (body emitter OFF、Success)
        boss->SetBodyParticleEmitterActive(false);
        return FinishAttack();  // OnCleanup → status = Success
    }

    return Tako::BTNodeStatus::Running;
}

void BTBossTeleport::OnCleanup() {
    // マテリアル状態を復帰 (中断時の safety 含む)
    if (cachedBoss_) {
        Object3d* model = cachedBoss_->GetModel();
        if (model) {
            model->SetMaterialColor(originalMaterialColor_);
            model->SetTransparent(originalTransparentState_);
        }
        cachedBoss_->SetBodyParticleEmitterActive(false);
        // テレポート中フラグ解除 → 次フレームの Boss::Update で aura が再有効化される
        cachedBoss_->SetTeleporting(false);
    }
    teleportFired_ = false;
}

void BTBossTeleport::UpdateModelAlpha(Tako::Object3d* model, float alpha) const {
    Vector4 c = originalMaterialColor_;
    c.w = alpha;
    model->SetMaterialColor(c);
}

void BTBossTeleport::OnApplyParameters(const nlohmann::json& params) {
    if (params.contains("fadeOutDuration"))   fadeOutDuration_   = params["fadeOutDuration"];
    if (params.contains("fadeInDuration"))    fadeInDuration_    = params["fadeInDuration"];
    if (params.contains("mode") && params["mode"].is_number_integer()) {
        mode_ = static_cast<TeleportMode>(params["mode"].get<int>());
    }
    else if (params.contains("useRandomPosition")) {
        // 後方互換: 旧 bool キーから新 enum へ変換 (true → RandomFromBoss, false → Fixed)
        mode_ = params["useRandomPosition"].get<bool>()
            ? TeleportMode::RandomFromBoss : TeleportMode::Fixed;
    }
    if (params.contains("targetPositionX"))         targetPositionX_         = params["targetPositionX"];
    if (params.contains("targetPositionY"))         targetPositionY_         = params["targetPositionY"];
    if (params.contains("targetPositionZ"))         targetPositionZ_         = params["targetPositionZ"];
    if (params.contains("randomMinDistance"))       randomMinDistance_       = params["randomMinDistance"];
    if (params.contains("randomMaxDistance"))       randomMaxDistance_       = params["randomMaxDistance"];
    if (params.contains("playerRandomMinDistance")) playerRandomMinDistance_ = params["playerRandomMinDistance"];
    if (params.contains("playerRandomMaxDistance")) playerRandomMaxDistance_ = params["playerRandomMaxDistance"];
}

void BTBossTeleport::OnExtractParameters(nlohmann::json& out) const {
    out["fadeOutDuration"]         = fadeOutDuration_;
    out["fadeInDuration"]          = fadeInDuration_;
    out["mode"]                    = static_cast<int>(mode_);
    out["targetPositionX"]         = targetPositionX_;
    out["targetPositionY"]         = targetPositionY_;
    out["targetPositionZ"]         = targetPositionZ_;
    out["randomMinDistance"]       = randomMinDistance_;
    out["randomMaxDistance"]       = randomMaxDistance_;
    out["playerRandomMinDistance"] = playerRandomMinDistance_;
    out["playerRandomMaxDistance"] = playerRandomMaxDistance_;
}

#ifdef _DEBUG
bool BTBossTeleport::OnDrawImGui() {
    bool changed = false;
    if (ImGui::DragFloat("Fade Out Duration##teleport", &fadeOutDuration_, 0.05f, 0.0f, 5.0f)) changed = true;
    if (ImGui::DragFloat("Fade In Duration##teleport",  &fadeInDuration_,  0.05f, 0.0f, 5.0f)) changed = true;

    // モード選択 (Combo)
    static const char* kModeLabels[] = {
        "Fixed (固定座標)",
        "Random From Boss (ボス中心)",
        "Random From Player (プレイヤー中心)",
    };
    int modeInt = static_cast<int>(mode_);
    if (ImGui::Combo("Mode##teleport", &modeInt, kModeLabels, IM_ARRAYSIZE(kModeLabels))) {
        mode_ = static_cast<TeleportMode>(modeInt);
        changed = true;
    }

    // モード別 UI
    switch (mode_) {
    case TeleportMode::Fixed: {
        float pos[3] = { targetPositionX_, targetPositionY_, targetPositionZ_ };
        if (ImGui::DragFloat3("Target Position##teleport", pos, 0.5f)) {
            targetPositionX_ = pos[0];
            targetPositionY_ = pos[1];
            targetPositionZ_ = pos[2];
            changed = true;
        }
        break;
    }
    case TeleportMode::RandomFromBoss: {
        if (ImGui::DragFloat("Boss Random Min Distance##teleport", &randomMinDistance_, 0.5f, 0.0f, 200.0f)) changed = true;
        if (ImGui::DragFloat("Boss Random Max Distance##teleport", &randomMaxDistance_, 0.5f, 0.0f, 200.0f)) changed = true;
        if (randomMaxDistance_ < randomMinDistance_) {
            randomMaxDistance_ = randomMinDistance_;
        }
        break;
    }
    case TeleportMode::RandomFromPlayer: {
        if (ImGui::DragFloat("Player Random Min Distance##teleport", &playerRandomMinDistance_, 0.5f, 0.0f, 200.0f)) changed = true;
        if (ImGui::DragFloat("Player Random Max Distance##teleport", &playerRandomMaxDistance_, 0.5f, 0.0f, 200.0f)) changed = true;
        if (playerRandomMaxDistance_ < playerRandomMinDistance_) {
            playerRandomMaxDistance_ = playerRandomMinDistance_;
        }
        break;
    }
    }

    return changed;
}
#endif
