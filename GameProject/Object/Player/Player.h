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

namespace Tako {
class OBBCollider;
class Object3d;
class Camera;
class EmitterManager;
class ForceFieldManager;
}

class PlayerStateMachine;
class InputHandler;
class MeleeAttackCollider;
class Boss;

/// <summary>
/// プレイヤーキャラクター。移動・戦闘・HP・状態遷移・当たり判定を統括する
/// </summary>
class Player
{
private: //定数
    static constexpr float kVelocityEpsilon = 0.01f;
    static constexpr float kBoundaryDisabled = 9999.0f;
    static constexpr float kExternalVelocityDamping = 0.9f; ///< 毎フレーム乗算
    static constexpr float kMoveArrivalThreshold = 0.5f;
    static constexpr float kMoveEasingCoeffA = 3.0f;
    static constexpr float kMoveEasingCoeffB = 2.0f;
    static constexpr float kDirectionEpsilon = 0.01f;
    static constexpr float kMaxHp = 185.0f;

public: //メンバー関数
    Player();
    ~Player();

    void Initialize();
    void Finalize();
    void Update();
    void Draw();
    void DrawSprite();

    /// <summary>
    /// 入力方向へ移動し、向きを補間更新する
    /// </summary>
    /// <param name="speedMultiplier">基準速度への倍率</param>
    /// <param name="isApplyDirCalulate">true で移動方向へ回転を補間。false なら向きを変えない（射撃中など）</param>
    void Move(float speedMultiplier = 1.0f, bool isApplyDirCalulate = true);

    /// <summary>
    /// targetPos の stopDistance 手前までイージング移動
    /// </summary>
    /// <param name="targetPos">移動先のワールド座標</param>
    /// <param name="deltaTime">経過時間（秒）</param>
    /// <param name="stopDistance">目標手前で停止する距離</param>
    void MoveToTarget(const Tako::Vector3& targetPos, float deltaTime, float stopDistance = 0.0f);

    void ResetMoveToTarget();
    void DrawImGui();
    void SetupColliders();
    void UpdateCollider();
    void LookAtBoss();

    /// <summary>
    /// 被弾処理。HP を減算し、ヒットフラッシュ・シェイク・カメラ演出を発生させる
    /// </summary>
    /// <param name="damage">減算するダメージ量。無敵中は無視される</param>
    void OnHit(float damage);

    void ClearDynamicBounds();

    /// <summary>
    /// パリィ成功時の処理（HP 回復、エフェクト発生）
    /// </summary>
    void OnParrySuccess();

    void StartParryCooldown();
    void StartDashCooldown();

    //======================================================================================
    //弾生成リクエスト
    //======================================================================================
    /// <summary>
    /// 弾の生成リクエストを内部キューに積む（実際の生成は外部が後で行う）
    /// </summary>
    /// <param name="position">発射位置（ワールド座標）</param>
    /// <param name="velocity">初速度ベクトル</param>
    void RequestBulletSpawn(const Tako::Vector3& position, const Tako::Vector3& velocity);

    /// <summary>
    /// 溜まった弾生成リクエストを取り出してキューを空にする
    /// </summary>
    /// <returns>未処理の生成リクエスト一覧（取得後キューは空になる）</returns>
    std::vector<BulletSpawnRequest> ConsumePendingBullets();

    //======================================================================================
    //Setter
    //======================================================================================
    void SetSpeed(float speed) { speed_ = speed; }
    void SetBoss(Boss* target) { targetEnemy_ = target; }
    void SetShootingEnabled(bool enabled) { shootingEnabled_ = enabled; }
    void SetCamera(Tako::Camera* camera) { camera_ = camera; }

    /// <summary>
    /// true: ThirdPerson, false: TopDown
    /// </summary>
    void SetMode(bool mode) { mode_ = mode; }

    void SetTransform(const Tako::Transform& transform) { transform_ = transform; }
    void SetTranslate(const Tako::Vector3& translate) { transform_.translate = translate; }
    void SetRotate(const Tako::Vector3& rotate) { transform_.rotate = rotate; }
    void SetScale(const Tako::Vector3& scale) { transform_.scale = scale; }
    void SetHp(float hp) { hp_ = hp; if (hp_ < 0.f) hp_ = 0.f; }
    void SetInvincible(bool isInvincible) { isInvincible_ = isInvincible; }
    void SetInputHandler(InputHandler* inputHandler) { inputHandlerPtr_ = inputHandler; }
    void SetAttackBlockVisible(bool visible) { attackBlockVisible_ = visible; }

    /// <summary>
    /// 動的移動範囲を最小・最大値で直接設定する
    /// </summary>
    /// <param name="xMin">X 軸の下限（ワールド座標）</param>
    /// <param name="xMax">X 軸の上限（ワールド座標）</param>
    /// <param name="zMin">Z 軸の下限（ワールド座標）</param>
    /// <param name="zMax">Z 軸の上限（ワールド座標）</param>
    void SetDynamicBounds(float xMin, float xMax, float zMin, float zMax);

    /// <summary>
    /// 中心と片側範囲から動的移動範囲を設定（範囲は中心 ± range）
    /// </summary>
    /// <param name="center">範囲の中心（ワールド座標、Y は無視）</param>
    /// <param name="xRange">中心から X 方向への片側幅</param>
    /// <param name="zRange">中心から Z 方向への片側幅</param>
    void SetDynamicBoundsFromCenter(const Tako::Vector3& center, float xRange, float zRange);

    void SetEmitterManager(Tako::EmitterManager* emitterManager) { emitterManager_ = emitterManager; }
    void SetIsPause(bool isPause) { isPause_ = isPause; }

    /// <summary>
    /// ForceFieldManager を設定（非所有）
    /// </summary>
    void SetForceFieldManager(Tako::ForceFieldManager* manager) { forceFieldManager_ = manager; }

    //======================================================================================
    //Getter
    //======================================================================================
    float GetSpeed() const { return speed_; }
    Tako::Camera* GetCamera() const { return camera_; }
    Boss* GetTargetEnemy() const { return targetEnemy_; }

    /// <summary>
    /// true: ThirdPerson, false: TopDown
    /// </summary>
    bool GetMode() const { return mode_; }

    float GetHp() const { return hp_; }
    bool IsDead() const { return isDead_; }
    bool IsInvincible() const { return isInvincible_; }
    bool IsShootingEnabled() const { return shootingEnabled_; }

    /// <summary>
    /// 射撃可能か（死亡中・ボスフェーズ2では false）
    /// </summary>
    bool CanShoot() const;

    /// <summary>
    /// 現在射撃状態か
    /// </summary>
    bool IsShooting() const;

    /// <summary>
    /// 射撃ステートの照準方向を返す（非射撃中は最後の照準方向）
    /// </summary>
    Tako::Vector3 GetAimDirection() const;

    const Tako::Transform& GetTransform() const { return transform_; }
    Tako::Transform* GetTransformPtr() { return &transform_; }
    Tako::Vector3 GetTranslate() const { return transform_.translate; }
    Tako::Vector3 GetRotate() const { return transform_.rotate; }
    Tako::Vector3 GetScale() const { return transform_.scale; }
    Tako::Object3d* GetModel() const { return model_.get(); }
    PlayerStateMachine* GetStateMachine() const { return stateMachine_.get(); }
    MeleeAttackCollider* GetMeleeAttackCollider() const { return meleeAttackCollider_.get(); }
    Tako::Object3d* GetAttackBlock() const { return attackBlock_.get(); }
    bool IsAttackBlockVisible() const { return attackBlockVisible_; }
    Tako::Vector3& GetVelocity() { return velocity_; }
    InputHandler* GetInputHandler() { return inputHandlerPtr_; }
    float GetAttackMinDistance() const { return attackMinDist_; }
    Tako::EmitterManager* GetEmitterManager() const { return emitterManager_; }

    /// <summary>
    /// 現在パリィ状態か
    /// </summary>
    bool IsParrying() const;

    /// <summary>
    /// パリィ可能か（クールダウン中は false）
    /// </summary>
    /// <returns>パリィクールダウンが完了していれば true</returns>
    bool CanParry() const;

    /// <summary>
    /// ダッシュ可能か（クールダウン中は false）
    /// </summary>
    /// <returns>ダッシュクールダウンが完了していれば true</returns>
    bool CanDash() const;

    /// <summary>
    /// 現在の向き（rotate.y）に沿って前方 offset だけ進めたワールド座標を返す
    /// </summary>
    /// <param name="offset">前方への距離。Y は変化せず XZ 平面上で前進する</param>
    /// <returns>プレイヤー位置を前方へ offset ずらしたワールド座標</returns>
    Tako::Vector3 GetFrontPosition(float offset) const;

    /// <summary>
    /// MoveToTarget の目標に到達したか
    /// </summary>
    bool HasReachedTarget() const;

private: //非公開関数
    void SyncGlobalVariables();

    /// <summary>
    /// HP、クールダウン、死亡判定の更新
    /// </summary>
    void UpdateCombat(float deltaTime);

    void UpdateStateMachine(float deltaTime);

    /// <summary>
    /// 位置制限を適用
    /// </summary>
    void UpdateTransform();

    /// <summary>
    /// ForceField の外力を反映し減衰
    /// </summary>
    void UpdatePhysics(float deltaTime);

    void UpdateVisuals(float deltaTime);

private: //メンバー変数
    //動的移動制限（ボス近接戦闘エリア）
    DynamicBoundary dynamicBounds_;

    //コア
    std::unique_ptr<Tako::Object3d> model_;
    Tako::Camera*                   camera_             = nullptr;
    Tako::Transform                 transform_{};
    Tako::Vector3                   velocity_{};
    Tako::Vector3                   externalVelocity_{};            ///< m/s、ForceField による反発
    Tako::ForceFieldManager*        forceFieldManager_  = nullptr;  ///< 非所有
    float                           speed_              = 0.5f;
    float                           targetAngle_        = 0.f;
    float                           hp_                 = kMaxHp;
    bool                            isDead_             = false;

    //フラグ
    bool mode_                = false;  ///< true: ThirdPerson, false: TopDown
    bool isDisModelDebugInfo_ = false;
    bool isInvincible_        = false;
    bool isPause_             = false;
    bool shootingEnabled_     = true;   ///< false でフェーズ2射撃無効化

    //システム
    std::unique_ptr<PlayerStateMachine> stateMachine_;
    InputHandler*                       inputHandlerPtr_;
    Tako::EmitterManager*               emitterManager_  = nullptr;

    //コライダー
    std::unique_ptr<Tako::OBBCollider>   bodyCollider_;
    std::unique_ptr<MeleeAttackCollider> meleeAttackCollider_;

    //攻撃ブロック
    std::unique_ptr<Tako::Object3d> attackBlock_;                 ///< 攻撃時に表示される回転ブロック
    bool                            attackBlockVisible_ = false;

    //攻撃
    Boss*       targetEnemy_     = nullptr;
    bool        isAttackHit_     = false;
    float       attackMoveSpeed_ = 2.0f;
    EasingMover attackMover_;                ///< MoveToTarget 用

    //クールダウン
    CooldownTimer parryCooldown_;
    CooldownTimer dashCooldown_;

    //弾生成
    BulletSpawner bulletSpawner_;

    //調整パラメータ（ImGui 編集用）
    float initialY_               = 2.5f;
    float initialZ_               = -120.0f;
    float attackMinDist_          = 5.0f;     ///< 攻撃開始距離
    float attackMoveRotationLerp_ = 0.3f;     ///< 攻撃移動中の回転補間速度
    float bossLookatLerp_         = 1.15f;    ///< ボス視線追従補間速度

    //UI・エフェクト
    HPBarUI        hpBar_;
    HitFlashEffect hitFlashEffect_;
    ShakeEffect    shakeEffect_;
};
