#pragma once

#include "Sprite.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace rhythmus
{

struct FontAttributes
{
  /* font size in pt. (multiplied by 4) */
  int size;

  /* font size in pixel. (set internally) */
  int height;

  /* height of baseline. set internally if zero */
  int baseline_offset;
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

struct TextVertexInfo
{
  GLuint texid;
  VertexInfo vi[4];
};

enum FontAlignments
{
  kFontAlignLeft,
  kFontAlignRight,
  kFontAlignCenter,
  kFontAlignFitMaxsize,
  kFontAlignCenterFitMaxsize,
  kFontAlignRightFitMaxsize,
  kFontAlignStretch,
};

/* Font bitmap cache */
class FontBitmap
{
public:
  FontBitmap(int w, int h);
  ~FontBitmap();
  void Write(uint32_t* bitmap, int w, int h, FontGlyph &glyph_out);
  void Update();
  bool IsWritable(int w, int h) const;
  GLuint get_texid() const;

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

class Font : public Sprite
{
public:
  Font();
  ~Font();

  bool LoadFont(const char* ttfpath, const FontAttributes& attrs);
  bool LoadLR2Font(const char* lr2fontpath);
  void PrepareGlyph(uint32_t *chrs, int count);
  void Commit();

  const FontGlyph* GetGlyph(uint32_t chr) const;
  bool IsNullGlyph(const FontGlyph* g) const;
  void SetNullGlyphAsCodePoint(uint32_t chr);
  float GetTextWidth(const std::string& s);
  void SetText(const std::string& s);
  void SetAlignment(FontAlignments align);
  void SetLineBreaking(bool line_break);

  void ClearGlyph();
  void ReleaseFont();

  virtual void Render();

  virtual void Update();

private:
  // FT_Face type
  void *ftface_;

  // cached glyph
  std::vector<FontGlyph> glyph_;

  // store bitmap / texture
  std::vector<FontBitmap*> fontbitmap_;
  int bitmap_cache_width, bitmap_cache_height;

  // current font attributes
  FontAttributes fontattr_;

  // text to be rendered
  std::string text_;

  // text_rendering related context
  struct {
    // glyphs prepared to be rendered (generated by text)
    std::vector<const FontGlyph*> textglyph;

    // glyph vertex to be rendered (generated by textglyph)
    std::vector<TextVertexInfo> textvertex;

    // calculated text width / height
    float width, height;
  } text_render_ctx_;

  // font alignment(stretch) option
  FontAlignments font_alignment_;

  // additional font attributes, which is set internally by font_alignment_ option.
  struct {
    // scale x / y
    float sx, sy;
    // translation x / y
    float tx, ty;
  } alignment_attrs_;

  // is line-breaking enabled?
  bool do_line_breaking_;

  // glyph which is returned when glyph is not found
  FontGlyph null_glyph_;

  void ConvertStringToCodepoint(const std::string& s, uint32_t *cp, int& lenout, int maxlen = -1);
  FontBitmap* GetWritableBitmapCache(int w, int h);
};

}