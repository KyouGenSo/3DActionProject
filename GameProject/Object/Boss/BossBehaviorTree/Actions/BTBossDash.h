#pragma once
#include "BTNode.h"
#include "BTBlackboard.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスのダッシュアクションノード
/// </summary>
class BTBossDash : public Tako::BTNode {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr float kDirectionEpsilon = 0.01f;  ///< 方向判定の閾値

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossDash();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossDash() = default;

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
    float GetDashSpeed() const { return dashSpeed_; }
    void SetDashSpeed(float speed) { dashSpeed_ = speed; }
    float GetDashDuration() const { return dashDuration_; }
    void SetDashDuration(float duration) { dashDuration_ = duration; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("dashSpeed")) {
            dashSpeed_ = params["dashSpeed"];
        }
        if (params.contains("dashDuration")) {
            dashDuration_ = params["dashDuration"];
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
    /// ダッシュパラメータの初期化
    /// </summary>
    /// <param name="boss">ボス</param>
    void InitializeDash(Boss* boss);

    /// <summary>
    /// ダッシュ移動の更新
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="deltaTime">経過時間</param>
    void UpdateDashMovement(Boss* boss, float deltaTime);

    // ダッシュ方向
    Tako::Vector3 dashDirection_;

    // ダッシュ速度
    float dashSpeed_ = 60.0f;

    // ダッシュ時間
    float dashDuration_ = 0.5f;

    // ダッシュ開始位置
    Tako::Vector3 startPosition_;

    // ダッシュ目標位置
    Tako::Vector3 targetPosition_;

    // 経過時間
    float elapsedTime_ = 0.0f;

    // 初回実行フラグ
    bool isFirstExecute_ = true;

    // ダッシュ距離範囲（ImGui 調整用）
    float minDistance_ = 10.0f;  ///< 最小ダッシュ距離
    float maxDistance_ = 50.0f;  ///< 最大ダッシュ距離

    // 調整可能パラメータ（ImGui 編集用）
    float vibrationFreq_ = 50.0f;  ///< 振動周波数
    float vibrationAmp_ = 0.05f;   ///< 振動振幅
};