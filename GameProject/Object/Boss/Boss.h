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
#include "BehaviorTree.h"

#ifdef _DEBUG
#include "BehaviorTreeEditor.h"
#endif

// Tako namespace 前方宣言
namespace Tako {
class OBBCollider;
class Object3d;
class EmitterManager;
class ForceFieldManager;
}

// GameProject 前方宣言
class BossStateMachine;
class BossStunnedState;
class Player;
class BossMeleeAttackCollider;

/// <summary>
/// ボスキャラクター。HP・フェーズ・状態遷移・BehaviorTree・各種エフェクト/当たり判定を統括する
/// </summary>
class Boss
{
public: //定数
    static constexpr float kRingShockwaveWidthFraction = 0.15f;  ///< リング衝撃波メッシュの帯幅比（外半径 1 基準）

private: //定数
    static constexpr float kMaxHp = 200.0f;
    static constexpr float kPhase2Threshold = 105.0f;
    static constexpr float kPhase2InitialHp = 100.0f;
    static constexpr float kPhaseTransitionStunThreshold = 105.0f;

public: //メンバー関数
    Boss();
    ~Boss();

    void Initialize();
    void Finalize();
    void Update(float deltaTime);

    void Draw();
    void DrawShadow();
    void DrawSprite();
    void DrawImGui();

    /// <summary>
    /// ダメージ処理。HP を減算し、フェーズ移行スタン判定とヒットフラッシュ・シェイクを行う
    /// </summary>
    /// <param name="damage">減算するダメージ量</param>
    /// <param name="shakeIntensityOverride">シェイク強度。0 以下なら既定値を使用</param>
    void OnHit(float damage, float shakeIntensityOverride = 0.0f);

    /// <summary>
    /// シェイク開始
    /// </summary>
    /// <param name="intensity">シェイク強度。0 以下なら既定値を使用</param>
    void StartShake(float intensity = 0.0f);

    /// <summary>
    /// 通常弾の生成リクエストをスポナーに登録する（生成は後で Consume 時に行う）
    /// </summary>
    /// <param name="position">弾の初期ワールド座標</param>
    /// <param name="velocity">弾の速度ベクトル</param>
    void RequestBulletSpawn(const Tako::Vector3& position, const Tako::Vector3& velocity);

    /// <summary>
    /// 保留中の弾生成リクエストを move で取り出す（取り出し後はスポナー内が空になる）
    /// </summary>
    /// <returns>登録済みの通常弾リクエスト一覧</returns>
    std::vector<BulletSpawnRequest> ConsumePendingBullets();

    /// <summary>
    /// 貫通弾の生成リクエストをスポナーに登録する（生成は後で Consume 時に行う）
    /// </summary>
    /// <param name="position">弾の初期ワールド座標</param>
    /// <param name="velocity">弾の速度ベクトル</param>
    void RequestPenetratingBulletSpawn(const Tako::Vector3& position, const Tako::Vector3& velocity);

    /// <summary>
    /// 保留中の貫通弾生成リクエストを move で取り出す（取り出し後はスポナー内が空になる）
    /// </summary>
    /// <returns>登録済みの貫通弾リクエスト一覧</returns>
    std::vector<BulletSpawnRequest> ConsumePendingPenetratingBullets();

    /// <summary>
    /// 近接攻撃ヒット時の統合処理。現在のステートに応じてダメージ・スタン・離脱を判定する。
    /// ダッシュ中は無効。スタン中/フェーズ移行スタン中/硬直中または強制脆弱時のみダメージが入る
    /// </summary>
    /// <param name="damage">減算するダメージ量</param>
    /// <param name="knockbackDir">ノックバック方向（呼び出し側で正規化済みを想定）</param>
    /// <param name="isKnockbackCombo">true ならスタン時にノックバックを付与する</param>
    void OnMeleeAttackHit(float damage, const Tako::Vector3& knockbackDir, bool isKnockbackCombo);

    /// <summary>
    /// 行動状態をリセット（BT 中断時のクリーンアップ）
    /// </summary>
    void ResetActionState();

    /// <summary>
    /// ヒットフラッシュを開始してスタン中の色変化として使う
    /// </summary>
    /// <param name="color">フラッシュ色（RGBA）</param>
    /// <param name="duration">フラッシュ継続時間（秒）</param>
    void StartStunFlash(const Tako::Vector4& color, float duration);

    void EnterRecovery();
    void ExitRecovery();

    /// <summary>
    /// 反発衝撃波スフィアの半径を設定（ForceField の radius と同期）
    /// </summary>
    /// <param name="radius">スフィア半径。XYZ スケールに同値で適用</param>
    void SetRepelShockwaveSphereScale(float radius);

    /// <summary>
    /// 射撃予兆エフェクトのスケール範囲 X を設定（min=max=value）
    /// </summary>
    /// <param name="value">X スケールの最小・最大に同値で設定する値（Y は 1.0 固定）</param>
    void SetBulletSignEmitterScaleRangeX(float value);

    /// <summary>
    /// ボスモデルをスポーン形状とする MeshEmitter "boss_aura" を初期化
    /// </summary>
    void InitializeAuraEmitter();

    /// <summary>
    /// ボスモデルをスポーン形状とする MeshEmitter "boss_particle_body" を初期化
    /// </summary>
    void InitializeBodyParticleEmitter();

    //=================================================================
    //Setter
    //=================================================================
    void SetTransform(const Tako::Transform& transform) { transform_ = transform; }
    void SetTranslate(const Tako::Vector3& translate) { transform_.translate = translate; }
    void SetRotate(const Tako::Vector3& rotate) { transform_.rotate = rotate; }
    void SetScale(const Tako::Vector3& scale) { transform_.scale = scale; }
    void SetHp(float hp) { hp_ = hp; }
    void SetPhase(uint32_t phase) { phaseManager_.SetPhase(phase); }
    void SetPlayer(Player* player);
    void SetIsPause(bool isPause) { isPause_ = isPause; }
    void SetCanAttackSignEmitterActive(bool active);
    void SetCanAttackSignEmitterPosition(const Tako::Vector3& position);

    /// <summary>
    /// 強制脆弱化フラグを設定（true の間は硬直外でもスタン誘発を許可）
    /// </summary>
    void SetForceVulnerable(bool v) { forceVulnerable_ = v; }

    /// <summary>
    /// テレポート中フラグを設定（true の間は aura を無効化）
    /// </summary>
    void SetTeleporting(bool teleporting) { isTeleporting_ = teleporting; }

    void SetDashing(bool dashing);
    void SetMeleeAttackBlockVisible(bool visible) { meleeAttackBlockVisible_ = visible; }
    void SetRepelShockwaveSphereVisible(bool visible) { repelShockwaveSphereVisible_ = visible; }
    void SetRingShockwaveVisible(bool visible) { ringShockwaveVisible_ = visible; }
    void SetAttackSignEmitterActive(bool active);
    void SetAttackSignEmitterPosition(const Tako::Vector3& position);
    void SetBulletSignEmitterActive(bool active);
    void SetBulletSignEmitterPosition(const Tako::Vector3& position);
    void SetEmitterManager(Tako::EmitterManager* emitterManager) { emitterManager_ = emitterManager; }
    void SetForceFieldManager(Tako::ForceFieldManager* manager) { forceFieldManager_ = manager; }
    void SetAuraEmitterActive(bool active);
    void SetBodyParticleEmitterActive(bool active);

    //=================================================================
    //Getter
    //=================================================================
    BossStateMachine* GetStateMachine() const { return stateMachine_.get(); }
    Tako::BehaviorTree* GetBehaviorTree() const { return behaviorTree_.get(); }

    /// <summary>
    /// スタンまたはフェーズ移行スタン中か
    /// </summary>
    /// <returns>現在ステートが "Stunned" または "PhaseTransitionStun" なら true</returns>
    bool IsStunned() const;

    /// <summary>
    /// フェーズ移行スタン中か
    /// </summary>
    /// <returns>現在ステートが "PhaseTransitionStun" なら true</returns>
    bool IsInPhaseTransitionStun() const;

    const Tako::Vector3& GetPendingStunDirection() const { return pendingStunDirection_; }
    bool GetPendingStunWithKnockback() const { return pendingStunWithKnockback_; }
    bool IsInRecovery() const { return isInRecovery_; }
    bool IsForceVulnerable() const { return forceVulnerable_; }
    bool IsTeleporting() const { return isTeleporting_; }
    bool IsDashing() const { return isDashing_; }
    const Tako::Transform& GetTransform() const { return transform_; }
    Tako::Transform& GetWorldTransform() { return transform_; }
    Tako::Transform* GetTransformPtr() { return &transform_; }
    Tako::Vector3 GetTranslate() const { return transform_.translate; }
    Tako::Vector3 GetRotate() const { return transform_.rotate; }
    Tako::Vector3 GetScale() const { return transform_.scale; }
    float GetHp() const { return hp_; }
    static constexpr float GetMaxHp() { return kMaxHp; }
    uint8_t GetPhase() const { return phaseManager_.GetPhase(); }
    bool IsDead() const { return phaseManager_.IsDead(); }
    BossPhaseManager& GetPhaseManager() { return phaseManager_; }
    Tako::OBBCollider* GetCollider() const { return bodyCollider_.get(); }
    Tako::Object3d* GetMeleeAttackBlock() const { return meleeAttackBlock_.get(); }
    bool IsMeleeAttackBlockVisible() const { return meleeAttackBlockVisible_; }
    Tako::Object3d* GetRingShockwaveModel() const { return ringShockwaveModel_.get(); }
    BossMeleeAttackCollider* GetMeleeAttackCollider() const { return meleeAttackCollider_.get(); }
    Tako::EmitterManager* GetEmitterManager() const { return emitterManager_; }
    Tako::ForceFieldManager* GetForceFieldManager() const { return forceFieldManager_; }
    const std::string& GetAuraEmitterName() const { return auraEmitterName_; }

    /// <summary>
    /// ボス本体モデルを取得する（MeshEmitter のスポーン形状などに使う）
    /// </summary>
    /// <returns>ボス本体の Object3d。未初期化なら nullptr</returns>
    Tako::Object3d* GetModel() const { return model_.get(); }

    /// <summary>
    /// ボス本体の素のマテリアル色（被弾フラッシュ・テレポート復帰の基準色）
    /// </summary>
    /// <returns>ボディの基準色 RGBA</returns>
    const Tako::Vector4& GetBaseColor() const { return baseColor_; }

    const std::string& GetBodyParticleEmitterName() const { return bodyParticleEmitterName_; }

private: //非公開関数
    void InitializeModel();
    void InitializeHealth();
    void InitializeColliders();
    void InitializeEffects();
    void InitializeAI();
    void InitializeStateMachine();

    /// <summary>
    /// フェーズ移行スタンを発動（HP 閾値以下で呼ばれる）
    /// </summary>
    void TriggerPhaseTransitionStun();

    /// <summary>
    /// フェーズ移行を完了（移行スタン中に近接攻撃を受けた時）
    /// </summary>
    void CompletePhaseTransition();

private: //メンバー変数
    std::unique_ptr<Tako::Object3d> model_;
    Tako::Transform                 transform_{};
    Tako::Vector4                   baseColor_{ 1.0f, 0.0f, 0.0f, 1.0f };  ///< ボディの素の色（フラッシュ/テレポート復帰の基準）

    std::unique_ptr<BossStateMachine>   stateMachine_;
    std::unique_ptr<Tako::BehaviorTree> behaviorTree_;

#ifdef _DEBUG
    std::unique_ptr<Tako::BehaviorTreeEditor> nodeEditor_;
    bool                                      showNodeEditor_ = false;
#endif

    Player*          player_       = nullptr;
    float            hp_           = kMaxHp;
    BossPhaseManager phaseManager_;
    bool             isPause_      = false;

    //ステートマシン遷移用データ
    Tako::Vector3 pendingStunDirection_;
    bool          pendingStunWithKnockback_ = true;

    //フェーズ移行スタン
    bool        hasTriggeredPhaseTransitionStun_ = false;              ///< 一度きりのトリガーフラグ
    std::string canAttackSignEmitterName_        = "can_attack_sign";

    //BT 内サブ状態フラグ（ステートマシンの責務外）
    bool isInRecovery_    = false;
    bool isDashing_       = false;
    bool forceVulnerable_ = false;  ///< true の間は硬直外でもスタン誘発を許可
    bool isTeleporting_   = false;  ///< true の間は aura を無効化

    std::unique_ptr<Tako::OBBCollider> bodyCollider_;

    //近接攻撃関連
    std::unique_ptr<Tako::Object3d> meleeAttackBlock_;
    bool                            meleeAttackBlockVisible_ = false;

    std::unique_ptr<Tako::Object3d> repelShockwaveSphere_;
    bool                            repelShockwaveSphereVisible_ = false;

    //リング衝撃波（BTBossRingShockwave が transform を駆動）
    std::unique_ptr<Tako::Object3d> ringShockwaveModel_;
    bool                            ringShockwaveVisible_ = false;

    std::unique_ptr<BossMeleeAttackCollider> meleeAttackCollider_;

    Tako::EmitterManager* emitterManager_ = nullptr;

    //非所有 / null 許容
    Tako::ForceFieldManager* forceFieldManager_ = nullptr;

    std::string attackSignEmitterName_ = "boss_melee_attack_sign";
    std::string bulletSignEmitterName_ = "boss_bullet_sign";

    std::string auraEmitterName_     = "boss_aura";
    std::string darkAuraEmitterName_ = "boss_dark_aura";

    std::string bodyParticleEmitterName_ = "boss_particle_body";

    //エフェクト関連
    HitFlashEffect hitFlashEffect_;
    ShakeEffect    shakeEffect_;

    BulletSpawner bulletSpawner_;
    BulletSpawner penetratingBulletSpawner_;  ///< 貫通弾用

    HPBarUI hpBar_;

    float initialY_ = 2.5f;
    float initialZ_ = 10.0f;
};

