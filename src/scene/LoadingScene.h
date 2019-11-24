#pragma once

#include "Scene.h"
#include "object/Text.h"

namespace rhythmus
{

class LoadingScene: public Scene
{
public:
  virtual ~LoadingScene();

  virtual void LoadScene();
  virtual void StartScene();
  virtual void ProcessInputEvent(const InputEvent& e);

private:
  Text message_text_;
  Text current_file_text_;
  Image img_;
  Sprite loading_bar_;

  virtual void doUpdate(float);
};

}