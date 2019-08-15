#pragma once

#include "Scene.h"
#include "ResourceManager.h"

#include <vector>
#include <string>

namespace rhythmus
{

class LR2Scene : public Scene
{
public:
  LR2Scene();
  virtual ~LR2Scene();

  virtual void LoadScene();
  virtual void CloseScene();
  virtual void Render();
  virtual void ProcessEvent(const GameEvent& e);

private:
  std::string scene_filepath_;
  std::string folder_;

  // image resources loaded by this scene
  std::vector<ImageAuto> images_;

  // font resource loaded by this scene
  std::vector<FontAuto> fonts_;

  // sprites to be rendered
  std::vector<Sprite> sprites_;
};

}