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

#define RENDER_WITH_HLSL 1
int errorcode_;
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


// -------------------------------------------------------------- class Graphic

void Graphic::CreateGraphic()
{
  // TODO: selectable graphic engine from option.
  GRAPHIC = new GraphicGL();
}

void Graphic::Cleanup()
{
  delete GRAPHIC;
  GRAPHIC = nullptr;
}

Graphic::Graphic()
  : frame_delay_weight_avg_(1000.0f), last_render_time_(.0), is_game_running_(true)
{
  video_mode_.bpp = 32;
  video_mode_.width = 1280;
  video_mode_.height = 800;
  video_mode_.vsync = true;
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

int Graphic::width() const { return video_mode_.height; }
int Graphic::height() const { return video_mode_.width; }

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
  ASSERT(mat_world_.size() > 1);
  mat_world_.pop_back();
}

void Graphic::Translate(float x, float y, float z)
{
  Translate(Vector3{ x, y, z });
}

void Graphic::Translate(const Vector3 &v)
{
  glm::translate(mat_world_.back(), v);
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
  glm::scale(mat_world_.back(), v);
}

void Graphic::Rotate(float x, float y, float z)
{
  Rotate(Vector3{ x, y, z });
}

void Graphic::Rotate(const Vector3 &v)
{
  glm::rotate(mat_world_.back(), 1.0f, v);
}

void Graphic::RotateLocal(float x, float y, float z)
{
  RotateLocal(Vector3{ x, y, z });
}

void Graphic::RotateLocal(const Vector3 &v)
{
  Matrix &m = mat_world_.back();
  m = glm::rotate(Matrix(1.0f), 1.0f, v) * m;
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
  ASSERT(mat_tex_.size() > 1);
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
  ASSERT(mat_proj_.size() > 1);
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
#if 0
  m_projection_ = glm::perspective(
    60.0f,
    (float)Game::getInstance().GetAspect(),
    1.0f, 2000.0f
  );
#endif

  //const float fWidth = width_;
  //const float fHeight = height_;
  //const float fVanishPointX = cx;
  //const float fVanishPointY = cy;
  //float fovRadians = glm::radians(30.0f);
  //float theta = fovRadians / 2;
  //float fDistCameraFromImage = fWidth / 2 / std::tan(theta);
  //float fDistCameraFromImage = 1000.0f;

  if (fOV == 0)
  {
    CameraLoadOrtho(w, h);
  }
  else
  {
    fOV = fOV < 0.1f ? 0.1f : (fOV > 179.0f ? 179.0f : fOV);
    float theta = fOV / 180.f * 3.1417 / 2;
    float fDistCamera = std::tan(theta);
    Matrix &m = mat_proj_.back();

    m = glm::frustum(
      (vanishx - w / 2) / fDistCamera,
      (vanishx + w / 2) / fDistCamera,
      (vanishy + h / 2) / fDistCamera,
      (vanishy - h / 2) / fDistCamera,
      1.0f, fDistCamera + 1000.0f
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
  float time = (float)Timer::SystemTimer().GetDeltaTime() * 1000.0f;
  if (last_render_time_ != 0)
  {
    frame_delay_weight_avg_ =
      frame_delay_weight_avg_ * 0.8 + (time - last_render_time_) * 0.2;
  }
  last_render_time_ = time;
}

void Graphic::EndFrame() {}

float Graphic::GetFPS() const { return 1000.0f / frame_delay_weight_avg_; }

void Graphic::SignalWindowClose()
{
  is_game_running_ = true;
}

bool Graphic::IsWindowShouldClose() const
{
  return !is_game_running_;
}


#if USE_GLEW == 1
// ------------------------------------------------------------ class GraphicGL

// An global variable indicating window width / height
// (not rendering canvas size)
int sWidth, sHeight;

static void error_callback(int error, const char* description)
{
  Logger::Error("Graphic engine Error: %s", description);
}

#endif

GraphicGL::GraphicGL()
  : window_(nullptr),
    blendmode_(-1), use_multi_texture_(true), texunit_(0),
    max_texture_count_(0), max_texture_size_(0)
{
  vi_ = (VertexInfo*)malloc(kVertexMaxSize * sizeof(VertexInfo));
}

GraphicGL::~GraphicGL()
{
  if (window_)
  {
    glfwDestroyWindow(window_);
    window_ = 0;
  }

  glfwTerminate();

  if (vi_)
  {
    free(vi_);
    vi_ = 0;
  }
}

GLFWwindow* GraphicGL::window()
{
  return window_;
}

void GraphicGL::Initialize()
{
  // Load width / height
  VideoModeParams p;
  {
    std::string w, h;
    Split( *PREFERENCE->resolution, 'x', w, h );
    p.width = width_ = atoi(w.c_str());
    p.height = height_ = atoi(h.c_str());
  }

  // create GL context
  SetVideoMode(p);
  ASSERT(window_);

  // create shader
  if (!CompileDefaultShader())
  {
    Logger::Error("Failed to compile shader.");
  }

  // fetch params
  gl_vendor_ = (const char*)glGetString(GL_VENDOR);
  gl_renderer_ = (const char*)glGetString(GL_RENDERER);
  gl_version_ = (const char*)glGetString(GL_VERSION);
  glu_version_ = (const char*)glGetString(GLU_VERSION);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size_);
  if (use_multi_texture_)
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, (GLint *)&max_texture_count_);
  else
    max_texture_count_ = 1;
}

bool GraphicGL::TryVideoMode(const VideoModeParams &p)
{
  // Destroy previous window if exists...
  if (window_)
  {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }

  // create new window and swap context
  GLFWwindow *w = glfwCreateWindow(p.width, p.height, "TEST", 0, NULL);
  if (!w)
    return false;

  glfwSetErrorCallback(error_callback);
  glfwMakeContextCurrent(w);
  glfwSwapInterval(p.vsync);

  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    Logger::Error("glewInit() failed : %s", glewGetErrorString(err));
  }

  window_ = w;
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

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
    GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)p);

  /* do not render outside texture, clamp it. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  /* prevent font mumbling when minimized, prevent cracking when magnified. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  return texid;
}

void GraphicGL::UpdateTexture(unsigned tex_id, const uint8_t *p,
  unsigned xoffset, unsigned yoffset, unsigned width, unsigned height)
{
  glBindTexture(GL_TEXTURE_2D, tex_id);
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
  //  GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)p);
  glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height,
    GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)p);
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
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, tex_id);
    }
    else
    {
      glDisable(GL_TEXTURE_2D);
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
#if RENDER_WITH_HLSL
  ASSERT(count < 4);
  
  // pre-process for special blendmode
  // - See SetBlendMode() function for detail.
  VertexInfo vi_tmp[4];
  if (blendmode_ == 0)
  {
    for (int i = 0; i < count; ++i)
    {
      vi_tmp[i].c = vi[i].c;
      vi_tmp[i].p = vi[i].p;
      vi_tmp[i].t = vi[i].t;
      vi_tmp[i].c.a = 1.0f;
    }
    vi = vi_tmp;
  }

  // set model matrix
  // TODO: set texture matrix

  // write buffer
  glBindBuffer(GL_ARRAY_BUFFER, quad_shader_.buffer_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * count, nullptr, GL_STREAM_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VertexInfo) * count, vi);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // read buffer
  glDrawArrays(GL_QUADS, 0, count);

#else
  glBegin(GL_QUADS);
  glTexCoord2d(vi[0].sx, vi[0].sy);   // TL
  glVertex3f(vi[0].x, vi[0].y, 0.0f);
  glTexCoord2d(vi[1].sx, vi[1].sy);   // TR
  glVertex3f(vi[1].x, vi[1].y, 0.0f);
  glTexCoord2d(vi[2].sx, vi[2].sy);   // BR
  glVertex3f(vi[2].x, vi[2].y, 0.0f);
  glTexCoord2d(vi[3].sx, vi[3].sy);   // BL
  glVertex3f(vi[3].x, vi[3].y, 0.0f);
  glEnd();
#endif
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
  CameraLoadPerspective(30, vp.width, vp.height, vp.width / 2, vp.height / 2);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(&GetProjectionMatrix()[0][0]);

  Matrix modelView = GetViewMatrix() * GetWorldMatrix();
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(&modelView[0][0]);

  // set viewport
  glViewport(0, 0, sWidth, sHeight);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if RENDER_WITH_HLSL
  // set shader
  glUseProgram(quad_shader_.prog_id);
  glBindVertexArray(quad_shader_.VAO_id);
#endif
}

void GraphicGL::EndFrame()
{
  glFlush();
  glfwSwapBuffers(window_);
  glfwPollEvents();

  ResetMatrix();
}

void GraphicGL::SignalWindowClose()
{
  glfwSetWindowShouldClose(window_, 1);
}

bool GraphicGL::IsWindowShouldClose() const
{
  return glfwWindowShouldClose(window_) != 0;
}

void GraphicGL::CenterWindow()
{
  int totalMonitor;
  GLFWmonitor** monitors = glfwGetMonitors(&totalMonitor);
  GLFWmonitor* monitor;
  if (!totalMonitor || !monitors) return;
  monitor = monitors[0];

  const GLFWvidmode *mode = glfwGetVideoMode(monitor);
  if (!mode)
    return;

  int monitorX, monitorY;
  glfwGetMonitorPos(monitor, &monitorX, &monitorY);

  int windowWidth, windowHeight;
  glfwGetWindowSize(window_, &windowWidth, &windowHeight);

  glfwSetWindowPos(window_,
    monitorX + (mode->width - windowWidth) / 2,
    monitorY + (mode->height - windowHeight) / 2);
}

/* Returns single quad vertex buffer. */
VertexInfo* GraphicGL::get_vertex_buffer()
{
  return get_vertex_buffer(1);
}

VertexInfo* GraphicGL::get_vertex_buffer(int size)
{
  int _ = vi_idx_;
  vi_idx_ += size;
  if (vi_idx_ >= kVertexMaxSize)
  {
    ASSERT(size < kVertexMaxSize);
    // prevent vertex buffer overflow
    DrawQuads(vi_, size);
    // now flushed buffer. clear vertex index.
    vi_idx_ = 0;
    return get_vertex_buffer(size);
  }
  return vi_ + _;
}

#if RENDER_WITH_HLSL
bool GraphicGL::CompileDefaultShader()
{
  memset(&quad_shader_, 0, sizeof(ShaderInfo));

  quad_shader_.vertex_shader =
    "#version 330 core\n"
    "#extension GL_ARB_explicit_uniform_location : require\n"
    "in vec3 positionAttribute;"
    "in vec4 colorAttribute;"
    "in vec2 texCoordinate;"
    "out vec4 passColor;"
    "out vec2 passTextureCoord;"
    "layout (location=0) uniform mat4 projection;"
    "layout (location=1) uniform mat4 view;"
    "layout (location=2) uniform mat4 model;"
    ""
    "void main()"
    "{"
    "gl_Position = projection * view * model * vec4(positionAttribute, 1.0);"
    "passColor = colorAttribute;"
    "passTextureCoord = texCoordinate;"
    "}";

  quad_shader_.frag_shader =
    "#version 330 core\n"
    "in vec4 passColor;"
    "in vec2 passTextureCoord;"
    "uniform sampler2D tex;"
    "out vec4 fragmentColor;"
    "void main()"
    "{"
    "fragmentColor = texture(tex, passTextureCoord) * passColor;"
    "}";

  if (!CompileShaderInfo(quad_shader_))
  {
    return false;
  }

  return true;
}

bool GraphicGL::CompileShaderInfo(ShaderInfo& shader)
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
    std::cerr << "ERROR: vertex shader compile failed : " << errorLog << std::endl;
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
    std::cerr << "ERROR: fragment shader compile failed : " << errorLog << std::endl;
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
    std::cerr << "ERROR: fragment shader compile failed : " << errorLog << std::endl;
    return false;
  }

  // Prepare VAOs with buffer orphaning method.
  glGenVertexArrays(1, &shader.VAO_id);
  glGenBuffers(1, &shader.buffer_id);
  ASSERT(shader.VAO_id && shader.buffer_id);

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

#else
bool Graphic::CompileShader()
{
  return false;
}

bool Graphic::CompileShaderInfo(ShaderInfo& shader)
{
  return false;
}
#endif

}
