#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "Font.h"
#include "LR2/LR2Font.h"

namespace rhythmus
{

class TestScene : public Scene
{
public:
  TestScene();
  virtual ~TestScene();

  virtual void LoadScene(SceneLoader *scene_loader);
  virtual void StartScene();
  virtual void CloseScene();
  virtual void Render();
  virtual bool ProcessEvent(const EventMessage& e);

  virtual const std::string GetSceneName() const;
private:
  Sprite spr_;
  Sprite spr2_;
  Sprite spr_bg_;
  Font font_;
  Text text_;
  ImageAuto img_movie_;
  FontAuto lr2font_;
  Text lr2text_;
};

}