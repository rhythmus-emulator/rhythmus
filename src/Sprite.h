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
  void Render();

private:
  ImageAuto img_;
  SpriteAnimation ani_;
  SpriteFrame frame_;
  struct {
    float sx, sy, sx2, sy2, dx, dy, dx2, dy2;
  } rframe_;
  const char* hlsl_;
  struct hlsl_vert_s {
    float x, y, z;
    float sx, sy;
    float r, g, b, a;
  } hlsl_vert_[4];
  GLuint vertex_buffer_, vertex_array_, vertex_prog_;

  void InvalidateRFrame();
  void RenderDepreciated();
};

}