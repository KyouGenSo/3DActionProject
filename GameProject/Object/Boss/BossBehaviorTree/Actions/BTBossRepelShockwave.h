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
/// ForceField の Repel タイプを 1 個生成し、ボスを中心に放射状の押し戻し場を発生させる。
/// プレイヤー弾は速度逆転で吹き飛ばされ、速度低下で消滅する（PlayerBullet 側で対応）。
/// </summary>
/// <remarks>
/// 内部フェーズ:
///   Phase 0 (0〜warningTime_)              予兆: 弱め力場
///   Phase 1 (warningTime_ 〜 +expandTime_) 展開: 半径を 0 → maxRadius_ へランプアップ
///   Phase 2 (+expandTime_ 〜 +sustainTime_) 持続: 最大半径維持 + Recovery 突入（隙）
///   Phase 3                                 終了: Cleanup
///
/// </remarks>
class BTBossRepelShockwave : public AttackNode {
public:
    BTBossRepelShockwave();
    virtual ~BTBossRepelShockwave() = default;

    /// <summary>総所要時間（warning + expand + sustain）</summary>
    [[nodiscard]] float GetTotalDuration() const { return warningTime_ + expandTime_ + sustainTime_; }

protected:
    /// <summary>
    /// 固有攻撃ロジック本体（予兆 → ForceField 展開 → 持続 → 終了のフェーズ制御）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    /// <returns>Tako::BTNodeStatus::Running（攻撃継続中） / Tako::BTNodeStatus::Success（攻撃完了）</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理（ForceFieldManager のキャッシュと Repel ForceField の登録）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    /// <summary>
    /// 固有クリーンアップ処理（登録済み ForceField の解除とランタイム状態リセット）
    /// </summary>
    void OnCleanup() override;

    /// <summary>
    /// 固有のjsonパラメータ適用
    /// </summary>
    /// <param name="params">適用するjsonパラメータ</param>
    void OnApplyParameters(const nlohmann::json& params) override;

    /// <summary>
    /// 固有のjsonパラメータ抽出処理
    /// </summary>
    /// <param name="out">抽出したパラメータを格納するjsonオブジェクトへの参照</param>
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    /// <summary>
    /// 固有のImGuiデバッグ表示
    /// </summary>
    bool OnDrawImGui() override;
#endif

private:
    //======================== パラメータ ========================
    float warningTime_ = 0.5f;        ///< 予兆時間
    float expandTime_ = 0.4f;         ///< 衝撃波展開時間
    float sustainTime_ = 0.2f;        ///< 持続時間（最大半径での余韻 = 硬直）
    float maxRadius_ = 12.0f;         ///< 最大半径
    float strength_ = 50.0f;          ///< Repel 強度
    float falloff_ = 1.0f;            ///< 距離減衰

    /// <summary>影響対象 mask（既定: プレイヤー弾 + プレイヤー本体）</summary>
    uint32_t affectMask_ = GameForceField::AffectBullets | GameForceField::AffectPlayer;

    //======================== ランタイム状態 ========================
    bool ringTriggered_ = false;       ///< 衝撃波スフィア表示開始済みか（Phase 1 突入時に ON）
    int32_t forceFieldId_ = -1;        ///< 登録した ForceField のインデックス（-1 = 未登録）

    //======================== バリアパーティクル & 渦力場 ========================
    int32_t vortexFieldId_ = -1;                              ///< 追加で登録した渦力場のインデックス（-1 = 未登録）
    Tako::ForceFieldData vortexFieldBase_{};                  ///< preset から読み込んだ基底値（strength/falloff/direction/affectMask を保持）
    bool barrierEmitterLoaded_ = false;                       ///< LoadPreset 二重ロードガード（インスタンス寿命を跨いで保持）
    bool barrierActivated_ = false;                           ///< Phase 1 到達時の一度きり活性化フラグ

    //======================== Reset 用キャッシュ ========================
    Tako::ForceFieldManager* cachedForceFieldManager_ = nullptr;
    Tako::EmitterManager* cachedEmitterManager_ = nullptr;

    //======================== プリセット名（ハードコード） ========================
    const std::string barrierEmitterPreset_   = "repel_barrier_effect";   ///< Particles/Presets/ 配下の JSON 名
    const std::string barrierEmitterInstance_ = "boss_repel_barrier";     ///< emitterMap_ 上の登録名
    const std::string vortexFieldPreset_      = "repel_barrier_vortex";   ///< ParticlePresets/ForceFieldPresets/ 配下の JSON 名
};
