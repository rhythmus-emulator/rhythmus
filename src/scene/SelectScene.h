#pragma once

#include "Scene.h"
#include "Sound.h"
#include "object/MusicWheel.h"

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

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

  MusicWheel& get_wheel();

private:
  MusicWheel wheel_;

  void MakeSelectDataList();
};

}