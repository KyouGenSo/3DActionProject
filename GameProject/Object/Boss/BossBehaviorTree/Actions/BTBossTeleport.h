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
/// フェードアウト → 瞬間移動 → フェードイン で移動。フェード中は body MeshEmitter でディゾルブ演出。
/// テレポート先は固定座標かステージ内ランダムから選択。EnterAttackRecovery は呼ばない（硬直なし仕様）。
/// </summary>
class BTBossTeleport : public AttackNode {
public: //構造体
    enum class TeleportMode : int {
        Fixed            = 0,  ///< 固定座標 (targetPositionX/Y/Z)
        RandomFromBoss   = 1,  ///< ボス現在位置を中心にランダム
        RandomFromPlayer = 2,  ///< プレイヤー位置を中心にランダム
    };

public: //メンバー関数
    BTBossTeleport();
    virtual ~BTBossTeleport() = default;

protected: //メンバー関数
    /// <summary>
    /// フェードアウト→瞬間移動→フェードインのフェーズ制御。
    /// </summary>
    /// <returns>完了フェーズで FinishAttack の結果、進行中は Running</returns>
    Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) override;

    void OnCleanup() override;

    void OnApplyParameters(const nlohmann::json& params) override;

    void OnExtractParameters(nlohmann::json& out) const override;

#ifdef _DEBUG
    bool OnDrawImGui() override;
#endif

private: //非公開関数
    /// <summary>
    /// アルファのみ更新、RGB は originalMaterialColor_ を維持
    /// </summary>
    /// <param name="model">対象モデル</param>
    /// <param name="alpha">不透明度（0.0=透明〜1.0=不透明）</param>
    void UpdateModelAlpha(Tako::Object3d* model, float alpha) const;

private: //メンバー変数
    //パラメータ
    float        fadeOutDuration_         = 0.3f;                          ///< 秒
    float        fadeInDuration_          = 0.3f;                          ///< 秒
    TeleportMode mode_                    = TeleportMode::RandomFromBoss;
    float        targetPositionX_         = 0.0f;                          ///< Fixed 用
    float        targetPositionY_         = 0.0f;                          ///< Fixed 用
    float        targetPositionZ_         = 0.0f;                          ///< Fixed 用
    float        randomMinDistance_       = 10.0f;                         ///< RandomFromBoss 用
    float        randomMaxDistance_       = 30.0f;                         ///< RandomFromBoss 用
    float        playerRandomMinDistance_ = 10.0f;                         ///< RandomFromPlayer 用
    float        playerRandomMaxDistance_ = 30.0f;                         ///< RandomFromPlayer 用

    //ランタイム状態
    Tako::Vector3                                          targetTeleportPosition_{};          ///< OnInitialize で確定
    bool                                                   teleportFired_            = false;
    Tako::Vector4 originalMaterialColor_{1.0f, 0.0f, 0.0f, 0.0f};                              ///< OnInitialize 時の値
    bool                                                   originalTransparentState_ = false;  ///< OnInitialize 時の値
};
