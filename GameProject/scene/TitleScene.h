#pragma once
#include "BaseScene.h"
#include"Sprite.h"
#include"Object3d.h"
#include "AABB.h"
#include "EmitterManager.h"
#include "ForceFieldManager.h"
#include "PostEffectManager.h"
#include "CameraSystem/CameraConfig.h"
#include <vector>
#include <memory>

/// <summary>
/// タイトル画面の演出・UI・ゲーム開始処理を管理
/// </summary>
class TitleScene : public Tako::BaseScene
{
public: //メンバー関数

	void Initialize() override;

	void Finalize() override;

	void Update() override;

	void Draw() override;
	void DrawWithoutEffect() override;

	void DrawImGui() override;

	void PlayTitleAnimation();

	void StopTitleAnimation();

	void ResetTitleAnimation();

private: //非公開関数

	// === 初期化系関数 === //

	void InitializeDebugUI();

	void InitializeCamera();

	/// <summary>
	/// RGBSplit と Vignette のパラメータを設定
	/// </summary>
	void InitializePostEffects();

	/// <summary>
	/// 背景・タイトルテキスト・スタートボタンを生成
	/// </summary>
	void InitializeSprites();

	void InitializeParticles();

	// === 更新系関数 === //

	/// <summary>
	/// ウィンドウサイズに合わせてスプライト位置・サイズを調整
	/// </summary>
	void UpdateWindowResize();

	/// <summary>
	/// sin カーブでアルファ値を変化させ点滅させる
	/// </summary>
	void UpdateStartButtonBlink();

	/// <summary>
	/// 10枚のスプライトを順次表示するフレームアニメーション
	/// </summary>
	void UpdateTitleTextAnimation();

	/// <summary>
	/// タイトルアニメーションと同期しパーティクル発生量を増加
	/// </summary>
	void UpdateSlashParticleAnimation();

	/// <summary>
	/// タイトルテキストの拡大フェードアウト
	/// </summary>
	void UpdateTitleEffectAnimation();

	/// <summary>
	/// スペース/A ボタンでゲームシーンへ遷移
	/// </summary>
	void UpdateInput();

private: //メンバー変数
    //エミッター・フォースフィールド
    std::unique_ptr<Tako::EmitterManager>    emitterManager_;
    std::unique_ptr<Tako::ForceFieldManager> forceFieldManager_;

    //スプライト
    std::unique_ptr<Tako::Sprite>              titleBG_;
    std::vector<std::unique_ptr<Tako::Sprite>> titleTextSprites_;  ///< 10フレーム分
    std::unique_ptr<Tako::Sprite>              startButtonText_;
    std::unique_ptr<Tako::Sprite>              titleTextEffect_;   ///< 拡大フェードアウト用

    //ポストエフェクトパラメータ
    Tako::RGBSplitParam rgbSplitParam_{};
    Tako::VignetteParam vignetteParam_{};

    float offsetY = CameraConfig::HIDDEN_Y;

    //カメラ位置用変数
    float cameraY_ = 9.0f;
    float cameraZ_ = -34.0f;

    //UI 位置・サイズ用変数
    float titleTextWidth_          = 500.0f;
    float titleTextHeight_         = 200.0f;
    float titleTextY_              = 100.0f;
    float startButtonBottomOffset_ = 250.0f;
    float sceneTransitionProgress_ = 0.9f;    ///< シーン遷移トリガーの進行度 0.0-1.0

    //タイトルテキストアニメーション制御用変数
    int  currentFrame_      = 0;      ///< 0〜9
    int  frameCounter_      = 0;
    int  animationSpeed_    = 1;      ///< 何フレームごとに切り替えるか
    bool isPlaying_         = false;
    bool isLoop_            = false;
    bool animationComplete_ = false;

    //スタートボタン点滅アニメーション用変数
    float blinkTimer_       = 0.0f;
    float blinkSpeed_       = 1.2f;
    float blinkMinAlpha_    = 0.0f;
    float blinkMaxAlpha_    = 1.0f;
    bool  isButtonBlinking_ = true;

    //タイトルテキスト拡大エフェクト用変数
    bool  isEffectPlaying_    = false;
    float effectTimer_        = 0.0f;
    float effectDuration_     = 1.5f;   ///< 秒
    float effectScale_        = 1.0f;
    float effectMaxScale_     = 1.5f;
    float effectAlpha_        = 0.5f;
    float effectInitialAlpha_ = 0.5f;
    bool  effectTriggered_    = false;  ///< 一度だけ実行するためのフラグ

    //slash パーティクルエミッターアニメーション用変数
    bool     isSlashEmitterAnimating_  = false;
    float    slashEmitterAnimTimer_    = 0.0f;
    float    slashEmitterAnimDuration_ = 2.5f;    ///< タイトルテキストと同期：10フレーム * animationSpeed / 60fps
    uint32_t slashEmitterStartCount_   = 1;
    uint32_t slashEmitterEndCount_     = 300;
    float    slashEmitterStartFreq_    = 1.0f;
    float    slashEmitterEndFreq_      = 0.001f;
};
