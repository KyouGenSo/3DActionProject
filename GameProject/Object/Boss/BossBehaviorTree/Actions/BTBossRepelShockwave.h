#pragma once
#include "AttackNode.h"
#include "../../../../Common/ForceFieldAffectMask.h"
#include "Vector3.h"
#include <cstdint>
#include <string>

namespace Tako {
    class ForceFieldManager;
}

class Boss;

/// <summary>
/// ボス Phase1 新攻撃：リパルス・ショックウェーブ
/// </summary>
/// <remarks>
/// ForceField の Repel タイプを 1 個生成し、ボスを中心に放射状の押し戻し場を発生させる。
/// プレイヤー弾は速度逆転で吹き飛ばされ、速度低下で消滅する（PlayerBullet 側で対応）。
///
/// 内部フェーズ:
///   Phase 0 (0〜warningTime_)              予兆: 弱め力場
///   Phase 1 (warningTime_ 〜 +expandTime_) 展開: 半径を 0 → maxRadius_ へランプアップ
///   Phase 2 (+expandTime_ 〜 +sustainTime_) 持続: 最大半径維持 + Recovery 突入（隙）
///   Phase 3                                 終了: Cleanup
///
/// クールダウン・発動条件は本ノードの責務外（BT 親側で表現）。
/// </remarks>
class BTBossRepelShockwave : public AttackNode {
public:
    BTBossRepelShockwave();
    virtual ~BTBossRepelShockwave() = default;

    /// <summary>総所要時間（warning + expand + sustain）</summary>
    [[nodiscard]] float GetTotalDuration() const { return warningTime_ + expandTime_ + sustainTime_; }

protected:
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;
    void OnCleanup() override;
    void OnApplyParameters(const nlohmann::json& params) override;
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
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

    /// <summary>衝撃波リング演出のエミッタ名</summary>
    std::string ringEmitterName_ = "boss_repel_ring";
    /// <summary>中心フラッシュ演出のエミッタ名</summary>
    std::string flashEmitterName_ = "boss_repel_flash";

    //======================== ランタイム状態 ========================
    bool ringTriggered_ = false;       ///< 衝撃波エミッター起動済みか（Phase 1 突入時に ON）
    int32_t forceFieldId_ = -1;        ///< 登録した ForceField のインデックス（-1 = 未登録）

    //======================== Reset 用キャッシュ ========================
    Tako::ForceFieldManager* cachedForceFieldManager_ = nullptr;
};
