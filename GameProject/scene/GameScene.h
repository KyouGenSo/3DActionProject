#pragma once
#include "BaseScene.h"
#include "Transform.h"
#include  "SkyBox.h"
#include "Object/Boss/Boss.h"
#include "Object/Player/Player.h"
#include "Input/InputHandler.h"
#include "../Object/Projectile/BossBullet.h"
#include "../Object/Projectile/PlayerBullet.h"
#include "../Object/Projectile/PenetratingBossBullet.h"
#include "../Effect/OverEffectManager.h"
#include "../Effect/ClearEffectManager.h"
#include "../Effect/BossBorderParticleManager.h"
#include "../Effect/DashEffectManager.h"
#include "UI/ControllerUI.h"
#include "UI/PauseMenu.h"

#include "Decal.h"

#include <memory>
#include <vector>

// Tako namespace の前方宣言
namespace Tako {
    class Object3d;
    class EmitterManager;
    class Sprite;
    class BoneTracker;
    enum class DecalShape;
}

// GameProject 前方宣言
class CameraManager;
class ThirdPersonController;
class TopDownController;
class CameraAnimationController;
class BossBullet;
class PlayerBullet;

/// <summary>
/// ゲームメインシーンクラス
/// プレイヤーとボスの戦闘、ゲームプレイの中核を管理
/// </summary>
class GameScene : public Tako::BaseScene
{
public: // メンバ関数

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize() override;

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize() override;

    /// <summary>
    /// 更新
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画
    /// </summary>
    void Draw() override;
    void DrawWithoutEffect() override;

    /// <summary>
    /// ImGui の描画
    /// </summary>
    void DrawImGui() override;

    /// <summary>
    /// カメラモードの更新処理
    /// </summary>
    void UpdateCameraMode();

    /// <summary>
    /// 入力処理の更新
    /// </summary>
    void UpdateInput();

    /// <summary>
    /// プロジェクタイル（弾）の更新処理
    /// </summary>
    void UpdateProjectiles(float deltaTime);

    /// <summary>
    /// ボスの弾の生成処理
    /// </summary>
    void CreateBossBullet();

    /// <summary>
    /// プレイヤーの弾の生成処理
    /// </summary>
    void CreatePlayerBullet();

    /// <summary>
    /// 貫通弾の生成処理
    /// </summary>
    void CreatePenetratingBossBullet();

    /// <summary>
    /// デバッグ用オプション初期化
    /// </summary>
    void InitializeDebugOption();

    /// <summary>
    /// ポストエフェクト初期化
    /// </summary>
    void InitializePostEffect();

    /// <summary>
    /// Object3d 初期化
    /// </summary>
    void InitializeObject3d();

    /// <summary>
    /// カメラシステム初期化
    /// </summary>
    void InitializeCameraSystem();

    /// <summary>
    /// 衝突判定マスクの設定
    /// </summary>
    void SetCollisionMask();

    /// <summary>
    /// エミッターマネージャー初期化
    /// </summary>
    void InitializeEmitterManger();

    /// <summary>
    /// エフェクトマネージャー初期化
    /// </summary>
    void InitializeEffectManager();

    /// <summary>
    /// カメラアニメーションの設定
    /// </summary>
    void SetCameraAnimation();

    /// <summary>
    /// ポーズメニューの更新処理
    /// </summary>
    void UpdatePause();

private: // メンバ変数

    std::unique_ptr<Tako::SkyBox> skyBox_;                      // スカイボックス（環境マップ）

    std::unique_ptr<Tako::Object3d> ground_;                    // 地面オブジェクト

    std::unique_ptr<Player> player_;                            // プレイヤーキャラクター

    std::unique_ptr<Boss> boss_;                                // ボスキャラクター

    std::vector<std::unique_ptr<BossBullet>> bossBullets_;      // ボスの弾のコンテナ

    std::vector<std::unique_ptr<PlayerBullet>> playerBullets_;  // プレイヤーの弾のコンテナ

    std::vector<std::unique_ptr<PenetratingBossBullet>> penetratingBossBullets_;  // 貫通弾のコンテナ

    std::unique_ptr<InputHandler> inputHandler_;                // 入力ハンドラー

    // Camera system components
    CameraManager* cameraManager_ = nullptr;                    // カメラシステム管理
    ThirdPersonController* thirdPersonController_ = nullptr;    // 一人称視点コントローラー
    TopDownController* topDownController_ = nullptr;            // トップダウン視点コントローラー
    CameraAnimationController* animationController_ = nullptr;  // カメラアニメーションコントローラー
    bool cameraMode_ = false;                                   // カメラモード (true: ThirdPerson, false: TopDown)

    Tako::Transform groundUvTransform_{};                       // 地面の UV トランスフォーム（テクスチャスクロール等に使用）

    std::unique_ptr<Tako::EmitterManager> emitterManager_;      // パーティクルエミッター管理

    bool isStart_ = false;                                      // ゲーム開始フラグ

    bool isDebug_ = false;                                      // デバッグモードフラグ

    // エフェクトマネージャー
    std::unique_ptr<OverEffectManager> overEffectManager_;           // ゲームオーバー演出管理
    std::unique_ptr<ClearEffectManager> clearEffectManager_;         // ゲームクリア演出管理
    std::unique_ptr<BossBorderParticleManager> bossBorderManager_;   // ボーダーパーティクル管理
    std::unique_ptr<DashEffectManager> dashEffectManager_;           // ダッシュエフェクト管理

    // UI マネージャー
    std::unique_ptr<ControllerUI> controllerUI_;                     // コントローラー UI 表示
    std::unique_ptr<PauseMenu> pauseMenu_;                           // ポーズメニュー
    bool isPaused_ = false;                                          // ポーズ中フラグ
};