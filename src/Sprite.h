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

  /* Set sprite's image by path */
  void SetImageByPath(const std::string &path);

  /* Set sprite's image by prefetch image.
   * @warn The image is not owned by this sprite,
   * so be aware of texture leaking. */
  void SetImage(Image *img);
  
  void ReplaySprite();
  const Rect& GetImageCoordRect();
  const RectF& GetTextureCoordRect();
  const SpriteAnimationInfo& GetSpriteAnimationInfo();

  void GetVertexInfoOfFrame(VertexInfo* vi, size_t frame);

  /* Set sprite animation frame by number manually.
   * @warn This function won't work
   * when sprite duration is bigger than zero. */
  virtual void SetNumber(int number);
  virtual void Refresh();

  Image *image();

  /* @brief Load property(resource). */
  virtual void Load(const Metric &metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);

protected:
  Image *img_;
  bool img_owned_;

  SpriteAnimationInfo ani_info_;

  // current sprite animation frame
  int idx_;

  // eclipsed time of sprite animation
  int eclipsed_time_;

  // texture coordination source
  RectF texcoord_;

  // source sprite size spec
  Rect imgcoord_;

  // texture attribute (TODO)
  float tex_attribute_;

  virtual void doUpdate(float delta);
  virtual void doRender();

  virtual const CommandFnMap& GetCommandFnMap();
};

/* Sprite may not need to be shared. */
using SpriteAuto = std::unique_ptr<Sprite>;

}