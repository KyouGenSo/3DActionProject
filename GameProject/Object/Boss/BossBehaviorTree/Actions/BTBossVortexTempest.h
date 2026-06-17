#pragma once
#include "AttackNode.h"
#include "Decal.h"
#include "ParticleStruct.h"
#include "Vector3.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace Tako {
    class ForceFieldManager;
}

class Boss;

/// <summary>
/// ボスを公転する 4 渦点それぞれに 3 種の ForceField プリセット（Attract/Directional/Vortex）を
/// 重ね、合計 12 力場で合成トルネードを作る。持続中は内側ほど強い DoT を与える。
/// </summary>
class BTBossVortexTempest : public AttackNode {
public:
    static constexpr int kVortexCount = 4;

    static constexpr int kFieldsPerVortex = 3;  ///< Attract + Dir + Vortex

    enum FieldSlot {
        kSlotAttract = 0,
        kSlotDirectional = 1,
        kSlotVortex = 2,
    };

    BTBossVortexTempest();
    virtual ~BTBossVortexTempest() = default;

    [[nodiscard]] float GetTotalDuration() const {
        return warningTime_ + expandTime_ + sustainTime_ + decayTime_;
    }

protected:
    /// <summary>
    /// 予兆→展開→持続→終息で 12 力場の強度をランプ制御し、内側ほど強い DoT を与える。
    /// </summary>
    /// <returns>ForceFieldManager 不在で Failure、終息後に FinishAttack の結果、それ以外は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnCleanup() override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>
    /// 渦点番号からエミッター名を生成する。
    /// </summary>
    /// <param name="i">渦点インデックス（0〜kVortexCount-1）</param>
    /// <returns>"vortexEmitterBaseName_\_i" 形式の名前</returns>
    [[nodiscard]] std::string MakeVortexEmitterName(int i) const;

    //======================== パラメータ ========================
    float warningTime_ = 1.5f;
    float expandTime_ = 0.5f;            ///< 強度ランプアップ
    float sustainTime_ = 4.5f;
    float decayTime_ = 0.5f;             ///< ランプダウン + 硬直

    float orbitRadius_ = 8.0f;           ///< 公転半径
    float angularSpeed_ = 0.6f;          ///< rad/秒

    float minDoT_ = 5.0f;                ///< 外周時の DoT（damage/秒）
    float maxDoT_ = 15.0f;               ///< 中心時の DoT（damage/秒）

    /// <summary>
    /// DoT 適用間隔（秒）。毎フレーム OnHit を呼ぶと HitFlash/Shake が連打されるため、
    /// 累積バッファ + Tick 方式で間引く。
    /// </summary>
    float damageTickInterval_ = 0.5f;

    //======================== ForceField プリセット名 ========================
    std::string attractPresetName_ = "boss_vortexattacck_attract";
    std::string dirPresetName_ = "boss_vortexattacck_dir";
    std::string vortexPresetName_ = "boss_vortexattack_vortex";

    //======================== エミッター名 ========================
    std::string vortexEmitterBaseName_ = "boss_vortex";

    //======================== デカール演出 ========================
    static constexpr float kDecalBaseAlpha = 0.5f;
    static constexpr float kBlinkAlphaMin = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

    /// <summary>
    /// 予兆点滅周波数（Hz）
    /// </summary>
    float markerBlinkFrequency_ = 4.0f;

    //======================== ランタイム状態 ========================
    struct LoadedField {
        Tako::ForceFieldData base{};
        int32_t index = -1;
    };
    LoadedField loadedFields_[kVortexCount][kFieldsPerVortex];

    float pendingDamage_ = 0.0f;        ///< DoT 累積バッファ
    float damageTickTimer_ = 0.0f;

    std::array<std::unique_ptr<Tako::Decal>, kVortexCount> vortexDecals_{};  ///< 渦点ごとの地面マーカー

    //======================== Reset 用キャッシュ ========================
    Tako::ForceFieldManager* cachedForceFieldManager_ = nullptr;
};
