#pragma once

#include "Sprite.h"
#include "ResourceManager.h"
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

namespace rhythmus
{

class Image;
class MetricGroup;

// @brief Bitmap property for filling font outline or foreground
struct FontFillBitmap
{
  // image data ptr. only 32bit unsigned RGBA allowed
  char *p;

  // image width / height.
  int width, height;
};

// @brief Font parameters required for creating font object
struct FontAttribute
{
  /* specified name of font (optional) */
  std::string name;

  /* font size in pixel. (set internally) */
  unsigned height;

  /* height of baseline. set internally if zero */
  int baseline_offset;

  /* color of font */
  uint32_t color;

  /* thickness of font outline */
  int outline_width;

  /* color of font outline */
  uint32_t outline_color;

  /* Texture of font foreground. color option is ignored when tex is set. */
  FontFillBitmap tex;

  /* Texture of font border. color option is ignored when tex is set. */
  FontFillBitmap outline_tex;
};


struct TextVertexInfo
{
  unsigned texid;
  VertexInfo vi[4];
};

class FontBitmap;

struct FontGlyph
{
  /* codepoint of the glyph */
  uint32_t codepoint;

  /* bitmap spec */
  unsigned width, height;

  /* glyph pos relative to baseline */
  int pos_x, pos_y;

  /* advancing pos x for next character (pixel) */
  int adv_x;

  /* font bitmap (for getting texture id) */
  FontBitmap* fbitmap;

  /* texture index(glew) and srcx, srcy */
  int srcx, srcy;

  /* texture coord in float (for rendering) */
  float sx1, sy1, sx2, sy2;
};

/* Font bitmap cache */
class FontBitmap
{
public:
  FontBitmap(int w, int h);
  FontBitmap(uint32_t* bitmap, int w, int h);
  FontBitmap(const uint32_t* bitmap, int w, int h);
  ~FontBitmap();
  void Write(uint32_t* bitmap, int w, int h, FontGlyph &glyph_out);
  bool Update();
  bool IsWritable(int w, int h) const;
  int width() const;
  int height() const;
  unsigned get_texid() const;
  
  int get_error_code() const;
  const char *get_error_msg() const;

  /**
   * Delete memory cache and set as read-only.
   * Texture remains, so we can still use font texture while saving memory.
   */
  void SetToReadOnly();

private:
  uint32_t* bitmap_;
  unsigned texid_;

  /* width / height of font bitmap cache */
  int width_, height_;

  int cur_line_height_;
  int cur_x_, cur_y_;
  bool committed_;

  int error_code_;
  const char *error_msg_;

  void GetGlyphTexturePos(FontGlyph &glyph_out);
};

class Font : public ResourceElement
{
public:
  Font();
  virtual ~Font();

  void Load(const std::string &path);
  void Load(const char *p, size_t len, const char *ext_hint_opt);
  void Load(const MetricGroup& metrics);
  void Clear();
  void Update(float ms);

  // This indicates glyph and bitmap info is loaded,
  // but texture might not be loaded.
  bool is_glyph_loaded() const;

  // This indicates is font fully loaded, including texture.
  bool is_loaded() const;

  bool is_empty() const;

  void PrepareText(const std::string& text_utf8);
  void PrepareGlyph(uint32_t *chrs, int count);

  const FontAttribute &GetAttribute() const;
  const FontGlyph* GetGlyph(uint32_t chr) const;
  bool IsNullGlyph(const FontGlyph* g) const;
  void SetNullGlyphAsCodePoint(uint32_t chr);
  float GetTextWidth(const std::string& s);
  void GetTextVertexInfo(const std::string& s,
    std::vector<TextVertexInfo>& tvi,
    bool do_line_breaking) const;

  const std::string& get_path() const;
  bool is_ttf_font() const;
  int height() const;

private:
  void LoadFreetypeFont(const std::string &path);
  void LoadLR2BitmapFont(const std::string &path);
  void ClearGlyph();
  void ReleaseFont();
  void Commit();

  // Font data path. used for identification.
  std::string path_;

  // is bitmap font or ttf font?
  bool is_ttf_font_;

  // FT_Face, FT_Stroker type
  void *ftface_, *ftstroker_;

  // font attribute
  FontAttribute fontattr_;

  // glyph which is returned when glyph is not found
  FontGlyph null_glyph_;

  // cached glyph
  std::vector<FontGlyph> glyph_;

  // store bitmap / texture
  std::vector<FontBitmap*> fontbitmap_;

  // status of current font.
  // 0: is not loaded
  // 1: loading
  // 2: loaded, but texture is not prepared
  // 3: fully loaded, ready to render.
  int status_;

  int error_code_;
  const char *error_msg_;

  void ConvertStringToCodepoint(const std::string& s, uint32_t *cp, int& lenout, int maxlen = -1) const;
  FontBitmap* GetWritableBitmapCache(int w, int h);
};

using FontAuto = std::shared_ptr<Font>;

}
