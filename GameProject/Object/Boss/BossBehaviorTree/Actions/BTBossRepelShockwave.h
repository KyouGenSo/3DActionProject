#pragma once
#include "AttackNode.h"
#include "../../../../Common/ForceFieldAffectMask.h"
#include "ParticleStruct.h"
#include "Vector3.h"
#include <cstdint>
#include <string>

namespace Tako {
    class ForceFieldManager;
    class EmitterManager;
}

class Boss;

/// <summary>
/// ボス中心の放射状 Repel 力場。プレイヤー弾は速度逆転で吹き飛び、速度低下で消滅する（PlayerBullet 側で対応）。
/// </summary>
class BTBossRepelShockwave : public AttackNode {
public: //メンバー関数
    BTBossRepelShockwave();
    virtual ~BTBossRepelShockwave() = default;

    //==========================================
    //Getter
    //==========================================
    /// <summary>
    /// 全フェーズ合計時間（予兆 + 展開 + 持続）
    /// </summary>
    [[nodiscard]] float GetTotalDuration() const { return warningTime_ + expandTime_ + sustainTime_; }

protected: //非公開関数
    /// <summary>
    /// 予兆→展開→持続→終了のフェーズで力場・バリア・渦を毎フレーム更新する。
    /// </summary>
    /// <returns>ForceFieldManager 不在で Failure、終了フェーズで FinishAttack の結果、それ以外は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnCleanup() override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private: //メンバー変数
    //パラメータ
    float warningTime_ = 0.5f;
    float expandTime_  = 0.4f;
    float sustainTime_ = 0.2f;   ///< 最大半径での余韻 = 硬直
    float maxRadius_   = 12.0f;
    float strength_    = 50.0f;
    float falloff_     = 1.0f;   ///< 距離減衰

    /// <summary>
    /// 既定: プレイヤー弾 + プレイヤー本体
    /// </summary>
    uint32_t affectMask_ = GameForceField::AffectBullets | GameForceField::AffectPlayer;

    //ランタイム状態
    bool    ringTriggered_ = false;  ///< スフィア表示開始済みか（Phase 1 突入時に ON）
    int32_t forceFieldId_  = -1;     ///< -1 = 未登録

    //バリアパーティクル & 渦力場
    int32_t              vortexFieldId_        = -1;     ///< -1 = 未登録
    Tako::ForceFieldData vortexFieldBase_{};             ///< preset 基底値（strength/falloff/direction/affectMask）
    bool                 barrierEmitterLoaded_ = false;  ///< LoadPreset 二重ロードガード（インスタンス寿命を跨ぐ）
    bool                 barrierActivated_     = false;  ///< Phase 1 で一度だけ活性化

    //Reset 用キャッシュ
    Tako::ForceFieldManager* cachedForceFieldManager_ = nullptr;
    Tako::EmitterManager*    cachedEmitterManager_    = nullptr;

    //プリセット名（ハードコード）
    const std::string barrierEmitterPreset_   = "repel_barrier_effect";  ///< Particles/Presets/ 配下の JSON 名
    const std::string barrierEmitterInstance_ = "boss_repel_barrier";    ///< emitterMap_ 上の登録名
    const std::string vortexFieldPreset_      = "repel_barrier_vortex";  ///< ParticlePresets/ForceFieldPresets/ 配下の JSON 名
};
