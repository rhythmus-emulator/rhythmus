#include "Text.h"
#include "ResourceManager.h"
#include "Script.h"
#include "KeyPool.h"
#include "common.h"
#include "config.h"

RHYTHMUS_NAMESPACE_BEGIN

// --------------------------------- class Text

Text::Text()
  : font_(nullptr),
    text_alignment_(TextAlignments::kTextAlignLeft),
    text_fitting_(TextFitting::kTextFitNone), width_multiply_(1.0f),
    blending_(0), counter_(0), is_texture_loaded_(true),
    res_id_(nullptr), do_line_breaking_(true)
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
  ClearFont();
}

void Text::Load(const MetricGroup &m)
{
  BaseObject::Load(m);

  if (m.exist("path"))
    SetFont(m);
  if (m.exist("text"))
    SetText(m.get_str("text"));
  if (m.exist("align"))
    SetTextAlignment((TextAlignments)m.get<int>("align"));

#if USE_LR2_FEATURE == 1
  if (m.exist("lr2src"))
  {
    std::string lr2src;
    m.get_safe("lr2src", lr2src);
    CommandArgs args(lr2src);
    SetFont(args.Get<std::string>(0));

    /* track change of text table */
    std::string eventname = "Text" + args.Get<std::string>(1);
    AddCommand(eventname, "refresh");
    SubscribeTo(eventname);
    std::string resname = "S" + args.Get<std::string>(1);
    KeyData<std::string> kdata = KEYPOOL->GetString(resname);
    res_id_ = &*kdata;
    Refresh();

    /* alignment */
    SetLR2Alignment(args.Get<int>(2));

    /* editable */
    //args.Get<int>(2);
  }

  // TODO: load blending from LR2DST
#endif
}

void Text::SetFont(const std::string& path)
{
  ClearFont();
  font_ = FONTMAN->Load(path);

  /* if text previously exists, call SetText() internally */
  if (!text_.empty())
    SetText(text_);
}

void Text::SetFont(const MetricGroup &m)
{
  ClearFont();
  font_ = FONTMAN->Load(m);

  /* if text previously exists, call SetText() internally */
  if (!text_.empty())
    SetText(text_);
}

void Text::SetSystemFont()
{
  // TODO
  R_ASSERT(0);
#if 0
  ClearFont();
  SetFont(FONTMAN->Load("TODO"));

  /* if text previously exists, call SetText() internally */
  if (!text_.empty())
    SetText(text_);
#endif
}

void Text::ClearFont()
{
  if (font_)
  {
    FONTMAN->Unload(font_);
    font_ = nullptr;
  }
}

float Text::GetTextWidth() const
{
  return text_render_ctx_.width;
}

void Text::SetText(const std::string& s)
{
  if (!font_) return;

  ClearText();

  text_ = s;
  font_->PrepareText(s);
  font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);
  UpdateTextRenderContext();

}

void Text::ClearText()
{
  text_render_ctx_.textvertex.clear();
  text_render_ctx_.height = text_render_ctx_.width = 0;
  text_.clear();
}

void Text::UpdateTextRenderContext()
{
  text_render_ctx_.width = 0;
  text_render_ctx_.height = 0;
  // XXX: breaking loop if first cycle is over should be better
  is_texture_loaded_ = true;
  for (const auto& tvi : text_render_ctx_.textvertex)
  {
    text_render_ctx_.width = std::max(text_render_ctx_.width, (float)tvi.vi[2].p.z);
    text_render_ctx_.height = std::max(text_render_ctx_.height, (float)tvi.vi[2].p.z);
    if (tvi.texid == 0)
      is_texture_loaded_ = false;
  }
}

TextVertexInfo& Text::AddTextVertex(const TextVertexInfo &tvi)
{
  text_render_ctx_.textvertex.push_back(tvi);
  return text_render_ctx_.textvertex.back();
}

void Text::SetTextVertexCycle(size_t cycle, size_t duration)
{
  R_ASSERT_M(cycle == 0 || text_render_ctx_.textvertex.size() % cycle == 0,
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
  if (res_id_)
    SetText(*res_id_);
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

Font *Text::font() { return font_; }

void Text::doUpdate(double delta)
{
  counter_ = (counter_ + 1) % 60;
  if (counter_ == 0 && !is_texture_loaded_ && font_)
  {
    font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);
    UpdateTextRenderContext();
  }

  if (text_render_ctx_.duration)
  {
    text_render_ctx_.time += (size_t)delta;
    text_render_ctx_.time %= text_render_ctx_.duration;
  }
}

void Text::doRender()
{
  // for cycle attribute of animated text(lr2number type)
  size_t tvi_start, tvi_end, tvi_delta;
  tvi_delta = text_render_ctx_.textvertex.size() / text_render_ctx_.cycles;
  if (text_render_ctx_.duration == 0)
    tvi_start = 0;
  else
    tvi_start = tvi_delta * text_render_ctx_.time * text_render_ctx_.cycles / text_render_ctx_.duration;
  tvi_end = tvi_start + tvi_delta;


  // additional transition for text (alignment)
  GRAPHIC->PushMatrix();
  const float width = GetWidth(GetCurrentFrame().pos);
  Vector3 scale{ 1.0f, 1.0f, 1.0f };
  Vector3 move{ 0.0f, 0.0f, 0.0f };
  if (width > 0 && text_render_ctx_.width > 0 && text_render_ctx_.height > 0)
  {
    switch (text_fitting_)
    {
    case TextFitting::kTextFitNone:
      break;
    case TextFitting::kTextFitStretch:
      scale.x = width / text_render_ctx_.width;
      break;
    case TextFitting::kTextFitMaxSize:
      scale.x = std::min(1.0f, width / text_render_ctx_.width);
      break;
    }

    switch (text_alignment_)
    {
    case TextAlignments::kTextAlignLeft:
      break;
    case TextAlignments::kTextAlignRight:
      move.x += std::max(width * width_multiply_ - text_render_ctx_.width, 0.0f);
      break;
    case TextAlignments::kTextAlignCenter:
      move.x += std::max(width * width_multiply_ - text_render_ctx_.width, 0.0f) / 2;
      break;
    case TextAlignments::kTextAlignLR2Right:
      move.x -= width * width_multiply_ - std::max(width * width_multiply_ - text_render_ctx_.width, 0.0f);
      break;
    case TextAlignments::kTextAlignLR2Center:
      move.x -= (width * width_multiply_ - std::max(width * width_multiply_ - text_render_ctx_.width, 0.0f)) / 2;
      break;
    }
  }
  GRAPHIC->Scale(scale);
  GRAPHIC->Translate(move);
  GRAPHIC->SetBlendMode(blending_);

  // Draw vertex by given quad.
  // TODO: alpha?
  float alpha = GetCurrentFrame().color.a;
  for (auto i = tvi_start; i < tvi_end; ++i)
  {
    auto &tvi = text_render_ctx_.textvertex[i];
    tvi.vi[0].c.a = alpha;
    tvi.vi[1].c.a = alpha;
    tvi.vi[2].c.a = alpha;
    tvi.vi[3].c.a = alpha;
    GRAPHIC->SetTexture(0, tvi.texid);
    // TODO:
    // need optimizing -- Texture state is always changing, bad performance.
    // Maybe need to check texture id, and flush at once if same texture.
    GRAPHIC->DrawQuad(tvi.vi);
  }

  GRAPHIC->PopMatrix();

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

RHYTHMUS_NAMESPACE_END
