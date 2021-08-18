#pragma once
#include "BaseObject.h"
#include "Image.h"
#include <vector>
#include <string>

namespace rhythmus
{

class LR2FnArgs;

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
  Sprite(const Sprite& spr);
  virtual ~Sprite();
  virtual BaseObject *clone();

  virtual void Load(const MetricGroup &metric);
  virtual void OnReady();
  using BaseObject::RunCommand;
  virtual void RunCommand(const std::string& command, const LR2FnArgs& args);

  /* Set sprite's image by path */
  void SetImage(const std::string &path);

  Image *image();
  
  void SetBlending(int blend);
  void SetFiltering(int filtering);
  void SetImageCoord(const Rect &r);
  void SetTextureCoord(const Rect &r);
  void SetAnimatedTexture(int divx, int divy, int duration);
  void ReplaySprite();
  const SpriteAnimationInfo& GetSpriteAnimationInfo();

  /* Set sprite animation frame by number manually.
   * @warn This function won't work
   * when sprite duration is bigger than zero. */
  virtual void SetNumber(int number);
  void SetResourceId(const std::string &res_id);
  virtual void Refresh();

  /* Set animated sprite duration.
   * If zero, sprite won't be animated but changed by value. */
  void SetDuration(int milisecond);

  virtual const char* type() const;
  virtual std::string toString() const;

protected:
  Image *img_;

  SpriteAnimationInfo sprani_;

  std::string path_;

  // current sprite time
  double time_;

  // current sprite frame
  int frame_;

  // resource id
  int *res_id_;

  // blending mode
  int blending_;

  // filtering
  int filtering_;

  // texture coord
  Rect texcoord_;

  // use coord by texture pos or pixel
  bool use_texture_coord_;

  // texture attribute (TODO)
  float tex_attribute_;

  virtual void doUpdate(double);
  virtual void doRender();
};

}
