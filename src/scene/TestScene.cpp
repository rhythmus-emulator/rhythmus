#include "TestScene.h"

namespace rhythmus
{

TestScene::TestScene()
{
  set_name("TestScene");
}

TestScene::~TestScene()
{
}

void TestScene::LoadScene()
{
  Scene::LoadScene();
}

void TestScene::ProcessInputEvent(const InputEvent& e)
{
  if (e.type() == InputEvents::kOnKeyUp && e.KeyCode() == RI_KEY_ESCAPE)
  {
    Game::Exit();
  }
}

}