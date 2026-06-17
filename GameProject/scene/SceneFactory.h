#pragma once
#include "AbstractSceneFactory.h"
#include <memory>

/// <summary>
/// シーン名から対応するシーンを生成するファクトリー
/// </summary>
class SceneFactory : public Tako::AbstractSceneFactory
{
public: //メンバー関数
	/// <summary>
	/// シーン名に対応するシーンインスタンスを生成する
	/// </summary>
	/// <param name="sceneName">"title"/"game"/"clear"/"over" のいずれか。clear/over は結果テクスチャ付きの ResultScene を生成</param>
	/// <returns>生成したシーン。未知の名前は nullptr</returns>
	std::unique_ptr<Tako::BaseScene> CreateScene(const std::string& sceneName) override;
};
