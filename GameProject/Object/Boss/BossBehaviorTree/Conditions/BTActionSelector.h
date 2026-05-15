#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

/// <summary>
/// アクション選択条件ノード
/// ActionCounter の値に基づいて成功/失敗を返す
/// </summary>
class BTActionSelector : public Tako::BTNode {
public:
    /// <summary>
    /// 期待するアクションタイプ
    /// </summary>
    enum class ActionType {
        Dash = 0,   // 偶数の場合
        Shoot = 1   // 奇数の場合
    };

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="type">期待するアクションタイプ</param>
    explicit BTActionSelector(ActionType type);

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTActionSelector() = default;

    /// <summary>
    /// ノードの実行
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <returns>実行結果</returns>
    Tako::BTNodeStatus Execute(Tako::BTBlackboard* blackboard) override;

    /// <summary>
    /// ノードのリセット
    /// </summary>
    void Reset() override;

    /// <summary>
    /// アクションタイプを取得
    /// </summary>
    /// <returns>期待するアクションタイプ</returns>
    ActionType GetActionType() const { return expectedType_; }

    /// <summary>
    /// アクションタイプを設定
    /// </summary>
    /// <param name="type">新しいアクションタイプ</param>
    void SetActionType(ActionType type) { expectedType_ = type; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("actionType")) {
            expectedType_ = static_cast<ActionType>(params["actionType"].get<int>());
        }
    }

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
    // 期待するアクションタイプ
    ActionType expectedType_;
};