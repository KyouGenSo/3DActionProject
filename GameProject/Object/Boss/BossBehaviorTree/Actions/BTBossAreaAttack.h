#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "../../../../Collision/BossAreaAttackCollider.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <array>
#include <string>

class Boss;

/// <summary>
/// ボスのエリア攻撃アクションノード
/// フェーズ2の矩形境界を4象限に分割し、ランダムに1~3象限を攻撃する
/// 攻撃範囲をDecalで表示 → 点滅警告 → 攻撃発動（コライダー+パーティクル）
/// </summary>
class BTBossAreaAttack : public BTNode {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossAreaAttack();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossAreaAttack();

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
    float GetWarningDuration() const { return warningDuration_; }
    void SetWarningDuration(float time) { warningDuration_ = time; }
    float GetBlinkDuration() const { return blinkDuration_; }
    void SetBlinkDuration(float time) { blinkDuration_ = time; }
    float GetAttackDuration() const { return attackDuration_; }
    void SetAttackDuration(float time) { attackDuration_ = time; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void SetRecoveryTime(float time) { recoveryTime_ = time; }
    int GetMinQuadrants() const { return minQuadrants_; }
    void SetMinQuadrants(int count) { minQuadrants_ = count; }
    int GetMaxQuadrants() const { return maxQuadrants_; }
    void SetMaxQuadrants(int count) { maxQuadrants_ = count; }
    float GetDamage() const { return damage_; }
    void SetDamage(float damage) { damage_ = damage; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    void SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }

    /// <summary>
    /// JSON からパラメータを適用
    /// </summary>
    /// <param name="params">パラメータ JSON</param>
    void ApplyParameters(const nlohmann::json& params) override {
        if (params.contains("warningDuration")) {
            warningDuration_ = params["warningDuration"];
        }
        if (params.contains("blinkDuration")) {
            blinkDuration_ = params["blinkDuration"];
        }
        if (params.contains("attackDuration")) {
            attackDuration_ = params["attackDuration"];
        }
        if (params.contains("recoveryTime")) {
            recoveryTime_ = params["recoveryTime"];
        }
        if (params.contains("minQuadrants")) {
            minQuadrants_ = params["minQuadrants"];
        }
        if (params.contains("maxQuadrants")) {
            maxQuadrants_ = params["maxQuadrants"];
        }
        if (params.contains("damage")) {
            damage_ = params["damage"];
        }
        if (params.contains("blinkFrequency")) {
            blinkFrequency_ = params["blinkFrequency"];
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

private: // プライベートメンバー関数
    /// <summary>
    /// 攻撃の初期化
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    void InitializeAreaAttack(BTBlackboard* blackboard);

    /// <summary>
    /// リソースのクリーンアップ
    /// </summary>
    void Cleanup();

    /// <summary>
    /// ランダムに攻撃象限を選択（プレイヤー象限を確定枠として含める）
    /// </summary>
    /// <param name="blackboard">ブラックボード（プレイヤー位置の取得に使用）</param>
    void SelectRandomQuadrants(BTBlackboard* blackboard);

    /// <summary>
    /// プレイヤーが存在する象限インデックスを取得
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <returns>象限インデックス (0-3)</returns>
    int GetPlayerQuadrant(BTBlackboard* blackboard) const;

    /// <summary>
    /// 象限の中心座標を計算
    /// </summary>
    /// <param name="quadrantIndex">象限インデックス (0-3)</param>
    /// <param name="bossPos">ボスの位置</param>
    /// <returns>象限の中心座標</returns>
    Tako::Vector3 GetQuadrantCenter(int quadrantIndex, const Tako::Vector3& bossPos) const;

    /// <summary>
    /// Warning フェーズの更新
    /// </summary>
    void UpdateWarningPhase();

    /// <summary>
    /// Blinking フェーズの更新
    /// </summary>
    /// <param name="phaseElapsed">フェーズ内の経過時間</param>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// Attack フェーズの開始
    /// </summary>
    /// <param name="boss">ボス</param>
    void BeginAttackPhase(Boss* boss);

    /// <summary>
    /// Attack フェーズの終了
    /// </summary>
    /// <param name="boss">ボス</param>
    void EndAttackPhase(Boss* boss);

private:
    /// <summary>
    /// 象限の数
    /// </summary>
    static constexpr int kQuadrantCount = 4;

    /// <summary>
    /// Decal の基本アルファ値
    /// </summary>
    static constexpr float kDecalBaseAlpha = 0.3f;

    /// <summary>
    /// 点滅アルファ値の最小値
    /// </summary>
    static constexpr float kBlinkAlphaMin = 0.15f;

    /// <summary>
    /// 点滅アルファ値の振幅
    /// </summary>
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

    /// <summary>
    /// コライダーの Y 方向サイズ
    /// </summary>
    static constexpr float kColliderHeight = 10.0f;

    //=========================================================================================
    // パラメータ
    //=========================================================================================

    // === 時間制御 ===
    float warningDuration_ = 1.5f;    ///< 予兆表示時間
    float blinkDuration_ = 1.0f;      ///< 点滅警告時間
    float attackDuration_ = 0.5f;     ///< 攻撃発動時間
    float recoveryTime_ = 0.5f;       ///< 硬直時間

    // === 攻撃パラメータ ===
    int minQuadrants_ = 1;            ///< 最小攻撃象限数
    int maxQuadrants_ = 3;            ///< 最大攻撃象限数
    float damage_ = 15.0f;            ///< ダメージ量
    float blinkFrequency_ = 10.0f;    ///< 点滅周波数 (Hz)

    //=========================================================================================
    // 状態管理
    //=========================================================================================

    float elapsedTime_ = 0.0f;        ///< 経過時間
    float totalDuration_ = 0.0f;      ///< 状態の総時間
    bool isFirstExecute_ = true;      ///< 初回実行フラグ
    bool hasBegunAttack_ = false;     ///< 攻撃開始済みフラグ
    bool hasEndedAttack_ = false;     ///< 攻撃終了済みフラグ
    bool enteredRecovery_ = false;    ///< 硬直開始フラグ

    //=========================================================================================
    // 象限管理
    //=========================================================================================

    /// <summary>
    /// 各象限のアクティブフラグ（攻撃対象かどうか）
    /// </summary>
    std::array<bool, kQuadrantCount> activeQuadrants_{};

    //=========================================================================================
    // Decal
    //=========================================================================================

    /// <summary>
    /// 各象限の攻撃範囲表示用 Decal
    /// </summary>
    std::array<std::unique_ptr<Tako::Decal>, kQuadrantCount> quadrantDecals_;

    //=========================================================================================
    // コライダー
    //=========================================================================================

    /// <summary>
    /// 各象限のダメージ判定用コライダー
    /// </summary>
    std::array<std::unique_ptr<BossAreaAttackCollider>, kQuadrantCount> quadrantColliders_;

    /// <summary>
    /// 各象限コライダー用の Transform（コライダーが Transform* を参照するため保持）
    /// </summary>
    std::array<Tako::Transform, kQuadrantCount> colliderTransforms_;

    //=========================================================================================
    // パーティクル
    //=========================================================================================

    /// <summary>
    /// パーティクル初期化済みフラグ
    /// </summary>
    bool particlesInitialized_ = false;

    /// <summary>
    /// 各象限のパーティクルエミッター名
    /// </summary>
    std::array<std::string, kQuadrantCount> emitterNames_;
};
