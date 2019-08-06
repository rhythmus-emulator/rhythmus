#include "Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>

namespace rhythmus
{

/* Global FreeType font context. */
FT_Library ftLib;

/* How many font context exists? */
int ftLibCnt;

constexpr int defFontCacheWidth = 2048;
constexpr int defFontCacheHeight = 2048;


// --------------------------- class FontBitmap

FontBitmap::FontBitmap(int w, int h)
  : bitmap_(0), texid_(0), committed_(false)
{
  width_ = w;
  height_ = h;
  cur_x_ = cur_y_ = 0;
  cur_line_height_ = 0;

  bitmap_ = (uint32_t*)malloc(width_ * height_ * sizeof(uint32_t));
  ASSERT(bitmap_);

  glGenTextures(1, &texid_);
  if (texid_ == 0)
  {
    GLenum err = glGetError();
    std::cerr << "Font - Allocating textureID failed: " << (int)err << std::endl;
    return;
  }
}

FontBitmap::~FontBitmap()
{
  if (texid_)
  {
    glDeleteTextures(1, &texid_);
    texid_ = 0;
  }

  if (bitmap_)
  {
    free(bitmap_);
    bitmap_ = 0;
  }
}

void FontBitmap::Write(uint32_t* bitmap, int w, int h, FontGlyph &glyph_out)
{
  // writable?
  if (!IsWritable(w, h)) return;

  // need to break line?
  if (cur_x_ + w > width_)
  {
    cur_y_ += cur_line_height_;
    cur_x_ = 0;
    cur_line_height_ = 0;
  }

  // TODO: cut too big height

  // update new max height
  if (cur_line_height_ < h) cur_line_height_ = h;

  // write bitmap to destination
  for (int x = 0; x < w; ++x)
    for (int y = 0; y < h; ++y)
      bitmap_[(y + cur_y_)*width_ + x + cur_x_] = bitmap[w*y + x];

  // set texture pos before updating x pos
  GetGlyphTexturePos(glyph_out);

  // update x pos
  cur_x_ += w;

  // clear commit flag
  committed_ = false;
}

/* Get designated area(current x, y with given width / height) as texture position. */
void FontBitmap::GetGlyphTexturePos(FontGlyph &glyph_out)
{
  const int w = glyph_out.width;
  const int h = glyph_out.height;
  glyph_out.sx1 = (float)cur_x_ / width_;
  glyph_out.sx2 = (float)(cur_x_ + w) / width_;
  glyph_out.sy1 = (float)cur_y_ / height_;
  glyph_out.sy2 = (float)(cur_y_ + h) / height_;
}

void FontBitmap::Update()
{
  if (committed_ || !bitmap_) return;

  for (int i = 0; i < defFontCacheWidth * defFontCacheHeight; ++i)
    bitmap_[i] = (uint32_t)rand();

  // commit bitmap from memory
  glBindTexture(GL_TEXTURE_2D, texid_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)bitmap_);

  committed_ = true;
}

bool FontBitmap::IsWritable(int w, int h) const
{
  if (!bitmap_) return false;
  if (cur_x_ + w > width_ && cur_y_ + cur_line_height_ + h > height_) return false;
  else return true;
}

GLuint FontBitmap::get_texid() const
{
  return texid_;
}



// --------------------------------- class Font

Font::Font()
  : ftface_(0)
{
  if (ftLibCnt++ == 0)
  {
    if (FT_Init_FreeType(&ftLib))
    {
      std::cerr << "ERROR: Could not init FreeType Library." << std::endl;
      return;
    }
  }
}

Font::~Font()
{
  ClearGlyph();
  ReleaseFont();
}

bool Font::LoadFont(const char* ttfpath, FontAttributes& attrs)
{
  int r = FT_New_Face(ftLib, ttfpath, 0, (FT_Face*)&ftface_);
  if (r)
  {
    std::cerr << "ERROR: Could not load font: " << ttfpath << std::endl;
    return false;
  }
  fontattr_ = attrs;
  FT_Face ftface = (FT_Face)ftface_;

  // Set size to load glyphs as
  FT_Set_Pixel_Sizes(ftface, 0, fontattr_.size * 4);

  // Prepare some basic glyphs at first ...
  uint32_t glyph_init[128];
  for (int i = 0; i < 128; i++)
    glyph_init[i] = i;
  PrepareGlyph(glyph_init, 128);

  return true;
}

bool Font::LoadLR2Font(const char* lr2fontpath)
{
  return false;
}

void Font::PrepareGlyph(uint32_t *chrs, int count)
{
  FT_Face ftface = (FT_Face)ftface_;
  FontGlyph g;
  for (int i = 0; i < count; ++i)
  {
    /* just ignore failed character */
    if (FT_Load_Char(ftface, chrs[i], FT_LOAD_RENDER))
      continue;
    g.codepoint = chrs[i];
    g.width = ftface->glyph->bitmap.width;
    g.height = ftface->glyph->bitmap.rows;

    // XXX: generate writable bitmap instantly
    uint32_t* bitmap = (uint32_t*)malloc(g.width * g.height * sizeof(uint32_t));
    for (int x = 0; x < g.width; ++x) {
      for (int y = 0; y < g.height; ++y) {
        char a = ftface->glyph->bitmap.buffer[x + y * g.width];
        bitmap[x + y * g.width] = 0xffffffff;//a << 24 | 0x00ffffff;
      }
    }
    auto* cache = GetWritableBitmapCache(g.width, g.height);
    cache->Write(bitmap, g.width, g.height, g);
    g.texidx = cache->get_texid();
    free(bitmap);

    glyph_.push_back(g);
  }
}

void Font::Commit()
{
  for (auto* b : fontbitmap_)
    b->Update();
}

/**
 * Get glyph from codepoint.
 * NullGlyph is returned if codepoint not found(cached).
 */
const FontGlyph* Font::GetGlyph(uint32_t chr) const
{
  // XXX: linear search, but is there other way to search glyphs faster?
  for (int i = 0; i < glyph_.size(); ++i)
    if (glyph_[i].codepoint == chr)
      return &glyph_[i];
  return &null_glyph_;
}

/* Check is given glyph is NullGlyph */
bool Font::IsNullGlyph(const FontGlyph* g) const
{
  return g == &null_glyph_;
}

/* Set NullGlyph as specified codepoint */
void Font::SetNullGlyphAsCodePoint(uint32_t chr)
{
  const FontGlyph* g = GetGlyph(chr);
  if (!IsNullGlyph(g))
    null_glyph_ = *g;
}

float Font::GetTextWidth(const std::string& s)
{
  float r = 0;
  const FontGlyph* g = 0;
  uint32_t s32[1024];
  int s32len = 0;
  ConvertStringToCodepoint(s, s32, s32len, 1024);

  for (int i = 0; i < s32len; ++i)
  {
    g = GetGlyph(s32[i]);
    r += g->width;
  }

  return r;
}

void Font::SetText(const std::string& s)
{
  textglyph_.clear();

  uint32_t s32[1024];
  int s32len = 0;
  ConvertStringToCodepoint(s, s32, s32len, 1024);

  for (int i = 0; i < s32len; ++i)
    textglyph_.push_back(GetGlyph(s32[i]));
}

void Font::ConvertStringToCodepoint(const std::string& s, uint32_t *s32, int& lenout, int maxlen)
{
  if (maxlen < 0)
    maxlen = 0x7fffffff;

  // TODO: convert string to uint32 completely
  for (int i = 0; i < s.size() && i < maxlen; ++i) s32[i] = s[i], lenout++;
}

FontBitmap* Font::GetWritableBitmapCache(int w, int h)
{
  if (fontbitmap_.empty() || !fontbitmap_.back()->IsWritable(w, h))
    fontbitmap_.push_back(new FontBitmap(defFontCacheWidth, defFontCacheHeight));
  return fontbitmap_.back();
}

void Font::Render()
{
  // TODO: check animation is in motion

  // TODO: Set Proj / View matrix
  Graphic::getInstance().SetProjOrtho();
  Graphic::getInstance().SetModelIdentity();

  // Draw vertex by given quad
  VertexInfo vi[4];
  int cur_x = 0, cur_y = 0;
  for (const auto* g : textglyph_)
  {
    vi[0].r = vi[0].g = vi[0].b = vi[0].a = 1.0f;
    vi[0].x = cur_x;
    vi[0].y = cur_y;
    vi[0].z = .0f;
    vi[0].sx = g->sx1;
    vi[0].sy = g->sy1;

    vi[1].r = vi[1].g = vi[1].b = vi[1].a = 1.0f;
    vi[1].x = cur_x + g->width;
    vi[1].y = cur_y;
    vi[1].z = .0f;
    vi[1].sx = g->sx2;
    vi[1].sy = g->sy1;

    vi[2].r = vi[2].g = vi[2].b = vi[2].a = 1.0f;
    vi[2].x = cur_x + g->width;
    vi[2].y = cur_y + g->height;
    vi[2].z = .0f;
    vi[2].sx = g->sx2;
    vi[2].sy = g->sy2;

    vi[3].r = vi[3].g = vi[3].b = vi[3].a = 1.0f;
    vi[3].x = cur_x;
    vi[3].y = cur_y + g->height;
    vi[3].z = .0f;
    vi[3].sx = g->sx1;
    vi[3].sy = g->sy2;

    cur_x += g->width;

    glBindTexture(GL_TEXTURE_2D, g->texidx);
    Graphic::RenderQuad(vi);
  }

#if 0
  glBindTexture(GL_TEXTURE_2D, textglyph_[0]->texidx);
  vi[0].x = 10;
  vi[0].y = 10;
  vi[0].sx = .0f;
  vi[0].sy = .0f;

  vi[1].x = 200;
  vi[1].y = 10;
  vi[1].sx = 1.0f;
  vi[1].sy = .0f;

  vi[2].x = 200;
  vi[2].y = 200;
  vi[2].sx = 1.0f;
  vi[2].sy = 1.0f;

  vi[3].x = 10;
  vi[3].y = 200;
  vi[3].sx = .0f;
  vi[3].sy = 1.0f;

  Graphic::RenderQuad(vi);
#endif
}

void Font::ClearGlyph()
{
  // release texture and bitmap
  for (auto *b : fontbitmap_)
    delete b;
  fontbitmap_.clear();

  // release glyph & text cache
  textglyph_.clear();
  glyph_.clear();
  text_.clear();
  
  // reset null_glyph_ codepoint
  null_glyph_.codepoint = 0;
}

void Font::ReleaseFont()
{
  if (ftface_)
  {
    FT_Face ft = (FT_Face)ftface_;
    FT_Done_Face(ft);
    ftface_ = 0;
  }

  if (--ftLibCnt == 0)
    FT_Done_FreeType(ftLib);
}

}