#include "DecideScene.h"
#include "Song.h"
#include "Event.h"
#include "Script.h"

namespace rhythmus
{

DecideScene::DecideScene()
{
  set_name("DecideScene");
  prev_scene_ = "SelectScene";
  next_scene_ = "PlayScene";
}

void DecideScene::ProcessInputEvent(const InputEvent& e)
{
  Scene::ProcessInputEvent(e);
}

}