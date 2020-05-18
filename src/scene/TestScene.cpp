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
#if 0
  // XXX: test animation
  spr_.SetImageByPath("../test/test.png");
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

  spr2_.SetImageByPath("../test/test2.png");
  spr2_.SetPos(200, 350);
  spr2_.SetSize(120, 120);

  const std::string text_to_print = u8"Hello World!\nWith Line breaking\nあえいおう가나다ㅏ";

  text_.SetFontByPath(
    "../test/gyeonggi.ttf|"
    "size:12;color:0xffffffff;border:2,0xff666666"
  );
  text_.SetText(text_to_print);
  text_.SetPos(30, 10);
  //text_.SetScale(0.8f, 1.0f);
  //text_.SetRotation(0, 0, 0.3);
  //text_.SetSize(200, 300);
  text_.SetTextFitting(TextFitting::kTextFitStretch);

  spr_bg_.SetImageByPath("../test/msel.mpg");
  spr_bg_.SetPos(0, 0);
  spr_bg_.SetSize(800, 480);

  lr2text_.SetFontByPath("../test/artistfnt.dxa");
  ASSERT(lr2text_.font());
  lr2text_.font()->SetNullGlyphAsCodePoint(L'?');
  lr2text_.SetText(u8"1234abcdΘΙΚΛあえいおう楽しい熙ⅷ黑");
  //lr2text_.SetText(u8"!\"#$%&'()/df");
  lr2text_.SetPos(30, 200);
  lr2text_.SetScale(1.5, 1.5);

  // AddChild for rendering!
  AddChild(&spr_bg_);
  AddChild(&spr_);
  AddChild(&spr2_);
  AddChild(&text_);
  AddChild(&lr2text_);
#endif
  Scene::LoadScene();
}

void TestScene::ProcessInputEvent(const InputEvent& e)
{
  if (e.type() == InputEvents::kOnKeyUp && e.KeyCode() == GLFW_KEY_ESCAPE)
  {
    Game::Exit();
  }
}

}