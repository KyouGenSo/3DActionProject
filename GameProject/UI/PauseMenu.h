#pragma once
#include <array>
#include <memory>
#include <cstdint>

namespace Tako {
class Sprite;
class WinApp;
struct Vector2;
}

/// <summary>
/// ポーズメニュー。Resume/Title/Exit を DPAD で選択、A ボタンで決定
/// </summary>
class PauseMenu
{
private: //定数
    static constexpr int kButtonCount = 3;

    // 選択時の色（白）
    static constexpr float kSelectedColorR = 1.0f;
    static constexpr float kSelectedColorG = 1.0f;
    static constexpr float kSelectedColorB = 1.0f;

    // 非選択時の色（グレー）
    static constexpr float kUnselectedColorR = 0.5f;
    static constexpr float kUnselectedColorG = 0.5f;
    static constexpr float kUnselectedColorB = 0.5f;

    // UI 設計時の想定解像度
    static constexpr float kBaseWidth = 1920.0f;
    static constexpr float kBaseHeight = 1080.0f;

public: //構造体
    enum class Action {
        None,
        Resume,
        ToTitle,
        ExitGame
    };

public: //メンバー関数
    PauseMenu() = default;
    ~PauseMenu();

    void Initialize();

    /// <returns>選択されたアクション（未決定なら None）</returns>
    Action Update();

    void Draw();

    /// <summary>
    /// 選択を Resume に戻す
    /// </summary>
    void Reset();

    void DrawImGui();

private: //非公開関数
    /// <summary>
    /// 選択中は白、それ以外はグレーに設定
    /// </summary>
    void UpdateButtonColors();

    /// <summary>
    /// リサイズに合わせて全スプライトの位置・サイズを再計算
    /// </summary>
    void OnResize(const Tako::Vector2& newSize);

private: //メンバー変数
    // ボタンスプライト（Resume, Title, Exit）
    std::array<std::unique_ptr<Tako::Sprite>, kButtonCount> buttonSprites_;

    std::unique_ptr<Tako::Sprite> titleSprite_;
    std::unique_ptr<Tako::Sprite> overlaySprite_;

    // DPAD 操作ガイドスプライト
    std::unique_ptr<Tako::Sprite> dpadGuideSprite_;  ///< 中立
    std::unique_ptr<Tako::Sprite> dpadUpSprite_;
    std::unique_ptr<Tako::Sprite> dpadDownSprite_;

    std::unique_ptr<Tako::Sprite> aButtonUpSprite_;
    std::unique_ptr<Tako::Sprite> aButtonDownSprite_;

    std::unique_ptr<Tako::Sprite> ketteiSprite_;   ///< 決定
    std::unique_ptr<Tako::Sprite> sentakuSprite_;  ///< 選択

    // 入力状態（描画用）
    bool isDPadUpPressed_   = false;
    bool isDPadDownPressed_ = false;
    bool isAPressed_        = false;

    // 0:Resume, 1:Title, 2:Exit
    int selectedIndex_ = 0;

    Tako::WinApp* winApp_     = nullptr;
    uint32_t      onResizeId_ = 0;        ///< 登録したコールバックの ID
};
