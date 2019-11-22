#include "ResultScene.h"

namespace rhythmus
{

ResultScene::ResultScene()
{
  set_name("ResultScene");

  // TODO: check for bootmode to decide next scene
  next_scene_ = "SelectScene";
  prev_scene_ = "SelectScene";
}

void ResultScene::ProcessInputEvent(const InputEvent& e)
{
  if (!is_input_available())
    return;

  if (e.type() == InputEvents::kOnKeyUp)
  {
    FadeOutScene(true);
  }
}

}