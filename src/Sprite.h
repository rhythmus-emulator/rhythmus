#pragma once
#include "Image.h"
#include "Animation.h"
#include <string>

namespace rhythmus
{

class Sprite
{
public:
  Sprite();
  virtual ~Sprite();

  SpriteAnimation& get_animation();
  const DrawInfo& get_drawinfo();
  void SetImage(ImageAuto img);
  void SetPos(int x, int y);
  void SetSize(int w, int h);
  void SetAlpha(float a);
  void SetRGB(float r, float g, float b);
  void SetScale(float x, float y);

  /* Update before rendering */
  virtual void Update();

  /* Render based on updated information */
  virtual void Render();

private:
  ImageAuto img_;
  SpriteAnimation ani_;
  DrawInfo di_;
};

}