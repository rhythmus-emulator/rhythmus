#pragma once

#include "Scene.h"
#include "object/SelectWheel.h"

namespace rhythmus
{


class SelectScene : public Scene
{
public:
  SelectScene();

  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene();
  virtual bool ProcessEvent(const EventMessage& e);

  virtual const std::string GetSceneName() const;

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

  SelectWheel& get_wheel();

private:
  SelectWheel wheel_;
};

}