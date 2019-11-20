#include "Text.h"
#include "ResourceManager.h"
#include <algorithm>

RHYTHMUS_NAMESPACE_BEGIN

// --------------------------------- class Text

Text::Text()
  : font_(nullptr), text_alignment_(TextAlignments::kTextAlignLeft),
    text_position_(0), do_line_breaking_(true)
{
  alignment_attrs_.sx = alignment_attrs_.sy = 1.0f;
  alignment_attrs_.tx = alignment_attrs_.ty = .0f;
  text_render_ctx_.width = text_render_ctx_.height = .0f;
}

Text::~Text()
{
  ResourceManager::UnloadFont(font_);
}

float Text::GetTextWidth()
{
  return text_render_ctx_.width;
}

void Text::SetFontByPath(const std::string& path)
{
  if (font_)
    ResourceManager::UnloadFont(font_);
  font_ = ResourceManager::LoadFont(path);
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
void Text::SetAlignment(TextAlignments align)
{
  text_alignment_ = align;
}

void Text::SetTextPosition(int position_attr)
{
  text_position_ = position_attr;
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

void Text::doUpdate(float)
{
}

void Text::doRender()
{
  // use additional translation for text
  ProjectionInfo pi;
  memset(&pi, 0, sizeof(ProjectionInfo));
  pi.sx = pi.sy = 1.0f;

  // set alignment-related options
  if (get_draw_property().w > 0
    && text_render_ctx_.width > 0 && text_render_ctx_.height > 0)
  {
    const float w = get_draw_property().w;
    float ratio = 1.0f;
    switch (text_alignment_)
    {
    case TextAlignments::kTextAlignLeft:
      break;
    case TextAlignments::kTextAlignFitMaxsize:
      ratio = std::min(1.0f, w / text_render_ctx_.width);
      pi.sx = ratio;
      break;
    case TextAlignments::kTextAlignCenter:
      if (text_render_ctx_.width < w)
        pi.x = (text_render_ctx_.width - w) / 2;
      break;
    case TextAlignments::kTextAlignCenterFitMaxsize:
      ratio = std::min(1.0f, w / text_render_ctx_.width);
      pi.sx = ratio;
      if (ratio >= 1.0f)
        pi.x = (w - text_render_ctx_.width) / 2;
      break;
    case TextAlignments::kTextAlignRight:
      if (text_render_ctx_.width < w)
        pi.x = text_render_ctx_.width - w;
      break;
    case TextAlignments::kTextAlignRightFitMaxsize:
      ratio = std::min(1.0f, w / text_render_ctx_.width);
      pi.sx = ratio;
      if (ratio >= 1.0f)
        pi.x = w - text_render_ctx_.width;
      break;
    case TextAlignments::kTextAlignStretch:
      ratio = w / text_render_ctx_.width;
      pi.sx = ratio;
      break;
    }
  }

  switch (text_position_)
  {
  case 1:
    pi.x -= get_draw_property().w / 2;
    break;
  case 2:
    pi.x -= get_draw_property().w;
    break;
  }

  // XXX: must flush vertices before setting matrix ...
  // Graphic::SetTextureId(0);
  Graphic::PushMatrix();
  Graphic::SetMatrix(pi);

  // Draw vertex by given quad
  for (const TextVertexInfo& tvi : text_render_ctx_.textvertex)
  {
    Graphic::SetTextureId(tvi.texid);
    memcpy(Graphic::get_vertex_buffer(),
      tvi.vi,
      sizeof(VertexInfo) * 4);
  }

  Graphic::RenderQuad();
  Graphic::PopMatrix();

  /* TESTCODE for rendering whole glyph texture */
#if 0
  Graphic::SetTextureId(text_render_ctx_.textvertex[0].texid);
  VertexInfo *vi = Graphic::get_vertex_buffer();
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

  Graphic::RenderQuad();
#endif
}


// --------------------------- class NumberText

NumberText::NumberText()
  : number_(0), disp_number_(0),
    number_change_duration_(0), number_change_remain_(0), need_update_(true),
    number_format_("%d")
{
}

void NumberText::SetNumber(int number)
{
  number_ = (double)number;
  number_change_remain_ = number_change_duration_;
  need_update_ = true;
}

void NumberText::SetNumber(double number) {
  number_ = number;
  number_change_remain_ = number_change_duration_;
  need_update_ = true;
}

template <> int NumberText::GetNumber() const { return number_; }
template <> double NumberText::GetNumber() const { return number_; }
void NumberText::SetNumberChangeTime(float msec)
{ number_change_duration_ = (float)msec; }

void NumberText::SetFormat(const std::string& number_format)
{
  number_format_ = number_format;
}

void NumberText::doUpdate(float delta)
{
  char num_str[128];
  if (need_update_)
  {
    number_change_remain_ -= delta;
    if (number_change_remain_ <= 0)
      need_update_ = false;

    char* out_str = num_str;
    const char* fmt_str = number_format_.c_str();
    if (*fmt_str == '+')
    {
      if (disp_number_ >= 0)
      {
        *out_str = '+';
        ++out_str;
      }
      fmt_str++;
    }
    *out_str = '\0';
    char last = number_format_.back();
    if (last == 'd')
      sprintf(out_str, fmt_str, (int)disp_number_);
    else if (last == 'f')
      sprintf(out_str, fmt_str, disp_number_);
    SetText(num_str);
  }
}

RHYTHMUS_NAMESPACE_END