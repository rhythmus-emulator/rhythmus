#pragma once

#include "Sprite.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace rhythmus
{

struct TextVertexInfo
{
  GLuint texid;
  VertexInfo vi[4];
};

struct FontFillTexture
{
  // image data ptr. only 32bit unsigned RGBA allowed
  char *p;

  // image width / height.
  int width, height;
};

struct FontAttributes
{
  /* font size in pt. (multiplied by 4) */
  int size;

  /* font size in pixel. (set internally) */
  int height;

  /* height of baseline. set internally if zero */
  int baseline_offset;

  /* color of font */
  uint32_t color;

  /* thickness of font outline */
  int outline_width;

  /* color of font outline */
  uint32_t outline_color;

  /* Texture of font foreground. color option is ignored when tex is set. */
  FontFillTexture tex;

  /* Texture of font border. color option is ignored when tex is set. */
  FontFillTexture outline_tex;

  void SetFromCommand(const std::string &command);

  /* @warn this method is in development.
   * may cannot distinguish different font. */
  bool operator==(const FontAttributes& attr) const
  {
    return attr.size == size &&
      attr.color == color &&
      attr.outline_width == outline_width
      ;
  }
};

struct FontGlyph
{
  /* codepoint of the glyph */
  uint32_t codepoint;

  /* bitmap spec */
  int width, height;

  /* glyph pos relative to baseline */
  int pos_x, pos_y;

  /* advancing pos x for next character (pixel) */
  int adv_x;

  /* texture index(glew) and srcx, srcy */
  int texidx, srcx, srcy;

  /* texture position in float (for rendering) */
  float sx1, sy1, sx2, sy2;
};

/* Font bitmap cache */
class FontBitmap
{
public:
  FontBitmap(int w, int h);
  FontBitmap(const uint32_t* bitmap, int w, int h);
  ~FontBitmap();
  void Write(uint32_t* bitmap, int w, int h, FontGlyph &glyph_out);
  void Update();
  bool IsWritable(int w, int h) const;
  int width() const;
  int height() const;
  GLuint get_texid() const;

  /**
   * Delete memory cache and set as read-only.
   * Texture remains, so we can still use font texture while saving memory.
   */
  void SetToReadOnly();

private:
  uint32_t* bitmap_;
  GLuint texid_;

  /* width / height of font bitmap cache */
  int width_, height_;

  int cur_line_height_;
  int cur_x_, cur_y_;
  bool committed_;

  void GetGlyphTexturePos(FontGlyph &glyph_out);
};

class Font
{
public:
  Font();
  virtual ~Font();

  void set_name(const std::string& name);
  const std::string& get_name() const;

  virtual bool LoadFont(const char* ttfpath, const FontAttributes& attrs);
  void PrepareText(const std::string& text_utf8);
  void PrepareGlyph(uint32_t *chrs, int count);
  void Commit();

  const FontGlyph* GetGlyph(uint32_t chr) const;
  bool IsNullGlyph(const FontGlyph* g) const;
  void SetNullGlyphAsCodePoint(uint32_t chr);
  float GetTextWidth(const std::string& s);
  void GetTextVertexInfo(const std::string& s,
    std::vector<TextVertexInfo>& tvi,
    bool do_line_breaking) const;

  void ClearGlyph();
  void ReleaseFont();

  const std::string& get_path() const;
  bool is_ttf_font() const;
  const FontAttributes& get_attribute() const;

protected:
  // Font data path. used for identification.
  std::string path_;

  // font resource name
  std::string name_;

  // is bitmap font or ttf font?
  bool is_ttf_font_;

  // FT_Face, FT_Stroker type
  void *ftface_, *ftstroker_;

  // cached glyph
  std::vector<FontGlyph> glyph_;

  // store bitmap / texture
  std::vector<FontBitmap*> fontbitmap_;

  // current font attributes
  FontAttributes fontattr_;
  // glyph which is returned when glyph is not found
  FontGlyph null_glyph_;

  void ConvertStringToCodepoint(const std::string& s, uint32_t *cp, int& lenout, int maxlen = -1) const;
  FontBitmap* GetWritableBitmapCache(int w, int h);
};

using FontAuto = std::shared_ptr<Font>;

}