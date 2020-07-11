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
  Scene::ProcessInputEvent(e);

  if (e.type() == InputEvents::kOnKeyUp)
  {
    switch (e.KeyCode())
    {
    case RI_KEY_ESCAPE:
      Game::Exit();
      break;
    case RI_KEY_SPACE:
      EVENTMAN->SendEvent("Load");
      break;
    }
  }
}

}