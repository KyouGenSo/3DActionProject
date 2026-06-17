#pragma once
#include <memory>
#include <array>
#include <cstdint>
#include "Sprite.h"
#include "Vector2.h"

// 前方宣言
class Boss;
namespace Tako { class WinApp; }

/// <summary>
/// ゲームパッドの入力状態を表示する UI。
/// ボタン押下で Up/Down、スティック方向でスプライトを切り替える
/// </summary>
class ControllerUI
{
private: //定数
    // UI 設計時の想定解像度
    static constexpr float kBaseWidth = 1920.0f;
    static constexpr float kBaseHeight = 1080.0f;

public: //メンバー関数
    ControllerUI() = default;
    ~ControllerUI();

    void Initialize();
    void Update();
    void Draw();
    void DrawImGui();

    //=============================
    //Setter
    //=============================
    /// <summary>
    /// フェーズ判定用にボス参照を設定
    /// </summary>
    void SetBoss(Boss* boss) { boss_ = boss; }

    void SetIsPaused(bool isPaused) { isPaused_ = isPaused; }

private: //非公開関数
    /// <summary>
    /// スティック入力を8方向のインデックスへ量子化する
    /// </summary>
    /// <param name="stick">スティック入力。x=左右(右が正)、y=上下(上が正)</param>
    /// <returns>上を0とし時計回りに増える方向インデックス (0:上, 1:右上, 2:右, 3:右下, 4:下, 5:左下, 6:左, 7:左上)。デッドゾーン未満は0</returns>
    int GetStickDirectionIndex(const Tako::Vector2& stick) const;

    /// <summary>
    /// リサイズに合わせて全スプライトの位置・サイズを再計算
    /// </summary>
    void OnResize(const Tako::Vector2& newSize);

private: //メンバー変数
    Tako::WinApp* winApp_     = nullptr;
    uint32_t      onResizeId_ = 0;        ///< 登録したコールバックの ID

    // ボタンスプライト（Up/Down 各4ボタン）
    std::unique_ptr<Tako::Sprite> aButtonUpSprite_;
    std::unique_ptr<Tako::Sprite> aButtonDownSprite_;
    std::unique_ptr<Tako::Sprite> bButtonUpSprite_;
    std::unique_ptr<Tako::Sprite> bButtonDownSprite_;
    std::unique_ptr<Tako::Sprite> xButtonUpSprite_;
    std::unique_ptr<Tako::Sprite> xButtonDownSprite_;
    std::unique_ptr<Tako::Sprite> yButtonUpSprite_;
    std::unique_ptr<Tako::Sprite> yButtonDownSprite_;

    // ジョイスティックスプライト（左右各8方向）
    std::array<std::unique_ptr<Tako::Sprite>, 8> lJoystickSprites_;
    std::array<std::unique_ptr<Tako::Sprite>, 8> rJoystickSprites_;

    // アクションアイコンスプライト
    std::unique_ptr<Tako::Sprite> kougekiSprite_;
    std::unique_ptr<Tako::Sprite> dashSprite_;
    std::unique_ptr<Tako::Sprite> parrySprite_;
    std::unique_ptr<Tako::Sprite> shagekiSprite_;
    std::unique_ptr<Tako::Sprite> idouSprite_;

    // ポーズ操作ヒントスプライト
    std::unique_ptr<Tako::Sprite> pauseHintIconSprite_;  ///< Menu ボタンアイコン
    std::unique_ptr<Tako::Sprite> pauseHintTextSprite_;  ///< PAUSE テキスト

    // 現在の表示状態
    bool isAPressed_    = false;
    bool isBPressed_    = false;
    bool isXPressed_    = false;
    bool isYPressed_    = false;
    int  leftStickDir_  = 0;      ///< 0-7
    int  rightStickDir_ = 0;      ///< 0-7

    // スティック入力がこの値未満なら無視
    float stickDeadzone_ = 0.3f;

    Boss* boss_     = nullptr;  ///< フェーズ判定用
    bool  isPaused_ = false;
};
