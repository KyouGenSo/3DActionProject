#pragma once
#include "AttackNode.h"
#include "../../../../Collision/MeteorImpactCollider.h"
#include "Decal.h"
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <string>

class Boss;

/// <summary>
/// ボスの遠距離斬撃攻撃アクションノード
/// プレイヤー位置を狙い、円形 Decal で攻撃範囲を予告した後に斬撃を発動する。
/// Warning（予兆表示）→ Blinking（点滅警告）→ Attack（斬撃発動）→ Recovery（硬直）
/// </summary>
class BTBossSlashAttack : public AttackNode {
public:
    BTBossSlashAttack();
    virtual ~BTBossSlashAttack();

    // パラメータ取得・設定
    float GetWarningDuration() const { return warningDuration_; }
    void  SetWarningDuration(float time) { warningDuration_ = time; }
    float GetBlinkDuration() const { return blinkDuration_; }
    void  SetBlinkDuration(float time) { blinkDuration_ = time; }
    float GetAttackDuration() const { return attackDuration_; }
    void  SetAttackDuration(float time) { attackDuration_ = time; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    float GetAttackRadius() const { return attackRadius_; }
    void  SetAttackRadius(float radius) { attackRadius_ = radius; }
    float GetDamage() const { return damage_; }
    void  SetDamage(float damage) { damage_ = damage; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    void  SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }

protected:
    /// <summary>
    /// 固有攻撃ロジック本体（Warning → Blinking → Attack → Recovery のフェーズ制御）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    /// <returns>BTNodeStatus::Running（攻撃継続中） / BTNodeStatus::Success（攻撃完了）</returns>
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理（プレイヤー位置への着弾点確定、Decal の準備、totalDuration の算出）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;

    /// <summary>
    /// 固有クリーンアップ処理（Decal / コライダー / パーティクルの解放とフラグリセット）
    /// </summary>
    void OnCleanup() override;

    /// <summary>
    /// 固有のjsonパラメータ適用
    /// </summary>
    /// <param name="params">適用するjsonパラメータ</param>
    void OnApplyParameters(const nlohmann::json& params) override;

    /// <summary>
    /// 固有のjsonパラメータ抽出処理
    /// </summary>
    /// <param name="out">抽出したパラメータを格納するjsonオブジェクトへの参照</param>
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    /// <summary>
    /// 固有のImGuiデバッグ表示
    /// </summary>
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>
    /// 点滅フェーズの更新処理（攻撃範囲 Decal の透明度をサインカーブで点滅させる）
    /// </summary>
    /// <param name="phaseElapsed">点滅フェーズの経過時間</param>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// 攻撃フェーズの開始処理（コライダー有効化と斬撃パーティクル起動）
    /// </summary>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void BeginAttackPhase(Boss* boss);

    /// <summary>
    /// 攻撃フェーズの終了処理（コライダー無効化と Decal の非表示化）
    /// </summary>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void EndAttackPhase(Boss* boss);

    static constexpr float kDecalBaseAlpha = 0.3f;
    static constexpr float kBlinkAlphaMin = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float warningDuration_ = 1.0f;
    float blinkDuration_ = 0.8f;
    float attackDuration_ = 0.3f;
    float recoveryTime_ = 0.5f;
    float attackRadius_ = 8.0f;
    float damage_ = 15.0f;
    float blinkFrequency_ = 10.0f;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;
    bool  hasBegunAttack_ = false;
    bool  hasEndedAttack_ = false;

    //=========================================================================================
    // Decal / コライダー / パーティクル
    //=========================================================================================
    std::unique_ptr<Tako::Decal> slashDecal_;
    std::unique_ptr<MeteorImpactCollider> slashCollider_;
    Tako::Transform colliderTransform_;
    bool particleInitialized_ = false;
    std::string emitterName_;
};
