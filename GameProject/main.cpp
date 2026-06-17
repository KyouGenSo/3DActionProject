#include"TakoFramework.h"
#include "MyGame/MyGame.h"
#include <memory>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  auto game = std::make_unique<MyGame>();

  game->Run();

  return 0;
}