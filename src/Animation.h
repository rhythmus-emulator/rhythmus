#pragma once

#include <vector>

namespace rhythmus
{

struct Position
{
  int x, y;
};

struct Size
{
  int w, h;
};

struct SpriteFrame
{
  Position srcpos;
  Size src;
  Position dstpos;
  Size dst;
};

class SpriteAnimation
{
public:
  SpriteAnimation();
  ~SpriteAnimation();
  bool IsActive();
  void Tick(int delta_ms);
  void GetFrame(SpriteFrame& f) const;
private:
  std::vector<SpriteFrame> motions_;
};

}