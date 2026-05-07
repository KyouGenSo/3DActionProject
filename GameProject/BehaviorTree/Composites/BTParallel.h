#pragma once
#include "../Core/BTComposite.h"
#include "../Core/BTBlackboard.h"

/// <summary>
/// パラレルノード（並列実行コンポジット）
/// 全ての子ノードを 1 フレームで並列に Execute し、
/// 終了判定は Policy に応じて切り替える。
/// </summary>
/// <remarks>
/// 既存の BTSequence/BTSelector は子を直列実行するため、
/// 「メイン攻撃が走っている間に別の攻撃を同時に行う」といった
/// 並列構成が表現できなかった。BTParallel はそのギャップを埋める。
/// </remarks>
class BTParallel : public BTComposite {
public:
    /// <summary>
    /// 終了判定ポリシー
    /// </summary>
    enum class Policy : uint32_t {
        /// <summary>
        /// 全ての子が Success → Success
        /// 1 つでも Failure になった瞬間 → 残り Running 子を Reset し Failure
        /// </summary>
        AllSuccess = 0,

        /// <summary>
        /// 1 つでも Success → 残り Running 子を Reset し Success
        /// 全 Failure → Failure
        /// </summary>
        AnySuccess = 1,

        /// <summary>
        /// 子[0] が Running 以外になった瞬間 → 残り Running 子を Reset し、子[0] の結果を返す
        /// （メイン攻撃 + サブ攻撃の並列実行用。メインが終わればサブも終わる）
        /// </summary>
        MainChild = 2
    };

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="policy">終了判定ポリシー（既定: MainChild）</param>
    explicit BTParallel(Policy policy = Policy::MainChild);

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTParallel() = default;

    /// <summary>
    /// ノードの実行
    /// </summary>
    BTNodeStatus Execute(BTBlackboard* blackboard) override;

    /// <summary>
    /// ノードのリセット
    /// </summary>
    void Reset() override;

    /// <summary>
    /// ポリシー変更
    /// </summary>
    void SetPolicy(Policy policy) { policy_ = policy; }

    /// <summary>
    /// ポリシー取得
    /// </summary>
    Policy GetPolicy() const { return policy_; }

    /// <summary>
    /// JSON からパラメータを適用（policy: "AllSuccess" / "AnySuccess" / "MainChild"）
    /// </summary>
    void ApplyParameters(const nlohmann::json& params) override;

    /// <summary>
    /// パラメータを JSON として抽出
    /// </summary>
    nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    /// <summary>
    /// ImGui でパラメータ編集 UI を描画
    /// </summary>
    bool DrawImGui() override;
#endif

private:
    /// <summary>
    /// 終了判定ポリシー
    /// </summary>
    Policy policy_;

    /// <summary>
    /// 各子ノードの最新ステータス（初回 Execute 時に children_.size() に合わせて初期化）。
    /// 一度 Success/Failure になった子は、再 Execute されずに状態を保持する（ポリシー判定用）。
    /// </summary>
    std::vector<BTNodeStatus> childStatuses_;
};
