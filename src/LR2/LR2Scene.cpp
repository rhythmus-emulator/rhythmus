#include "LR2Scene.h"

namespace rhythmus
{

LR2Scene::LR2Scene()
{
  // XXX: fixed path for test purpose
  scene_filepath_ = "../themes/WMIX_HD/select/select.csv";

  // get folder path using scene filepath

}

LR2Scene::~LR2Scene()
{
}

void LR2Scene::LoadScene()
{
  // TODO: start csv parsing
}

void LR2Scene::CloseScene()
{

}

void LR2Scene::Render()
{

}

void LR2Scene::ProcessEvent(const GameEvent& e)
{

}

}