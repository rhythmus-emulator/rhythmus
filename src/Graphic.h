#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Error.h"
#include "config.h"

namespace rhythmus
{

class Image;

// @DEPRECIATED
struct RectF {
  float x1, y1, x2, y2;
  void set_points(float x1, float y1, float x2, float y2);
  void set_rect(float x, float y, float w, float h);
  void set_width(float w);
  void set_height(float h);
  float width() const;
  float height() const;
};

using Rect = glm::vec4;
using Point = glm::vec2;
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;
using Matrix = glm::mat4x4;

float GetWidth(const Rect &r);
float GetHeight(const Rect &r);
void SetWidth(Rect &r, float width);
void SetHeight(Rect &r, float height);

/* @brief Rendering vertex info. */
struct VertexInfo {
  // vertex pos
  Vector3 p;

  // texture pos
  Vector2 t;

  // diffuse color
  Vector4 c;
};

void ResetVertexInfo(VertexInfo &v);

/* @brief Fill color from string (ex. #FF000000) */
uint32_t HexStringToColor(const char *p);
uint32_t HexStringToColor(const std::string &s);
void FillColorFromString(Vector4 &color, const char* s);
void FillColorFromString(Vector4 &color, const std::string &s);

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
 * Video mode for current graphic.
 */
struct VideoModeParams
{
  int windowed;   // 0: fullscreen, 1: borderless window, 2: window
  std::string display_id;
  int width;
  int height;
  int bpp;
  int rate;
  bool vsync;
};

/**
 * @brief
 * Contains graphic context of game. Singleton class.
 *
 * @warn
 * - Pixel format is only supported for RGBA8.
 */
class Graphic
{
public:
  Graphic();
  virtual ~Graphic();

  static void CreateGraphic();
  static void DeleteGraphic();

  /* setting */
  virtual void Initialize() = 0;
  virtual void Cleanup() = 0;
  void SetVideoMode(VideoModeParams &p);
  const VideoModeParams &GetVideoMode() const;
  virtual bool TryVideoMode(VideoModeParams &p) = 0;
  virtual void GetSupportedVideoMode(std::vector<VideoModeParams> &p);


  // when external resolution is changed
  virtual void ResolutionChanged();


  /* This simulates vsync by checking last rendering time. */
  bool IsVsyncUpdatable() const;


  int width() const;
  int height() const;


  /* texture */
  virtual unsigned CreateTexture(const uint8_t *p, unsigned width, unsigned height) = 0;
  virtual void UpdateTexture(unsigned tex_id, const uint8_t *p,
    unsigned xoffset, unsigned yoffset, unsigned width, unsigned height) = 0;
  virtual void DeleteTexture(unsigned tex_id) = 0;
  virtual void ClearAllTextures() = 0;
  virtual unsigned GetNumTextureUnits() = 0;
  virtual unsigned CreateRenderTarget(unsigned &width, unsigned &height);
  void DeleteRenderTarget(unsigned tex_id);
  virtual Image *CreateScreenShot() = 0;
  virtual Image *GetTexture(unsigned tex_id) = 0;


  /* rendering state */
  virtual void SetBlendMode(int mode) = 0;
  virtual void SetRenderTarget(unsigned id, bool preserveTexture);
  virtual void SetTexture(unsigned texunit, unsigned tex_id) = 0;
  virtual void SetTextureMode(unsigned texunit, unsigned mode) = 0;
  virtual void SetTextureFiltering(unsigned texunit, unsigned do_filtering) = 0;
  virtual void SetZWrite(bool enable) = 0;
  void SetDefaultRenderState();


  /* matrix
   * @warn  normal: origin of world, local: origin of current object
   * \todo  add RotateXYZ ?
   */
  void PushMatrix();
  void PopMatrix();
  void Translate(float x, float y, float z);
  void Translate(const Vector3 &v);
  void TranslateLocal(float x, float y, float z);
  void TranslateLocal(const Vector3 &v);
  void Scale(float x, float y, float z);
  void Scale(const Vector3 &v);
  void Rotate(float x, float y, float z);
  void Rotate(const Vector3 &v);
  void RotateLocal(float x, float y, float z);
  void RotateLocal(const Vector3 &v);
  void MultiplyMatrix(const Matrix& mat);
  void MultiplyMatrixLocal(const Matrix& mat);
  const Matrix& GetWorldMatrix() const;

  void TexturePushMatrix();
  void TexturePopMatrix();
  void TextureTranslate(float x, float y);
  void TextureTranslate(const Vector2 &v);
  const Matrix& GetTextureMatrix() const;

  void CameraPushMatrix();
  void CameraPopMatrix();
  void CameraLoadOrtho(float w, float h);
  void CameraLoadPerspective(float fOV, float w, float h, float vanishx, float vanishy);
  const Matrix& GetProjectionMatrix() const;

  const Matrix &GetViewMatrix() const;

  void ResetMatrix();

  /* rendering */
  virtual void DrawQuads(const VertexInfo *vi, unsigned count);
  void DrawQuad(const VertexInfo *vi);

  virtual void BeginFrame();
  virtual void EndFrame();

  virtual void ClipViewArea(const Vector4 &area) = 0;
  virtual void ResetViewArea() = 0;

  /* status */
  float GetFPS() const;
  virtual void SignalWindowClose();
  virtual bool IsWindowShouldClose() const;
  double delta() const;

  virtual const char* name();

private:
  VideoModeParams video_mode_;

  std::vector<Matrix> mat_world_;
  std::vector<Matrix> mat_tex_;
  std::vector<Matrix> mat_proj_;
  Matrix mat_view_;

  double next_render_time_;
  double last_render_time_;
  float frame_delay_weight_avg_;
  bool is_game_running_;
};

}

#if USE_GLEW == 1
namespace rhythmus
{

/* @brief Shader info for OpenGL. */
struct ShaderInfo {
  const char* vertex_shader;
  const char* frag_shader;
  unsigned prog_id;
  const char* VAO_params[16];   // TODO?

  /* Round-robin buffering */
  unsigned VAO_id, buffer_id;
};

/* @brief Graphic engine based on GLES (OpenGL) */
class GraphicGL : public Graphic
{
public:
  GraphicGL();
  virtual ~GraphicGL();

  virtual void Initialize();
  virtual void Cleanup();
  virtual bool TryVideoMode(VideoModeParams &p);
  virtual void GetSupportedVideoMode(std::vector<VideoModeParams> &p);

  virtual unsigned CreateTexture(const uint8_t *p, unsigned width, unsigned height);
  virtual void UpdateTexture(unsigned tex_id, const uint8_t *p,
    unsigned xoffset, unsigned yoffset, unsigned width, unsigned height);
  virtual void DeleteTexture(unsigned tex_id);
  virtual void ClearAllTextures();
  virtual unsigned GetNumTextureUnits();
  virtual unsigned CreateRenderTarget(unsigned &width, unsigned &height);
  virtual Image *CreateScreenShot();
  virtual Image *GetTexture(unsigned tex_id);

  virtual void SetRenderTarget(unsigned id, bool preserveTexture);
  virtual void SetBlendMode(int mode);
  virtual void SetTexture(unsigned texunit, unsigned tex_id);
  virtual void SetTextureMode(unsigned texunit, unsigned mode);
  bool SetTextureUnit(unsigned texunit);
  virtual void SetTextureFiltering(unsigned texunit, unsigned do_filtering);
  virtual void SetZWrite(bool enable);

  virtual void DrawQuads(const VertexInfo *vi, unsigned count);
  virtual void BeginFrame();
  virtual void EndFrame();
  virtual void ClipViewArea(const Vector4 &area);
  virtual void ResetViewArea();

  virtual void SignalWindowClose();
  virtual bool IsWindowShouldClose() const;

  void CreateDefaultTexture();

  virtual const char* name();

private:
  /* parameters */
  std::string gl_vendor_, gl_renderer_, gl_version_, glu_version_;
  bool use_multi_texture_;
  unsigned max_texture_count_;
  int max_texture_size_;

protected:
  /* rendering state */
  int blendmode_;
  unsigned texunit_;
  unsigned tex_id_;
  unsigned def_tex_id_;
  int current_proj_mode_;
};

/* @brief Graphic Engine with GL shader */
class GraphicGLShader : public GraphicGL
{
public:
  GraphicGLShader();
  virtual ~GraphicGLShader();

  virtual void Initialize();
  bool CompileShaderInfo(ShaderInfo& shader);

  virtual void DrawQuads(const VertexInfo *vi, unsigned count);
  virtual void BeginFrame();

  // @DEPRECIATED
  VertexInfo* get_vertex_buffer();
  VertexInfo* get_vertex_buffer(int size);

  virtual const char* name();

private:
  int shader_mat_Projection_;
  int shader_mat_ModelView_;

  /* shader */
  ShaderInfo quad_shader_;

  VertexInfo *vi_;
  int vi_idx_;

  bool CompileDefaultShader();
};

}
#endif

namespace rhythmus
{
extern Graphic *GRAPHIC;
}