#pragma once
#include "Vector3.h"
#include <json.hpp>

class Boss;
class Player;

/// <summary>
/// プレイヤーから離れる離脱移動の実行器（壁回避付き）
/// </summary>
class BossRetreatExecutor {
public:
    struct Parameters {
        float retreatSpeed   = 60.0f;
        float targetDistance = 55.0f;  ///< プレイヤーからの目標距離
    };

    BossRetreatExecutor() = default;
    explicit BossRetreatExecutor(const Parameters& params) : params_(params) {}

    /// <summary>
    /// 開始位置 / 目標位置 / 所要時間を確定。既に目標距離以上離れていれば IsFinished() が即 true になる
    /// </summary>
    /// <param name="boss">離脱させるボス。現在位置を開始位置とし向きを離脱方向へ更新する</param>
    /// <param name="player">距離計算の基準。nullptr なら移動せず即完了扱い</param>
    void Begin(Boss* boss, const Player* player);

    /// <summary>
    /// smoothstep でボス位置を更新。即時完了 (retreatDuration_ <= 0) なら何もしない
    /// </summary>
    /// <param name="boss">位置を更新するボス</param>
    /// <param name="deltaTime">経過時間 (秒)</param>
    void Tick(Boss* boss, float deltaTime);

    /// <summary>
    /// 即時完了、または到達閾値以内なら true
    /// </summary>
    /// <param name="boss">現在位置を判定するボス</param>
    /// <returns>移動不要 (retreatDuration_ <= 0)、または目標まで kArrivalThreshold 未満なら true</returns>
    bool IsFinished(const Boss* boss) const;

    /// <summary>
    /// ボス位置を目標へスナップ（到達時に呼ぶ）
    /// </summary>
    /// <param name="boss">目標位置に移動させるボス</param>
    void SnapToTarget(Boss* boss) const;

    /// <summary>
    /// 内部状態をリセット（パラメータは保持）
    /// </summary>
    void Reset();

    const Parameters& GetParameters() const { return params_; }
    void SetParameters(const Parameters& params) { params_ = params; }

    void ApplyJson(const nlohmann::json& j);

    nlohmann::json ToJson() const;

#ifdef _DEBUG
    /// <summary>
    /// ImGui 編集 UI を描画。変更があれば true
    /// </summary>
    bool DrawImGui(const char* idSuffix);
#endif

private:
    /// <summary>
    /// 壁回避を含めた最適な離脱方向を選択。primary が kMinRetreatDistance 以上動けるならそれを返し、
    /// 不足時は左右90度・180度の候補から実移動距離が最大の方向を選ぶ
    /// </summary>
    /// <param name="boss">境界判定に使うボス</param>
    /// <param name="primaryDirection">プレイヤーから離れる正規化済みの基本方向</param>
    /// <param name="retreatDistance">目標距離まで離れるのに必要な移動距離</param>
    /// <returns>実際に最も移動できる正規化方向</returns>
    Tako::Vector3 FindBestRetreatDirection(const Boss* boss,
                                           const Tako::Vector3& primaryDirection,
                                           float retreatDistance) const;

    /// <summary>
    /// 境界クランプ後の実移動可能距離を評価
    /// </summary>
    /// <param name="boss">境界判定に使うボス</param>
    /// <param name="direction">評価する移動方向</param>
    /// <param name="retreatDistance">クランプ前に進ませたい距離</param>
    /// <returns>境界でクランプした後に実際に動ける XZ 距離</returns>
    float EvaluateDirection(const Boss* boss,
                            const Tako::Vector3& direction,
                            float retreatDistance) const;

    static constexpr float kDirectionEpsilon   = 0.01f;   ///< 方向有効判定の閾値
    static constexpr float kArrivalThreshold   = 0.5f;    ///< 到達判定の距離閾値
    static constexpr float kMinRetreatDistance = 10.0f;   ///< 代替方向検討の最小移動距離

    Parameters    params_{};
    Tako::Vector3 startPosition_  {};
    Tako::Vector3 targetPosition_ {};
    float         elapsedTime_     = 0.0f;
    float         retreatDuration_ = 0.0f;
};
