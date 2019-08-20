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
  /* TODO: movie image should be updated automatically ..? */
  img_movie_->Update();

  spr_bg_.Update();
  spr_.Update();
  spr2_.Update();
  text_.Update();
  lr2text_.Update();

  spr_bg_.Render();
  spr_.Render();
  spr2_.Render();
  text_.Render();
  lr2text_.Render();
}

void TestScene::StartScene()
{
}

void TestScene::LoadScene(SceneLoader *scene_loader)
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
  spr_.get_animation().AddTween({ {
      0, 0, 100, 100,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 50, 50, 100, 100, 1.0f, 1.0f, true
    }, 1000, 0, 0, true, TweenTypes::kTweenTypeEaseOut
    });
  spr_.get_animation().AddTween({ {
      0, 0, 110, 110,
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, glm::radians(90.0f), 55, 55, 100, 200, 1.0f, 1.5f, true
    }, 1500, 0, 0, true, TweenTypes::kTweenTypeEaseOut
    });

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

  lr2font_.ReadLR2Font("../test/artistfnt.dxa");
  lr2font_.SetNullGlyphAsCodePoint('?');

  lr2text_.SetFont(&lr2font_);
  lr2text_.SetText(u8"1234abcdあえいおう楽しい");
  lr2text_.SetPos(30, 200);
  lr2text_.SetScale(1.5, 1.5);
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

const std::string TestScene::GetSceneName() const
{
  return "TestScene";
}

}