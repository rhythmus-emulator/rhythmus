#include "Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include <iostream>
#include <algorithm>

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
  memset(bitmap_, 0, width_ * height_ * sizeof(uint32_t));

  glGenTextures(1, &texid_);
  if (texid_ == 0)
  {
    GLenum err = glGetError();
    std::cerr << "Font - Allocating textureID failed: " << (int)err << std::endl;
    return;
  }
}

/* Using this constructor, texture is uploaded directly to GPU. */
FontBitmap::FontBitmap(const uint32_t* bitmap, int w, int h)
  : bitmap_(0), texid_(0), committed_(false),
    width_(w), height_(h), cur_x_(0), cur_y_(0), cur_line_height_(0)
{
  // create texture & upload directly
  glGenTextures(1, &texid_);
  if (texid_ == 0)
  {
    GLenum err = glGetError();
    std::cerr << "Font - Allocating textureID failed: " << (int)err << std::endl;
    return;
  }

  glBindTexture(GL_TEXTURE_2D, texid_);
  /* XXX: FreeImage uses BGR bitmap. need to fix it? */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0,
    GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)bitmap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
    cur_y_ += cur_line_height_ + 1 /* padding */;
    cur_x_ = 0;
    cur_line_height_ = 0;
  }

  // update new max height
  if (cur_line_height_ < h) cur_line_height_ = h;

  // write bitmap to destination
  for (int x = 0; x < w; ++x)
    for (int y = 0; y < h; ++y)
      bitmap_[(y + cur_y_)*width_ + x + cur_x_] = bitmap[w*y + x];

  // set texture pos before updating x pos
  GetGlyphTexturePos(glyph_out);

  // update x pos
  cur_x_ += w + 1 /* padding */;

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

  // commit bitmap from memory
  glBindTexture(GL_TEXTURE_2D, texid_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)bitmap_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  committed_ = true;
}

bool FontBitmap::IsWritable(int w, int h) const
{
  if (!bitmap_) return false;
  if (cur_x_ + w > width_ && cur_y_ + cur_line_height_ + h > height_) return false;
  else return true;
}

int FontBitmap::width() const { return width_; }
int FontBitmap::height() const { return height_; }

GLuint FontBitmap::get_texid() const
{
  return texid_;
}

void FontBitmap::SetToReadOnly()
{
  // update before remove cache
  Update();

  if (bitmap_)
  {
    free(bitmap_);
    bitmap_ = 0;
  }
}



// --------------------------------- class Font

Font::Font()
  : ftface_(0), ftstroker_(0)
{
  memset(&fontattr_, 0, sizeof(fontattr_));

  if (ftLibCnt++ == 0)
  {
    if (FT_Init_FreeType(&ftLib))
    {
      std::cerr << "ERROR: Could not init FreeType Library." << std::endl;
      ftLibCnt = 0;
      return;
    }
  }
}

Font::~Font()
{
  ClearGlyph();
  ReleaseFont();

  // Freetype done is only called when Font object releasing.
  if (--ftLibCnt == 0)
    FT_Done_FreeType(ftLib);
}

bool Font::LoadFont(const char* ttfpath, const FontAttributes& attrs)
{
  /* invalid font ... */
  if (attrs.size <= 0)
  {
    std::cerr << "Invalid font size (0)" << std::endl;
    return false;
  }
  if (ftLibCnt == 0)
  {
    std::cerr << "Freetype is not initalized." << std::endl;
    return false;
  }

  /* clear before start */
  ClearGlyph();
  ReleaseFont();

  /* Create freetype font */
  int r = FT_New_Face(ftLib, ttfpath, 0, (FT_Face*)&ftface_);
  if (r)
  {
    std::cerr << "ERROR: Could not load font: " << ttfpath << std::endl;
    return false;
  }
  fontattr_ = attrs;
  FT_Face ftface = (FT_Face)ftface_;

  /* Create freetype stroker (if necessary) */
  if (fontattr_.outline_width > 0)
  {
    FT_Stroker stroker;
    FT_Stroker_New(ftLib, &stroker);
    FT_Stroker_Set(stroker, fontattr_.outline_width * 32 /* XXX: is it good enough? */,
      FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
    ftstroker_ = stroker;
  }
  else ftstroker_ = 0;

  // Set size to load glyphs as
  const int fntsize_pixel = fontattr_.size * 4;
  fontattr_.height = fntsize_pixel;
  FT_Set_Pixel_Sizes(ftface, 0, fntsize_pixel);

  /* Set fontattributes in advance */

  // Set font baseline
  if (fontattr_.baseline_offset == 0)
  {
    //fontattr_.baseline_offset = fntsize_pixel;
    fontattr_.baseline_offset = fntsize_pixel * (1.0f + (float)ftface->descender / ftface->height);
  }

  // Prepare some basic glyphs at first ...
  uint32_t glyph_init[128];
  for (int i = 0; i < 128; i++)
    glyph_init[i] = i;
  PrepareGlyph(glyph_init, 128);

  return true;
}

#define BLEND_RGBA
#ifdef BLEND_RGBA
inline void __blend(unsigned char result[4], unsigned char fg[4], unsigned char bg[4])
{
  unsigned int alpha = fg[3] + 1;
  unsigned int inv_alpha = 256 - fg[3];
  result[0] = (unsigned char)((alpha * fg[0] + inv_alpha * bg[0]) >> 8);
  result[1] = (unsigned char)((alpha * fg[1] + inv_alpha * bg[1]) >> 8);
  result[2] = (unsigned char)((alpha * fg[2] + inv_alpha * bg[2]) >> 8);
  result[3] = bg[3];
}
#else
inline void __blend(unsigned char result[4], unsigned char fg[4], unsigned char bg[4])
{
  unsigned int alpha = fg[0] + 1;
  unsigned int inv_alpha = 256 - fg[0];
  result[1] = (unsigned char)((alpha * fg[1] + inv_alpha * bg[1]) >> 8);
  result[2] = (unsigned char)((alpha * fg[2] + inv_alpha * bg[2]) >> 8);
  result[3] = (unsigned char)((alpha * fg[3] + inv_alpha * bg[3]) >> 8);
  result[0] = bg[0];
}
#endif

void Font::PrepareGlyph(uint32_t *chrs, int count)
{
  FT_Face ftface = (FT_Face)ftface_;
  FT_Stroker ftStroker = (FT_Stroker)ftstroker_;
  FT_Glyph glyph;
  FT_BitmapGlyph bglyph;
  FontGlyph g;
  for (int i = 0; i < count; ++i)
  {
    /* Use FT_Load_Char, which calls and find Glyph index internally ... */
    /* and... just ignore failed character. */
    if (FT_Load_Char(ftface, chrs[i], FT_LOAD_NO_BITMAP))
      continue;

    FT_Get_Glyph(ftface->glyph, &glyph);
    FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
    bglyph = (FT_BitmapGlyph)glyph;

    g.codepoint = chrs[i];
    g.width = bglyph->bitmap.width;
    g.height = bglyph->bitmap.rows;
    g.pos_x = bglyph->left;
    g.pos_y = bglyph->top;
    g.adv_x = ftface->glyph->advance.x >> 6;
    g.srcx = g.srcy = g.texidx = 0;

    /* generate bitmap only when size is valid */
    if (ftface->glyph->bitmap.width > 0)
    {
      // generate foreground bitmap instantly
      // XXX: need texture option
      uint32_t* bitmap = (uint32_t*)malloc(g.width * g.height * sizeof(uint32_t));
      for (int x = 0; x < g.width; ++x) {
        for (int y = 0; y < g.height; ++y) {
          char a = bglyph->bitmap.buffer[x + y * g.width];
          bitmap[x + y * g.width] = a << 24 | (0x00ffffff & fontattr_.color);
        }
      }
      FT_Done_Glyph(glyph);

      // create outline if necessary.
      // XXX: need to check error code
      if (ftStroker)
      {
        FT_Get_Glyph(ftface->glyph, &glyph);
        FT_Glyph_Stroke(&glyph, ftStroker, 1);
        FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
        bglyph = (FT_BitmapGlyph)glyph;

        // blending to bitmap
        // XXX: it's quite corpse code, but ... it works! anyway.
        uint32_t bit_, fgbit_;
        int border_offset_x = g.pos_x - bglyph->left;
        int border_offset_y = bglyph->top - g.pos_y;
        for (int x = 0; x < bglyph->bitmap.width && x - border_offset_x < g.width; ++x) {
          for (int y = 0; y < bglyph->bitmap.rows && y - border_offset_y < g.height; ++y) {
            if (x < border_offset_x || y < border_offset_y) continue;
            char a = bglyph->bitmap.buffer[x + y * bglyph->bitmap.width];
            fgbit_ = a << 24 | (0x00ffffff & fontattr_.outline_color);
            const size_t bitmap_offset = x - border_offset_x + (y - border_offset_y) * g.width;
            __blend((unsigned char*)&bit_, (unsigned char*)&fgbit_, (unsigned char*)&bitmap[bitmap_offset]);
            bitmap[bitmap_offset] = bit_;
          }
        }

        FT_Done_Glyph(glyph);
      }

      /* cut glyph if it's too big height */
      if (g.height > fontattr_.height)
        g.height = fontattr_.height;

      /* upload bitmap to cache */
      auto* cache = GetWritableBitmapCache(g.width, g.height);
      cache->Write(bitmap, g.width, g.height, g);
      g.texidx = cache->get_texid();
      free(bitmap);
    }

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

void Font::GetTextVertexInfo(const std::string& s,
  std::vector<TextVertexInfo>& vtvi,
  bool do_line_breaking) const
{
  uint32_t s32[1024];
  int s32len = 0;
  ConvertStringToCodepoint(s, s32, s32len, 1024);

  // create textglyph
  std::vector<const FontGlyph*> textglyph;
  for (int i = 0; i < s32len; ++i)
    textglyph.push_back(GetGlyph(s32[i]));

  // create textvertex
  TextVertexInfo tvi;
  VertexInfo* vi = tvi.vi;
  int cur_x = 0, cur_y = 0, line_y = 0;
  for (const auto* g : textglyph)
  {
    // line-breaking
    if (g->codepoint == '\n')
    {
      if (do_line_breaking)
      {
        cur_x = 0;
        line_y += fontattr_.height;
      }
      continue;
    }

    // no-size glyph is ignored
    if (g->codepoint == 0)
      continue;

    cur_x += g->pos_x;
    cur_y = line_y + fontattr_.baseline_offset - g->pos_y;

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

    cur_x += g->adv_x - g->pos_x;
    tvi.texid = g->texidx;

    vtvi.push_back(tvi);
  }
}

void Font::ConvertStringToCodepoint(const std::string& s,
  uint32_t *s32, int& lenout, int maxlen) const
{
  if (maxlen < 0)
    maxlen = 0x7fffffff;

  // TODO: convert string to uint32 completely
  for (int i = 0; i < s.size() && i < maxlen; ++i) s32[i] = s[i], lenout++;
}

FontBitmap* Font::GetWritableBitmapCache(int w, int h)
{
  if (fontbitmap_.empty() || !fontbitmap_.back()->IsWritable(w, h))
  {
    if (!fontbitmap_.empty())
      fontbitmap_.back()->SetToReadOnly();
    fontbitmap_.push_back(new FontBitmap(defFontCacheWidth, defFontCacheHeight));
  }
  return fontbitmap_.back();
}

void Font::ClearGlyph()
{
  // release texture and bitmap
  for (auto *b : fontbitmap_)
    delete b;
  fontbitmap_.clear();

  // release glyph cache
  glyph_.clear();
  
  // reset null_glyph_ codepoint
  null_glyph_.codepoint = 0;
}

void Font::ReleaseFont()
{
  if (ftstroker_)
  {
    FT_Stroker ftstroker = (FT_Stroker)ftstroker_;
    FT_Stroker_Done(ftstroker);
    ftstroker_ = 0;
  }

  if (ftface_)
  {
    FT_Face ft = (FT_Face)ftface_;
    FT_Done_Face(ft);
    ftface_ = 0;
  }
}



// --------------------------------- class Text

Text::Text()
  : font_alignment_(FontAlignments::kFontAlignLeft), do_line_breaking_(true)
{
  alignment_attrs_.sx = alignment_attrs_.sy = 1.0f;
  alignment_attrs_.tx = alignment_attrs_.ty = .0f;
  text_render_ctx_.width = text_render_ctx_.height = .0f;
  SetFont(nullptr);
}

Text::Text(Font* font)
  : font_alignment_(FontAlignments::kFontAlignLeft), do_line_breaking_(true)
{
  alignment_attrs_.sx = alignment_attrs_.sy = 1.0f;
  alignment_attrs_.tx = alignment_attrs_.ty = .0f;
  text_render_ctx_.width = text_render_ctx_.height = .0f;
  SetFont(font);
}

Text::~Text()
{
}

float Text::GetTextWidth()
{
  return text_render_ctx_.width;
}

void Text::SetFont(Font* font)
{
  font_ = font;
}

void Text::SetText(const std::string& s)
{
  if (!font_) return;

  Clear();
  
  text_ = s;
  font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);

  for (const auto& tvi : text_render_ctx_.textvertex)
  {
    text_render_ctx_.width = std::max(text_render_ctx_.width, (float)tvi.vi[2].x);
    text_render_ctx_.height = std::max(text_render_ctx_.height, (float)tvi.vi[2].y);
  }
}

// XXX: Font alignment to right won't work when text is multiline.
void Text::SetAlignment(FontAlignments align)
{
  font_alignment_ = align;
}

void Text::SetLineBreaking(bool enable_line_break)
{
  do_line_breaking_ = enable_line_break;
}

void Text::Clear()
{
  text_render_ctx_.textvertex.clear();
  text_render_ctx_.height = text_render_ctx_.width = 0;
  text_.clear();
}

void Text::Update()
{
  Sprite::Update();

  // set alignment-related options
  if (get_animation().GetCurrentTweenInfo().w > 0
    && text_render_ctx_.width > 0 && text_render_ctx_.height > 0)
  {
    const float w = get_animation().GetCurrentTweenInfo().w;
    float ratio = 1.0f;
    switch (font_alignment_)
    {
    case FontAlignments::kFontAlignLeft:
      break;
    case FontAlignments::kFontAlignFitMaxsize:
      ratio = std::min(1.0f, w / text_render_ctx_.width);
      get_drawinfo().pi.sx *= ratio;
      break;
    case FontAlignments::kFontAlignCenter:
      if (text_render_ctx_.width < w)
        get_drawinfo().pi.x += (text_render_ctx_.width - w) / 2;
      break;
    case FontAlignments::kFontAlignCenterFitMaxsize:
      ratio = std::min(1.0f, w / text_render_ctx_.width);
      get_drawinfo().pi.sx *= ratio;
      if (ratio >= 1.0f)
        get_drawinfo().pi.x += (w - text_render_ctx_.width) / 2;
      break;
    case FontAlignments::kFontAlignRight:
      if (text_render_ctx_.width < w)
        get_drawinfo().pi.x += text_render_ctx_.width - w;
      break;
    case FontAlignments::kFontAlignRightFitMaxsize:
      ratio = std::min(1.0f, w / text_render_ctx_.width);
      get_drawinfo().pi.sx *= ratio;
      if (ratio >= 1.0f)
        get_drawinfo().pi.x += w - text_render_ctx_.width;
      break;
    case FontAlignments::kFontAlignStretch:
      ratio = w / text_render_ctx_.width;
      get_drawinfo().pi.sx *= ratio;
      break;
    }
  }
}

void Text::Render()
{
  // Set Proj / View matrix
  Graphic::getInstance().SetProjOrtho();
  Graphic::getInstance().SetModel(get_drawinfo().pi);

  // Draw vertex by given quad
  // XXX: is it better to cache vertex?
  for (const TextVertexInfo& tvi : text_render_ctx_.textvertex)
  {
    glBindTexture(GL_TEXTURE_2D, tvi.texid);
    Graphic::RenderQuad(tvi.vi);
  }

  /* TESTCODE for rendering whole glyph texture */
#if 0
  VertexInfo vi[4];
  glBindTexture(GL_TEXTURE_2D, text_render_ctx_.textvertex[0].texid);
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

  vi[0].r = vi[0].g = vi[0].b = vi[0].a = 1.0f;
  vi[1].r = vi[1].g = vi[1].b = vi[1].a = 1.0f;
  vi[2].r = vi[2].g = vi[2].b = vi[2].a = 1.0f;
  vi[3].r = vi[3].g = vi[3].b = vi[3].a = 1.0f;

  Graphic::getInstance().SetModelIdentity();
  Graphic::RenderQuad(vi);
#endif
}

}