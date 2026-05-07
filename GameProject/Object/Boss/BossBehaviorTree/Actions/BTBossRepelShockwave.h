#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "../../../../Common/ForceFieldAffectMask.h"
#include "Vector3.h"
#include <cstdint>
#include <string>

namespace Tako {
    class ForceFieldManager;
    class EmitterManager;
}

class Boss;

/// <summary>
/// ボス Phase1 新攻撃：リパルス・ショックウェーブ
/// </summary>
/// <remarks>
/// ForceField の Repel タイプを 1 個生成し、ボスを中心に放射状の押し戻し場を発生させる。
/// プレイヤー弾は速度逆転で吹き飛ばされ、速度低下で消滅する（PlayerBullet 側で対応済み）。
/// プレイヤー本体に作用させたい場合は、Player::Update が `EvaluateForceAt(pos, AffectPlayer)`
/// を呼び出すよう別途統合する必要がある（ここでは ForceField 生成のみ責任を持つ）。
///
/// ノードの内部フェーズ:
///   Phase 0 (0〜warningTime_)              予兆: ボス溜め演出 / 弱め力場
///   Phase 1 (warningTime_ 〜 +expandTime_) 展開: ForceField 半径を 0 → maxRadius_ へランプアップ
///   Phase 2 (+expandTime_ 〜 +sustainTime_) 持続: 力場維持 / 衝撃波粒子拡散
///   Phase 3                                 終了: ForceField 削除 / エミッター停止
///
/// クールダウン・発動条件は本ノードの責務外（BT 親側の Selector / DistanceCondition 等で表現）。
/// </remarks>
class BTBossRepelShockwave : public BTNode {
public:
    BTBossRepelShockwave();
    virtual ~BTBossRepelShockwave() = default;

    /// <summary>ノードの実行</summary>
    BTNodeStatus Execute(BTBlackboard* blackboard) override;

    /// <summary>ノードのリセット（スタン等による中断時の状態復旧含む）</summary>
    void Reset() override;

    /// <summary>JSON からパラメータを適用</summary>
    void ApplyParameters(const nlohmann::json& params) override;

    /// <summary>パラメータを JSON として抽出</summary>
    [[nodiscard]] nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    /// <summary>ImGui でパラメータ編集 UI を描画</summary>
    bool DrawImGui() override;
#endif

    /// <summary>総所要時間（warning + expand + sustain）</summary>
    [[nodiscard]] float GetTotalDuration() const { return warningTime_ + expandTime_ + sustainTime_; }

private:
    /// <summary>ForceField 削除 / エミッター停止 / 状態フラグ初期化（成功終了・Reset 共通）</summary>
    void Cleanup();

    //======================== パラメータ ========================
    float warningTime_ = 0.5f;        ///< 予兆時間
    float expandTime_ = 0.4f;         ///< 衝撃波展開時間（半径ランプアップ）
    float sustainTime_ = 0.2f;        ///< 持続時間（最大半径での余韻）
    float maxRadius_ = 12.0f;         ///< 最大半径
    float strength_ = 50.0f;          ///< Repel 強度（押し戻し力）
    float falloff_ = 1.0f;            ///< 距離減衰（線形）

    /// <summary>影響対象 mask（既定: プレイヤー弾 + プレイヤー本体）</summary>
    uint32_t affectMask_ = GameForceField::AffectBullets | GameForceField::AffectPlayer;

    /// <summary>衝撃波リング演出のエミッタ名</summary>
    std::string ringEmitterName_ = "boss_repel_ring";
    /// <summary>中心フラッシュ演出のエミッタ名</summary>
    std::string flashEmitterName_ = "boss_repel_flash";

    //======================== ランタイム状態 ========================
    float elapsedTime_ = 0.0f;
    bool isFirstExecute_ = true;
    bool ringTriggered_ = false;       ///< 衝撃波エミッター起動済みか（Phase 1 突入時に一度だけ ON）
    int32_t forceFieldId_ = -1;        ///< 登録した ForceField のインデックス（-1 = 未登録）

    //======================== Reset 用キャッシュ ========================
    Tako::ForceFieldManager* cachedForceFieldManager_ = nullptr;
    Tako::EmitterManager* cachedEmitterManager_ = nullptr;
    /// <summary>
    /// Reset 経由（BTParallel が子ノードを中断するケース含む）でも Boss::ExitRecovery を
    /// 確実に呼ぶためのキャッシュ。Reset() からは Blackboard が取れないため Boss* を保持する。
    /// </summary>
    Boss* cachedBoss_ = nullptr;
    /// <summary>EnterRecovery 済みか（多重 ExitRecovery と未呼び出しの両方を防ぐガード）</summary>
    bool enteredRecovery_ = false;
};
