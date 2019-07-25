#include "Graphic.h"
#include "Game.h"
#include "SceneManager.h"
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

}

void on_text(GLFWwindow *w, unsigned int codepoint)
{

}

void on_cursormove(GLFWwindow *w, double xpos, double ypos)
{
}

void on_cursorbutton(GLFWwindow *w, int button, int action, int mods)
{
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

  // set rendering context
  current_proj_mode_ = -1;  // no projection mode initially.
}

void Graphic::LoopRendering()
{
  while (!glfwWindowShouldClose(window_))
  {
    glViewport(0, 0, sWidth, sHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // XXX: "Some" objects might want to be drawn as perspective ...
    // XXX: should pass projection matrix to shader using glUniformMatrix4fv, or bindbufferdata() ...
    glMatrixMode(GL_PROJECTION);
    SetProjOrtho();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    SceneManager::getInstance().Render();
    glFlush();

    glfwSwapBuffers(window_);
    glfwPollEvents();
  }
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
    "in vec3 positionAttribute;"
    "in vec4 colorAttribute;"
    "in vec2 texCoordinate;"
    "out vec4 passColorAttribute;"
    "out vec2 passTextureCoordinateAttribute;"
    "uniform mat4 projection;"
    "void main()"
    "{"
    "gl_Position = projection * vec4(positionAttribute, 1.0);"
    "passColorAttribute = colorAttribute;"
    "passTextureCoordinateAttribute = texCoordinate;"
    "}";

  quad_shader_.frag_shader =
    "#version 330 core\n"
    "in vec4 passColorAttribute;"
    "in vec2 passTextureCoordinateAttribute;"
    "uniform sampler2D tex;"
    "out vec4 fragmentColor;"
    "void main()"
    "{"
    "fragmentColor = texture(tex, passTextureCoordinateAttribute);"
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
  if (current_proj_mode_ == 1) return;
  current_proj_mode_ = 1;

  projection_ = glm::ortho(
    0.f,
    (float)Game::getInstance().get_window_width(),
    (float)Game::getInstance().get_window_height(),
    0.f
  );

  GLint matID = glGetUniformLocation(quad_shader_.prog_id, "projection");
  glUseProgram(quad_shader_.prog_id);
  glUniformMatrix4fv(matID, 1, GL_FALSE, &projection_[0][0]);
  ASSERT_GL_VAL(errorcode_);
}

/**
 * @brief Set projection mode as perspective.
 *        No effect if it is already perspective.
 */
void Graphic::SetProjPerspective()
{
  if (current_proj_mode_ == 2) return;
  current_proj_mode_ = 2;

  projection_ = glm::perspective(
    60.0f,
    (float)Game::getInstance().GetAspect(),
    1.0f, 2000.0f
  );

  GLuint matID = glGetUniformLocation(quad_shader_.prog_id, "projection");
  glUniformMatrix4fv(matID, 1, GL_FALSE, &projection_[0][0]);
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

  glLoadIdentity();
  glOrtho(
    0,
    Game::getInstance().get_window_width(),
    Game::getInstance().get_window_height(),
    0,
    -1, 1
  );
}

void Graphic::SetProjPerspective()
{
  if (current_proj_mode_ == 2) return;
  current_proj_mode_ = 2;

  glLoadIdentity();
  gluPerspective(60, Game::getInstance().GetAspect(), 1.0, 2000.0);
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