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

void TestScene::StartScene()
{
}

void TestScene::LoadScene()
{
  ImageAuto img_ = ResourceManager::getInstance().LoadImage("../test/test.png");
  ImageAuto img2_ = ResourceManager::getInstance().LoadImage("../test/test2.png");
  img_movie_ = ResourceManager::getInstance().LoadImage("../test/msel.mpg");
  img_->CommitImage();
  img2_->CommitImage();
  img_movie_->CommitImage(); /* tex id create & an black image would committed */
  img_movie_->SetLoopMovie(true);

  spr_.SetImage(img_);

  // XXX: test animation
  spr_.AddTweenState({ 0, 0, 100, 100,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 50, 50, 100, 100, 1.0f, 1.0f, true
    }, 1000, EaseTypes::kEaseOut, true);
  spr_.AddTweenState({
      0, 0, 110, 110,
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, glm::radians(90.0f), 55, 55, 100, 200, 1.0f, 1.5f, true
    }, 1500, EaseTypes::kEaseOut, true);

  spr2_.SetImage(img2_);
  spr2_.SetPos(200, 350);
  spr2_.SetSize(120, 120);

  FontAttributes attr;
  memset(&attr, 0, sizeof(FontAttributes));
  attr.size = 12;
  attr.outline_width = 2;
  attr.color = 0xfffffff;
  attr.outline_color = 0xff666666;

  const std::string text_to_print = u8"Hello World!\nWith Line breaking\nあえいおう가나다ㅏ";

  font_.LoadFont("../test/gyeonggi.ttf", attr);
  font_.PrepareText(text_to_print);
  font_.Commit();

  text_.SetFont(&font_);
  text_.SetText(text_to_print);
  text_.SetPos(30, 10);
  //text_.SetScale(0.8f, 1.0f);
  //text_.SetRotation(0, 0, 0.3);
  //text_.SetSize(200, 300);
  text_.SetAlignment(FontAlignments::kFontAlignStretch);

  spr_bg_.SetImage(img_movie_);
  spr_bg_.SetPos(0, 0);
  spr_bg_.SetSize(800, 480);

  lr2font_ = ResourceManager::getInstance().LoadLR2Font("../test/artistfnt.dxa");
  lr2font_->SetNullGlyphAsCodePoint('?');

  lr2text_.SetFont(lr2font_.get());
  lr2text_.SetText(u8"1234abcdΘΙΚΛあえいおう楽しい熙ⅷ黑");
  //lr2text_.SetText(u8"!\"#$%&'()/df");
  lr2text_.SetPos(30, 200);
  lr2text_.SetScale(1.5, 1.5);

  // AddChild for rendering!
  // (order is important!)
  AddChild(&spr_bg_);
  AddChild(&spr_);
  AddChild(&spr2_);
  AddChild(&text_);
  AddChild(&lr2text_);

  // RegisterImage for movie update!
  RegisterImage(img_movie_);
}

void TestScene::CloseScene()
{
}

bool TestScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyDown())
  {
    if (e.GetKeycode() == GLFW_KEY_ESCAPE)
    {
      Graphic::getInstance().ExitRendering();
      return false;
    }
  }
  return true;
}

const std::string TestScene::GetSceneName() const
{
  return "TestScene";
}

}