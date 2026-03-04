#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの連続追尾射撃アクションノード
/// プレイヤー方向に連続で弾を発射する攻撃パターン
/// 発射中もプレイヤーの方向を追尾し続ける
/// </summary>
class BTBossRapidFire : public BTNode {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossRapidFire();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossRapidFire() = default;

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
    float GetChargeTime() const { return chargeTime_; }
    void SetChargeTime(float time) { chargeTime_ = time; }
    int GetBulletCount() const { return bulletCount_; }
    void SetBulletCount(int count) { bulletCount_ = count; }
    float GetFireInterval() const { return fireInterval_; }
    void SetFireInterval(float interval) { fireInterval_ = interval; }
    float GetBulletSpeed() const { return bulletSpeed_; }
    void SetBulletSpeed(float speed) { bulletSpeed_ = speed; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("chargeTime")) {
            chargeTime_ = params["chargeTime"];
        }
        if (params.contains("bulletCount")) {
            bulletCount_ = params["bulletCount"];
        }
        if (params.contains("fireInterval")) {
            fireInterval_ = params["fireInterval"];
        }
        if (params.contains("bulletSpeed")) {
            bulletSpeed_ = params["bulletSpeed"];
        }
        if (params.contains("recoveryTime")) {
            recoveryTime_ = params["recoveryTime"];
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
    /// 射撃パラメータの初期化
    /// </summary>
    /// <param name="boss">ボス</param>
    void InitializeRapidFire(Boss* boss);

    /// <summary>
    /// プレイヤーを狙う処理
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="deltaTime">経過時間</param>
    void AimAtPlayer(BTBlackboard* blackboard, float deltaTime);

    /// <summary>
    /// 弾を1発発射
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    void FireBullet(BTBlackboard* blackboard);

    /// <summary>
    /// プレイヤーへの方向を計算
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <returns>プレイヤーへの正規化された方向ベクトル</returns>
    Tako::Vector3 CalculateDirectionToPlayer(BTBlackboard* blackboard);

    // 射撃前の準備時間
    float chargeTime_ = 0.9f;

    // 発射する弾の数
    int bulletCount_ = 5;

    // 発射間隔（秒）
    float fireInterval_ = 0.15f;

    // 射撃後の硬直時間
    float recoveryTime_ = 0.5f;

    // 状態の総時間
    float totalDuration_ = 0.0f;

    // 弾の速度
    float bulletSpeed_ = 20.0f;

    // 経過時間
    float elapsedTime_ = 0.0f;

    // 発射済み弾数
    int firedCount_ = 0;

    // 前回発射からの経過時間
    float timeSinceLastFire_ = 0.0f;

    // 初回実行フラグ
    bool isFirstExecute_ = true;

    // 射撃予兆エフェクト
    BulletSignEffect bulletSignEffect_;
};
