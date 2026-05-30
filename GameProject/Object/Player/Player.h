#pragma once
#include <memory>
#include <vector>

#include "Transform.h"
#include "Vector3.h"

#include "../../Common/CooldownTimer.h"
#include "../../Common/BulletSpawnRequest.h"
#include "../../Common/BulletSpawner.h"
#include "../../Common/EasingMover.h"
#include "../../Common/DynamicBoundary.h"
#include "../../UI/HPBarUI.h"
#include "../../Effect/HitFlashEffect.h"
#include "../../Effect/ShakeEffect.h"

// Tako namespace 前方宣言
namespace Tako {
class OBBCollider;
class Object3d;
class Camera;
class EmitterManager;
class ForceFieldManager;
}

// GameProject 前方宣言
class PlayerStateMachine;
class InputHandler;
class MeleeAttackCollider;
class Boss;

/// <summary>
/// プレイヤーキャラクタークラス
/// 移動、攻撃、ステート管理などプレイヤーの動作を制御
/// </summary>
class Player
{
	//=========================================================================================
	// 定数
	//=========================================================================================
private:
	static constexpr float kVelocityEpsilon = 0.01f;   ///< 速度判定の閾値
	static constexpr float kBoundaryDisabled = 9999.0f; ///< 境界無効化マーカー
	static constexpr float kExternalVelocityDamping = 0.9f; ///< 外力速度の減衰係数（毎フレーム乗算）
    // イージング用定数
    static constexpr float kMoveArrivalThreshold = 0.5f;
    static constexpr float kMoveEasingCoeffA = 3.0f;
    static constexpr float kMoveEasingCoeffB = 2.0f;
    static constexpr float kDirectionEpsilon = 0.01f;

public: // メンバ関数
    Player();
    ~Player();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// スプライト描画
    /// </summary>
    void DrawSprite();

    /// <summary>
    /// 移動
    /// </summary>
    /// <param name="speedMultiplier">速度倍率（デフォルト1.0）</param>
    /// <param name="isApplyDirCalulate"> 方向計算を適用するか（デフォルト true）</param>
    void Move(float speedMultiplier = 1.0f, bool isApplyDirCalulate = true);

    /// <summary>
    /// ターゲットへ移動（イージング移動）
    /// </summary>
    /// <param name="target">移動先のボスエネミー</param>
    /// <param name="deltaTime">前フレームからの経過時間</param>
    void MoveToTarget(Boss* target, float deltaTime);

    /// <summary>
    /// MoveToTarget の状態をリセット
    /// </summary>
    void ResetMoveToTarget();

    /// <summary>
    /// 目標位置に到達したか判定
    /// </summary>
    /// <returns>true: 到達, false: 未到達</returns>
    bool HasReachedTarget() const;

    /// <summary>
    /// ImGui の描画
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// コライダーの初期設定
    /// </summary>
    void SetupColliders();

    /// <summary>
    /// 攻撃コライダーの更新
    /// </summary>
    void UpdateCollider();

    /// <summary>
    /// ボス方向を向く
    /// </summary>
    void LookAtBoss();

    /// <summary>
    /// 近接攻撃ヒット時の処理
    /// </summary>
    /// <param name="other">衝突相手のコライダー</param>
    void OnHit(float damage);

    /// <summary>
    /// 動的移動範囲をクリア（無効化）
    /// </summary>
    void ClearDynamicBounds();

    /// <summary>
    /// パリィ成功時の処理（HP 回復、エフェクト発生）
    /// </summary>
    void OnParrySuccess();

    //-----------------------------弾生成リクエストシステム------------------------------//
    /// <summary>
    /// 弾生成リクエストを追加
    /// </summary>
    /// <param name="position">発射位置</param>
    /// <param name="velocity">弾の速度ベクトル</param>
    void RequestBulletSpawn(const Tako::Vector3& position, const Tako::Vector3& velocity);

    /// <summary>
    /// 保留中の弾生成リクエストを取得して消費
    /// </summary>
    /// <returns>弾生成リクエストのリスト</returns>
    std::vector<BulletSpawnRequest> ConsumePendingBullets();

    //----------------------------------Setters-----------------------------------//
    /// <summary>
    /// 移動速度を設定
    /// </summary>
    /// <param name="speed">新しい移動速度</param>
    void SetSpeed(float speed) { speed_ = speed; }

    /// <summary>
    /// Boss をターゲットに設定
    /// </summary>
    void SetBoss(Boss* target) { targetEnemy_ = target; }

    /// <summary>
    /// 射撃の有効/無効を設定（Boss フェーズ変更時に呼ばれる）
    /// </summary>
    void SetShootingEnabled(bool enabled) { shootingEnabled_ = enabled; }

    /// <summary>
    /// 射撃が有効かどうかを取得
    /// </summary>
    bool IsShootingEnabled() const { return shootingEnabled_; }

    /// <summary>
    /// カメラを設定
    /// </summary>
    /// <param name="camera">使用するカメラのポインタ</param>
    void SetCamera(Tako::Camera* camera) { camera_ = camera; }

    /// <summary>
    /// カメラモードを設定
    /// </summary>
    /// <param name="mode">true: 一人称視点, false: トップダウン視点</param>
    void SetMode(bool mode) { mode_ = mode; }

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
    /// <param name="hp">新しい HP 値（0未満は0に補正）</param>
    void SetHp(float hp) { hp_ = hp; if (hp_ < 0.f) hp_ = 0.f; }

    /// <summary>
    /// 無敵フラグを設定
    /// </summary>
    /// <param name="isInvincible">新しい無敵フラグの値</param>
    void SetInvincible(bool isInvincible) { isInvincible_ = isInvincible; }

    /// <summary>
    /// 入力ハンドラを設定
    /// </summary>
    void SetInputHandler(InputHandler* inputHandler) { inputHandlerPtr_ = inputHandler; }

    /// <summary>
    /// 攻撃ブロックの表示/非表示を設定
    /// </summary>
    /// <param name="visible">true: 表示, false: 非表示</param>
    void SetAttackBlockVisible(bool visible) { attackBlockVisible_ = visible; }

    /// <summary>
    /// 動的移動範囲を設定
    /// </summary>
    /// <param name="xMin">X 座標の最小値</param>
    /// <param name="xMax">X 座標の最大値</param>
    /// <param name="zMin">Z 座標の最小値</param>
    /// <param name="zMax">Z 座標の最大値</param>
    void SetDynamicBounds(float xMin, float xMax, float zMin, float zMax);

    /// <summary>
    /// 中心点と範囲から動的移動範囲を設定
    /// </summary>
    /// <param name="center">中心座標</param>
    /// <param name="xRange">X 方向の範囲（片側）</param>
    /// <param name="zRange">Z 方向の範囲（片側）</param>
    void SetDynamicBoundsFromCenter(const Tako::Vector3& center, float xRange, float zRange);

    /// <summary>
    /// EmitterManager を設定
    /// </summary>
    /// <param name="emitterManager">エミッターマネージャーのポインタ</param>
    void SetEmitterManager(Tako::EmitterManager* emitterManager) { emitterManager_ = emitterManager; }

    /// <summary>
    /// ポーズ状態を設定
    /// </summary>
    /// <param name="isPause">ポーズ中なら true</param>
    void SetIsPause(bool isPause) { isPause_ = isPause; }

    //----------------------------------Getters-----------------------------------//
    /// <summary>
    /// 移動速度を取得
    /// </summary>
    /// <returns>現在の移動速度</returns>
    float GetSpeed() const { return speed_; }

    /// <summary>
    /// カメラを取得
    /// </summary>
    /// <returns>現在のカメラのポインタ</returns>
    Tako::Camera* GetCamera() const { return camera_; }

    /// <summary>
    /// カメラモードを取得
    /// </summary>
    /// <returns>true: 一人称視点, false: トップダウン視点</returns>
    bool GetMode() const { return mode_; }

    /// <summary>
    /// HP を取得
    /// </summary>
    /// <returns>現在の HP 値</returns>
    float GetHp() const { return hp_; }

    /// <summary>
    /// 死亡フラグを取得
    /// </summary>
    /// <returns>true: 死亡, false: 生存</returns>
    bool IsDead() const { return isDead_; }

    /// <summary>
    /// 無敵フラグを取得
    /// </summary>
    /// <returns>true: 無敵, false: 通常状態</returns>
    bool IsInvincible() const{ return isInvincible_; }

    /// <summary>
    /// Shoot できるか
    /// </summary>
    /// <returns>true: 可能, false: 不可能</returns>
    bool CanShoot() const;

    /// <summary>
    /// 座標変換情報を取得
    /// </summary>
    /// <returns>現在の座標変換情報の参照</returns>
    const Tako::Transform& GetTransform() const { return transform_; }

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
    /// 3D モデルを取得
    /// </summary>
    /// <returns>プレイヤーモデルのポインタ</returns>
    Tako::Object3d* GetModel() const { return model_.get(); }

    /// <summary>
    /// ステートマシンを取得
    /// </summary>
    /// <returns>プレイヤーステートマシンのポインタ</returns>
    PlayerStateMachine* GetStateMachine() const { return stateMachine_.get(); }

    /// <summary>
    /// 近接攻撃コライダーを取得
    /// </summary>
    /// <returns>近接攻撃コライダーのポインタ</returns>
    MeleeAttackCollider* GetMeleeAttackCollider() const { return meleeAttackCollider_.get(); }

    /// <summary>
    /// 攻撃ブロックを取得
    /// </summary>
    /// <returns>攻撃ブロックの Object3d ポインタ</returns>
    Tako::Object3d* GetAttackBlock() const { return attackBlock_.get(); }

    /// <summary>
    /// 攻撃ブロックが表示中か取得
    /// </summary>
    /// <returns>true: 表示中, false: 非表示</returns>
    bool IsAttackBlockVisible() const { return attackBlockVisible_; }

    /// <summary>
    /// Velocity を取得
    /// </summary>
    /// <returns>現在の Velocity 値の参照</returns>
    Tako::Vector3& GetVelocity() { return velocity_; }

    /// <summary>
    /// InputHandler を取得
    /// </summary>
    /// <returns>InputHandler のポインタ</returns>
    InputHandler* GetInputHandler() { return inputHandlerPtr_; };

    /// <summary>
    /// 攻撃最小距離を取得
    /// </summary>
    /// <returns>攻撃最小距離</returns>
    float GetAttackMinDistance() const { return attackMinDist_; }

    /// <summary>
    /// EmitterManager を取得
    /// </summary>
    /// <returns>エミッターマネージャーのポインタ</returns>
    Tako::EmitterManager* GetEmitterManager() const { return emitterManager_; }

    /// <summary>
    /// ForceFieldManager を設定（BTBossRepelShockwave などの反発衝撃波を受ける）
    /// </summary>
    /// <param name="manager">ForceFieldManager のポインタ（非所有）</param>
    void SetForceFieldManager(Tako::ForceFieldManager* manager) { forceFieldManager_ = manager; }

    /// <summary>
    /// パリィ状態かどうかを判定
    /// </summary>
    /// <returns>true: パリィ中, false: それ以外</returns>
    bool IsParrying() const;

    /// <summary>
    /// パリィ可能か判定（クールダウン中は false）
    /// </summary>
    /// <returns>パリィ可能なら true</returns>
    bool CanParry() const;

    /// <summary>
    /// パリィクールダウンを開始
    /// </summary>
    void StartParryCooldown();

    /// <summary>
    /// ダッシュ可能か判定（クールダウン中は false）
    /// </summary>
    /// <returns>ダッシュ可能なら true</returns>
    bool CanDash() const;

    /// <summary>
    /// ダッシュクールダウンを開始
    /// </summary>
    void StartDashCooldown();

    /// <summary>
    /// プレイヤーの前方位置を取得
    /// </summary>
    /// <param name="offset">前方へのオフセット距離</param>
    /// <returns>前方位置のワールド座標</returns>
    Tako::Vector3 GetFrontPosition(float offset) const;

private:
    /// <summary>
    /// GlobalVariables から値を同期
    /// </summary>
    void SyncGlobalVariables();

    /// <summary>
    /// 戦闘関連の更新（HP、クールダウン、死亡判定）
    /// </summary>
    /// <param name="deltaTime">フレーム時間</param>
    void UpdateCombat(float deltaTime);

    /// <summary>
    /// ステートマシンの更新
    /// </summary>
    /// <param name="deltaTime">フレーム時間</param>
    void UpdateStateMachine(float deltaTime);

    /// <summary>
    /// 座標変換の更新（位置制限、回転）
    /// </summary>
    void UpdateTransform();

    /// <summary>
    /// 物理更新（ForceField による外力を反映、減衰処理）
    /// </summary>
    /// <param name="deltaTime">フレーム時間</param>
    void UpdatePhysics(float deltaTime);

    /// <summary>
    /// 視覚エフェクトの更新
    /// </summary>
    /// <param name="deltaTime">フレーム時間</param>
    void UpdateVisuals(float deltaTime);

private: // メンバ変数

    // 動的移動制限（ボス近接戦闘エリア）
    DynamicBoundary dynamicBounds_;

    std::unique_ptr<Tako::Object3d> model_; ///< モデル
    Tako::Camera* camera_ = nullptr;        ///< カメラ
    Tako::Transform transform_{};           ///< 変形情報
    Tako::Vector3 velocity_{};              ///< 速度
    Tako::Vector3 externalVelocity_{};      ///< 外力速度 (m/s、ForceField による反発)
    Tako::ForceFieldManager* forceFieldManager_ = nullptr; ///< ForceFieldManager への参照（非所有）
    float speed_ = 0.5f;              ///< 移動速度
    float targetAngle_ = 0.f;         ///< 目標角度
    float hp_ = 100.f;                ///< 体力
    bool isDead_ = false;             ///< 死亡フラグ

    bool mode_ = false;               ///< true: ThirdPersonMode, false: TopDownMode
    bool isDisModelDebugInfo_ = false;///< モデルデバッグ情報の表示

    bool isInvincible_ = false;       ///< 無敵フラグ
    bool isPause_ = false;            ///< ポーズ中フラグ
    bool shootingEnabled_ = true;     ///< 射撃有効フラグ（false でフェーズ2射撃無効化）

    // システム
    std::unique_ptr<PlayerStateMachine> stateMachine_;
    InputHandler* inputHandlerPtr_;
    Tako::EmitterManager* emitterManager_ = nullptr;  ///< エミッターマネージャーへの参照

    // Colliders
    std::unique_ptr<Tako::OBBCollider> bodyCollider_;
    std::unique_ptr<MeleeAttackCollider> meleeAttackCollider_;

    // 攻撃ブロック
    std::unique_ptr<Tako::Object3d> attackBlock_;  ///< 攻撃時に表示される回転ブロック
    bool attackBlockVisible_ = false;         ///< 攻撃ブロック表示フラグ

    // 攻撃関連
    Boss* targetEnemy_ = nullptr;
    bool isAttackHit_ = false;
    float attackMoveSpeed_ = 2.0f;

    // MoveToTarget 用イージング移動システム
    EasingMover attackMover_;             ///< 攻撃時のイージング移動

    // クールダウン管理
    CooldownTimer parryCooldown_;         ///< パリィクールダウン
    CooldownTimer dashCooldown_;          ///< ダッシュクールダウン

    // 弾生成管理
    BulletSpawner bulletSpawner_;

    // 調整可能パラメータ（ImGui 編集用）
    float initialY_ = 2.5f;                   ///< 初期 Y 座標
    float initialZ_ = -120.0f;                ///< 初期 Z 座標
    float attackMinDist_ = 5.0f;             ///< 攻撃開始距離
    float attackMoveRotationLerp_ = 0.3f;     ///< 攻撃移動中の回転補間速度
    float bossLookatLerp_ = 1.15f;            ///< ボス視線追従補間速度

    // HP バー UI
    HPBarUI hpBar_;                      ///< HP バー表示

    // HP 最大値
    static constexpr float kMaxHp = 100.0f;  ///< HP 最大値

    // ===== エフェクト関連 =====
    HitFlashEffect hitFlashEffect_;     ///< ヒット時の色変化エフェクト
    ShakeEffect shakeEffect_;           ///< シェイクエフェクト
};

