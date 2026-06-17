#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"
#include "Vector3.h"
#include "../../../../Common/GameConst.h"

namespace Tako {
    class EmitterManager;
}

class Boss;

/// <summary>
/// ボス攻撃アクションノードの基底クラス。
/// </summary>
class AttackNode : public Tako::BTNode {
public:
    AttackNode();
    virtual ~AttackNode() = default;

    /// <summary>
    /// 共通フローの固定実装（派生は OnExecute を override）
    /// </summary>
    /// <param name="blackboard">boss / player ポインタを保持する共有ストレージ</param>
    /// <returns>boss 未取得なら Failure。それ以外は OnExecute の結果</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) final;

    void Reset() override;

    void ApplyParameters(const nlohmann::json& params) override;

    [[nodiscard]] nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    bool DrawImGui() override;
#endif

protected:
    //=================== 派生クラスが override するフック ===================

    /// <summary>
    /// 攻撃ロジック本体。派生は elapsedTime_ += deltaTime も自分で行う。
    /// </summary>
    /// <param name="blackboard">boss / player ポインタを保持する共有ストレージ</param>
    /// <param name="boss">操作対象のボス（非 null 保証）</param>
    /// <param name="deltaTime">前フレームからの経過秒</param>
    /// <returns>攻撃継続中は Running、全フェーズ完了で Success（FinishAttack 経由）</returns>
    virtual Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) = 0;

    /// <summary>
    /// 初回 Execute 時の派生固有初期化
    /// </summary>
    virtual void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) { (void)blackboard; (void)boss; }

    /// <summary>
    /// 後始末。Reset / 成功終了 / Failure 経路で共通に呼ばれる。
    /// </summary>
    virtual void OnCleanup() {}

    virtual void OnApplyParameters(const nlohmann::json& params) { (void)params; }

    virtual void OnExtractParameters(nlohmann::json& out) const { (void)out; }

#ifdef _DEBUG
    virtual bool OnDrawImGui() { return false; }
#endif

    //=================== 派生クラスが利用する Helper ===================

    /// <summary>
    /// Boss を硬直状態に遷移させる（多重呼び出しは enteredRecovery_ がガード）
    /// </summary>
    /// <param name="boss">硬直させるボス。null なら何もしない</param>
    void EnterAttackRecovery(Boss* boss);

    /// <summary>
    /// 攻撃成功終了: ExitRecovery → ForceVulnerable 解除 → OnCleanup → Success
    /// </summary>
    /// <returns>常に Success</returns>
    Tako::BTNodeStatus FinishAttack();

    /// <summary>
    /// プレイヤーへの水平方向（y=0・正規化）。近すぎ / 未取得は零ベクトル。
    /// </summary>
    /// <param name="blackboard">boss / player ポインタを保持する共有ストレージ</param>
    /// <param name="epsilon">この水平距離以下なら零ベクトルを返す閾値</param>
    /// <returns>正規化済み水平方向。boss/player 未取得または近すぎは零ベクトル</returns>
    Tako::Vector3 PlanarDirToPlayer(Tako::BTBlackboard* blackboard, float epsilon = GameConst::kDirectionEpsilon) const;

    /// <summary>
    /// プレイヤー方向へ Boss を即時旋回させ使用方向を返す。近すぎ / 未取得は旋回せず零ベクトル。
    /// </summary>
    /// <param name="blackboard">boss / player ポインタを保持する共有ストレージ</param>
    /// <param name="epsilon">この水平距離以下なら旋回しない閾値</param>
    /// <returns>旋回に用いた正規化済み水平方向。旋回しなかった場合は零ベクトル</returns>
    Tako::Vector3 FacePlayerInstant(Tako::BTBlackboard* blackboard, float epsilon = GameConst::kDirectionEpsilon);

protected:
    //=================== 共通メンバ ===================

    float elapsedTime_ = 0.0f;

    bool isFirstExecute_ = true;

    /// true で攻撃中も常時スタン誘発可（Recovery 状態と無関係に近接でスタン）
    bool isBypassRecoveryGuard_ = false;

    bool enteredRecovery_ = false;

    Boss* cachedBoss_ = nullptr;

    Tako::EmitterManager* cachedEmitterManager_ = nullptr;
};
