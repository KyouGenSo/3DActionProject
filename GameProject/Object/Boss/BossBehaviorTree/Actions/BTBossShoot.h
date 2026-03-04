#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "../../../../Effect/BulletSignEffect.h"
#include "Vector3.h"

class Boss;

/// <summary>
/// ボスの射撃アクションノード
/// </summary>
class BTBossShoot : public BTNode {
    //=========================================================================================
    // 定数
    //=========================================================================================
private:
    static constexpr float kDirectionEpsilon = 0.01f;  ///< 方向判定の閾値
    static constexpr float kAngleEpsilon = 0.001f;     ///< 角度判定の閾値

public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossShoot();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossShoot() = default;

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

    /// パラメータ取得・設定
    [[nodiscard]] float GetChargeTime() const { return chargeTime_; }
    void SetChargeTime(float time) { chargeTime_ = time; }

    [[nodiscard]] float GetBulletSpeed() const { return bulletSpeed_; }
    void SetBulletSpeed(float speed) { bulletSpeed_ = speed; }

    [[nodiscard]] float GetSpreadAngle() const { return spreadAngle_; }
    void SetSpreadAngle(float angle) { spreadAngle_ = angle; }

    [[nodiscard]] float GetRecoveryTime() const { return recoveryTime_; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }

    [[nodiscard]] int GetBulletCount() const { return bulletCount_; }
    void SetBulletCount(int count) { bulletCount_ = count; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("chargeTime")) {
            chargeTime_ = params["chargeTime"];
        }
        if (params.contains("bulletSpeed")) {
            bulletSpeed_ = params["bulletSpeed"];
        }
        if (params.contains("spreadAngle")) {
            spreadAngle_ = params["spreadAngle"];
        }
        if (params.contains("recoveryTime")) {
            recoveryTime_ = params["recoveryTime"];
        }
        if (params.contains("bulletCount")) {
            bulletCount_ = params["bulletCount"];
        }
    }

    /// <summary>
    /// パラメータを JSON として抽出
    /// </summary>
    [[nodiscard]] nlohmann::json ExtractParameters() const override;

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
    void InitializeShoot(Boss* boss);

    /// <summary>
    /// プレイヤーを狙う処理
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="deltaTime">経過時間</param>
    void AimAtPlayer(Boss* boss, float deltaTime);

    /// <summary>
    /// 弾を発射
    /// </summary>
    /// <param name="boss">ボス</param>
    void FireBullets(Boss* boss);

    /// <summary>
    /// 弾の発射方向を計算
    /// </summary>
    /// <param name="baseDirection">基準方向</param>
    /// <param name="angleOffset">角度オフセット（ラジアン）</param>
    /// <returns>発射方向</returns>
    Tako::Vector3 CalculateBulletDirection(const Tako::Vector3& baseDirection, float angleOffset);

    // 射撃前の準備時間
    float chargeTime_ = 0.9f;

    // 射撃後の硬直時間
    float recoveryTime_ = 0.5f;

    // 状態の総時間
    float totalDuration_ = 1.0f;

    // 弾が発射済みかどうか
    bool hasFired_ = false;

    // 弾の速度
    float bulletSpeed_ = 20.0f;

    // 扇状発射の角度（ラジアン）
    float spreadAngle_ = 0.2618f; // 約15度

    // 経過時間
    float elapsedTime_ = 0.0f;

    // 初回実行フラグ
    bool isFirstExecute_ = true;

    // 弾数（ImGui 調整用）
    int bulletCount_ = 3;  ///< 発射する弾数

    // 射撃予兆エフェクト
    BulletSignEffect bulletSignEffect_;
};