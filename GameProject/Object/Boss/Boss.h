#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Transform.h"
#include "vector2.h"
#include "Vector4.h"
#include "Vector3.h"

// 共通定義
#include "../../Common/BulletSpawnRequest.h"
#include "../../Effect/ShakeEffect.h"
#include "../../Effect/HitFlashEffect.h"
#include "../../Common/BulletSpawner.h"
#include "../../UI/HPBarUI.h"
#include "BossPhaseManager.h"

// Tako namespace 前方宣言
namespace Tako {
class OBBCollider;
class Object3d;
class EmitterManager;
class ForceFieldManager;
}

#include "BehaviorTree.h"
#ifdef _DEBUG
#include "BehaviorTreeEditor.h"
#endif

// GameProject 前方宣言
class BossStateMachine;
class BossStunnedState;
class Player;
class BossMeleeAttackCollider;

/// <summary>
/// ボスエネミークラス
/// HP とフェーズ管理、ダメージ処理を制御
/// </summary>
class Boss
{
    // 定数
private:
    // 最大 HP
    static constexpr float kMaxHp = 200.0f;

    // フェーズ2開始 HP 閾値
    static constexpr float kPhase2Threshold = 105.0f;

    // フェーズ2開始時の HP
    static constexpr float kPhase2InitialHp = 100.0f;

    // フェーズ移行スタン発動 HP 閾値
    static constexpr float kPhaseTransitionStunThreshold = 105.0f;

public:
    Boss();
    ~Boss();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// 更新（deltaTime 版）
    /// </summary>
    void Update(float deltaTime);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();
    void DrawShadow();

    /// <summary>
    /// スプライト描画
    /// </summary>
    void DrawSprite();

    /// <summary>
    /// ImGui の描画
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// ダメージ処理
    /// </summary>
    /// <param name="damage">受けるダメージ量</param>
    /// <param name="shakeIntensityOverride">シェイク強度（0以下でデフォルト値使用）</param>
    void OnHit(float damage, float shakeIntensityOverride = 0.0f);

    /// <summary>
    /// シェイクエフェクトを開始
    /// </summary>
    /// <param name="intensity">シェイク強度（0以下でデフォルト値使用）</param>
    void StartShake(float intensity = 0.0f);

    /// <summary>
    /// 弾生成リクエストを追加
    /// </summary>
    /// <param name="position">弾の発射位置</param>
    /// <param name="velocity">弾の速度ベクトル</param>
    void RequestBulletSpawn(const Tako::Vector3& position, const Tako::Vector3& velocity);

    /// <summary>
    /// 保留中の弾生成リクエストを取得して消費
    /// </summary>
    /// <returns>弾生成リクエストのリスト（move で返される）</returns>
    std::vector<BulletSpawnRequest> ConsumePendingBullets();

    /// <summary>
    /// 貫通弾生成リクエストを追加
    /// </summary>
    /// <param name="position">弾の発射位置</param>
    /// <param name="velocity">弾の速度ベクトル</param>
    void RequestPenetratingBulletSpawn(const Tako::Vector3& position, const Tako::Vector3& velocity);

    /// <summary>
    /// 保留中の貫通弾生成リクエストを取得して消費
    /// </summary>
    /// <returns>貫通弾生成リクエストのリスト（move で返される）</returns>
    std::vector<BulletSpawnRequest> ConsumePendingPenetratingBullets();

    //-----------------------------Getters/Setters------------------------------//
    /// <summary>
    /// 座標変換情報を設定
    /// </summary>
    /// <param name="transform">新しい座標変換情報</param>
    void SetTransform(const Tako::Transform& transform) { transform_ = transform; }

    /// <summary>
    /// 平行移動情報を設定
    /// </summary>
    /// <param name="translate">新しい位置情報</param>
    void SetTranslate(const Tako::Vector3& translate) { transform_.translate = translate; }

    /// <summary>
    /// 回転情報を設定
    /// </summary>
    /// <param name="rotate">新しい回転情報（ラジアン）</param>
    void SetRotate(const Tako::Vector3& rotate) { transform_.rotate = rotate; }

    /// <summary>
    /// スケール情報を設定
    /// </summary>
    /// <param name="scale">新しいスケール情報</param>
    void SetScale(const Tako::Vector3& scale) { transform_.scale = scale; }

    /// <summary>
    /// HP を設定
    /// </summary>
    /// <param name="hp">新しい HP 値</param>
    void SetHp(float hp) { hp_ = hp; }

    /// <summary>
    /// フェーズを設定
    /// </summary>
    /// <param name="phase">新しいフェーズ番号</param>
    void SetPhase(uint32_t phase) { phaseManager_.SetPhase(phase); }

    /// <summary>
    /// プレイヤーの参照を設定
    /// </summary>
    /// <param name="player">プレイヤーのポインタ</param>
    void SetPlayer(Player* player);

    /// <summary>
    /// 一時行動停止フラグを設定
    /// </summary>
    /// <param name="isPause">一時行動停止フラグの値</param>
    void SetIsPause(bool isPause) { isPause_ = isPause; }

    //-----------------------------ステートマシン------------------------------//
    /// <summary>
    /// ステートマシンを取得
    /// </summary>
    /// <returns>ステートマシンのポインタ</returns>
    BossStateMachine* GetStateMachine() const { return stateMachine_.get(); }

    /// <summary>
    /// ビヘイビアツリーを取得
    /// </summary>
    /// <returns>ビヘイビアツリーのポインタ</returns>
    Tako::BehaviorTree* GetBehaviorTree() const { return behaviorTree_.get(); }

    /// <summary>
    /// 近接攻撃ヒット時の統合処理
    /// 現在のステートに応じてダメージ・スタン・離脱を判定
    /// </summary>
    /// <param name="damage">攻撃ダメージ</param>
    /// <param name="knockbackDir">ノックバック方向（正規化済み）</param>
    /// <param name="isKnockbackCombo">4コンボ目のノックバック付き攻撃かどうか</param>
    void OnMeleeAttackHit(float damage, const Tako::Vector3& knockbackDir, bool isKnockbackCombo);

    /// <summary>
    /// 行動状態をリセット（BT 中断時のクリーンアップ）
    /// </summary>
    void ResetActionState();

    /// <summary>
    /// スタン状態かどうか（ステートマシン経由）
    /// </summary>
    /// <returns>スタン中なら true</returns>
    bool IsStunned() const;

    /// <summary>
    /// フェーズ移行スタン中かどうか（ステートマシン経由）
    /// </summary>
    /// <returns>フェーズ移行スタン中なら true</returns>
    bool IsInPhaseTransitionStun() const;

    /// <summary>
    /// スタン用フラッシュを開始
    /// </summary>
    /// <param name="color">フラッシュ色</param>
    /// <param name="duration">持続時間</param>
    void StartStunFlash(const Tako::Vector4& color, float duration);

    /// <summary>
    /// 攻撃可能サインエミッターの有効/無効を設定
    /// </summary>
    /// <param name="active">有効にする場合 true</param>
    void SetCanAttackSignEmitterActive(bool active);

    /// <summary>
    /// 攻撃可能サインエミッターの位置を設定
    /// </summary>
    /// <param name="position">設定する位置</param>
    void SetCanAttackSignEmitterPosition(const Tako::Vector3& position);

    /// <summary>
    /// 保留中のスタン方向を取得（BossStunnedState::Enter 用）
    /// </summary>
    const Tako::Vector3& GetPendingStunDirection() const { return pendingStunDirection_; }

    /// <summary>
    /// 保留中のスタンノックバック有効フラグを取得
    /// </summary>
    bool GetPendingStunWithKnockback() const { return pendingStunWithKnockback_; }

    //-----------------------------硬直状態システム------------------------------//
    /// <summary>
    /// 硬直フェーズ開始（アクションノードから呼ばれる）
    /// </summary>
    void EnterRecovery();

    /// <summary>
    /// 硬直フェーズ終了（アクションノードから呼ばれる）
    /// </summary>
    void ExitRecovery();

    /// <summary>
    /// 硬直中かどうか
    /// </summary>
    /// <returns>硬直中なら true</returns>
    bool IsInRecovery() const { return isInRecovery_; }

    /// <summary>
    /// 強制脆弱化フラグを設定（攻撃中も常時スタン誘発を許可するオプション）。
    /// AttackNode の isBypassRecoveryGuard が ON のときに攻撃ノードが立てる。
    /// Boss::OnHit 時の分岐で isInRecovery_ と OR で評価される。
    /// </summary>
    void SetForceVulnerable(bool v) { forceVulnerable_ = v; }

    /// <summary>強制脆弱化中かどうか</summary>
    bool IsForceVulnerable() const { return forceVulnerable_; }

    //-----------------------------ダッシュ状態システム------------------------------//
    /// <summary>
    /// ダッシュ状態を設定
    /// </summary>
    /// <param name="dashing">ダッシュ中なら true</param>
    void SetDashing(bool dashing);

    /// <summary>
    /// ダッシュ中かどうか
    /// </summary>
    /// <returns>ダッシュ中なら true</returns>
    bool IsDashing() const { return isDashing_; }

    //-----------------------------離脱（互換性スタブ）------------------------------//
    /// <summary>
    /// 座標変換情報を取得
    /// </summary>
    /// <returns>現在の座標変換情報の参照</returns>
    const Tako::Transform& GetTransform() const { return transform_; }

    /// <summary>
    /// 座標変換情報を取得（非 const 版）
    /// </summary>
    /// <returns>現在の座標変換情報の参照</returns>
    Tako::Transform& GetWorldTransform() { return transform_; }

    /// <summary>
    /// 座標変換情報のポインタを取得
    /// </summary>
    /// <returns>座標変換情報への非 const ポインタ</returns>
    Tako::Transform* GetTransformPtr() { return &transform_; }

    /// <summary>
    /// 平行移動情報を取得
    /// </summary>
    /// <returns>現在の位置情報の参照</returns>
    Tako::Vector3 GetTranslate() const { return transform_.translate; }

    /// <summary>
    /// 回転情報を取得
    /// </summary>
    /// <returns>現在の回転情報の参照（ラジアン）</returns>
    Tako::Vector3 GetRotate() const { return transform_.rotate; }

    /// <summary>
    /// スケール情報を取得
    /// </summary>
    /// <returns>現在のスケール情報の参照</returns>
    Tako::Vector3 GetScale() const { return transform_.scale; }

    /// <summary>
    /// HP を取得
    /// </summary>
    /// <returns>現在の HP 値</returns>
    float GetHp() const { return hp_; }

    /// <summary>
    /// 最大 HP を取得
    /// </summary>
    /// <returns>最大 HP 値</returns>
    static constexpr float GetMaxHp() { return kMaxHp; }

    /// <summary>
    /// 現在のフェーズを取得
    /// </summary>
    /// <returns>フェーズ番号</returns>
    uint8_t GetPhase() const { return phaseManager_.GetPhase(); }

    /// <summary>
    /// 死亡状態を取得
    /// </summary>
    /// <returns> 死亡しているかどうかの真偽値 </returns>
    bool IsDead() const { return phaseManager_.IsDead(); }

    /// <summary>
    /// フェーズマネージャーを取得
    /// </summary>
    /// <returns>フェーズマネージャーの参照</returns>
    BossPhaseManager& GetPhaseManager() { return phaseManager_; }

    /// <summary>
    /// コライダーを取得
    /// </summary>
    /// <returns>ボスの OBB コライダーのポインタ</returns>
    Tako::OBBCollider* GetCollider() const { return bodyCollider_.get(); }

    //-----------------------------近接攻撃関連------------------------------//
    /// <summary>
    /// 攻撃ブロックを取得
    /// </summary>
    /// <returns>攻撃ブロックの Object3d ポインタ</returns>
    Tako::Object3d* GetMeleeAttackBlock() const { return meleeAttackBlock_.get(); }

    /// <summary>
    /// 攻撃ブロックの表示/非表示を設定
    /// </summary>
    /// <param name="visible">表示する場合 true</param>
    void SetMeleeAttackBlockVisible(bool visible) { meleeAttackBlockVisible_ = visible; }

    /// <summary>
    /// 攻撃ブロックが表示中か取得
    /// </summary>
    /// <returns>表示中の場合 true</returns>
    bool IsMeleeAttackBlockVisible() const { return meleeAttackBlockVisible_; }

    /// <summary>
    /// 反発衝撃波スフィアの表示/非表示を設定（BTBossRepelShockwave から呼ばれる）
    /// </summary>
    /// <param name="visible">表示する場合 true</param>
    void SetRepelShockwaveSphereVisible(bool visible) { repelShockwaveSphereVisible_ = visible; }

    /// <summary>
    /// 反発衝撃波スフィアのスケール（実効半径）を設定（BTBossRepelShockwave から呼ばれる）
    /// </summary>
    /// <param name="radius">スフィアの半径（ForceField の radius と同期）</param>
    void SetRepelShockwaveSphereScale(float radius);

    /// <summary>
    /// 近接攻撃コライダーを取得
    /// </summary>
    /// <returns>近接攻撃コライダーのポインタ</returns>
    BossMeleeAttackCollider* GetMeleeAttackCollider() const { return meleeAttackCollider_.get(); }

    /// <summary>
    /// 予兆エフェクトをアクティブ化/非アクティブ化
    /// </summary>
    /// <param name="active">アクティブにする場合 true</param>
    void SetAttackSignEmitterActive(bool active);

    /// <summary>
    /// 予兆エフェクトの位置を設定
    /// </summary>
    /// <param name="position">設定する位置</param>
    void SetAttackSignEmitterPosition(const Tako::Vector3& position);

    //-----------------------------射撃予兆エフェクト------------------------------//
    /// <summary>
    /// 射撃予兆エフェクトをアクティブ化/非アクティブ化
    /// </summary>
    /// <param name="active">アクティブにする場合 true</param>
    void SetBulletSignEmitterActive(bool active);

    /// <summary>
    /// 射撃予兆エフェクトの位置を設定
    /// </summary>
    /// <param name="position">設定する位置</param>
    void SetBulletSignEmitterPosition(const Tako::Vector3& position);

    /// <summary>
    /// 射撃予兆エフェクトのスケール範囲 X を設定
    /// </summary>
    /// <param name="value">スケール値（min=max=value）</param>
    void SetBulletSignEmitterScaleRangeX(float value);

    /// <summary>
    /// EmitterManager を設定
    /// </summary>
    /// <param name="emitterManager">EmitterManager のポインタ</param>
    void SetEmitterManager(Tako::EmitterManager* emitterManager) { emitterManager_ = emitterManager; }

    /// <summary>
    /// EmitterManager を取得
    /// </summary>
    /// <returns>EmitterManager のポインタ</returns>
    Tako::EmitterManager* GetEmitterManager() const { return emitterManager_; }

    /// <summary>
    /// ForceFieldManager を設定（BTBossRepelShockwave / BTBossVortexTempest が利用）
    /// </summary>
    void SetForceFieldManager(Tako::ForceFieldManager* manager) { forceFieldManager_ = manager; }

    /// <summary>
    /// ForceFieldManager を取得
    /// </summary>
    Tako::ForceFieldManager* GetForceFieldManager() const { return forceFieldManager_; }

    /// <summary>
    /// ボスのモデルをスポーン形状とする MeshEmitter "boss_aura" を初期化
    /// </summary>
    /// <remarks>
    /// 事前条件: model_ 生成済み、emitterManager_ 注入済み。
    /// preset JSON が存在すれば LoadPreset で復元 (object3dKey は sentinel 上書きで model_ にバインド)、
    /// 存在しなければ programmatic 既定値で CreateMeshEmitter を呼ぶ。
    /// 初期状態は <c>!isInRecovery_ &amp;&amp; !IsStunned()</c>。以降は <c>Update()</c> で毎フレーム同期。
    /// </remarks>
    void InitializeAuraEmitter();

    /// <summary>
    /// boss_aura エミッターのアクティブ状態を切り替え
    /// </summary>
    /// <param name="active">true で発火、false で停止</param>
    void SetAuraEmitterActive(bool active);

    /// <summary>
    /// boss_aura エミッター名を取得 (preset 名と同一)
    /// </summary>
    const std::string& GetAuraEmitterName() const { return auraEmitterName_; }

    /// <summary>
    /// ボス本体モデルを取得 (BT ノードから transparent / material color 操作用)
    /// </summary>
    Tako::Object3d* GetModel() const { return model_.get(); }

    /// <summary>
    /// ボスモデルをスポーン形状とする MeshEmitter "boss_particle_body" を初期化
    /// </summary>
    /// <remarks>
    /// 事前条件: model_ 生成済み、emitterManager_ 注入済み。
    /// preset JSON が存在すれば LoadPreset で復元 (object3dKey は sentinel 上書きで model_ にバインド)、
    /// 存在しなければ programmatic 既定値で CreateMeshEmitter を呼ぶ。
    /// 初期状態は非アクティブ (BT ノードが必要時に有効化)。
    /// </remarks>
    void InitializeBodyParticleEmitter();

    /// <summary>
    /// boss_particle_body エミッターのアクティブ状態を切り替え
    /// </summary>
    /// <param name="active">true で発火、false で停止</param>
    void SetBodyParticleEmitterActive(bool active);

    /// <summary>
    /// boss_particle_body エミッター名を取得 (preset 名と同一)
    /// </summary>
    const std::string& GetBodyParticleEmitterName() const { return bodyParticleEmitterName_; }

private:
    /// <summary>
    /// モデルとトランスフォームの初期化
    /// </summary>
    void InitializeModel();

    /// <summary>
    /// HP バーとフェーズマネージャーの初期化
    /// </summary>
    void InitializeHealth();

    /// <summary>
    /// コライダーの初期化
    /// </summary>
    void InitializeColliders();

    /// <summary>
    /// エフェクトパラメータの初期化
    /// </summary>
    void InitializeEffects();

    /// <summary>
    /// AI システム（ビヘイビアツリー）の初期化
    /// </summary>
    void InitializeAI();

    /// <summary>
    /// ステートマシンの初期化
    /// </summary>
    void InitializeStateMachine();

    /// <summary>
    /// フェーズ移行スタンを発動（HP 閾値以下で呼ばれる）
    /// </summary>
    void TriggerPhaseTransitionStun();

    /// <summary>
    /// フェーズ移行を完了（PhaseTransitionStun 中に近接攻撃を受けた時）
    /// </summary>
    void CompletePhaseTransition();

private:
    // ボスの3D モデルオブジェクト（描画とアニメーション管理）
    std::unique_ptr<Tako::Object3d> model_;

    // ボスの座標変換情報（位置、回転、スケール）
    Tako::Transform transform_{};

    // ステートマシン（外部イベント駆動の状態管理）
    std::unique_ptr<BossStateMachine> stateMachine_;

    // ビヘイビアツリー (AI 意思決定)
    std::unique_ptr<Tako::BehaviorTree> behaviorTree_;

#ifdef _DEBUG
    // ビヘイビアツリーノードエディタ
    std::unique_ptr<Tako::BehaviorTreeEditor> nodeEditor_;

    // ノードエディタの表示フラグ
    bool showNodeEditor_ = false;
#endif

    // プレイヤーへの参照
    Player* player_ = nullptr;

    // ボスの現在 HP（0になると撃破、初期値200）
    float hp_ = kMaxHp;

    // フェーズ・ライフ管理
    BossPhaseManager phaseManager_;

    // 一時行動停止フラグ
    bool isPause_ = false;

    // ===== ステートマシン遷移用データ =====
    Tako::Vector3 pendingStunDirection_;          ///< スタン遷移時のノックバック方向
    bool pendingStunWithKnockback_ = true;       ///< スタン遷移時のノックバック有効フラグ

    // ===== フェーズ移行スタン =====
    bool hasTriggeredPhaseTransitionStun_ = false;  ///< 一度きりのトリガーフラグ
    std::string canAttackSignEmitterName_ = "can_attack_sign";  ///< 攻撃可能サインエミッター名

    // ===== BT 内サブ状態フラグ（ステートマシンの責務外） =====
    bool isInRecovery_ = false;                   ///< 硬直中フラグ（BT アクションが設定）
    bool isDashing_ = false;                      ///< ダッシュ中フラグ（BT アクションが設定）
    bool forceVulnerable_ = false;                ///< 強制脆弱化フラグ（AttackNode::isBypassRecoveryGuard ON 時に立つ）

    // ボス本体の衝突判定用 AABB コライダー
    std::unique_ptr<Tako::OBBCollider> bodyCollider_;

    //-----------------------------近接攻撃関連------------------------------//
    // 近接攻撃用武器ブロック
    std::unique_ptr<Tako::Object3d> meleeAttackBlock_;

    // 攻撃ブロック表示フラグ
    bool meleeAttackBlockVisible_ = false;

    // 反発衝撃波エフェクト用スフィア（BTBossRepelShockwave アクション中に半透明赤で表示）
    std::unique_ptr<Tako::Object3d> repelShockwaveSphere_;
    bool repelShockwaveSphereVisible_ = false;

    // 近接攻撃用コライダー
    std::unique_ptr<BossMeleeAttackCollider> meleeAttackCollider_;

    // 予兆エフェクト管理
    Tako::EmitterManager* emitterManager_ = nullptr;

    // ForceField 管理（非所有 / null 許容） — Phase1/Phase2 新攻撃で力場を生成・追従させる
    Tako::ForceFieldManager* forceFieldManager_ = nullptr;

    // 予兆エフェクト名
    std::string attackSignEmitterName_ = "boss_melee_attack_sign";
    std::string bulletSignEmitterName_ = "boss_bullet_sign";

    // ボスモデルをスポーン形状とする MeshEmitter (ランタイム名 = preset 名)
    std::string auraEmitterName_ = "boss_aura";

    // テレポート演出用 MeshEmitter (BTBossTeleport が ON/OFF 制御)
    std::string bodyParticleEmitterName_ = "boss_particle_body";

    // ===== エフェクト関連 =====
    HitFlashEffect hitFlashEffect_;     ///< ヒット時の色変化エフェクト
    ShakeEffect shakeEffect_;           ///< シェイクエフェクト

    // 弾生成管理
    BulletSpawner bulletSpawner_;              ///< 通常弾生成リクエスト管理
    BulletSpawner penetratingBulletSpawner_;   ///< 貫通弾生成リクエスト管理

    // HP バー UI
    HPBarUI hpBar_;                      ///< HP バー表示

    // 初期座標
    float initialY_ = 2.5f;   ///< 初期 Y 座標
    float initialZ_ = 10.0f;  ///< 初期 Z 座標
};

