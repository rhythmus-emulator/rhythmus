#include "Quad.h"
#include "common.h"

namespace rhythmus
{

Quad::Quad()
{
  VertexInfo *vi = vi_;
  vi[0] = { 0, 0, 0.1f, 0, 0, 1, 1, 1, 1 };
  vi[1] = { 0, 0, 0.1f, 0, 0, 1, 1, 1, 1 };
  vi[2] = { 0, 0, 0.1f, 0, 0, 1, 1, 1, 1 };
  vi[3] = { 0, 0, 0.1f, 0, 0, 1, 1, 1, 1 };
}

Quad::~Quad() { }

void Quad::SetAlpha(float a)
{
  vi_[0].a =
    vi_[1].a =
    vi_[2].a =
    vi_[3].a = a;
}

void Quad::SetAlpha(const RectF &alpha)
{
  vi_[0].a = alpha.x1;
  vi_[1].a = alpha.y1;
  vi_[2].a = alpha.x2;
  vi_[3].a = alpha.y2;
}

void Quad::doUpdate(float delta) { }

void Quad::doRender()
{
  Graphic::SetTextureId(0);
  Graphic::SetBlendMode(1);
  glColor3f(0, 0, 0);
  memcpy(Graphic::get_vertex_buffer(), vi_, sizeof(VertexInfo) * 4);
  Graphic::RenderQuad();
}

}
