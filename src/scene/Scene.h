#pragma once

namespace rhythmus
{

class Scene
{
public:
  virtual void LoadScene() = 0;
  virtual void CloseScene() = 0;
  virtual void Render() = 0;
private:
};

}