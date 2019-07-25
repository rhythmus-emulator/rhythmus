#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <assert.h> /* TODO: move it to Error.h */
#define ASSERT(x) assert(x)
#define ASSERT_GL() ASSERT(glGetError() == 0)
#define ASSERT_GL_VAL(x) ASSERT((x = glGetError()) == 0)

namespace rhythmus
{

/**
 * @brief Rendering vertex info.
 */
struct VertexInfo {
  float x, y, z;
  float sx, sy;
  float r, g, b, a;
};

/**
 * @brief Shader info.
 */
struct ShaderInfo {
  const char* vertex_shader;
  const char* frag_shader;
  GLuint prog_id;
  const char* VAO_params[16];   // TODO?

  /* Round-robin buffering */
  GLuint VAO_id, buffer_id;
};

/**
 * @brief
 * Contains graphic context of game.
 * Singleton class.
 */

class Graphic
{
public:
  void Initialize();
  void Cleanup();
  void LoopRendering();
  void SetProjOrtho();
  void SetProjPerspective();

  static Graphic& getInstance();

  static void RenderQuad(const VertexInfo* vi);
private:
  Graphic();
  ~Graphic();
  bool CompileShader();
  bool CompileShaderInfo(ShaderInfo& shader);

  GLFWwindow* window_;
  ShaderInfo quad_shader_;
  int current_proj_mode_;
  glm::mat4 projection_;
};

}