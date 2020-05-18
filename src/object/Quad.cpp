#include "Quad.h"
#include "common.h"

namespace rhythmus
{

Quad::Quad()
{
  VertexInfo *vi = vi_;
  vi_[0].c = Vector4{ 1.0f };
  vi_[1].c = Vector4{ 1.0f };
  vi_[2].c = Vector4{ 1.0f };
  vi_[3].c = Vector4{ 1.0f };
  vi_[0].p = Vector3{ 0.0f, 0.0f, 0.0f };
  vi_[1].p = Vector3{ 0.0f, 0.0f, 0.0f };
  vi_[2].p = Vector3{ 0.0f, 0.0f, 0.0f };
  vi_[3].p = Vector3{ 0.0f, 0.0f, 0.0f };
  vi_[0].t = Vector2{ 0.0f };
  vi_[1].t = Vector2{ 0.0f };
  vi_[2].t = Vector2{ 0.0f };
  vi_[3].t = Vector2{ 0.0f };
}

Quad::~Quad() { }

void Quad::SetAlpha(float a)
{
  vi_[0].c.a =
    vi_[1].c.a =
    vi_[2].c.a =
    vi_[3].c.a = a;
}

void Quad::SetAlpha(const Vector4 &alpha)
{
  vi_[0].c.a = alpha.x;
  vi_[1].c.a = alpha.y;
  vi_[2].c.a = alpha.z;
  vi_[3].c.a = alpha.w;
}

void Quad::doUpdate(float delta) { }

void Quad::doRender()
{
  GRAPHIC->SetTexture(0, 0);
  GRAPHIC->SetBlendMode(1);
  //glColor3f(0, 0, 0);
  GRAPHIC->DrawQuad(vi_);
}

}
