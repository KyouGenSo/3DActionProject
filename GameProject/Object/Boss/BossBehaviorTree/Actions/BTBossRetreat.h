#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "Vector3.h"

class Boss;
class Player;

/// <summary>
/// ボスの離脱アクションノード
/// プレイヤーを向いたまま後方にイージング移動で離れる
/// </summary>
class BTBossRetreat : public BTNode {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr float kDirectionEpsilon = 0.01f;  ///< 方向判定の閾値
    static constexpr float kArrivalThreshold = 0.5f;   ///< 到達判定の閾値
    static constexpr float kEasingCoeffA = 3.0f;       ///< イージング係数 A
    static constexpr float kEasingCoeffB = 2.0f;       ///< イージング係数 B
    static constexpr float kMinRetreatDistance = 10.0f; ///< 代替方向を検討する最小移動距離

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossRetreat();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossRetreat() = default;

    /// <summary>
    /// ノードの実行
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <returns>実行結果</returns>
    BTNodeStatus Execute(BTBlackboard* blackboard) override;

    /// <summary>
    /// ノードのリセット
    /// </summary>
    void Reset() override;

    // パラメータ取得・設定
    float GetRetreatSpeed() const { return retreatSpeed_; }
    void SetRetreatSpeed(float speed) { retreatSpeed_ = speed; }
    float GetTargetDistance() const { return targetDistance_; }
    void SetTargetDistance(float distance) { targetDistance_ = distance; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("retreatSpeed")) {
            retreatSpeed_ = params["retreatSpeed"];
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
    /// 離脱パラメータの初期化
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="player">プレイヤー</param>
    void InitializeRetreat(const Player* player);

    /// <summary>
    /// 離脱移動の更新
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="deltaTime">経過時間</param>
    void UpdateRetreatMovement(float deltaTime);

    /// <summary>
    /// エリア内に収まる位置を計算
    /// </summary>
    /// <param name="position">調整前の位置</param>
    /// <param name="boss">ボスのポインター</param>
    /// <returns>エリア内に収まる位置</returns>
    Tako::Vector3 ClampToArea(const Tako::Vector3& position);

    /// <summary>
    /// 最適な離脱方向を探索（壁回避）
    /// </summary>
    /// <param name="primaryDirection">基本の離脱方向</param>
    /// <param name="retreatDistance">離脱距離</param>
    /// <returns>最適な離脱方向</returns>
    Tako::Vector3 FindBestRetreatDirection(const Tako::Vector3& primaryDirection, float retreatDistance);

    /// <summary>
    /// 指定方向での移動距離を評価
    /// </summary>
    /// <param name="direction">評価する方向</param>
    /// <param name="retreatDistance">離脱距離</param>
    /// <returns>実際に移動できる距離</returns>
    float EvaluateDirection(const Tako::Vector3& direction, float retreatDistance);

    //=========================================================================================
    // メンバ変数
    //=========================================================================================
private:
    // ボスのポインター
    Boss* boss_ = nullptr;             ///< ボスのポインター

    // パラメータ
    float retreatSpeed_ = 60.0f;       ///< 離脱速度
    float targetDistance_ = 55.0f;     ///< 目標距離（プレイヤーからの距離）

    // 状態管理
    Tako::Vector3 startPosition_;            ///< 開始位置
    Tako::Vector3 targetPosition_;           ///< 目標位置（計算済み）
    float elapsedTime_ = 0.0f;         ///< 経過時間
    float retreatDuration_ = 0.0f;     ///< 離脱所要時間（距離から動的計算）
    bool isFirstExecute_ = true;       ///< 初回実行フラグ
};
