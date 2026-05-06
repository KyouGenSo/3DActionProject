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
/// タイトルシーンクラス
/// タイトル画面の演出、UI 表示、ゲーム開始処理を管理
/// </summary>
class TitleScene : public Tako::BaseScene
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
	/// タイトルテキストアニメーションを再生
	/// </summary>
	void PlayTitleAnimation();

	/// <summary>
	/// タイトルテキストアニメーションを停止
	/// </summary>
	void StopTitleAnimation();

	/// <summary>
	/// タイトルテキストアニメーションをリセット
	/// </summary>
	void ResetTitleAnimation();

private: // メンバ関数

	// === 初期化系関数 === //

	/// <summary>
	/// デバッグ UI の初期化
	/// GlobalVariables にタイトルシーンのパラメータグループを登録
	/// </summary>
	void InitializeDebugUI();

	/// <summary>
	/// カメラの初期化
	/// カメラ位置と注視点を設定
	/// </summary>
	void InitializeCamera();

	/// <summary>
	/// ポストエフェクトの初期化
	/// RGBSplit と Vignette エフェクトのパラメータを設定
	/// </summary>
	void InitializePostEffects();

	/// <summary>
	/// スプライトの初期化
	/// 背景、タイトルテキスト、スタートボタンなどの UI 要素を作成
	/// </summary>
	void InitializeSprites();

	/// <summary>
	/// パーティクルの初期化
	/// エミッターマネージャーの設定とエフェクト用パーティクルの準備
	/// </summary>
	void InitializeParticles();

	// === 更新系関数 === //

	/// <summary>
	/// ウィンドウリサイズ処理
	/// スプライトの位置やサイズを新しいウィンドウサイズに合わせて調整
	/// </summary>
	void UpdateWindowResize();

	/// <summary>
	/// スタートボタンの点滅アニメーション更新
	/// sin カーブを使用してアルファ値を滑らかに変化させる
	/// </summary>
	void UpdateStartButtonBlink();

	/// <summary>
	/// タイトルテキストアニメーション更新
	/// フレームアニメーション（0〜9のスプライトを順次表示）を制御
	/// </summary>
	void UpdateTitleTextAnimation();

	/// <summary>
	/// slash パーティクルアニメーション更新
	/// タイトルテキストアニメーションと同期してパーティクル発生量を増加
	/// </summary>
	void UpdateSlashParticleAnimation();

	/// <summary>
	/// タイトルエフェクトアニメーション更新
	/// タイトルテキストの拡大フェードアウトエフェクトを制御
	/// </summary>
	void UpdateTitleEffectAnimation();

	/// <summary>
	/// 入力処理
	/// スペースキーまたは A ボタンでゲームシーンへ遷移
	/// </summary>
	void UpdateInput();

private: // メンバ変数

	std::unique_ptr<Tako::EmitterManager> emitterManager_;  ///< パーティクルエミッター管理
	std::unique_ptr<Tako::ForceFieldManager> forceFieldManager_;  ///< パーティクル用フォースフィールド管理

	std::unique_ptr<Tako::Sprite> titleBG_;  ///< 背景スプライト

	std::vector<std::unique_ptr<Tako::Sprite>> titleTextSprites_;  ///< タイトルテキストアニメーション用スプライト（10フレーム分）

	std::unique_ptr<Tako::Sprite> startButtonText_;  ///< スタートボタンスプライト（点滅アニメーション対象）

	std::unique_ptr<Tako::Sprite> titleTextEffect_;  ///< タイトルテキストエフェクト用スプライト（拡大フェードアウト用）

	Tako::RGBSplitParam rgbSplitParam_{};  ///< RGBSplit ポストエフェクトのパラメータ
	Tako::VignetteParam vignetteParam_{};  ///< Vignette ポストエフェクトのパラメータ

	float offsetY = CameraConfig::HIDDEN_Y;  ///< カメラ非表示用 Y 方向オフセット

	// === カメラ位置用変数 === //
	float cameraY_ = 9.0f;  ///< カメラ Y 座標
	float cameraZ_ = -34.0f;  ///< カメラ Z 座標

	// === UI 位置・サイズ用変数 === //
	float titleTextWidth_ = 500.0f;  ///< タイトルテキスト幅
	float titleTextHeight_ = 200.0f;  ///< タイトルテキスト高さ
	float titleTextY_ = 100.0f;  ///< タイトルテキスト Y 座標
	float startButtonBottomOffset_ = 250.0f;  ///< スタートボタン下端からのオフセット
	float sceneTransitionProgress_ = 0.9f;  ///< シーン遷移トリガー進行度

	// === タイトルテキストアニメーション制御用変数 === //
	int currentFrame_ = 0;  ///< 現在表示中のフレーム番号（0〜9）

	int frameCounter_ = 0;  ///< フレームカウンター（アニメーション速度制御用）

	int animationSpeed_ = 1;  ///< アニメーション速度（何フレームごとに切り替えるか）

	bool isPlaying_ = false;  ///< アニメーション再生中フラグ

	bool isLoop_ = false;  ///< ループ再生フラグ

	bool animationComplete_ = false;  ///< アニメーション完了フラグ

	// === スタートボタン点滅アニメーション用変数 === //
	float blinkTimer_ = 0.0f;  ///< 点滅用タイマー（経過時間）

	float blinkSpeed_ = 1.2f;  ///< 点滅速度（周期の速さ）

	float blinkMinAlpha_ = 0.0f;  ///< 点滅の最小アルファ値（完全透明）

	float blinkMaxAlpha_ = 1.0f;  ///< 点滅の最大アルファ値（完全不透明）

	bool isButtonBlinking_ = true;  ///< 点滅アニメーション有効フラグ

	// === タイトルテキスト拡大エフェクト用変数 === //
	bool isEffectPlaying_ = false;  ///< エフェクト再生中フラグ

	float effectTimer_ = 0.0f;  ///< エフェクト経過時間タイマー

	float effectDuration_ = 1.5f;  ///< エフェクトの持続時間（秒）

	float effectScale_ = 1.0f;  ///< 現在のスケール値

	float effectMaxScale_ = 1.5f;  ///< 最終スケール値（拡大の上限）

	float effectAlpha_ = 0.5f;  ///< 現在のアルファ値

	float effectInitialAlpha_ = 0.5f;  ///< 初期アルファ値

	bool effectTriggered_ = false;  ///< エフェクト開始トリガー（一度だけ実行）

	// === slash パーティクルエミッターアニメーション用変数 === //
	bool isSlashEmitterAnimating_ = false;  ///< アニメーション中フラグ

	float slashEmitterAnimTimer_ = 0.0f;  ///< アニメーション経過時間タイマー

	float slashEmitterAnimDuration_ = 2.5f;  ///< アニメーション持続時間（タイトルテキストと同期：10フレーム * animationSpeed / 60fps）

	uint32_t slashEmitterStartCount_ = 1;  ///< Count 値のアニメーション開始値（初期発生数）
	uint32_t slashEmitterEndCount_ = 300;  ///< Count 値のアニメーション終了値（最大発生数）

	float slashEmitterStartFreq_ = 1.0f;  ///< Frequency 値のアニメーション開始値（初期発生頻度）
	float slashEmitterEndFreq_ = 0.001f;  ///< Frequency 値のアニメーション終了値（最終発生頻度）
};
