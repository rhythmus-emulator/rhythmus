#include "Text.h"
#include "ResourceManager.h"
#include "Script.h"
#include <algorithm>

RHYTHMUS_NAMESPACE_BEGIN

// --------------------------------- class Text

Text::Text()
  : font_(nullptr),
    text_alignment_(TextAlignments::kTextAlignLeft),
    text_fitting_(TextFitting::kTextFitNone), width_multiply_(1.0f),
    do_line_breaking_(true)
{
  alignment_attrs_.sx = alignment_attrs_.sy = 1.0f;
  alignment_attrs_.tx = alignment_attrs_.ty = .0f;
  text_render_ctx_.width = text_render_ctx_.height = .0f;
  text_render_ctx_.cycles = 1;
  text_render_ctx_.duration = 0;
  text_render_ctx_.time = 0;
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
  font_->Commit();
  font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);
  UpdateTextRenderContext();
}

void Text::UpdateTextRenderContext()
{
  text_render_ctx_.width = 0;
  text_render_ctx_.height = 0;
  // XXX: breaking loop if first cycle is over should be better
  for (const auto& tvi : text_render_ctx_.textvertex)
  {
    text_render_ctx_.width = std::max(text_render_ctx_.width, (float)tvi.vi[2].x);
    text_render_ctx_.height = std::max(text_render_ctx_.height, (float)tvi.vi[2].y);
  }
}

void Text::ClearTextVertex()
{
  text_render_ctx_.textvertex.clear();
}

TextVertexInfo& Text::AddTextVertex(const TextVertexInfo &tvi)
{
  text_render_ctx_.textvertex.push_back(tvi);
  return text_render_ctx_.textvertex.back();
}

void Text::SetTextVertexCycle(size_t cycle, size_t duration)
{
  ASSERT_M(cycle == 0 || text_render_ctx_.textvertex.size() % cycle == 0,
    "TextVertexCycle is invalid; cycle number is not correct.");
  text_render_ctx_.cycles = cycle;
  text_render_ctx_.duration = duration;
}

void Text::SetWidthMultiply(float multiply)
{
  width_multiply_ = multiply;
}

void Text::Refresh()
{
  SetText(Script::getInstance().GetString(GetResourceId()));
}

// XXX: Font alignment to right won't work when text is multiline.
void Text::SetTextAlignment(TextAlignments align)
{
  text_alignment_ = align;
}

void Text::SetTextFitting(TextFitting fitting)
{
  text_fitting_ = fitting;
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

void Text::doUpdate(float delta)
{
  if (text_render_ctx_.duration)
  {
    text_render_ctx_.time += (size_t)delta;
    text_render_ctx_.time %= text_render_ctx_.duration;
  }
}

void Text::doRender()
{
  // for cycle attribute of animated text(number)
  size_t tvi_start, tvi_end, tvi_delta;
  // use additional transition for text
  ProjectionInfo pi;
  memset(&pi, 0, sizeof(ProjectionInfo));
  pi.sx = pi.sy = 1.0f;

  tvi_delta = text_render_ctx_.textvertex.size() / text_render_ctx_.cycles;
  if (text_render_ctx_.duration == 0)
    tvi_start = 0;
  else
    tvi_start = tvi_delta * text_render_ctx_.time * text_render_ctx_.cycles / text_render_ctx_.duration;
  tvi_end = tvi_start + tvi_delta;

  // set alignment-related options
  if (get_draw_property().w > 0
    && text_render_ctx_.width > 0 && text_render_ctx_.height > 0)
  {
    const float w = get_draw_property().w;
    float ratio = 1.0f;
    switch (text_fitting_)
    {
    case TextFitting::kTextFitNone:
      break;
    case TextFitting::kTextFitStretch:
      ratio = w / text_render_ctx_.width;
      pi.sx = ratio;
      break;
    case TextFitting::kTextFitMaxSize:
      ratio = std::min(1.0f, w / text_render_ctx_.width);
      pi.sx = ratio;
      break;
    }

    switch (text_alignment_)
    {
    case TextAlignments::kTextAlignLeft:
      break;
    case TextAlignments::kTextAlignRight:
      pi.x += std::max(w * width_multiply_ - text_render_ctx_.width, 0.0f);
      break;
    case TextAlignments::kTextAlignCenter:
      pi.x += std::max(w * width_multiply_ - text_render_ctx_.width, 0.0f) / 2;
      break;
    case TextAlignments::kTextAlignLR2Right:
      pi.x -= w * width_multiply_ - std::max(w * width_multiply_ - text_render_ctx_.width, 0.0f);
      break;
    case TextAlignments::kTextAlignLR2Center:
      pi.x -= (w * width_multiply_ - std::max(w * width_multiply_ - text_render_ctx_.width, 0.0f)) / 2;
      break;
    }
  }

  // XXX: must flush vertices before setting matrix ...
  // Graphic::SetTextureId(0);
  Graphic::PushMatrix();
  Graphic::SetMatrix(pi);
  Graphic::SetBlendMode(blending_);

  // Draw vertex by given quad
  // TODO: alpha?
  for (auto i = tvi_start; i < tvi_end; ++i)
  {
    auto &tvi = text_render_ctx_.textvertex[i];
    Graphic::SetTextureId(tvi.texid);
    memcpy(Graphic::get_vertex_buffer(),
      tvi.vi,
      sizeof(VertexInfo) * 4);
  }

  Graphic::RenderQuad();
  Graphic::PopMatrix();

  /* TESTCODE for rendering whole glyph texture */
#if 0
  if (text_render_ctx_.textvertex.empty())
    return;
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

/* do LR2 type alignment. */
void Text::SetLR2Alignment(int alignment)
{
  switch (alignment)
  {
  case 0:
    // left
    SetTextFitting(TextFitting::kTextFitMaxSize);
    SetTextAlignment(TextAlignments::kTextAlignLeft);
    break;
  case 1:
    // center
    SetTextFitting(TextFitting::kTextFitMaxSize);
    SetTextAlignment(TextAlignments::kTextAlignLR2Center);
    break;
  case 2:
    // right
    SetTextFitting(TextFitting::kTextFitMaxSize);
    SetTextAlignment(TextAlignments::kTextAlignLR2Right);
    break;
  }
}

void Text::Load(const Metric& metric)
{
  BaseObject::Load(metric);

  if (metric.exist("font"))
    SetFontByPath(metric.get<std::string>("font"));
  if (metric.exist("text"))
    SetText(metric.get<std::string>("text"));
  if (metric.exist("align"))
    SetTextAlignment((TextAlignments)metric.get<int>("align"));
}

void Text::LoadFromLR2SRC(const std::string &cmd)
{
  CommandArgs args(cmd);
  SetFontByPath(args.Get<std::string>(0));

  /* track change of text table */
  int eventid = args.Get<int>(1);
  std::string eventname = "Text" + args.Get<std::string>(1);
  AddCommand(eventname, "refresh");
  SubscribeTo(eventname);
  
  /* set text index for update */
  SetResourceId(eventid);
  Refresh();

  /* alignment */
  SetLR2Alignment(args.Get<int>(2));

  /* editable */
  //args.Get<int>(2);
}

RHYTHMUS_NAMESPACE_END