#pragma once
#include "Vector3.h"
#include <json.hpp>

class Boss;
class Player;

/// <summary>
/// ボス離脱移動の共通実行器
/// BTBossRetreat (BehaviorTree ノード) と BossRetreatingState (StateMachine ステート) の
/// 両方からコンポジションで利用される。プレイヤーから離れる方向に smoothstep イージングで移動し、
/// 壁回避 (90 / 180 度回転候補) でステージ端での詰まりを回避する。
/// </summary>
class BossRetreatExecutor {
public:
    /// <summary>
    /// 離脱パラメータ
    /// </summary>
    struct Parameters {
        float retreatSpeed   = 60.0f;  ///< 離脱速度
        float targetDistance = 55.0f;  ///< 目標距離 (プレイヤーからの距離)
    };

    BossRetreatExecutor() = default;
    explicit BossRetreatExecutor(const Parameters& params) : params_(params) {}

    /// <summary>
    /// 離脱開始: 開始位置 / 目標位置 / 所要時間を確定し、ボスの向きをセット。
    /// 既に目標距離以上離れている / プレイヤーと同位置の場合は内部状態のみ初期化し、
    /// IsFinished() がすぐ true を返す状態になる。
    /// </summary>
    void Begin(Boss* boss, const Player* player);

    /// <summary>
    /// フレーム更新: 経過時間を加算し smoothstep でボス位置を更新。
    /// 内部状態が「即時完了」(retreatDuration_ <= 0) の場合は何もしない。
    /// </summary>
    void Tick(Boss* boss, float deltaTime);

    /// <summary>
    /// 終了判定: 即時完了状態、または到達閾値以内に入っている場合に true。
    /// </summary>
    bool IsFinished(const Boss* boss) const;

    /// <summary>
    /// ボス位置を目標位置にスナップ。Tick / IsFinished 後の到達時に呼び出す。
    /// </summary>
    void SnapToTarget(Boss* boss) const;

    /// <summary>
    /// 内部状態をリセット (パラメータは保持)
    /// </summary>
    void Reset();

    const Parameters& GetParameters() const { return params_; }
    void SetParameters(const Parameters& params) { params_ = params; }

    /// <summary>
    /// JSON からパラメータを読み込み
    /// </summary>
    void ApplyJson(const nlohmann::json& j);

    /// <summary>
    /// パラメータを JSON として出力
    /// </summary>
    nlohmann::json ToJson() const;

#ifdef _DEBUG
    /// <summary>
    /// ImGui でパラメータ編集 UI を描画。戻り値: 変更があったか。
    /// </summary>
    /// <param name="idSuffix">複数 UI 共存時の ID サフィックス (例: "##retreat")</param>
    bool DrawImGui(const char* idSuffix);
#endif

private:
    /// <summary>
    /// 壁回避を含めた最適な離脱方向を選択
    /// </summary>
    Tako::Vector3 FindBestRetreatDirection(const Boss* boss,
                                           const Tako::Vector3& primaryDirection,
                                           float retreatDistance) const;

    /// <summary>
    /// 指定方向での実際の移動可能距離 (境界クランプ後) を評価
    /// </summary>
    float EvaluateDirection(const Boss* boss,
                            const Tako::Vector3& direction,
                            float retreatDistance) const;

    static constexpr float kDirectionEpsilon   = 0.01f;   ///< 方向有効判定の閾値
    static constexpr float kArrivalThreshold   = 0.5f;    ///< 到達判定の距離閾値
    static constexpr float kEasingCoeffA       = 3.0f;    ///< smoothstep 係数 A
    static constexpr float kEasingCoeffB       = 2.0f;    ///< smoothstep 係数 B
    static constexpr float kMinRetreatDistance = 10.0f;   ///< 代替方向検討の最小移動距離

    Parameters    params_{};
    Tako::Vector3 startPosition_  {};
    Tako::Vector3 targetPosition_ {};
    float         elapsedTime_     = 0.0f;
    float         retreatDuration_ = 0.0f;
};
