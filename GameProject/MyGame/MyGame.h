#pragma once
#include"TakoFramework.h"
#include"Vector2.h"

/// <summary>
/// 3D アクションゲームのメインクラス
/// ゲーム全体の初期化、更新、描画処理を管理する
/// </summary>
class MyGame : public Tako::TakoFramework
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

private: // プライベートメンバ関数

    /// <summary>
    /// GlobalVariables にパラメータを登録
    /// </summary>
    void RegisterGlobalVariables();

    /// <summary>
    /// 入力関連パラメータを登録
    /// </summary>
    void RegisterInputVariables();

    /// <summary>
    /// ゲームシーン関連パラメータを登録
    /// </summary>
    void RegisterGameSceneVariables();

    /// <summary>
    /// プレイヤー関連パラメータを登録
    /// </summary>
    void RegisterPlayerVariables();

    /// <summary>
    /// ボス関連パラメータを登録
    /// </summary>
    void RegisterBossVariables();

    /// <summary>
    /// 弾丸関連パラメータを登録
    /// </summary>
    void RegisterProjectileVariables();

    /// <summary>
    /// 状態関連パラメータを登録
    /// </summary>
    void RegisterStateVariables();

    /// <summary>
    /// AttackState 関連パラメータを登録
    /// </summary>
    void RegisterAttackStateVariables();

    /// <summary>
    /// DashState 関連パラメータを登録
    /// </summary>
    void RegisterDashStateVariables();

    /// <summary>
    /// ParryState 関連パラメータを登録
    /// </summary>
    void RegisterParryStateVariables();

    /// <summary>
    /// ShootState 関連パラメータを登録
    /// </summary>
    void RegisterShootStateVariables();

    /// <summary>
    /// テクスチャリソースの読み込み
    /// </summary>
    void LoadTextures();

    /// <summary>
    /// タイトル画面用テクスチャを読み込み
    /// </summary>
    void LoadTitleTextures();

    /// <summary>
    /// ボタン画像を読み込み
    /// </summary>
    void LoadButtonTextures();

    /// <summary>
    /// ジョイスティック画像を読み込み
    /// </summary>
    void LoadJoystickTextures();

    /// <summary>
    /// アクションアイコンを読み込み
    /// </summary>
    void LoadActionIconTextures();

private: // メンバ変数

    
};