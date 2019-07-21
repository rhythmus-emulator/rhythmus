#include "Sprite.h"
#include "Game.h"
#include <iostream>

#define RENDER_WITH_HLSL 1

namespace rhythmus
{

Sprite::Sprite()
{
  memset(&frame_, 0, sizeof(frame_));
  frame_.src.w = frame_.src.h = -1;
  memset(&rframe_, 0, sizeof(rframe_));
  memset(&hlsl_vert_, 0, sizeof(hlsl_vert_));
}

Sprite::~Sprite()
{
}

SpriteAnimation& Sprite::get_animation()
{
  return ani_;
}

void Sprite::SetImage(ImageAuto img)
{
  img_ = img;

#if RENDER_WITH_HLSL
  /* Generate HLSL */
  hlsl_ = "#version 330 core\n"
		"in vec3 positionAttribute;"
		"in vec3 colorAttribute;"
		"out vec3 passColorAttribute;"
		"void main()"
		"{"
		"gl_Position = vec4(positionAttribute, 1.0);"
    "passColorAttribute = colorAttribute;"
    "}";

  hlsl_vert_[0].r = hlsl_vert_[0].g = hlsl_vert_[0].b = hlsl_vert_[0].a = 1.0f;
  hlsl_vert_[1].r = hlsl_vert_[1].g = hlsl_vert_[1].b = hlsl_vert_[1].a = 1.0f;
  hlsl_vert_[2].r = hlsl_vert_[2].g = hlsl_vert_[2].b = hlsl_vert_[2].a = 1.0f;
  hlsl_vert_[3].r = hlsl_vert_[3].g = hlsl_vert_[3].b = hlsl_vert_[3].a = 1.0f;

  int errcode = 0;
  ASSERT_GL_VAL(errcode);

  /**
   * Add buffer (VBO)
   */
  glGenBuffers(1, &vertex_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(hlsl_vert_), hlsl_vert_, GL_STATIC_DRAW);
  ASSERT_GL_VAL(errcode);

  /**
   * Compile shader & VAO
   * TODO: deleting shader ...?
   * VAO is dependent to VBO so should be called every time.
   */
  static bool is_shader_compiled = false;
  static GLuint vprog = 0;
  if (!is_shader_compiled)
  {
    GLint result;

    // shader compile
    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    ASSERT(vshader);
    glShaderSource(vshader, 1, &hlsl_, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
      char errorLog[512];
      glGetShaderInfoLog(vshader, 512, NULL, errorLog);
      std::cerr << "ERROR: fragment shader compile failed : " << errorLog << std::endl;
      return;
    }

    // shader linking
    vprog = glCreateProgram();
    glAttachShader(vprog, vshader);
    glLinkProgram(vprog);
    glDeleteShader(vshader);
    glGetProgramiv(vprog, GL_LINK_STATUS, &result);
    if (!result)
    {
      char errorLog[512];
      glGetShaderInfoLog(vshader, 512, NULL, errorLog);
      std::cerr << "ERROR: fragment shader compile failed : " << errorLog << std::endl;
      return;
    }

    is_shader_compiled = true;
  }
  vertex_prog_ = vprog;

  // VAO begin
  glGenVertexArrays(1, &vertex_array_);
  glBindVertexArray(vertex_array_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

  GLint vpossrc = glGetAttribLocation(vertex_prog_, "positionAttribute");
  if (vpossrc == -1)
  {
    std::cerr << "Sprite: glGetAttribLocation (positionAttribute) failed." << std::endl;
  } else
  {
    glVertexAttribPointer(vpossrc, 3, GL_FLOAT, GL_FALSE, sizeof(hlsl_vert_s), (void*)offsetof(hlsl_vert_s, x));
    glEnableVertexAttribArray(vpossrc);
    ASSERT_GL_VAL(errcode);
  }

  GLint vclr = glGetAttribLocation(vertex_prog_, "colorAttribute");
  if (vclr == -1)
  {
    std::cerr << "Sprite: glGetAttribLocation (colorAttribute) failed." << std::endl;
  } else
  {
    glVertexAttribPointer(vclr, 4, GL_FLOAT, GL_FALSE, sizeof(hlsl_vert_s), (void*)offsetof(hlsl_vert_s, r));
    glEnableVertexAttribArray(vclr);
    ASSERT_GL_VAL(errcode);
  }

  glBindVertexArray(0);
  // VAO end

#endif
}

void Sprite::SetPos(int x, int y)
{
  frame_.dstpos.x = x;
  frame_.dstpos.y = y;
  InvalidateRFrame();
}

void Sprite::SetSize(int w, int h)
{
  frame_.dst.w = w;
  frame_.dst.h = h;
  InvalidateRFrame();
}

// TODO: optimize with SSE
void Sprite::InvalidateRFrame()
{
  const float ww = Game::getInstance().get_window_width();
  const float wh = Game::getInstance().get_window_height();
  const float iw = img_->get_width();
  const float ih = img_->get_height();

  rframe_.sx = frame_.srcpos.x / iw;
  rframe_.sy = frame_.srcpos.y / ih;
  rframe_.sx2 = rframe_.sx + frame_.src.w / iw;
  rframe_.sy2 = rframe_.sy + frame_.src.h / ih;
  rframe_.dx = frame_.dstpos.x / ww;
  rframe_.dy = frame_.dstpos.y / wh;
  rframe_.dx2 = rframe_.dx + frame_.dst.w / ww;
  rframe_.dy2 = rframe_.dy + frame_.dst.h / wh;

  if (frame_.src.w == -1) rframe_.sx2 = 1.0;
  if (frame_.src.h == -1) rframe_.sy2 = 1.0;

  // apply to HLSL
#if RENDER_WITH_HLSL
  hlsl_vert_[0].x = rframe_.dx;
  hlsl_vert_[0].y = rframe_.dy;
  hlsl_vert_[0].z = 0;
  hlsl_vert_[0].sx = rframe_.sx;
  hlsl_vert_[0].sy = rframe_.sy;

  hlsl_vert_[1].x = rframe_.dx2;
  hlsl_vert_[1].y = rframe_.dy;
  hlsl_vert_[1].z = 0;
  hlsl_vert_[1].sx = rframe_.sx2;
  hlsl_vert_[1].sy = rframe_.sy;

  hlsl_vert_[2].x = rframe_.dx2;
  hlsl_vert_[2].y = rframe_.dy2;
  hlsl_vert_[2].z = 0;
  hlsl_vert_[2].sx = rframe_.sx2;
  hlsl_vert_[2].sy = rframe_.sy2;

  hlsl_vert_[3].x = rframe_.dx;
  hlsl_vert_[3].y = rframe_.dy2;
  hlsl_vert_[3].z = 0;
  hlsl_vert_[3].sx = rframe_.sx;
  hlsl_vert_[3].sy = rframe_.sy2;
#endif
}

void Sprite::Render()
{
  // check animation is now in active
  // if active, call Tick() and invalidate new frame
  if (ani_.IsActive())
  {
    ani_.Tick(0); //  TODO: implement timer
    ani_.GetFrame(frame_);
    InvalidateRFrame();
  }

  // render with given frame
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, img_->get_texture_ID());

#if RENDER_WITH_HLSL
  glUseProgram(vertex_prog_);
  glBindVertexArray(vertex_array_);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
  RenderDepreciated();
#endif
}

void Sprite::RenderDepreciated()
{
  // XXX: Render with depreciated method using glBegin ~ glEnd.
  //      Just for some test purpose ...
  //      Some rendering details might not work! (e.g. color blending)

  glBegin(GL_QUADS);
  glTexCoord2d(rframe_.sx, rframe_.sy);   // TL
  glVertex3f(rframe_.dx, rframe_.dy, 0.0f);
  glTexCoord2d(rframe_.sx2, rframe_.sy);  // TR
  glVertex3f(rframe_.dx2, rframe_.dy, 0.0f);
  glTexCoord2d(rframe_.sx2, rframe_.sy2); // BR
  glVertex3f(rframe_.dx2, rframe_.dy2, 0.0f);
  glTexCoord2d(rframe_.sx, rframe_.sy2);  // BL
  glVertex3f(rframe_.dx, rframe_.dy2, 0.0f);
  glEnd();
}

}