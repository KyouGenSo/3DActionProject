#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"
#include "Vector3.h"

class Boss;
class Player;

/// <summary>
/// ボスのプレイヤー接近アクションノード
/// プレイヤー方向にイージング移動で素早く接近し、一定距離で停止する
/// </summary>
class BTBossApproach : public Tako::BTNode {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr float kDirectionEpsilon = 0.01f;  ///< 方向判定の閾値
    static constexpr float kArrivalThreshold = 0.5f;   ///< 到達判定の閾値
    static constexpr float kEasingCoeffA = 3.0f;       ///< イージング係数 A
    static constexpr float kEasingCoeffB = 2.0f;       ///< イージング係数 B

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossApproach();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossApproach() = default;

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

    // パラメータ取得・設定
    float GetApproachSpeed() const { return approachSpeed_; }
    void SetApproachSpeed(float speed) { approachSpeed_ = speed; }
    float GetTargetDistance() const { return targetDistance_; }
    void SetTargetDistance(float distance) { targetDistance_ = distance; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("approachSpeed")) {
            approachSpeed_ = params["approachSpeed"];
        }
        if (params.contains("targetDistance")) {
            targetDistance_ = params["targetDistance"];
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
    /// 接近パラメータの初期化
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="player">プレイヤー</param>
    void InitializeApproach(Boss* boss, Player* player);

    /// <summary>
    /// 接近移動の更新
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="deltaTime">経過時間</param>
    void UpdateApproachMovement(Boss* boss, float deltaTime);

    //=========================================================================================
    // メンバ変数
    //=========================================================================================
private:
    // パラメータ
    float approachSpeed_ = 80.0f;      ///< 接近速度
    float targetDistance_ = 12.0f;     ///< 目標距離（プレイヤーからの距離）

    // 状態管理
    Tako::Vector3 startPosition_;            ///< 開始位置
    Tako::Vector3 targetPosition_;           ///< 目標位置（計算済み）
    float elapsedTime_ = 0.0f;         ///< 経過時間
    float approachDuration_ = 0.0f;    ///< 接近所要時間（距離から動的計算）
    bool isFirstExecute_ = true;       ///< 初回実行フラグ
};
