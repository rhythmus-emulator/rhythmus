#pragma once
#include "BaseObject.h"
#include "Image.h"
#include <vector>
#include <string>

namespace rhythmus
{

/* @brief Renderable object with texture */
class Sprite : public BaseObject
{
public:
  Sprite();
  Sprite(const Sprite& spr) = default;
  virtual ~Sprite();

  /* Set sprite's image */
  void SetImage(ImageAuto img);

  /* Set sprite's image from current scene resource scope. */
  void SetImageByName(const std::string& name);

  /**
   * 0: Don't use blending (always 100% alpha)
   * 1: Use basic alpha blending (Default)
   * 2: Use color blending instead of alpha channel
   */
  void SetBlend(int blend_mode);

  /**
   * Not only image but also copy all sprite information from other sprite.
   * (But it does not copy Tween information)
   *
   * @depreciated
   * We should not try to use this function. Use copy constructor instead.
   */
  void LoadSprite(const Sprite& spr);

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

  virtual void Load();

protected:
  ImageAuto img_;

  // sprite animation property
  // divx : sprite division by x pos
  // divy : sprite division by y pos
  // cnt : total frame (smaller than divx * divy)
  // interval : loop time for sprite animation (milisecond)
  int divx_, divy_, cnt_, interval_;

  // current sprite animation frame
  int idx_;

  // eclipsed time of sprite animation
  int eclipsed_time_;

  // current sprite blending
  int blending_;

  // texture coordination
  float sx_, sy_, sw_, sh_;

  // texture attribute (TODO)
  float tex_attribute_;

  virtual void doUpdate(float delta);
  virtual void doRender();
};

/* Sprite may not need to be shared. */
using SpriteAuto = std::unique_ptr<Sprite>;

}