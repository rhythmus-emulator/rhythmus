#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Error.h"

namespace rhythmus
{

struct Rect {
  int x1, y1, x2, y2;
  void set_points(int x1, int y1, int x2, int y2);
  void set_rect(int x, int y, int w, int h);
  void set_width(int w);
  void set_height(int h);
  int width() const;
  int height() const;
};

struct RectF {
  float x1, y1, x2, y2;
  void set_points(float x1, float y1, float x2, float y2);
  void set_rect(float x, float y, float w, float h);
  void set_width(float w);
  void set_height(float h);
  float width() const;
  float height() const;
};

/* @brief Rendering vertex info. */
struct VertexInfo {
  // vertex pos
  float x, y, z;

  // texture pos
  float sx, sy;

  // color blending
  float r, g, b, a;

  void Clear();
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

enum BlendMode {
  kBlendNone,
  kBlendAlpha,
  kBlendAdd,
  kBlendSub,
  kBlendMul,
  kBlendXOR,
  kBlendMulInv,
  kBlendInv,
  kBlendMulAlpha,
  kBlendEnd,
};

/**
 * @brief
 * Contains graphic context of game.
 * Singleton class.
 */

class Graphic
{
public:
  static void Render();
  static void Initialize();
  static void Cleanup();
  static void SignalWindowClose();
  static bool IsWindowShouldClose();

  static Graphic& getInstance();

  void SetProjOrtho();
  void SetProjPerspective(float cx, float cy);
  void SetProjPerspectiveCenter();
  void SetModelIdentity();
  void SetModel(const ProjectionInfo& pi);
  static void GetResolutions(std::vector<std::string>& out);
  void CenterWindow();
  GLFWwindow* window();
  void SetWindowSize(int width, int height);
  float width() const;
  float height() const;
  int window_width() const;
  int window_height() const;
  float GetAspect() const;
  void CreateFBO(int w, int h);

  static void RenderQuad();
  static void SetMatrix(const ProjectionInfo& pi);
  static void PushMatrix();
  static void PopMatrix();
  static VertexInfo* get_vertex_buffer();
  static VertexInfo* get_vertex_buffer(int size);
  static bool is_vertex_buffer_remains();
  static void SetTextureId(GLuint tex_id);
  static void SetBlendMode(int blend_mode);
  static void SetRenderToTexture();
  static void SetRenderToScreen();
  static void SetOffscreenTextureId();
  static void Flush();

private:
  Graphic();
  ~Graphic();

  bool CompileShader();
  bool CompileShaderInfo(ShaderInfo& shader);
  void SetView(float x, float y, float dist);

  GLFWwindow* window_;
  float width_, height_;
  ShaderInfo quad_shader_;
  int current_proj_mode_;
  glm::mat4 m_projection_, m_view_, m_model_;
  std::vector<glm::mat4> m_model_stack_;

  VertexInfo *vi_;
  int vi_idx_;
  GLuint tex_id_;
  int blendmode_;
  GLuint fbo_id_, fbo_tex_id_, rbo_id_;

  void InitializeInternal();
  void RenderInternal();
  void CleanupInternal();
};

}