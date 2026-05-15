#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

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
    /// 共通フローを実行する固定実装（派生は OnExecute を override）
    /// </summary>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) final;

    /// <summary>
    /// 標準 Reset。OnCleanup を呼んで Recovery / ForceVulnerable を確実に解除
    /// </summary>
    void Reset() override;

    /// <summary>
    /// 共通キーを読み込み、派生は OnApplyParameters で固有キー処理
    /// </summary>
    void ApplyParameters(const nlohmann::json& params) override;

    /// <summary>
    /// 共通キーを書き込み、派生は OnExtractParameters で固有キー追加
    /// </summary>
    [[nodiscard]] nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    /// <summary>
    /// 共通の Bypass Checkbox を描画、派生は OnDrawImGui で固有 UI
    /// </summary>
    bool DrawImGui() override;
#endif

protected:
    //=================== 派生クラスが override するフック ===================

    /// <summary>
    /// 攻撃ロジック本体（純粋仮想）。派生は elapsedTime_ += deltaTime も自分で行う。
    /// </summary>
    virtual Tako::BTNodeStatus OnExecute(Tako::BTBlackboard* blackboard, Boss* boss, float deltaTime) = 0;

    /// <summary>
    /// 初回実行時の派生固有初期化（totalDuration 計算など）
    /// </summary>
    virtual void OnInitialize(Tako::BTBlackboard* blackboard, Boss* boss) { (void)blackboard; (void)boss; }

    /// <summary>
    /// 後始末。Reset / 成功終了 / Failure 経路で共通に呼ばれる。
    /// </summary>
    virtual void OnCleanup() {}

    /// <summary>
    /// 派生固有 JSON 適用（共通 isBypassRecoveryGuard は基底が処理済み）
    /// </summary>
    virtual void OnApplyParameters(const nlohmann::json& params) { (void)params; }

    /// <summary>
    /// 派生固有 JSON 抽出。out にキーを追加する。
    /// </summary>
    virtual void OnExtractParameters(nlohmann::json& out) const { (void)out; }

#ifdef _DEBUG
    /// <summary>
    /// 派生固有 ImGui描画。
    /// </summary>
    virtual bool OnDrawImGui() { return false; }
#endif

    //=================== 派生クラスが利用する Helper ===================

    /// <summary>
    /// Boss を硬直状態に遷移させる。多重呼び出し safe（enteredRecovery_ がガード）。
    /// </summary>
    void EnterAttackRecovery(Boss* boss);

    /// <summary>
    /// 攻撃成功終了の標準パス: ExitRecovery → ForceVulnerable 解除 → OnCleanup → status=Success。
    /// 派生は終了条件を満たしたら `return FinishAttack();` で抜ける。
    /// </summary>
    Tako::BTNodeStatus FinishAttack();

protected:
    //=================== 共通メンバ ===================

    /// 攻撃開始からの経過時間
    float elapsedTime_ = 0.0f;

    /// 初回 Execute 時に true。基底が OnInitialize 後に false にする。
    bool isFirstExecute_ = true;

    /// <summary>
    /// 攻撃中も常時スタン誘発を許可するかのオプション（既定 false）。
    /// true なら攻撃開始時に Boss::SetForceVulnerable(true) を立て、Recovery 状態と
    /// 無関係にプレイヤー近接攻撃でスタン誘発できる。同じ攻撃ノードを BT 上で
    /// 配置単位に「中断耐性 ON / OFF」を切り替える用途。
    /// </summary>
    bool isBypassRecoveryGuard_ = false;

    /// Boss::EnterRecovery を呼んだか（多重 / 未呼び出しの両方を防ぐガード）
    bool enteredRecovery_ = false;

    /// Reset 経由でも Boss にアクセスできるよう初回 Execute で自動キャッシュ
    Boss* cachedBoss_ = nullptr;

    /// 派生クラスが Emitter を停止したい場合に使う共通キャッシュ（初回 Execute で自動セット）
    Tako::EmitterManager* cachedEmitterManager_ = nullptr;
};
