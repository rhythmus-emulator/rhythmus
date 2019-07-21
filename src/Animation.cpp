#include "Animation.h"

namespace rhythmus
{

SpriteAnimation::SpriteAnimation()
{
}

SpriteAnimation::~SpriteAnimation()
{
}

bool SpriteAnimation::IsActive()
{
  return false;
}

void SpriteAnimation::Tick(int delta_ms)
{
}

void SpriteAnimation::GetFrame(SpriteFrame& f) const
{
}


}