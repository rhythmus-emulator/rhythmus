#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include "Error.h"

namespace rhythmus
{

/* @brief Rendering vertex info. */
struct VertexInfo {
  // vertex pos
  float x, y, z;

  // texture pos
  float sx, sy;

  // color blending
  float r, g, b, a;
};

/* @brief Projection info for each object. */
struct ProjectionInfo
{
  // rotation for each axis
  float rotx, roty, rotz;

  // center of rotation
  float tx, ty;

  // translation offset
  float x, y;

  // scale x / y
  float sx, sy;
};

/* @brief Total info required for rendering a object. */
struct DrawInfo
{
  VertexInfo vi[4];
  ProjectionInfo pi;
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
  void LoopRendering();
  void ExitRendering();
  void Cleanup();
  void SetProjOrtho();
  void SetProjPerspective(float cx, float cy);
  void SetProjPerspectiveCenter();
  void SetModelIdentity();
  void SetModel(const ProjectionInfo& pi);
  GLFWwindow* window();

  static Graphic& getInstance();

  static void RenderQuad();
  static void SetMatrix(const ProjectionInfo& pi);
  static void PushMatrix();
  static void PopMatrix();
  static VertexInfo* get_vertex_buffer();
  static VertexInfo* get_vertex_buffer(int size);
  static void SetTextureId(GLuint tex_id);

  static double GetFPS();
private:
  Graphic();
  ~Graphic();

  bool CompileShader();
  bool CompileShaderInfo(ShaderInfo& shader);
  void SetView(float x, float y, float dist);

  GLFWwindow* window_;
  ShaderInfo quad_shader_;
  int current_proj_mode_;
  glm::mat4 m_projection_, m_view_, m_model_;
  std::vector<glm::mat4> m_model_stack_;

  VertexInfo *vi_;
  int vi_idx_;
  GLuint tex_id_;
};

}