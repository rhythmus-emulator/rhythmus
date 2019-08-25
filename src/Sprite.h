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
  Sprite(const Sprite& spr);
  virtual ~Sprite();

  SpriteAnimation& get_animation();
  void SetImage(ImageAuto img);
  void SetPos(int x, int y);
  void SetSize(int w, int h);
  void SetAlpha(float a);
  void SetRGB(float r, float g, float b);
  void SetScale(float x, float y);
  void SetRotation(float x, float y, float z);
  void SetCenter(float x, float y);

  void Hide();
  void Show();

  /* @brief Update before rendering */
  virtual void Update();

  /* @brief Render based on updated information */
  virtual void Render();

  /* @brief Get Sprite name */
  const std::string& get_name() const;

protected:
  const DrawInfo& get_drawinfo() const;
  DrawInfo& get_drawinfo();

  ImageAuto img_;
  SpriteAnimation ani_;
  DrawInfo di_;

  /* invalidate drawinfo with force (internal use) */
  bool invalidate_drawinfo_;

  /* sprite name */
  std::string sprite_name_;
};

/* Sprite may not need to be shared. */
using SpriteAuto = std::unique_ptr<Sprite>;

}