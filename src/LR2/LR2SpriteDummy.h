#pragma once

#include "BaseObject.h"

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
class LR2SpriteDummy : public BaseObject
{
public:
  LR2SpriteDummy();
};

}