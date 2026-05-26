#pragma once
#include <memory>
#include "vector2.h"

class Player;

/// <summary>
/// 入力処理クラス
/// キーボード、マウス、ゲームパッドからの入力を統合管理
/// </summary>
class InputHandler
{
public:
	InputHandler();
	~InputHandler();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

    /// <summary>
    /// 全ての入力状態をリセット
    /// </summary>
    void ResetInputs();

	/// <summary>
	/// 移動入力があるか判定
	/// </summary>
	/// <returns>移動入力中の場合 true</returns>
	bool IsMoving() const;

	/// <summary>
	/// ダッシュ入力があるか判定
	/// </summary>
	/// <returns>ダッシュボタン押下中の場合 true</returns>
	bool IsDashing() const;

	/// <summary>
	/// 攻撃入力があるか判定
	/// </summary>
	/// <returns>攻撃ボタン押下中の場合 true</returns>
	bool IsAttacking() const;

	/// <summary>
	/// 射撃入力があるか判定
	/// </summary>
	/// <returns>射撃ボタン押下中の場合 true</returns>
	bool IsShooting() const;

	/// <summary>
	/// パリィ入力があるか判定
	/// </summary>
	/// <returns>パリィボタン押下中の場合 true</returns>
	bool IsParrying() const;

	/// <summary>
	/// ポーズ入力があるか判定
	/// </summary>
	/// <returns>ポーズボタン押下中の場合 true</returns>
	bool IsPaused() const;

	/// <summary>
	/// 移動方向を取得
	/// </summary>
	/// <returns>正規化された移動方向ベクトル</returns>
    Tako::Vector2 GetMoveDirection() const;

	/// <summary>
	/// 照準方向を取得（右スティック入力）
	/// </summary>
	/// <returns>正規化された照準方向ベクトル</returns>
    Tako::Vector2 GetAimDirection() const;

private:

	// 移動入力が有効かどうかのキャッシュフラグ
	bool isMoving_ = false;

	// ダッシュ入力が有効かどうかのキャッシュフラグ
	bool isDashing_ = false;

	// 攻撃入力が有効かどうかのキャッシュフラグ
	bool isAttacking_ = false;

	// 射撃入力が有効かどうかのキャッシュフラグ
	bool isShooting_ = false;

	// パリィ入力が有効かどうかのキャッシュフラグ
	bool isParrying_ = false;

	// ポーズ入力が有効かどうかのキャッシュフラグ
	bool isPaused_ = false;

	// キャッシュされた移動方向ベクトル（正規化済み）
    Tako::Vector2 moveDirection_;

	// キャッシュされた照準方向ベクトル（正規化済み）
    Tako::Vector2 aimDirection_;
};