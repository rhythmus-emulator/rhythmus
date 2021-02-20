#include "Graphic.h"
#include "Game.h"
#include "Setting.h"
#include "Util.h"
#include "SceneManager.h"
#include "Timer.h"
#include "Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include <memory.h>

#if USE_GLEW
  #if USE_GLFW == 0
    #error GLEW only usable when GLFW is used.
  #endif
  #define GLEW_STATIC
  #include <GL/glew.h>
#endif

#if USE_GLFW
  #include <GLFW/glfw3.h>
#endif

#define PERSPECTIVE_ANGLE 30

constexpr int kVertexMaxSize = 1024 * 4;

namespace rhythmus
{

Graphic *GRAPHIC = nullptr;

void RectF::set_points(float x1, float y1, float x2, float y2)
{
  this->x1 = x1;
  this->y1 = y1;
  this->x2 = x2;
  this->y2 = y2;
}

void RectF::set_rect(float x, float y, float w, float h)
{
  this->x1 = x;
  this->y1 = y;
  this->x2 = x1 + w;
  this->y2 = y1 + h;
}

void RectF::set_width(float w) { x2 = x1 + w; }
void RectF::set_height(float h) { y2 = y1 + h; }
float RectF::width() const { return x2 - x1; }
float RectF::height() const { return y2 - y1; }

float GetWidth(const Rect &r) { return r.z - r.x; }
float GetHeight(const Rect &r) { return r.w - r.y; }
void SetWidth(Rect &r, float width) { r.z = r.x + width; }
void SetHeight(Rect &r, float height) { r.w = r.y + height; }

void ResetVertexInfo(VertexInfo &v)
{
  v.p.x = v.p.y = v.p.z = v.t.x = v.t.y = 0.f;
  v.c.a = v.c.r = v.c.g = v.c.b = 1.f;
}


uint32_t HexStringToColor(const char *p)
{
  if (p[0] == '#') return HexStringToColor(p + 1);

  uint32_t r = 0;
  while (*p)
  {
    r <<= 4;
    if (*p >= 'a' && *p <= 'f')
      r += *p - 'a' + 10;
    else if (*p >= 'A' && *p <= 'F')
      r += *p - 'A' + 10;
    else if (*p >= '0' && *p <= '9')
      r += *p - '0';
    ++p;
  }
  return r;
}

uint32_t HexStringToColor(const std::string &s)
{
  return HexStringToColor(s.c_str());
}

void FillColorFromString(Vector4 &color, const char* s)
{
  uint32_t c = HexStringToColor(s);
  color.b = (c & 0xFF) / 255.0f;
  color.g = ((c >> 8) & 0xFF) / 255.0f;
  color.r = ((c >> 16) & 0xFF) / 255.0f;
  color.a = ((c >> 24) & 0xFF) / 255.0f;
}

void FillColorFromString(Vector4 &color, const std::string &s)
{
  FillColorFromString(color, s.c_str());
}

void StringSafeAssign(std::string &s, const void *p)
{
  if (p) s = (const char*)p;
}

// -------------------------------------------------------------- class Graphic

void Graphic::CreateGraphic()
{
  // TODO: selectable graphic engine from option.
  GRAPHIC = new GraphicGLShader();
  GRAPHIC->Initialize();
  Logger::Info("Using graphic type: %s", GRAPHIC->name());
}

void Graphic::DeleteGraphic()
{
  GRAPHIC->Cleanup();
  delete GRAPHIC;
  GRAPHIC = nullptr;
}

Graphic::Graphic()
  : frame_delay_weight_avg_(1000.0f), next_render_time_(.0),
    last_render_time_(.0), is_game_running_(true)
{
  video_mode_.bpp = 32;
  video_mode_.width = 1280;
  video_mode_.height = 800;
  video_mode_.vsync = false;
  video_mode_.rate = 60;
  video_mode_.windowed = true;

  // start with identity matrix
  mat_world_.emplace_back(Matrix(1.0f));
  mat_tex_.emplace_back(Matrix(1.0f));
  mat_proj_.emplace_back(Matrix(1.0f));
}

Graphic::~Graphic()
{
}

void Graphic::SetVideoMode(VideoModeParams &p)
{
  if (TryVideoMode(p))
  {
    video_mode_ = p;
    return;
  }

  Logger::Error("Video mode failure: %d x %d, %d bpp",
    video_mode_.width, video_mode_.height, video_mode_.bpp);

  // fallback to queried displayspec
  std::vector<VideoModeParams> params;
  for (auto &pa : params)
    if (p.width == pa.width)
      p = pa;
  if (TryVideoMode(p))
  {
    video_mode_ = p;
    return;
  }

  Logger::Error("Video mode failure (fallback): %d x %d, %d bpp",
    video_mode_.width, video_mode_.height, video_mode_.bpp);

  // fallback to last resolution expected to work.
  p.width = 800;
  p.height = 600;
  p.windowed = false;
  p.bpp = 32;
  if (TryVideoMode(p))
  {
    video_mode_ = p;
    return;
  }

  Logger::Error("Video mode failure (last fallback): %d x %d, %d bpp",
    video_mode_.width, video_mode_.height, video_mode_.bpp);
}

const VideoModeParams &Graphic::GetVideoMode() const
{
  return video_mode_;
}

void Graphic::GetSupportedVideoMode(std::vector<VideoModeParams> &p)
{
}

void Graphic::ResolutionChanged()
{
}

bool Graphic::IsVsyncUpdatable() const
{
  return Timer::GetUncachedSystemTime() > next_render_time_;
}

int Graphic::width() const { return video_mode_.width; }
int Graphic::height() const { return video_mode_.height; }

unsigned Graphic::CreateRenderTarget(unsigned &width, unsigned &height)
{
  return 0;
}

void Graphic::DeleteRenderTarget(unsigned tex_id)
{
  // alias.
  DeleteTexture(tex_id);
}

void Graphic::SetRenderTarget(unsigned id, bool preserveTexture)
{
}

void Graphic::SetDefaultRenderState()
{
  SetBlendMode(kBlendAlpha);
  SetTextureFiltering(0, true);
  SetZWrite(true);
}

void Graphic::PushMatrix()
{
  mat_world_.push_back(mat_world_.back());
}

void Graphic::PopMatrix()
{
  R_ASSERT(mat_world_.size() > 1);
  mat_world_.pop_back();
}

void Graphic::Translate(float x, float y, float z)
{
  Translate(Vector3{ x, y, z });
}

void Graphic::Translate(const Vector3 &v)
{
  Matrix &m = mat_world_.back();
  m = glm::translate(m, v);
}

void Graphic::TranslateLocal(float x, float y, float z)
{
  TranslateLocal(Vector3{ x, y, z });
}

void Graphic::TranslateLocal(const Vector3 &v)
{
  Matrix &m = mat_world_.back();
  m = glm::translate(Matrix(1.0f), v) * m;
}

void Graphic::Scale(float x, float y, float z)
{
  Scale(Vector3{ x, y, z });
}

void Graphic::Scale(const Vector3 &v)
{
  Matrix &m = mat_world_.back();
  m = glm::scale(m, v);
}

void Graphic::Rotate(float x, float y, float z)
{
  Rotate(Vector3{ x, y, z });
}

void Graphic::Rotate(const Vector3 &v)
{
  Matrix &m = mat_world_.back();
  m = glm::rotate(m, glm::length(v), v);
}

void Graphic::RotateLocal(float x, float y, float z)
{
  RotateLocal(Vector3{ x, y, z });
}

void Graphic::RotateLocal(const Vector3 &v)
{
  Matrix &m = mat_world_.back();
  m = glm::rotate(Matrix(1.0f), glm::length(v), v) * m;
}

void Graphic::MultiplyMatrix(const Matrix& mat)
{
  Matrix &m = mat_world_.back();
  m = mat * m;
}

void Graphic::MultiplyMatrixLocal(const Matrix& mat)
{
  Matrix &m = mat_world_.back();
  m = m * mat;
}

const Matrix& Graphic::GetWorldMatrix() const
{
  return mat_world_.back();
}

void Graphic::TexturePushMatrix()
{
  mat_tex_.push_back(mat_tex_.back());
}

void Graphic::TexturePopMatrix()
{
  R_ASSERT(mat_tex_.size() > 1);
  mat_tex_.pop_back();
}

void Graphic::TextureTranslate(float x, float y)
{
  TextureTranslate(Vector2{ x, y });
}

void Graphic::TextureTranslate(const Vector2 &v)
{
  // use local translation
  //glm::translate(mat_tex_.back(), Vector3{ v.x, v.y, .0f });
  Matrix &m = mat_tex_.back();
  m = glm::translate(Matrix(1.0f), Vector3{ v.x, v.y, .0f }) * m;
}

const Matrix& Graphic::GetTextureMatrix() const
{
  return mat_tex_.back();
}

void Graphic::CameraPushMatrix()
{
  mat_proj_.push_back(mat_proj_.back());
}

void Graphic::CameraPopMatrix()
{
  R_ASSERT(mat_proj_.size() > 1);
  mat_proj_.pop_back();
}

void Graphic::CameraLoadOrtho(float w, float h)
{
  Matrix &m = mat_proj_.back();

  m = glm::ortho(
    0.0f, w, h, 0.0f,
    -1000.0f, 1000.0f
  );
  mat_view_ = Matrix(1.0f);
}

void Graphic::CameraLoadPerspective(float fOV, float w, float h, float vanishx, float vanishy)
{
  if (fOV == 0)
  {
    CameraLoadOrtho(w, h);
  }
  else
  {
    fOV = fOV < 0.1f ? 0.1f : (fOV > 179.0f ? 179.0f : fOV);
    float theta = fOV / 180.f * 3.141592f / 2;
    float fDistCamera = std::tan(theta);
    Matrix &m = mat_proj_.back();

    m = glm::frustum(
      (vanishx - w / 2) / fDistCamera,
      (vanishx + w / 2) / fDistCamera,
      (vanishy + h / 2) / fDistCamera,
      (vanishy - h / 2) / fDistCamera,
      1.0f, fDistCamera
    );

    mat_view_ = glm::lookAt(
      glm::vec3{ -vanishx + w / 2, -vanishy + h / 2, fDistCamera },
      glm::vec3{ -vanishx + w / 2, -vanishy + h / 2, 0.0f },
      glm::vec3{ 0.0f, 1.0f, 0.0f }
    );
  }
}

const Matrix& Graphic::GetProjectionMatrix() const
{
  return mat_proj_.back();
}

const Matrix &Graphic::GetViewMatrix() const
{
  return mat_view_;
}

void Graphic::ResetMatrix()
{
  mat_world_.clear();
  mat_world_.emplace_back(Matrix(1.0f));
  mat_proj_.clear();
  mat_proj_.emplace_back(Matrix(1.0f));
  mat_view_ = Matrix(1.0f);
}

void Graphic::DrawQuads(const VertexInfo *vi, unsigned count) {}

void Graphic::DrawQuad(const VertexInfo *vi) { DrawQuads(vi, 4); }

// This method should be called every time to calculate FPS
void Graphic::BeginFrame()
{
  frame_delay_weight_avg_ = frame_delay_weight_avg_ * 0.8f + (float)delta() * 1000.0f * 0.2f;
  last_render_time_ = Timer::GetUncachedSystemTime();

  // remove rare gap of render time (about 20FPS)
  if (next_render_time_ + 0.05 < last_render_time_)
    next_render_time_ = last_render_time_;

  next_render_time_ += 1.0 / GetVideoMode().rate;
}

void Graphic::EndFrame() {}

void Graphic::DrawRect(float x, float y, float w, float h, uint32_t argb)
{
  VertexInfo vi[4];
  memset(vi, 0, sizeof(VertexInfo) * 4);
  vi[0].p.x = x;
  vi[0].p.y = y;
  vi[1].p.x = x + w;
  vi[1].p.y = y;
  vi[2].p.x = x + w;
  vi[2].p.y = y + h;
  vi[3].p.x = x;
  vi[3].p.y = y + h;
  vi[1].t.x = 1.0f;
  vi[2].t.x = 1.0f;
  vi[2].t.y = 1.0f;
  vi[3].t.y = 1.0f;
  for (unsigned i = 0; i < 4; ++i) {
    vi[i].c.a = (argb >> 24) / 255.0f;
    vi[i].c.r = ((argb >> 16) & 0xFF) / 255.0f;
    vi[i].c.g = ((argb >> 8) & 0xFF) / 255.0f;
    vi[i].c.b = (argb & 0xFF) / 255.0f;
  }
  SetTexture(0, 0);
  DrawQuad(vi);
}

float Graphic::GetFPS() const { return 1000.0f / frame_delay_weight_avg_; }

void Graphic::SignalWindowClose()
{
  is_game_running_ = true;
}

bool Graphic::IsWindowShouldClose() const
{
  return !is_game_running_;
}

double Graphic::delta() const
{
  return Timer::SystemTimer().GetTime() - last_render_time_;
}

const char* Graphic::name() { return "Base"; }

#if USE_GLEW == 1
// ------------------------------------------------------------ class GraphicGL

static void error_callback(int error, const char* description)
{
  Logger::Error("Graphic engine Error: %s", description);
}

#endif

GraphicGL::GraphicGL()
  : blendmode_(-1), use_multi_texture_(true), texunit_(0),
    tex_id_(0), def_tex_id_(0), max_texture_count_(0), max_texture_size_(0)
{}

GraphicGL::~GraphicGL()
{}

void GraphicGL::Initialize()
{
  R_ASSERT(glfwInit());

  if (!glfwInit())
  {
    Logger::Error("glfwInit() failed, exit program.");
    exit(EXIT_FAILURE);
  }

  // glfw w/ 2.0
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // Load width / height
  VideoModeParams p = GetVideoMode();
  {
    const PrefValue<std::string> resolution("resolution");
    std::string w, h;
    Split( resolution.get().c_str(), 'x', w, h );
    p.width = atoi(w.c_str());
    p.height = atoi(h.c_str());
  }

  // create GL context
  SetVideoMode(p);
  if (GAME->handler() == nullptr)
  {
    Logger::Error("GLFW window context initialization failed.");
    exit(EXIT_FAILURE);
  }
  GAME->CenterWindow();

  // initialize glew
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    Logger::Error("glewInit() failed : %s", glewGetErrorString(err));
    exit(EXIT_FAILURE);
  }

  // fetch params
  StringSafeAssign(gl_vendor_, glGetString(GL_VENDOR));
  StringSafeAssign(gl_renderer_, glGetString(GL_RENDERER));
  StringSafeAssign(gl_version_, glGetString(GL_VERSION));
  StringSafeAssign(glu_version_, glGetString(GLU_VERSION));
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size_);
  if (use_multi_texture_)
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, (GLint *)&max_texture_count_);
  else
    max_texture_count_ = 1;

  Logger::Info("Graphic info: %s, %s, %s, %s, multitexture?(%d)",
    gl_vendor_.c_str(),
    gl_renderer_.c_str(),
    gl_version_.c_str(),
    glu_version_.c_str(), max_texture_count_);

  // create default texture, which is used for rendering 'NON-TEXTURE' state.
  CreateDefaultTexture();
  if (!def_tex_id_)
  {
    Logger::Error("Failed to create default texture.");
  }
}

void GraphicGL::Cleanup()
{
  // TODO: destroy shader, program.
}

bool GraphicGL::TryVideoMode(VideoModeParams &p)
{
  GLFWwindow *w = (GLFWwindow*)GAME->handler();
  int totalMonitor;
  GLFWmonitor** monitors;

  // Destroy previous window if exists...
  if (w)
  {
    glfwDestroyWindow(w);
    w = nullptr;
  }

  // create new window and swap context
  monitors = glfwGetMonitors(&totalMonitor);
  if (p.windowed)
  {
    w = glfwCreateWindow(p.width, p.height,
      GAME->get_window_title().c_str(), 0, NULL);
  }
  else
  {
    R_ASSERT(totalMonitor > 0);
    w = glfwCreateWindow(p.width, p.height,
      GAME->get_window_title().c_str(), monitors[0], NULL);
  }
  const GLFWvidmode* mode = glfwGetVideoMode(monitors[0]);
  p.rate = mode->refreshRate;

  glfwSetErrorCallback(error_callback);
  glfwMakeContextCurrent(w);
  glfwSwapInterval(p.vsync ? 1 : 0);

  GAME->set_handler(w);
  return true;
}

void GraphicGL::GetSupportedVideoMode(std::vector<VideoModeParams> &out)
{
  int totalMonitor;
  GLFWmonitor** monitors = glfwGetMonitors(&totalMonitor);
  for (int currMonitor = 0; currMonitor < totalMonitor; currMonitor++)
  {
    //printf("\n monitor name: [%s]", glfwGetMonitorName(monitors[currMonitor]));
    int count;
    const GLFWvidmode* modes = glfwGetVideoModes(monitors[currMonitor], &count);
    for (int i = 0; i < count; i++)
    {
      // ignore refresh rate
      VideoModeParams p;
      p.bpp = modes[i].redBits + modes[i].blueBits + modes[i].greenBits;
      p.rate = modes[i].refreshRate;
      p.width = modes[i].width;
      p.height = modes[i].height;

      out.push_back(p);
    }
  }
}

unsigned GraphicGL::CreateTexture(const uint8_t *p, unsigned width, unsigned height)
{
  unsigned texid;
  glGenTextures(1, &texid);
  if (texid == 0)
  {
    GLenum err = glGetError();
    Logger::Error("Allocating textureID failed: %d", (int)err);
    return 0;
  }

  glBindTexture(GL_TEXTURE_2D, texid);

  /* do not render outside texture, clamp it. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  /* prevent font mumbling when minimized, prevent cracking when magnified. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
    GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)p);

  return texid;
}

void GraphicGL::UpdateTexture(unsigned tex_id, const uint8_t *p,
  unsigned xoffset, unsigned yoffset, unsigned width, unsigned height)
{
  glBindTexture(GL_TEXTURE_2D, tex_id);
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
  //  GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)p);
  glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height,
    GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)p);
}

void GraphicGL::DeleteTexture(unsigned tex_id)
{
  glDeleteTextures(1, &tex_id);
}

void GraphicGL::ClearAllTextures()
{
  for (unsigned i = 0; i < max_texture_count_; ++i)
    SetTexture(i, 0);
  SetTextureUnit(0);
}

unsigned GraphicGL::GetNumTextureUnits()
{
  return max_texture_count_;
}

unsigned GraphicGL::CreateRenderTarget(unsigned &w, unsigned &h)
{
  // TODO: not fully implemented.
  GLuint fbo_id, rbo_id, fbo_tex_id;

  glGenFramebuffers(1, &fbo_id);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);

  glGenTextures(1, &fbo_tex_id);
  glBindTexture(GL_TEXTURE_2D, fbo_tex_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex_id, 0);

  glGenRenderbuffers(1, &rbo_id);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_id);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return fbo_tex_id;
}

Image *GraphicGL::CreateScreenShot()
{
  // TODO
  return nullptr;
}

Image *GraphicGL::GetTexture(unsigned tex_id)
{
  // TODO
  return nullptr;
}

void GraphicGL::SetRenderTarget(unsigned id, bool preserveTexture)
{
  // TODO: not fully implemented.
  /* change frame / render buffer to our own FBOs and clear it */
  glBindFramebuffer(GL_FRAMEBUFFER, id);
  glBindRenderbuffer(GL_RENDERBUFFER, id);
  if (!preserveTexture)
  {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
}

void GraphicGL::SetBlendMode(int blend_mode)
{
  static GLuint glBlendmodeTbl[] = {
    GL_ONE_MINUS_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_ONE,
    GL_MINUS_NV,
    GL_MULTIPLY_NV,
    GL_XOR,
    GL_MULTIPLY_NV,
    GL_MULTIPLY_NV,
    0
  };
  GLuint glBlendmode = GL_ONE;
  if (blend_mode >= 0 && blend_mode < BlendMode::kBlendEnd)
    glBlendmode = glBlendmodeTbl[blend_mode];

  // optimization: only call GL function if state is different
  if (blendmode_ != blend_mode)
  {
    blendmode_ = blend_mode;
    glBlendFunc(GL_SRC_ALPHA, glBlendmode);
  }
}

void GraphicGL::SetTexture(unsigned texunit, unsigned tex_id)
{
  if (!SetTextureUnit(texunit))
    return;

  // @warn  This might cause bug... remove later
  if (tex_id_ != tex_id)
  {
    tex_id_ = tex_id;
    if (tex_id)
    {
      //glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, tex_id);
    }
    else
    {
      //glDisable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, def_tex_id_);
    }
  }
}

bool GraphicGL::SetTextureUnit(unsigned texunit)
{
  if (texunit > max_texture_count_)
    return false;
  if (texunit_ != texunit)
  {
    texunit_ = texunit;
    glActiveTextureARB(GL_TEXTURE0_ARB + texunit);
  }
  return true;
}

void GraphicGL::SetTextureMode(unsigned texunit, unsigned mode)
{
  if (!SetTextureUnit(texunit)) return;
}

void GraphicGL::SetTextureFiltering(unsigned texunit, unsigned do_filtering)
{
  if (!SetTextureUnit(texunit)) return;
  if (do_filtering)
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  else
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NONE);
  }
}

void GraphicGL::SetZWrite(bool enable)
{
  glDepthMask(enable);
}

void GraphicGL::DrawQuads(const VertexInfo *vi, unsigned count)
{
  if (count == 0) return;
  R_ASSERT(count >= 4);
  
  // pre-process for special blendmode
  // - See SetBlendMode() function for detail.
  VertexInfo vi_tmp[4];
  if (blendmode_ == 0)
  {
    for (unsigned i = 0; i < count; ++i)
    {
      vi_tmp[i].c = vi[i].c;
      vi_tmp[i].p = vi[i].p;
      vi_tmp[i].t = vi[i].t;
      vi_tmp[i].c.a = 1.0f;
    }
    vi = vi_tmp;
  }

  // set model matrix
  Matrix modelView = GetViewMatrix() * GetWorldMatrix();
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(&modelView[0][0]);

  // TODO: set texture matrix

  glBegin(GL_QUADS);
  glTexCoord2d(vi[0].t.x, vi[0].t.y);   // TL
  glVertex3f(vi[0].p.x, vi[0].p.y, vi[0].p.z);
  glColor4f(vi[0].c.r, vi[0].c.g, vi[0].c.b, vi[0].c.a);
  glTexCoord2d(vi[1].t.x, vi[1].t.y);   // TR
  glVertex3f(vi[1].p.x, vi[1].p.y, vi[1].p.z);
  glColor4f(vi[1].c.r, vi[1].c.g, vi[1].c.b, vi[1].c.a);
  glTexCoord2d(vi[2].t.x, vi[2].t.y);   // BR
  glVertex3f(vi[2].p.x, vi[2].p.y, vi[2].p.z);
  glColor4f(vi[2].c.r, vi[2].c.g, vi[2].c.b, vi[2].c.a);
  glTexCoord2d(vi[3].t.x, vi[3].t.y);   // BL
  glVertex3f(vi[3].p.x, vi[3].p.y, vi[3].p.z);
  glColor4f(vi[3].c.r, vi[3].c.g, vi[3].c.b, vi[3].c.a);
  glEnd();
}

void GraphicGL::BeginFrame()
{
  const VideoModeParams &vp = GetVideoMode();

  // To calculate FPS
  Graphic::BeginFrame();

  SetDefaultRenderState();
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  // expect to set View / Proj matrix here and consider it as constant.
  CameraLoadPerspective(PERSPECTIVE_ANGLE,
    (float)vp.width, (float)vp.height, vp.width / 2.0f, vp.height / 2.0f);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(&GetProjectionMatrix()[0][0]);

  glViewport(0, 0, vp.width, vp.height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
  // test purpose code. big red rect must be drawn to surface.
  glBegin(GL_QUADS);
  glColor3f(1, 0, 0);
  glVertex2f(10, 10);
  glVertex2f(10, 600);
  glVertex2f(1000, 600);
  glVertex2f(1000, 10);
  glEnd();
#endif
}

void GraphicGL::EndFrame()
{
  glFlush();
  glfwSwapBuffers((GLFWwindow*)GAME->handler());
  ResetMatrix();
}

void GraphicGL::ClipViewArea(const Vector4 &area)
{
  // bottomleft is the origin.
  const VideoModeParams &vp = GetVideoMode();
  glEnable(GL_SCISSOR_TEST);
  glScissor((int)area.x, vp.height - (int)area.w, (int)(area.z - area.x), (int)(area.w - area.y));
}

void GraphicGL::ResetViewArea()
{
  glDisable(GL_SCISSOR_TEST);
}

void GraphicGL::SignalWindowClose()
{
  glfwSetWindowShouldClose((GLFWwindow*)GAME->handler(), 1);
}

bool GraphicGL::IsWindowShouldClose() const
{
  return glfwWindowShouldClose((GLFWwindow*)GAME->handler()) != 0;
}

void GraphicGL::CreateDefaultTexture()
{
  uint8_t p[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
  def_tex_id_ = CreateTexture(p, 1, 1);
}

const char* GraphicGL::name() { return "GLBasic"; }

// ------------------------------------------------------------ class GraphicGL

GraphicGLShader::GraphicGLShader()
  : shader_mat_Projection_(-1), shader_mat_ModelView_(-1), vi_idx_(0)
{
  vi_ = (VertexInfo*)malloc(kVertexMaxSize * sizeof(VertexInfo));
}

GraphicGLShader::~GraphicGLShader()
{
  if (vi_)
  {
    free(vi_);
    vi_ = 0;
  }
}

void GraphicGLShader::Initialize()
{
  GraphicGL::Initialize();

  // create shader
  if (!CompileDefaultShader())
  {
    Logger::Error("Failed to compile shader.");
  }
}
bool GraphicGLShader::CompileShaderInfo(ShaderInfo& shader)
{
  GLint result;
  GLuint vertex_shader_id, frag_shader_id;
  char errorLog[512];

  // create vertex shader
  vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader_id, 1, &shader.vertex_shader, NULL);
  glCompileShader(vertex_shader_id);
  glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
  if (!result)
  {
    glGetShaderInfoLog(vertex_shader_id, 512, NULL, errorLog);
    Logger::Error("ERROR: vertex shader compile failed : %s", errorLog);
    return false;
  }

  // create fragment shader
  frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag_shader_id, 1, &shader.frag_shader, NULL);
  glCompileShader(frag_shader_id);
  glGetShaderiv(frag_shader_id, GL_COMPILE_STATUS, &result);
  if (!result)
  {
    glGetShaderInfoLog(frag_shader_id, 512, NULL, errorLog);
    Logger::Error("ERROR: fragment shader compile failed : %s", errorLog);
    return false;
  }

  // link program
  shader.prog_id = glCreateProgram();
  glAttachShader(shader.prog_id, vertex_shader_id);
  glAttachShader(shader.prog_id, frag_shader_id);
  glLinkProgram(shader.prog_id);
  glDeleteShader(vertex_shader_id);
  glDeleteShader(frag_shader_id);
  glGetProgramiv(shader.prog_id, GL_LINK_STATUS, &result);
  if (!result)
  {
    glGetShaderInfoLog(shader.prog_id, 512, NULL, errorLog);
    Logger::Error("ERROR: shader program compile failed : %s", errorLog);
    return false;
  }

  // Prepare VAOs with buffer orphaning method.
  glGenVertexArrays(1, &shader.VAO_id);
  glGenBuffers(1, &shader.buffer_id);
  R_ASSERT(shader.VAO_id && shader.buffer_id);

  glBindVertexArray(shader.VAO_id);
  glBindBuffer(GL_ARRAY_BUFFER, shader.buffer_id);

  //glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * 4, nullptr, GL_STATIC_DRAW);
  // attr: positionAttribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void*)offsetof(VertexInfo, p));
  glEnableVertexAttribArray(0);
  // attr: colorAttribute
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void*)offsetof(VertexInfo, c));
  glEnableVertexAttribArray(1);
  // attr: texCoordinate
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void*)offsetof(VertexInfo, t));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);

  return true;
}

bool GraphicGLShader::CompileDefaultShader()
{
  memset(&quad_shader_, 0, sizeof(ShaderInfo));

  quad_shader_.vertex_shader =
#if USE_GLEW_ES
    "#version 300 es\n"
    "in mediump vec3 positionAttribute;"
    "in mediump vec4 colorAttribute;"
    "in mediump vec2 texCoordinate;"
    "out mediump vec4 passColor;"
    "out mediump vec2 passTextureCoord;"
    "uniform mat4 projection;"
    "uniform mat4 modelview;"
#else
    "#version 330 core\n"
    "in vec3 positionAttribute;"
    "in vec4 colorAttribute;"
    "in vec2 texCoordinate;"
    "out vec4 passColor;"
    "out vec2 passTextureCoord;"
    "uniform mat4 projection;"
    "uniform mat4 modelview;"
#endif
    ""
    "void main()"
    "{"
    "gl_Position = projection * modelview * vec4(positionAttribute, 1.0);"
    "passColor = colorAttribute;"
    "passTextureCoord = texCoordinate;"
    "}";

  quad_shader_.frag_shader =
#if USE_GLEW_ES
    "#version 300 es\n"
    "in mediump vec4 passColor;"
    "in mediump vec2 passTextureCoord;"
    "uniform sampler2D tex;"
    "out mediump vec4 fragmentColor;"
#else
    "#version 330 core\n"
    "in vec4 passColor;"
    "in vec2 passTextureCoord;"
    "uniform sampler2D tex;"
    "out vec4 fragmentColor;"
#endif
    "void main()"
    "{"
    "fragmentColor = texture(tex, passTextureCoord) * passColor;"
    "}";

  if (!CompileShaderInfo(quad_shader_))
  {
    return false;
  }

  shader_mat_Projection_ = glGetUniformLocation(quad_shader_.prog_id, "projection");
  shader_mat_ModelView_ = glGetUniformLocation(quad_shader_.prog_id, "modelview");

  return true;
}

void GraphicGLShader::DrawQuads(const VertexInfo *vi, unsigned count)
{
  if (count == 0) return;
  R_ASSERT(count >= 4);

  // pre-process for special blendmode
  // - See SetBlendMode() function for detail.
  VertexInfo *vi_tmp = nullptr;
  if (blendmode_ == 0)
  {
    vi_tmp = get_vertex_buffer(count);
    for (unsigned i = 0; i < count; ++i)
    {
      vi_tmp[i].c = vi[i].c;
      vi_tmp[i].p = vi[i].p;
      vi_tmp[i].t = vi[i].t;
      vi_tmp[i].c.a = 1.0f;
    }
    vi = vi_tmp;
  }

  // set model matrix
  Matrix modelView = GetViewMatrix() * GetWorldMatrix();
  glUniformMatrix4fv(shader_mat_ModelView_, 1, GL_FALSE, &modelView[0][0]);

  // TODO: set texture matrix

  glBindBuffer(GL_ARRAY_BUFFER, quad_shader_.buffer_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * count, nullptr, GL_STREAM_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VertexInfo) * count, vi);
  glDrawArrays(GL_QUADS, 0, count);
}

void GraphicGLShader::BeginFrame()
{
  const VideoModeParams &vp = GetVideoMode();

  // To calculate FPS
  Graphic::BeginFrame();

  SetDefaultRenderState();
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  // set shader
  glUseProgram(quad_shader_.prog_id);
  glBindVertexArray(quad_shader_.VAO_id);

  // expect to set View / Proj matrix here and consider it as constant.
  CameraLoadPerspective(PERSPECTIVE_ANGLE,
    (float)vp.width, (float)vp.height, vp.width / 2.0f, vp.height / 2.0f);
  glUniformMatrix4fv(shader_mat_Projection_, 1, GL_FALSE, &GetProjectionMatrix()[0][0]);

  glViewport(0, 0, vp.width, vp.height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* Returns single quad vertex buffer. */
VertexInfo* GraphicGLShader::get_vertex_buffer()
{
  return get_vertex_buffer(4);
}

VertexInfo* GraphicGLShader::get_vertex_buffer(int size)
{
  R_ASSERT(size < kVertexMaxSize);
  if (vi_idx_ + size >= kVertexMaxSize) vi_idx_ = 0;
  int _ = vi_idx_;
  vi_idx_ += size;
  return vi_ + _;
}

const char* GraphicGLShader::name() { return "GLwithShader"; }

}
