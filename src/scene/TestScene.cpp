#include "TestScene.h"
#include "../ResourceManager.h"
#include <iostream>

namespace rhythmus
{

TestScene::TestScene()
{
}

void TestScene::Render()
{
  spr_.Render();
}

void TestScene::LoadScene()
{
  ImageAuto img_ = ResourceManager::getInstance().LoadImage("test.png");
  img_->CommitImage();

  spr_.SetImage(img_);
  spr_.SetPos(50, 50);
  spr_.SetSize(100, 100);
}

void TestScene::CloseScene()
{
}

}