#pragma once
#include "AttackNode.h"
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
/// フェーズ2の矩形境界を 4 象限に分割し、ランダムに 1〜3 象限を攻撃する。
/// 攻撃範囲を Decal で表示 → 点滅警告 → 攻撃発動（コライダー + パーティクル）
/// </summary>
class BTBossAreaAttack : public AttackNode {
public:
    BTBossAreaAttack();
    virtual ~BTBossAreaAttack();

    // パラメータ取得・設定
    float GetWarningDuration() const { return warningDuration_; }
    void  SetWarningDuration(float time) { warningDuration_ = time; }
    float GetBlinkDuration() const { return blinkDuration_; }
    void  SetBlinkDuration(float time) { blinkDuration_ = time; }
    float GetAttackDuration() const { return attackDuration_; }
    void  SetAttackDuration(float time) { attackDuration_ = time; }
    float GetRecoveryTime() const { return recoveryTime_; }
    void  SetRecoveryTime(float time) { recoveryTime_ = time; }
    int   GetMinQuadrants() const { return minQuadrants_; }
    void  SetMinQuadrants(int count) { minQuadrants_ = count; }
    int   GetMaxQuadrants() const { return maxQuadrants_; }
    void  SetMaxQuadrants(int count) { maxQuadrants_ = count; }
    float GetDamage() const { return damage_; }
    void  SetDamage(float damage) { damage_ = damage; }
    float GetBlinkFrequency() const { return blinkFrequency_; }
    void  SetBlinkFrequency(float freq) { blinkFrequency_ = freq; }

private:
    /// <summary>
    /// 固有攻撃ロジック本体
    /// </summary>
    /// <param name= "blackboard">BTBlackboardへのポインタ</param>
    /// <param name= "boss">攻撃を行うBossへのポインタ</param>
    /// <param name= "deltaTime">1フレームの経過時間</param>
    /// <returns> BTNodeStatus::Running（攻撃継続中） / BTNodeStatus::Success（攻撃完了）</returns>
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理
    /// </summary>
    /// <param name= "blackboard">BTBlackboardへのポインタ</param>
    /// <param name= "boss">攻撃を行うBossへのポインタ</param>
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;

    /// <summary>
    ///　固有クリーンアップ処理
    /// </summary>
    void OnCleanup() override;

    /// <summary>
    /// 固有のjsonパラメータ適用
    /// </summary>
    /// <param name= "params">適用するjsonパラメータ</param>
    void OnApplyParameters(const nlohmann::json& params) override;

    /// <summary>
    /// 固有のjsonパラメータ抽出処理
    /// </summary>
    /// <param name= "out">抽出したパラメータを格納するjsonオブジェクトへの参照</param>
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    /// <summary>
    /// 固有のImGuiデバッグ表示
    /// </summary>
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>
    ///　攻撃対象の象限をランダムに選択して activeQuadrants_ を更新する
    /// </summary>
    /// <param name= "blackboard">BTBlackboardへのポインタ</param>
    void SelectRandomQuadrants(BTBlackboard* blackboard);

    /// <summary>
    ///　プレイヤーがどの象限にいるかを取得する
    /// </summary>
    /// <param name= "blackboard">BTBlackboardへのポインタ</param>
    int GetPlayerQuadrant(BTBlackboard* blackboard) const;

    /// <summary>
    ///　象限インデックスからその象限の中心座標を計算して返す
    /// </summary>
    /// <param name= "quadrantIndex">象限インデックス（0〜3）</param>
    /// <param name= "bossPos">ボスの現在位置</param>
    Tako::Vector3 GetQuadrantCenter(int quadrantIndex, const Tako::Vector3& bossPos) const;

    /// <summary>
    /// 点滅フェーズの更新処理
    /// </summary>
    /// <param name= "phaseElapsed">点滅フェーズの経過時間</param>
    void UpdateBlinkingPhase(float phaseElapsed);

    /// <summary>
    /// 攻撃フェーズの開始処理
    /// </summary>
    /// <param name= "boss">攻撃を行うBossへのポインタ</param>
    void BeginAttackPhase(Boss* boss);

    /// <summary>
    /// 攻撃フェーズの終了処理
    /// </summary>
    /// <param name= "boss">攻撃を行うBossへのポインタ</param>
    void EndAttackPhase(Boss* boss);

    //=========================================================================================
    // 定数
    //=========================================================================================

    // 象限の数は固定で4つ（右上、右下、左下、左上）
    static constexpr int   kQuadrantCount = 4;
    // 攻撃範囲の基本アルファ値
    static constexpr float kDecalBaseAlpha = 0.3f;
    // 攻撃範囲表示の点滅アルファ最小値
    static constexpr float kBlinkAlphaMin = 0.15f;
    // 攻撃範囲表示の点滅幅（kBlinkAlphaMin からの増加分）
    static constexpr float kBlinkAlphaAmplitude = 0.55f;
    // 攻撃コライダーの高さ(Y座標)
    static constexpr float kColliderHeight = 10.0f;

    //=========================================================================================
    // パラメータ
    //=========================================================================================
    float warningDuration_ = 1.5f;
    float blinkDuration_ = 1.0f;
    float attackDuration_ = 0.5f;
    float recoveryTime_ = 0.5f;
    int   minQuadrants_ = 1;
    int   maxQuadrants_ = 3;
    float damage_ = 15.0f;
    float blinkFrequency_ = 10.0f;

    //=========================================================================================
    // ランタイム状態
    //=========================================================================================
    float totalDuration_ = 0.0f;
    bool  hasBegunAttack_ = false;
    bool  hasEndedAttack_ = false;
    std::array<bool, kQuadrantCount> activeQuadrants_{};

    //=========================================================================================
    // Decal / コライダー / パーティクル
    //=========================================================================================
    std::array<std::unique_ptr<Tako::Decal>, kQuadrantCount> quadrantDecals_;
    std::array<std::unique_ptr<BossAreaAttackCollider>, kQuadrantCount> quadrantColliders_;
    std::array<Tako::Transform, kQuadrantCount> colliderTransforms_;

    bool particlesInitialized_ = false;
    std::array<std::string, kQuadrantCount> emitterNames_;
};
