#pragma once

#include "LR2Sprite.h"

namespace rhythmus
{

/**
 * @brief
 * This class is not renderable,
 * But contains information related to scene/object rendering.
 * (e.g. DST_BAR_BODY -
 *  as such object does not match 1:1 with rendering object
 *  and context required)
 */
class LR2SpriteDummy : public LR2Sprite
{
public:
  /* Set sprite(attr) name */
  void set_name(const std::string& name);

  /* This method actually uploads sprite information to theme param (or Scene state)
   * So, scene must be set(allocated) before this method is called. */
  virtual void SetSpriteFromLR2Data();

  /* As this object doesn't have it's own rendering context,
   * It delegates rendering to pre-set sprite. */
  virtual void Render();
};

}