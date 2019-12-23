#pragma once
#include "BaseObject.h"

namespace rhythmus
{

class Quad : public BaseObject
{
public:
  Quad();
  virtual ~Quad();
  void SetAlpha(float alpha);
  void SetAlpha(const RectF &alpha);

private:
  VertexInfo vi_[4];
  RectF alpha_;

  virtual void doUpdate(float delta);
  virtual void doRender();
};

}