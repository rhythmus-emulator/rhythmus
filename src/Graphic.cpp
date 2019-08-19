#include "Graphic.h"
#include "Game.h"
#include "SceneManager.h"
#include "Timer.h"
#include "Logger.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#define RENDER_WITH_HLSL 1
int errorcode_;

namespace rhythmus
{

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

void on_keyevent(GLFWwindow *w, int key, int scancode, int action, int mode)
{
  if (action == GLFW_PRESS)
    Game::SendKeyDownEvent(key);
  else if (action == GLFW_RELEASE)
    Game::SendKeyUpEvent(key);
}

void on_text(GLFWwindow *w, uint32_t codepoint)
{
  Game::SendTextEvent(codepoint);
}

void on_cursormove(GLFWwindow *w, double xpos, double ypos)
{
  Game::SendCursorMoveEvent((int)xpos, (int)ypos);
}

void on_cursorbutton(GLFWwindow *w, int button, int action, int mods)
{
  Game::SendCursorClickEvent(button);
}

void on_joystick_conn(int jid, int event)
{
}

void Graphic::Initialize()
{
  Game &game = Game::getInstance();
  sWidth = game.get_window_width();
  sHeight = game.get_window_height();

  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
  {
    std::cerr << "Failed to init GLFW." << std::endl;
    exit(EXIT_FAILURE);
  }

  window_ = glfwCreateWindow(sWidth, sHeight,
    game.get_window_title().c_str(), NULL, NULL
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

  // Callback function setting
  // XXX: https://www.glfw.org/docs/latest/input_guide.html
  // TODO: calling callback func is appropriate for other function ...?
  glfwSetWindowSizeCallback(window_, on_resize);
  glfwSetKeyCallback(window_, on_keyevent);
  glfwSetCharCallback(window_, on_text);
  glfwSetCursorPosCallback(window_, on_cursormove);
  glfwSetMouseButtonCallback(window_, on_cursorbutton);
  glfwSetJoystickCallback(on_joystick_conn);

  // GL flag setting
  glClearColor(0, 0, 0, 1);
  glMatrixMode(GL_PROJECTION);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

  // set rendering context
  current_proj_mode_ = -1;  // no projection mode initially.
}

void Graphic::LoopRendering()
{
  while (!glfwWindowShouldClose(window_))
  {
    glViewport(0, 0, sWidth, sHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Update main timer in graphic(main) thread. */
    Timer::Update();

    /* Update whole game context */
    Game::getInstance().Update();

    /* Process cached events in main thread. */
    Game::getInstance().ProcessEvent();

    /* Main Rendering */
    SceneManager::getInstance().Render();
    glFlush();

    glfwSwapBuffers(window_);
    glfwPollEvents();

    /* Flush stdout into log message */
    Logger::getInstance().Flush();
  }
}

/* Just send close message to system */
void Graphic::ExitRendering()
{
  glfwSetWindowShouldClose(window_, 1);
}

void Graphic::Cleanup()
{
  if (window_)
  {
    glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = 0;
  }
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
    0.0f, (float)Game::getInstance().get_window_width(),
    (float)Game::getInstance().get_window_height(), 0.0f,
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
  SetProjPerspective(Game::getInstance().get_window_width() / 2.0f,
                     Game::getInstance().get_window_height() / 2.0f);
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

  const float fWidth = Game::getInstance().get_window_width();
  const float fHeight = Game::getInstance().get_window_height();
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
  m_model_ = glm::mat4(1.0f);

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

/**
 * @brief Renders quad
 * @warn  MUST set glBindTexture() if it is necessary!
 */
void Graphic::RenderQuad(const VertexInfo* vi)
{
  Graphic& g = Graphic::getInstance();

  // write buffer
  glBindBuffer(GL_ARRAY_BUFFER, g.quad_shader_.buffer_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(VertexInfo) * 4, nullptr, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VertexInfo) * 4, vi);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // read buffer
  glUseProgram(g.quad_shader_.prog_id);
  glBindVertexArray(g.quad_shader_.VAO_id);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void Graphic::RenderQuad(const DrawInfo& di)
{
  SetProj(di.pi);
  RenderQuad(di.vi);
}

void Graphic::SetProj(const ProjectionInfo& pi)
{
  Graphic &g = Graphic::getInstance();

  /* Prefer Ortho projection if not rotX / rotY */
  if (pi.rotx == .0f && pi.roty == .0f)
    g.SetProjOrtho();
  else
    g.SetProjPerspectiveCenter();

  g.SetModel(pi);
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