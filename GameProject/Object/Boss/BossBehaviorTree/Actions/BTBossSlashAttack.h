#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "../../../../Collision/MeteorImpactCollider.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <string>

class Boss;

/// <summary>
/// ボスの遠距離斬撃攻撃アクションノード
/// プレイヤー位置を狙い、円形Decalで攻撃範囲を予告した後に斬撃を発動する
/// Warning(予兆表示) → Blinking(点滅警告) → Attack(斬撃発動) → Recovery(硬直)
/// </summary>
class BTBossSlashAttack : public BTNode {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTBossSlashAttack();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTBossSlashAttack();

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
    float GetAttackRadius() const { return attackRadius_; }
    void SetAttackRadius(float radius) { attackRadius_ = radius; }
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
        if (params.contains("attackRadius")) {
            attackRadius_ = params["attackRadius"];
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
    /// 斬撃攻撃の初期化（プレイヤー位置取得、Decal/コライダー/パーティクル初期化）
    /// </summary>
    /// <param name="boss">ボス</param>
    /// <param name="blackboard">ブラックボード（プレイヤー位置取得用）</param>
    void InitializeSlashAttack(Boss* boss, BTBlackboard* blackboard);

    /// <summary>
    /// Blinking フェーズの更新（sin波点滅）
    /// </summary>
    /// <param name="phaseElapsed">フェーズ内の経過時間</param>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// Attack フェーズの開始（Decal非表示→コライダー有効化→パーティクル発動）
    /// </summary>
    /// <param name="boss">ボス</param>
    void BeginAttackPhase(Boss* boss);

    /// <summary>
    /// Attack フェーズの終了（コライダー無効化→パーティクル停止）
    /// </summary>
    /// <param name="boss">ボス</param>
    void EndAttackPhase(Boss* boss);

    /// <summary>
    /// リソースのクリーンアップ
    /// </summary>
    void Cleanup();

private:
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

    //=========================================================================================
    // パラメータ
    //=========================================================================================

    // === 時間制御 ===
    float warningDuration_ = 1.0f;    ///< 予兆表示時間
    float blinkDuration_ = 0.8f;      ///< 点滅警告時間
    float attackDuration_ = 0.3f;     ///< 斬撃発動時間
    float recoveryTime_ = 0.5f;       ///< 硬直時間

    // === 攻撃パラメータ ===
    float attackRadius_ = 8.0f;       ///< 攻撃半径（Decal・コライダー共通）
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
    // Decal（単一ターゲット）
    //=========================================================================================

    /// <summary>
    /// 攻撃範囲表示用 Decal
    /// </summary>
    std::unique_ptr<Tako::Decal> slashDecal_;

    //=========================================================================================
    // コライダー（単一ターゲット）
    //=========================================================================================

    /// <summary>
    /// ダメージ判定用コライダー（MeteorImpactCollider を再利用、パリィ対応済み）
    /// </summary>
    std::unique_ptr<MeteorImpactCollider> slashCollider_;

    /// <summary>
    /// コライダー用の Transform（コライダーが Transform* を参照するため保持）
    /// </summary>
    Tako::Transform colliderTransform_;

    //=========================================================================================
    // パーティクル
    //=========================================================================================

    /// <summary>
    /// パーティクル初期化済みフラグ
    /// </summary>
    bool particleInitialized_ = false;

    /// <summary>
    /// パーティクルエミッター名
    /// </summary>
    std::string emitterName_;
};
