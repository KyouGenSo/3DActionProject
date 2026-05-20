#pragma once
#include "AttackNode.h"
#include "Vector3.h"
#include "Vector4.h"
#include <string>

namespace Tako {
    class Object3d;
    class EmitterManager;
}

class Boss;

/// <summary>
/// ボスを瞬間移動させる行動ノード。
/// フェードアウト → 位置瞬時更新 → フェードイン の 3 フェーズで実行する。
/// フェード中は boss_particle_body MeshEmitter を発火させてディゾルブ演出を作る。
/// テレポート先は固定座標 or ステージ全域内のランダム座標から選択可能。
/// </summary>
/// <remarks>
/// 内部フェーズ:
///   Phase 0 (0 〜 fadeOutDuration_)                       alpha 1 → 0、body emitter ON
///   Phase 1 (fadeOutDuration_ 到達)                       alpha = 0、body emitter OFF、座標瞬時更新
///   Phase 2 (〜 fadeOutDuration_ + fadeInDuration_)       alpha 0 → 1、body emitter ON
///   Phase 3 (完了)                                        body emitter OFF、FinishAttack で Success
///
/// AttackNode を継承するが EnterAttackRecovery は呼ばない (硬直なし仕様)。
/// </remarks>
class BTBossTeleport : public AttackNode {
public:
    BTBossTeleport();
    virtual ~BTBossTeleport() = default;

protected:
    /// <summary>
    /// フェーズ制御本体 (fade-out / teleport / fade-in / cleanup)
    /// </summary>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 目標座標の決定 + マテリアル状態のキャッシュ + 半透明モード有効化
    /// </summary>
    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    /// <summary>
    /// マテリアル状態を復帰し、body emitter を OFF にする (中断時の safety)
    /// </summary>
    void OnCleanup() override;

    /// <summary>JSON パラメータ適用</summary>
    void OnApplyParameters(const nlohmann::json& params) override;

    /// <summary>JSON パラメータ抽出</summary>
    void OnExtractParameters(nlohmann::json& out) const override;

#ifdef _DEBUG
    /// <summary>インスペクター UI 描画</summary>
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>
    /// model のマテリアルカラーのアルファ成分を更新 (RGB は originalMaterialColor_ を維持)
    /// </summary>
    void UpdateModelAlpha(Tako::Object3d* model, float alpha) const;

    //======================== パラメータ ========================
    float fadeOutDuration_ = 0.3f;          ///< フェードアウト時間 (秒)
    float fadeInDuration_  = 0.3f;          ///< フェードイン時間 (秒)
    bool  useRandomPosition_ = true;        ///< true: ランダム座標、false: 固定座標
    float targetPositionX_ = 0.0f;          ///< 固定座標 X
    float targetPositionY_ = 0.0f;          ///< 固定座標 Y
    float targetPositionZ_ = 0.0f;          ///< 固定座標 Z
    float randomMinDistance_ = 10.0f;       ///< 現在位置からの最小距離 (random 時)
    float randomMaxDistance_ = 30.0f;       ///< 現在位置からの最大距離 (random 時)

    //======================== ランタイム状態 ========================
    Tako::Vector3 targetTeleportPosition_{};        ///< OnInitialize で確定するテレポート先
    bool teleportFired_ = false;                    ///< Phase 1 の瞬間移動を実行したか
    Tako::Vector4 originalMaterialColor_{1.0f, 0.0f, 0.0f, 0.0f}; ///< OnInitialize 時の material color
    bool originalTransparentState_ = false;         ///< OnInitialize 時の SetTransparent 状態
};
