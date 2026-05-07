#include "BTBossVortexTempest.h"
#include "../../Boss.h"
#include "../../../Player/Player.h"
#include "EmitterManager.h"
#include "ForceFieldManager.h"
#include "ParticleStruct.h"
#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

namespace {
    constexpr float kPi = 3.14159265358979323846f;

    /// <summary>v 番目の渦点の基準角度（0, π/2, π, 3π/2）</summary>
    inline float BaseAngleFor(int v) {
        return static_cast<float>(v) * (kPi * 0.5f);
    }

    /// <summary>preset スロット名のヘルパ（コード読みやすさ用）</summary>
    inline const std::string& PresetNameForSlot(
        int slot,
        const std::string& attractName,
        const std::string& dirName,
        const std::string& vortexName)
    {
        switch (slot) {
        case BTBossVortexTempest::kSlotAttract:     return attractName;
        case BTBossVortexTempest::kSlotDirectional: return dirName;
        case BTBossVortexTempest::kSlotVortex:      return vortexName;
        default:                                    return attractName;
        }
    }
}

BTBossVortexTempest::BTBossVortexTempest() {
    name_ = "BossVortexTempest";
}

BTNodeStatus BTBossVortexTempest::Execute(BTBlackboard* blackboard) {
    Boss* boss = blackboard->GetBoss();
    if (!boss) {
        status_ = BTNodeStatus::Failure;
        return status_;
    }

    ForceFieldManager* ffm = boss->GetForceFieldManager();
    if (!ffm) {
        // ForceFieldManager 未注入：DI 配線漏れ。Phase2 攻撃は実行不可。
        status_ = BTNodeStatus::Failure;
        return status_;
    }

    Player* player = blackboard->GetPlayer();
    const float deltaTime = blackboard->GetDeltaTime();

    //=================================================================
    // 初回実行時の初期化
    //   1. 4 渦点 × 3 プリセット = 12 ForceField を登録
    //   2. 各力場の position を渦点に上書き、strength=0 でスタート
    //   3. パーティクルエミッター（4 個 + マーカー）を起動
    //=================================================================
    if (isFirstExecute_) {
        elapsedTime_ = 0.0f;

        // Reset 用にマネージャをキャッシュ（中断時の後始末で blackboard 不要にする）
        cachedForceFieldManager_ = ffm;
        cachedEmitterManager_ = boss->GetEmitterManager();

        const Vector3 bossPos = boss->GetTransform().translate;

        // 4 渦点を 90 度間隔の円周上に配置
        for (int v = 0; v < kVortexCount; ++v) {
            const float baseAngle = BaseAngleFor(v);
            const Vector3 offset = { std::cos(baseAngle), 0.0f, std::sin(baseAngle) };
            const Vector3 vortexPos = bossPos + offset * orbitRadius_;

            // 各渦点で 3 プリセットを重ねて配置
            for (int s = 0; s < kFieldsPerVortex; ++s) {
                const std::string& presetName = PresetNameForSlot(
                    s, attractPresetName_, dirPresetName_, vortexPresetName_);

                if (!ffm->LoadPresetToData(presetName, loadedFields_[v][s].base)) {
                    // 読み込み失敗：既に追加済みの分を後始末してから Failure
                    Cleanup();
                    status_ = BTNodeStatus::Failure;
                    return status_;
                }

                // ForceField を登録：position を渦点に上書き、strength=0 でスタート
                ForceFieldData initial = loadedFields_[v][s].base;
                initial.position.x = vortexPos.x;
                initial.position.z = vortexPos.z;
                initial.position.y = bossPos.y + loadedFields_[v][s].base.position.y;  // preset Y を相対オフセット扱い
                initial.strength = 0.0f;
                loadedFields_[v][s].index = ffm->AddForceField(initial);
            }

            // 渦点ごとに 1 個ずつエミッター起動（boss_vortex_0..3）
            if (cachedEmitterManager_) {
                const std::string emitterName = MakeVortexEmitterName(v);
                cachedEmitterManager_->SetEmitterActive(emitterName, true);
                cachedEmitterManager_->SetEmitterPosition(emitterName, vortexPos);
            }

            // 地面マーカーデカール生成。
            // 直径 = Attract preset radius * 2 で「見えている円 = ダメージ範囲」を保証。
            // Decal::Initialize() が DecalManager::AddDecal() を内部で呼ぶ。
            const float attractRadius = loadedFields_[v][kSlotAttract].base.radius;
            const float decalDiameter = attractRadius * 2.0f;
            vortexDecals_[v] = std::make_unique<Decal>();
            vortexDecals_[v]->Initialize();
            vortexDecals_[v]->SetShape(DecalShape::Circle);
            // 地面マーカーとして機能させるため Y=0 固定（既存 AreaAttack/MeteorRain/SlashAttack に統一）。
            // ボスが空中にいても地面に貼り付き、プレイヤーが上から範囲を視認できる。
            vortexDecals_[v]->SetTranslate(Vector3(vortexPos.x, 0.0f, vortexPos.z));
            vortexDecals_[v]->SetScale(Vector3(decalDiameter, 1.0f, decalDiameter));
            vortexDecals_[v]->SetEdgeSoftness(0.02f);
            vortexDecals_[v]->SetColor(Vector4(1.0f, 0.2f, 0.1f, kBlinkAlphaMin));
            vortexDecals_[v]->SetVisible(true);
        }

        // Boss 硬直フラグを ON。cachedBoss_ + enteredRecovery_ を保存しておくことで、
        // Reset 経由（BTParallel 中断含む）でも Cleanup 内で確実に解除できる。
        boss->EnterRecovery();
        cachedBoss_ = boss;
        enteredRecovery_ = true;
        isFirstExecute_ = false;
    }

    elapsedTime_ += deltaTime;

    //=================================================================
    // フェーズ境界の計算
    //=================================================================
    const float warningEnd = warningTime_;
    const float expandEnd = warningEnd + expandTime_;
    const float sustainEnd = expandEnd + sustainTime_;
    const float decayEnd = sustainEnd + decayTime_;

    //=================================================================
    // 強度マルチプライヤ（フェーズ別に 0〜1 の係数を計算）
    //=================================================================
    float strengthMul = 0.0f;
    bool isFinished = false;

    if (elapsedTime_ < warningEnd) {
        // Phase 0: 予兆 — 強度ゼロ（巻き込み無し）
        strengthMul = 0.0f;
    }
    else if (elapsedTime_ < expandEnd) {
        // Phase 1: 展開 — ランプアップ
        const float t = (elapsedTime_ - warningEnd) / std::max<float>(expandTime_, 0.0001f);
        strengthMul = std::clamp(t, 0.0f, 1.0f);
    }
    else if (elapsedTime_ < sustainEnd) {
        // Phase 2: 持続 — フル稼働
        strengthMul = 1.0f;
    }
    else if (elapsedTime_ < decayEnd) {
        // Phase 3: 終息 — ランプダウン
        const float t = (elapsedTime_ - sustainEnd) / std::max<float>(decayTime_, 0.0001f);
        strengthMul = 1.0f - std::clamp(t, 0.0f, 1.0f);
    }
    else {
        // Phase 4: 終了
        isFinished = true;
    }

    if (isFinished) {
        // 攻撃成功終了：累積した未適用 DoT を最後に一括 flush してから後始末。
        // Reset（スタン中断）経由の Cleanup ではこの flush を意図的に行わず、
        // 攻撃終了後の不自然な遅延ダメージを防ぐ。
        if (player && pendingDamage_ > 0.0f) {
            player->OnHit(pendingDamage_);
            pendingDamage_ = 0.0f;
        }
        // Boss::ExitRecovery は Cleanup 内で実施するためここでの個別呼び出しは不要。
        Cleanup();
        status_ = BTNodeStatus::Success;
        return status_;
    }

    //=================================================================
    // 4 渦点の公転位置更新 + 12 ForceField 更新 + 4 エミッター追従 + 4 デカール追従
    //=================================================================
    const Vector3 bossPos = boss->GetTransform().translate;
    Vector3 vortexPositions[kVortexCount];

    // デカールアルファをフェーズに応じて算出（力場 strengthMul とは独立した
    // ビジュアル意図：Phase 1 から即座に「危険ゾーン」を表示してプレイヤーに伝える）
    float decalAlpha = 0.0f;
    if (elapsedTime_ < warningEnd) {
        // Phase 0 予兆：sin 点滅で「これから渦が来る、避けろ」サイン
        const float sinValue = std::abs(std::sin(elapsedTime_ * markerBlinkFrequency_ * kPi));
        decalAlpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * sinValue;
    }
    else if (elapsedTime_ < sustainEnd) {
        // Phase 1-2 展開・持続：固定アルファで危険ゾーンを持続表示
        decalAlpha = kDecalBaseAlpha;
    }
    else {
        // Phase 3 終息：線形フェードアウトで「もうすぐ消える」を伝える
        const float t = (elapsedTime_ - sustainEnd) / std::max<float>(decayTime_, 0.0001f);
        decalAlpha = kDecalBaseAlpha * (1.0f - std::clamp(t, 0.0f, 1.0f));
    }

    for (int v = 0; v < kVortexCount; ++v) {
        const float baseAngle = BaseAngleFor(v);
        const float currentAngle = baseAngle + angularSpeed_ * elapsedTime_;
        const Vector3 offset = { std::cos(currentAngle), 0.0f, std::sin(currentAngle) };
        const Vector3 vortexPos = bossPos + offset * orbitRadius_;
        vortexPositions[v] = vortexPos;

        // 各渦点で 3 力場を更新（位置は渦点に上書き、Y は preset 相対オフセット）
        for (int s = 0; s < kFieldsPerVortex; ++s) {
            if (loadedFields_[v][s].index < 0) continue;

            ForceFieldData updated = loadedFields_[v][s].base;
            updated.position.x = vortexPos.x;
            updated.position.z = vortexPos.z;
            updated.position.y = bossPos.y + loadedFields_[v][s].base.position.y;
            // 強度：preset 値 × フェーズ係数
            updated.strength = loadedFields_[v][s].base.strength * strengthMul;

            ffm->UpdateForceField(static_cast<uint32_t>(loadedFields_[v][s].index), updated);
        }

        // 渦エミッターを公転位置に追従
        if (cachedEmitterManager_) {
            cachedEmitterManager_->SetEmitterPosition(MakeVortexEmitterName(v), vortexPos);
        }

        // 地面マーカーデカールを公転位置に追従、アルファをフェーズ値で更新
        if (vortexDecals_[v]) {
            // 地面マーカーとして機能させるため Y=0 固定（既存 AreaAttack/MeteorRain/SlashAttack に統一）。
            // ボスが空中にいても地面に貼り付き、プレイヤーが上から範囲を視認できる。
            vortexDecals_[v]->SetTranslate(Vector3(vortexPos.x, 0.0f, vortexPos.z));
            vortexDecals_[v]->SetColor(Vector4(1.0f, 0.2f, 0.1f, decalAlpha));
        }
    }

    //=================================================================
    // DoT 判定（持続フェーズと終息フェーズで適用）
    // 巻き込み範囲は Attract preset の radius を採用（巻き込みと DoT 範囲が一致）。
    // 4 渦点それぞれで距離判定し、最も近い渦に対して累積。
    // 1 つの渦に巻き込まれている間は他の渦判定をスキップして二重カウントを防ぐ。
    // 累積バッファ + Tick 方式：HitFlash/Shake/CameraShake 等の副作用連打を回避。
    //=================================================================
    if (player && elapsedTime_ >= expandEnd) {
        const Vector3 playerPos = player->GetTransform().translate;
        // Attract preset の radius を判定半径に採用（全渦点で同じ値を使用）
        const float dotRadius = loadedFields_[0][kSlotAttract].base.radius;

        if (dotRadius > 0.0f) {
            for (int v = 0; v < kVortexCount; ++v) {
                // 各渦点の Attract 中心 = 渦の公転位置 + Attract preset Y オフセット
                const Vector3 attractCenter = {
                    vortexPositions[v].x,
                    bossPos.y + loadedFields_[v][kSlotAttract].base.position.y,
                    vortexPositions[v].z,
                };
                const float dist = (playerPos - attractCenter).Length();
                if (dist < dotRadius) {
                    // 中心に近いほど高ダメージ（半径での線形補間）
                    const float t = std::clamp(dist / dotRadius, 0.0f, 1.0f);
                    const float dotPerSec = maxDoT_ * (1.0f - t) + minDoT_ * t;
                    pendingDamage_ += dotPerSec * deltaTime;
                    break;  // 二重ダメージ防止：最も近い 1 渦のみ
                }
            }
        }

        // Tick タイマー進行 — 渦の中にいなくても進める
        damageTickTimer_ += deltaTime;
        if (damageTickTimer_ >= damageTickInterval_ && pendingDamage_ > 0.0f) {
            // 累積分を一括適用 — Player::OnHit の副作用が tick ごとに 1 回だけ走る
            player->OnHit(pendingDamage_);
            pendingDamage_ = 0.0f;
            damageTickTimer_ = 0.0f;
        }
    }

    status_ = BTNodeStatus::Running;
    return status_;
}

void BTBossVortexTempest::Reset() {
    BTNode::Reset();
    // スタン等による中断時：12 力場と全エミッターを後始末
    Cleanup();
}

void BTBossVortexTempest::Cleanup() {
    // Boss 硬直状態の解除（成功終了 / Reset 経由の両方で確実に実行）。
    // BTParallel が子ノードを中断する場合、Boss::ExitRecovery が呼ばれないと
    // 硬直フラグが残ってボスの行動全体が固まるリスクがあるため、ここに集約する。
    if (cachedBoss_ && enteredRecovery_) {
        cachedBoss_->ExitRecovery();
    }
    enteredRecovery_ = false;

    // ForceField 12 個の削除：erase ベースのため、必ず逆順インデックスから削除する。
    // 内側ループ（s=2..0）と外側ループ（v=3..0）の両方を逆順にすることで、
    // インデックス順 (Add 時の昇順) の逆で削除して全 12 個のシフトを無効化。
    if (cachedForceFieldManager_) {
        for (int v = kVortexCount - 1; v >= 0; --v) {
            for (int s = kFieldsPerVortex - 1; s >= 0; --s) {
                if (loadedFields_[v][s].index >= 0) {
                    cachedForceFieldManager_->RemoveForceField(static_cast<uint32_t>(loadedFields_[v][s].index));
                    loadedFields_[v][s].index = -1;
                }
            }
        }
    }
    else {
        // マネージャ消失時もインデックスはクリア
        for (int v = 0; v < kVortexCount; ++v) {
            for (int s = 0; s < kFieldsPerVortex; ++s) {
                loadedFields_[v][s].index = -1;
            }
        }
    }

    // エミッター停止（4 渦）
    if (cachedEmitterManager_) {
        for (int v = 0; v < kVortexCount; ++v) {
            cachedEmitterManager_->SetEmitterActive(MakeVortexEmitterName(v), false);
        }
    }

    // 地面マーカーデカール解放（unique_ptr::reset → DecalManager から自動 RemoveDecal）
    for (auto& decal : vortexDecals_) {
        decal.reset();
    }

    // 状態フラグ初期化
    elapsedTime_ = 0.0f;
    isFirstExecute_ = true;

    // DoT 累積バッファとタイマーもリセット
    // （適用前の残り累積を切り捨てる：攻撃中断時の不自然な事後ダメージを防ぐ）
    pendingDamage_ = 0.0f;
    damageTickTimer_ = 0.0f;
}

std::string BTBossVortexTempest::MakeVortexEmitterName(int i) const {
    return vortexEmitterBaseName_ + "_" + std::to_string(i);
}

void BTBossVortexTempest::ApplyParameters(const nlohmann::json& params) {
    if (params.contains("warningTime"))        warningTime_ = params["warningTime"];
    if (params.contains("expandTime"))         expandTime_ = params["expandTime"];
    if (params.contains("sustainTime"))        sustainTime_ = params["sustainTime"];
    if (params.contains("decayTime"))          decayTime_ = params["decayTime"];
    if (params.contains("orbitRadius"))        orbitRadius_ = params["orbitRadius"];
    if (params.contains("angularSpeed"))       angularSpeed_ = params["angularSpeed"];
    if (params.contains("minDoT"))             minDoT_ = params["minDoT"];
    if (params.contains("maxDoT"))             maxDoT_ = params["maxDoT"];
    if (params.contains("damageTickInterval")) damageTickInterval_ = params["damageTickInterval"];

    if (params.contains("attractPresetName") && params["attractPresetName"].is_string()) {
        attractPresetName_ = params["attractPresetName"].get<std::string>();
    }
    if (params.contains("dirPresetName") && params["dirPresetName"].is_string()) {
        dirPresetName_ = params["dirPresetName"].get<std::string>();
    }
    if (params.contains("vortexPresetName") && params["vortexPresetName"].is_string()) {
        vortexPresetName_ = params["vortexPresetName"].get<std::string>();
    }
    if (params.contains("vortexEmitterBaseName") && params["vortexEmitterBaseName"].is_string()) {
        vortexEmitterBaseName_ = params["vortexEmitterBaseName"].get<std::string>();
    }
    if (params.contains("markerBlinkFrequency")) markerBlinkFrequency_ = params["markerBlinkFrequency"];
}

nlohmann::json BTBossVortexTempest::ExtractParameters() const {
    return {
        {"warningTime",            warningTime_},
        {"expandTime",             expandTime_},
        {"sustainTime",            sustainTime_},
        {"decayTime",              decayTime_},
        {"orbitRadius",            orbitRadius_},
        {"angularSpeed",           angularSpeed_},
        {"minDoT",                 minDoT_},
        {"maxDoT",                 maxDoT_},
        {"damageTickInterval",     damageTickInterval_},
        {"attractPresetName",      attractPresetName_},
        {"dirPresetName",          dirPresetName_},
        {"vortexPresetName",       vortexPresetName_},
        {"vortexEmitterBaseName",  vortexEmitterBaseName_},
        {"markerBlinkFrequency",   markerBlinkFrequency_},
    };
}

#ifdef _DEBUG
bool BTBossVortexTempest::DrawImGui() {
    bool changed = false;
    if (ImGui::DragFloat("Warning Time##vortex",      &warningTime_,        0.05f, 0.0f, 5.0f))   changed = true;
    if (ImGui::DragFloat("Expand Time##vortex",       &expandTime_,         0.05f, 0.0f, 3.0f))   changed = true;
    if (ImGui::DragFloat("Sustain Time##vortex",      &sustainTime_,        0.1f,  0.0f, 15.0f))  changed = true;
    if (ImGui::DragFloat("Decay Time##vortex",        &decayTime_,          0.05f, 0.0f, 3.0f))   changed = true;
    if (ImGui::DragFloat("Orbit Radius##vortex",      &orbitRadius_,        0.5f,  0.0f, 30.0f))  changed = true;
    if (ImGui::DragFloat("Angular Speed##vortex",     &angularSpeed_,       0.05f, -5.0f, 5.0f))  changed = true;
    if (ImGui::DragFloat("Min DoT##vortex",           &minDoT_,             0.5f,  0.0f, 100.0f)) changed = true;
    if (ImGui::DragFloat("Max DoT##vortex",           &maxDoT_,             0.5f,  0.0f, 100.0f)) changed = true;
    if (ImGui::DragFloat("DoT Tick Interval##vortex", &damageTickInterval_, 0.05f, 0.05f, 5.0f))  changed = true;
    if (ImGui::DragFloat("Marker Blink Hz##vortex",   &markerBlinkFrequency_, 0.1f, 0.5f, 20.0f)) changed = true;

    ImGui::Separator();
    ImGui::TextDisabled("ForceField Presets (read-only here; edit JSON)");
    ImGui::TextDisabled("  Attract: %s", attractPresetName_.c_str());
    ImGui::TextDisabled("  Dir    : %s", dirPresetName_.c_str());
    ImGui::TextDisabled("  Vortex : %s", vortexPresetName_.c_str());
    ImGui::TextDisabled("Total ForceField count: %d (= %d vortex x %d slots)",
        kVortexCount * kFieldsPerVortex, kVortexCount, kFieldsPerVortex);
    return changed;
}
#endif
