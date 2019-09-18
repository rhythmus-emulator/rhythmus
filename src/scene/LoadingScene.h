#pragma once

#include "Scene.h"
#include "object/Text.h"

namespace rhythmus
{

class LoadingScene: public Scene
{
public:
  virtual ~LoadingScene();
  virtual const std::string GetSceneName() const;

  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene();
  virtual bool ProcessEvent(const EventMessage& e);

  virtual void doUpdate(float);

private:
  FontAuto sys_font_;
  Text message_text_;
  Text current_file_text_;
  Image img_;
  Sprite loading_bar_;
};

}