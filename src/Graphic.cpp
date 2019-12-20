#include "Graphic.h"
#include "Game.h"
#include "Setting.h"
#include "Util.h"
#include "SceneManager.h"
#include "Timer.h"
#include "Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#define RENDER_WITH_HLSL 1
int errorcode_;
constexpr int kVertexMaxSize = 1024 * 4;

namespace rhythmus
{

void VertexInfo::Clear()
{
  x = y = z = sx = sy = 0.0f;
  r = g = b = a = 1.0f;
}

// An global variable indicating window width / height
int sWidth, sHeight;

static void error_callback(int error, const char* description)
{
  std::cerr << "Error: " << description << std::endl;
}

void on_resize(GLFWwindow* w, GLint width, GLint height)
{
  sWidth = width;
  sHeight = height;
}

GLFWwindow* Graphic::window()
{
  return window_;
}

float Graphic::width() const { return width_; }
float Graphic::height() const { return height_; }
float Graphic::GetAspect() const { return (float)width_ / height_; }

void Graphic::Initialize()
{
  getInstance().InitializeInternal();
}

void Graphic::InitializeInternal()
{
  // set width / height
  {
    std::string w, h;
    Split(
      Setting::GetSystemSetting().GetOption("Resolution")->value<std::string>(),
      'x', w, h
    );
    sWidth = width_ = atoi(w.c_str());
    sHeight = height_ = atoi(h.c_str());
  }

  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
  {
    std::cerr << "Failed to init GLFW." << std::endl;
    exit(EXIT_FAILURE);
  }

  window_ = glfwCreateWindow(sWidth, sHeight,
    Game::get_window_title().c_str(), NULL, NULL
  );
  if (!window_)
  {
    std::cerr << "Failed to create window." << std::endl;
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window_);
  if (glewInit() != GLEW_OK)
  {
    std::cerr << "Failed glewInit()." << std::endl;
    exit(EXIT_FAILURE);
  }

  // Center window
  CenterWindow();

  // Callback function setting (related to graphic)
  glfwSetWindowSizeCallback(window_, on_resize);

  // set rendering context
  current_proj_mode_ = -1;  // no projection mode initially.
  tex_id_ = 0;
  src_blend_mode_ = 0;
  vi_idx_ = 0;
  vi_ = (VertexInfo*)malloc(sizeof(VertexInfo) * kVertexMaxSize);
  ASSERT(vi_);
  m_model_ = glm::mat4(1.0f);   // set identitfy matrix by default

  // GL flag setting
  glClearColor(0, 0, 0, 1);
  glMatrixMode(GL_PROJECTION);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_SRC_ALPHA);
  SetBlendMode(GL_ONE_MINUS_SRC_ALPHA);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if RENDER_WITH_HLSL
  // Compile necessary shaders
  if (!CompileShader())
  {
    std::cerr << "Failed to compile shader." << std::endl;
    exit(EXIT_FAILURE);
  }
#endif

  // Vsync enabled
  glfwSwapInterval(1);
}

void Graphic::Render()
{
  getInstance().RenderInternal();
}

void Graphic::RenderInternal()
{
  glViewport(0, 0, sWidth, sHeight);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  SceneManager::getInstance().Update();
  SceneManager::getInstance().Render();
  Flush();

  glfwSwapBuffers(window_);
  glfwPollEvents();

  /* Flush stdout into log message */
  Logger::getInstance().Flush();
}

void Graphic::Cleanup()
{
  getInstance().CleanupInternal();
}

void Graphic::CleanupInternal()
{
  if (window_)
  {
    glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = 0;
  }

  if (vi_)
  {
    free(vi_);
    vi_ = 0;
  }
}

void Graphic::SignalWindowClose()
{
  glfwSetWindowShouldClose(getInstance().window_, 1);
}

bool Graphic::IsWindowShouldClose()
{
  return glfwWindowShouldClose(getInstance().window_) != 0;
}

Graphic& Graphic::getInstance()
{
  static class Graphic g;
  return g;
}

#if RENDER_WITH_HLSL
bool Graphic::CompileShader()
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

bool Graphic::CompileShaderInfo(ShaderInfo& shader)
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
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void*)offsetof(VertexInfo, x));
  glEnableVertexAttribArray(0);
  // attr: colorAttribute
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void*)offsetof(VertexInfo, r));
  glEnableVertexAttribArray(1);
  // attr: texCoordinate
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexInfo), (void*)offsetof(VertexInfo, sx));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);

  return true;
}

/**
 * @brief Set projection mode as ortho (2D).
 *        No effect if it is already ortho.
 */
void Graphic::SetProjOrtho()
{
  if (current_proj_mode_ == 1)
    return;

  current_proj_mode_ = 1;

  m_projection_ = glm::ortho(
    0.0f, (float)width_, (float)height_, 0.0f,
    -1.0f, 1.0f
  );
  glUseProgram(quad_shader_.prog_id);
  glUniformMatrix4fv(0, 1, GL_FALSE, &m_projection_[0][0]);
  SetView(.0f, .0f, .01f);
}

/*
 * @brief Set projection mode 
 */
void Graphic::SetProjPerspectiveCenter()
{
  SetProjPerspective(width_ / 2.0f,
                     height_ / 2.0f);
}

/**
 * @brief Set projection mode as frustrum (perspective)
 *        cx, cy : Center of the perspective
 */
void Graphic::SetProjPerspective(float cx, float cy)
{
  // force refresh as different cx/cy might come.
  //
  //if (current_proj_mode_ == 2)
  //  return;

  current_proj_mode_ = 2;

#if 0
  m_projection_ = glm::perspective(
    60.0f,
    (float)Game::getInstance().GetAspect(),
    1.0f, 2000.0f
  );
#endif

  const float fWidth = width_;
  const float fHeight = height_;
  const float fVanishPointX = cx;
  const float fVanishPointY = cy;
  //float fovRadians = glm::radians(30.0f);
  //float theta = fovRadians / 2;
  //float fDistCameraFromImage = fWidth / 2 / std::tan(theta);
  float fDistCameraFromImage = 1000.0f;

  m_projection_ = glm::frustum(
    (fVanishPointX - fWidth) / fDistCameraFromImage,
    fVanishPointX / fDistCameraFromImage,
    fVanishPointY / fDistCameraFromImage,
    (fVanishPointY - fHeight) / fDistCameraFromImage,
    1.0f, 2000.0f
  );

  glUseProgram(quad_shader_.prog_id);
  glUniformMatrix4fv(0, 1, GL_FALSE, &m_projection_[0][0]);
  SetView(fVanishPointX, fVanishPointY, fDistCameraFromImage);
}

void Graphic::SetView(float x, float y, float dist)
{
  m_view_ = glm::lookAt(
    glm::vec3{ x, y, dist },
    glm::vec3{ x, y, 0.0f },
    glm::vec3{ 0.0f, 1.0f, 0.0f }
    );

  glUniformMatrix4fv(1, 1, GL_FALSE, &m_view_[0][0]);
}

void Graphic::SetModelIdentity()
{
  m_model_ = glm::mat4(1.0f);

  glUseProgram(quad_shader_.prog_id);
  glUniformMatrix4fv(2, 1, GL_FALSE, &m_model_[0][0]);
}

void Graphic::SetModel(const ProjectionInfo& pi)
{
  if (pi.rotx == 0 && pi.roty == 0 && pi.rotz == 0)
  {
    /* Just do translation */
    m_model_ = glm::translate(m_model_, { pi.x, pi.y, 0.0f });

    /* scale if necessary. */
    if (pi.sx != 1.0f || pi.sy != 1.0f)
      m_model_ = glm::scale(m_model_, { pi.sx, pi.sy, 0.0f });
  }
  else
  {
    // NOTE: matrix is multiplied in reversed order.
    m_model_ = glm::translate(m_model_, { pi.tx + pi.x, pi.ty + pi.y, 0.0f });

    // rotation for each axis
    if (pi.rotx != 0)
      m_model_ = glm::rotate(m_model_, pi.rotx, { 1.0f, 0.0f, 0.0f });
    if (pi.roty != 0)
      m_model_ = glm::rotate(m_model_, pi.roty, { 0.0f, 1.0f, 0.0f });
    if (pi.rotz != 0)
      m_model_ = glm::rotate(m_model_, pi.rotz, { 0.0f, 0.0f, 1.0f });

    /* scale if necessary. */
    if (pi.sx != 1.0f || pi.sy != 1.0f)
      m_model_ = glm::scale(m_model_, { pi.sx, pi.sy, 0.0f });

    // TODO: glm::radians
    m_model_ = glm::translate(m_model_, { -pi.tx, -pi.ty, 0.0f });
  }

  glUseProgram(quad_shader_.prog_id);
  glUniformMatrix4fv(2, 1, GL_FALSE, &m_model_[0][0]);
}

void Graphic::GetResolutions(std::vector<std::string>& out)
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
      std::string s;
      s = std::to_string(modes[i].width) + "x" + std::to_string(modes[i].height);
      if (std::find(out.begin(), out.end(), s) == out.end())
        out.push_back(s);
    }
  }
}

void Graphic::CenterWindow()
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

/**
 * @brief Renders quad with cached vertices.
 */
void Graphic::RenderQuad()
{
  Graphic& g = Graphic::getInstance();
  if (g.vi_idx_ <= 0) return;

  // write buffer
  glBindBuffer(GL_ARRAY_BUFFER, g.quad_shader_.buffer_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * 4 * g.vi_idx_, nullptr, GL_STREAM_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VertexInfo) * 4 * g.vi_idx_, g.vi_);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // read buffer
  glUseProgram(g.quad_shader_.prog_id);
  glBindVertexArray(g.quad_shader_.VAO_id);
  glDrawArrays(GL_QUADS, 0, 4 * g.vi_idx_);

  // now flushed buffer. clear vertex index.
  g.vi_idx_ = 0;
}

void Graphic::SetMatrix(const ProjectionInfo& pi)
{
  Graphic &g = Graphic::getInstance();

  /* Prefer Ortho projection if not rotX / rotY */
  if (pi.rotx == .0f && pi.roty == .0f)
    g.SetProjOrtho();
  else
    g.SetProjPerspectiveCenter();

  g.SetModel(pi);
}

void Graphic::PushMatrix()
{
  Graphic &g = Graphic::getInstance();
  g.m_model_stack_.push_back(g.m_model_);
}

void Graphic::PopMatrix()
{
  Graphic &g = Graphic::getInstance();
  g.m_model_ = g.m_model_stack_.back();
  g.m_model_stack_.pop_back();
  glUniformMatrix4fv(2, 1, GL_FALSE, &g.m_model_[0][0]);
}

void Graphic::Flush()
{
  Graphic &g = Graphic::getInstance();

  /* need to clear texture idx.
     texture idx should be resetted in next rendering. */
  g.tex_id_ = 0xffffffff;

  glFlush();
}

/* Returns single quad vertex buffer. */
VertexInfo* Graphic::get_vertex_buffer()
{
  return get_vertex_buffer(1);
}

VertexInfo* Graphic::get_vertex_buffer(int size)
{
  Graphic &g = Graphic::getInstance();
  int _ = g.vi_idx_;
  g.vi_idx_ += size;
  if (g.vi_idx_ >= kVertexMaxSize)
  {
    ASSERT(size < kVertexMaxSize);
    // prevent vertex buffer overflow
    RenderQuad();
    return get_vertex_buffer(size);
  }
  return g.vi_ + _ * 4;
}

/* @warn MUST call texture first before setting vertices */
void Graphic::SetTextureId(GLuint tex_id)
{
  Graphic &g = Graphic::getInstance();
  if (g.tex_id_ != tex_id)
  {
    // flush out previous vertices
    RenderQuad();
    glBindTexture(GL_TEXTURE_2D, tex_id);
    g.tex_id_ = tex_id;
  }
  else return;
}

void Graphic::SetBlendMode(int blend_mode)
{
  Graphic &g = Graphic::getInstance();
  static GLuint glBlendmodeTbl[] = {
    GL_ZERO, GL_ONE_MINUS_SRC_ALPHA, GL_ONE
  };
  static constexpr int glBlendmodeSize =
    sizeof(glBlendmodeTbl) / sizeof(GLuint);
  GLuint glBlendmode = GL_ONE;
  if (blend_mode >= 0 && blend_mode < glBlendmodeSize)
    glBlendmode = glBlendmodeTbl[blend_mode];

  if (g.src_blend_mode_ != glBlendmode)
  {
    g.src_blend_mode_ = glBlendmode;
    glBlendFunc(GL_SRC_ALPHA, glBlendmode);
  }
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

void Graphic::SetProjOrtho()
{
  if (current_proj_mode_ == 1) return;
  current_proj_mode_ = 1;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //glLoadMatrixf
  glOrtho(
    0,
    Game::getInstance().get_window_width(),
    Game::getInstance().get_window_height(),
    0,
    -1, 1
  );
  glMatrixMode(GL_MODELVIEW);
}

void Graphic::SetProjPerspective()
{
  if (current_proj_mode_ == 2) return;
  current_proj_mode_ = 2;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, Game::getInstance().GetAspect(), 1.0, 2000.0);
  glMatrixMode(GL_MODELVIEW);
}

void Graphic::RenderQuad(const VertexInfo* vi)
{
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
}
#endif

Graphic::Graphic()
{
}

Graphic::~Graphic()
{
  /**
   * Cleanup function should be called manually ... !
   * But automatic Cleanup would occur if initialization failed.
   */
  Cleanup();
}

}