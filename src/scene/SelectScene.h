#pragma once

#include "Scene.h"

namespace rhythmus
{

struct SelectItem
{
  std::string name;
  std::string artist;
};

class SelectScene : public Scene
{
public:
  virtual void StartScene();
  virtual void CloseScene();
  virtual void Render();
  virtual void ProcessEvent(const GameEvent& e);

  virtual const std::string GetSceneName() const;

  const char* get_selected_title() const;

private:
  std::vector<SelectItem> select_list_;
  int select_index_;
};

}