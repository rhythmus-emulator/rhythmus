#include "SelectScene.h"

namespace rhythmus
{

void SelectScene::CloseScene()
{
  // TODO
}

void SelectScene::Render()
{
  for (auto spr : sprites_)
  {
    spr->Update();
    spr->Render();
  }
}

void SelectScene::ProcessEvent(const GameEvent& e)
{
  // TODO
}

const std::string SelectScene::GetSceneName() const
{
  return "SelectScene";
}

}