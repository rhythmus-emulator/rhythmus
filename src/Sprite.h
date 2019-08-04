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
  ~Sprite();
  SpriteAnimation& get_animation();
  void SetImage(ImageAuto img);
  void SetPos(int x, int y);
  void SetSize(int w, int h);
  void SetAlpha(float a);
  void SetRGB(float r, float g, float b);
  void Render();

private:
  ImageAuto img_;
  SpriteAnimation ani_;
  DrawInfo di_;
};

}