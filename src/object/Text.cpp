#include "Text.h"
#include "ResourceManager.h"
#include "Script.h"
#include <algorithm>

RHYTHMUS_NAMESPACE_BEGIN

// --------------------------------- class Text

Text::Text()
  : font_(nullptr), text_alignment_(TextAlignments::kTextAlignLeft),
    text_position_(0), do_line_breaking_(true), table_index_(0)
{
  alignment_attrs_.sx = alignment_attrs_.sy = 1.0f;
  alignment_attrs_.tx = alignment_attrs_.ty = .0f;
  text_render_ctx_.width = text_render_ctx_.height = .0f;
}

Text::~Text()
{
  SetFont(nullptr);
}

void Text::SetFontByPath(const std::string& path)
{
  SetFont(ResourceManager::LoadFont(path));

  /* if text previously exists, call SetText() internally */
  if (!text_.empty())
    SetText(text_);
}

void Text::SetSystemFont()
{
  SetFont(ResourceManager::LoadSystemFont());

  /* if text previously exists, call SetText() internally */
  if (!text_.empty())
    SetText(text_);
}

void Text::SetFont(Font *font)
{
  if (font_)
    ResourceManager::UnloadFont(font_);
  font_ = font;
}

float Text::GetTextWidth() const
{
  return text_render_ctx_.width;
}

void Text::SetText(const std::string& s)
{
  if (!font_) return;

  Clear();

  text_ = s;
  font_->PrepareText(s);
  font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);

  for (const auto& tvi : text_render_ctx_.textvertex)
  {
    text_render_ctx_.width = std::max(text_render_ctx_.width, (float)tvi.vi[2].x);
    text_render_ctx_.height = std::max(text_render_ctx_.height, (float)tvi.vi[2].y);
  }
}

void Text::SetTextTableIndex(size_t idx)
{
  table_index_ = idx;
}

size_t Text::GetTextTableIndex() const
{
  return table_index_;
}

void Text::SetTextFromTable()
{
  SetText(Script::getInstance().GetString(table_index_));
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

Font *Text::font() { return font_; }

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

void Text::Load(const Metric& metric)
{
  BaseObject::Load(metric);

  if (metric.exist("font"))
    SetFontByPath(metric.get<std::string>("font"));
  if (metric.exist("text"))
    SetText(metric.get<std::string>("text"));
  if (metric.exist("align"))
  {
    SetAlignment((TextAlignments)metric.get<int>("align"));
  }
  if (metric.exist("lr2font"))
  {
    LoadFromLR2SRC(metric.get<std::string>("lr2font"));
  }
}

/* do LR2 type alignment. */
void Text::SetLR2Alignment(int alignment)
{
  switch (alignment)
  {
    // left
    SetAlignment(TextAlignments::kTextAlignFitMaxsize);
    break;
  case 1:
    // center
    SetAlignment(TextAlignments::kTextAlignCenterFitMaxsize);
    SetTextPosition(1);
    break;
  case 2:
    // right
    SetAlignment(TextAlignments::kTextAlignRightFitMaxsize);
    SetTextPosition(2);
    break;
  }
}

void Text::LoadFromLR2SRC(const std::string &cmd)
{
  CommandArgs args(cmd);
  SetFontByPath(args.Get<std::string>(0));

  /* track change of text table */
  std::string eventname = args.Get<std::string>(1);
  cmdfnmap_[eventname] = [](void *o, CommandArgs& args) {
    static_cast<Text*>(o)->SetTextFromTable();
  };
  SubscribeTo(eventname);
  /* set value instantly */
  SetTextTableIndex(atoi(eventname.c_str()));

  /* alignment */
  SetLR2Alignment(args.Get<int>(2));

  /* editable */
  //args.Get<int>(2);
}

void Text::CreateCommandFnMap()
{
  BaseObject::CreateCommandFnMap();
  cmdfnmap_["text"] = [](void *o, CommandArgs& args) {
    static_cast<Text*>(o)->SetText(args.Get<std::string>(0));
  };
}

RHYTHMUS_NAMESPACE_END