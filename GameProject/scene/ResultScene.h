#pragma once
#include <memory>
#include <string>

#include "BaseScene.h"
#include "vector2.h"
#include "CameraSystem/CameraConfig.h"

namespace Tako {
class Sprite;
}

/// <summary>
/// 結果シーンクラス
/// クリア / ゲームオーバーの結果表示を管理。タイトルテクスチャの差し替えで両用途を兼ねる
/// </summary>
class ResultScene : public Tako::BaseScene
{
public: // メンバ関数

  /// <param name="titleTexture">中央に表示するタイトルテクスチャ名</param>
  explicit ResultScene(const std::string& titleTexture);

  void Initialize() override;

  void Finalize() override;

  void Update() override;

  void Draw() override;
  void DrawWithoutEffect() override;

  void DrawImGui() override;

private: // メンバ変数

    std::string titleTexture_;

    // sprite
    std::unique_ptr<Tako::Sprite> backGround_ = nullptr;
    std::unique_ptr<Tako::Sprite> titleText_ = nullptr;
    std::unique_ptr<Tako::Sprite> pressButtonText_ = nullptr;

    // カメラ非表示 Y 座標
    float cameraHiddenY_ = CameraConfig::HIDDEN_Y;

    // UI 位置・サイズ用変数
    float titleTextHalfWidth_ = 250.0f;  ///< タイトルテキスト半幅（センタリング用）
    float titleTextY_ = 300.0f;  ///< タイトルテキスト Y 座標
    float buttonBottomOffset_ = 300.0f;  ///< ボタン下端からのオフセット
};
