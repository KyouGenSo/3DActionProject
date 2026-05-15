#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"

class Boss;

/// <summary>
/// ボスの待機アクションノード
/// </summary>
class BTBossIdle : public Tako::BTNode {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr float kDirectionEpsilon = 0.01f;   ///< 方向判定の閾値

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossIdle();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossIdle() = default;

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
    /// 待機時間の設定
    /// </summary>
    /// <param name="duration">待機時間</param>
    void SetIdleDuration(float duration) { idleDuration_ = duration; }

    /// <summary>
    /// 待機時間の取得
    /// </summary>
    /// <returns>待機時間</returns>
    float GetIdleDuration() const { return idleDuration_; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("idleDuration")) {
            idleDuration_ = params["idleDuration"];
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
    /// <summary>
    /// プレイヤーの方向を向く処理
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="deltaTime">経過時間</param>
    void LookAtPlayer(Tako::BTBlackboard* blackboard, float deltaTime);


    // 待機時間（次の行動までの時間）
    float idleDuration_ = 2.0f;

    // 回転速度（ラジアン/秒）
    float rotationSpeed_ = 5.0f;

    // 経過時間
    float elapsedTime_ = 0.0f;

    // 初回実行フラグ
    bool isFirstExecute_ = true;
};