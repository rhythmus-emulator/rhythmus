#pragma once
#include "BaseObject.h"
#include "Image.h"
#include <vector>
#include <string>

namespace rhythmus
{

/**
 * sprite animation property
 * divx : sprite division by x pos
 * divy : sprite division by y pos
 * cnt : total frame (smaller than divx * divy)
 * interval : loop time for sprite animation (milisecond)
 */
struct SpriteAnimationInfo
{
  int divx, divy, cnt, duration;
};

/* @brief Renderable object with texture */
class Sprite : public BaseObject
{
public:
  Sprite();
  Sprite(const Sprite& spr) = default;
  virtual ~Sprite();

  virtual void Load(const MetricGroup &metric);

  /* Set sprite's image by path */
  void SetImage(const std::string &path);

  Image *image();
  
  void SetBlending(int blend);
  void SetImageCoord(const Rect &r);
  void SetTextureCoord(const Rect &r);
  void Replay();
  const SpriteAnimationInfo& GetSpriteAnimationInfo();

  /* Set sprite animation frame by number manually.
   * @warn This function won't work
   * when sprite duration is bigger than zero. */
  virtual void SetNumber(int number);
  virtual void Refresh();

protected:
  Image *img_;

  SpriteAnimationInfo sprani_;

  // current sprite time
  float time_;

  // current sprite frame
  int frame_;

  // resource id
  int *res_id_;

  // blending mode
  int blending_;

  // texture coord
  Rect texcoord_;

  // use coord by texture pos or pixel
  bool use_texture_coord_;

  // texture attribute (TODO)
  float tex_attribute_;

  virtual void doUpdate(float delta);
  virtual void doRender();
};

/* Sprite may not need to be shared. */
using SpriteAuto = std::unique_ptr<Sprite>;

}