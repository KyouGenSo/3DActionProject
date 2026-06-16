#include "SceneFactory.h"

#include "ResultScene.h"
#include "TitleScene.h"
#include "GameScene.h"

#ifdef _DEBUG
#include "DebugUIManager.h"
#endif

using namespace Tako;

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName)
{
  if (sceneName == "title") {
    return std::make_unique<TitleScene>();
  } else if (sceneName == "game") {
    return std::make_unique<GameScene>();
  } else if (sceneName == "clear") {
    return std::make_unique<ResultScene>("gameClear_Text.dds");
  } else if (sceneName == "over") {
    return std::make_unique<ResultScene>("gameOver_Text.dds");
  }

#ifdef _DEBUG
  DebugUIManager::GetInstance()->AddLog("Unknown scene name: " + sceneName, DebugUIManager::LogType::Error);
#endif

  return nullptr;
}
