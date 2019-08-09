#include "TestScene.h"
#include "../ResourceManager.h"
#include <iostream>

namespace rhythmus
{

TestScene::TestScene()
{
}

TestScene::~TestScene()
{
}

void TestScene::Render()
{
  spr_.Update();
  spr2_.Update();
  font_.Update();

  spr_.Render();
  spr2_.Render();
  font_.Render();
}

void TestScene::LoadScene()
{
  ImageAuto img_ = ResourceManager::getInstance().LoadImage("test.png");
  ImageAuto img2_ = ResourceManager::getInstance().LoadImage("test2.png");
  img_->CommitImage();
  img2_->CommitImage();

  spr_.SetImage(img_);

  // XXX: test animation
  spr_.get_animation().AddTween({ {
      0, 0, 100, 100,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 50, 50, 100, 100, 1.0f, 1.0f
    }, 1000, 0, true, TweenTypes::kTweenTypeEaseOut
    });
  spr_.get_animation().AddTween({ {
      0, 0, 110, 110,
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, glm::radians(90.0f), 55, 55, 100, 200, 1.0f, 1.5f
    }, 1500, 0, true, TweenTypes::kTweenTypeEaseOut
    });

  spr2_.SetImage(img2_);
  spr2_.SetPos(200, 350);
  spr2_.SetSize(120, 120);

  FontAttributes attr;
  memset(&attr, 0, sizeof(attr));
  attr.size = 12;
  font_.LoadFont("gyeonggi.ttf", attr);
  //font_.PrepareGlyph ...
  font_.Commit();
  font_.SetText("Hello World!");
  font_.SetPos(30, 10);
  font_.SetScale(1.0f, 1.0f);
}

void TestScene::CloseScene()
{
}

void TestScene::ProcessEvent(const GameEvent& e)
{
  if (IsEventKeyPress(e))
  {
    if (GetKeycode(e) == GLFW_KEY_ESCAPE)
      Graphic::getInstance().ExitRendering();
  }
}

}