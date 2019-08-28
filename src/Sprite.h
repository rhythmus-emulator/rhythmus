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

  void SetImage(ImageAuto img);

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

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

  // texture coordination of original
  float sx_, sy_, sw_, sh_;

  // texture attribute (TODO)
  float tex_attribute_;

  // TODO: should be removed later.
  VertexInfo vi_[4];

  virtual void doUpdate(float delta);
  virtual void doRender();
};

/* Sprite may not need to be shared. */
using SpriteAuto = std::unique_ptr<Sprite>;

}