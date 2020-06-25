#include "Text.h"
#include "ResourceManager.h"
#include "Script.h"
#include "KeyPool.h"
#include "Util.h"
#include "common.h"
#include "config.h"

RHYTHMUS_NAMESPACE_BEGIN

// --------------------------------- class Text

Text::Text()
  : font_(nullptr),
    text_fitting_(TextFitting::kTextFitNone), set_xy_aligncenter_(false),
    use_height_as_font_height_(false), autosize_(false), blending_(0),
    res_id_(nullptr), do_line_breaking_(true)
{
  set_xy_as_center_ = true;
  alignment_attrs_.sx = alignment_attrs_.sy = 1.0f;
  alignment_attrs_.tx = alignment_attrs_.ty = .0f;
  text_render_ctx_.width = text_render_ctx_.height = .0f;
  text_alignment_ = Vector2(0.0f, 0.0f);  // TOPLEFT
}

Text::Text(const Text &text) : BaseObject(text), font_(nullptr),
  text_fitting_(text.text_fitting_),
  text_alignment_(text.text_alignment_), set_xy_aligncenter_(text.set_xy_aligncenter_),
  use_height_as_font_height_(text.use_height_as_font_height_),
  alignment_attrs_(text.alignment_attrs_),
  autosize_(text.autosize_), blending_(text.blending_), counter_(text.counter_),
  res_id_(text.res_id_), do_line_breaking_(text.do_line_breaking_)
{
  text_render_ctx_.drawsize = Vector2(0, 0);
  text_render_ctx_.width = 0;
  text_render_ctx_.height = 0;

  if (text.font_)
    font_ = (Font*)text.font_->clone();
}

Text::~Text()
{
  ClearFont();
}

BaseObject *Text::clone()
{
  return new Text(*this);
}

void Text::Load(const MetricGroup &m)
{
  BaseObject::Load(m);

  // Load autosize first before loading text.
  m.get_safe("autosize", autosize_);

  if (m.exist("path"))
  {
    SetFont(m);
  }
  else if (m.exist("name"))
  {
    // search for font MetricGroup with given name
    MetricGroup *fontmetric = FONTMAN->GetFontMetricFromName(m.get_str("name"));
    if (fontmetric)
      SetFont(*fontmetric);
  }

  if (m.exist("align"))
  {
    std::string alignstr = Upper(m.get_str("align"));
    if (alignstr == "CENTER") text_alignment_.x = 0.5f;
    else if (alignstr == "RIGHT") text_alignment_.x = 1.0f;
  }
  if (m.exist("valign"))
  {
    std::string alignstr = Upper(m.get_str("valign"));
    if (alignstr == "CENTER") text_alignment_.y = 0.5f;
    else if (alignstr == "RIGHT") text_alignment_.y = 1.0f;
  }
  if (m.exist("fit"))
  {
    std::string alignstr = Upper(m.get_str("fit"));
    if (alignstr == "MAX") text_fitting_ = TextFitting::kTextFitMaxSize;
    else if (alignstr == "STRETCH") text_fitting_ = TextFitting::kTextFitStretch;
  }

#if USE_LR2_FEATURE == 1
  if (m.exist("lr2src"))
  {
    std::string lr2src;
    m.get_safe("lr2src", lr2src);
    CommandArgs args(lr2src);

    /* fetch font size from lr2dst ... */
    SetFont(args.Get<std::string>(1));

    /* track change of text table */
    std::string eventname = "Text" + args.Get<std::string>(2);
    AddCommand(eventname, "refresh");
    SubscribeTo(eventname);
    std::string resname = "S" + args.Get<std::string>(2);
    KeyData<std::string> kdata = KEYPOOL->GetString(resname);
    res_id_ = &*kdata;
    Refresh();

    /* alignment */
    const int lr2align = args.Get<int>(3);
    switch (lr2align)
    {
    case 0:
      // topleft
      SetTextFitting(TextFitting::kTextFitMaxSize);
      text_alignment_.x = 0.0;
      text_alignment_.y = 0.0;
      break;
    case 1:
      // topcenter
      SetTextFitting(TextFitting::kTextFitMaxSize);
      text_alignment_.x = 0.5;
      text_alignment_.y = 0.0;
      break;
    case 2:
      // topright
      SetTextFitting(TextFitting::kTextFitMaxSize);
      text_alignment_.x = 1.0;
      text_alignment_.y = 0.0;
      break;
    }
    set_xy_aligncenter_ = true;
    use_height_as_font_height_ = true;

    /* editable (focusable) */
    if (args.Get<int>(4) > 0)
      SetFocusable(true);

    /* TODO: panel */
  }

  // TODO: load blending from LR2DST
#endif

  ClearText();
  if (m.exist("value"))
    text_ = m.get_str("value");
  else if (m.exist("text"))
    text_ = m.get_str("text");
}

void Text::OnReady()
{
  if (font_ == nullptr)
    return;
  font_->PrepareText(text_);
  font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);
  UpdateTextRenderContext();
}

void Text::SetFont(const std::string& path)
{
  ClearFont();

  // Search for font MetricGroup with given name.
  MetricGroup *fontmetric = FONTMAN->GetFontMetricFromName(path);
  if (fontmetric)
  {
    SetFont(*fontmetric);
    return;
  }

  // if not, then load from path directly.
  font_ = FONTMAN->Load(path);

  // if text previously exists, call SetText() internally.
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
  if (text_ == s) return;
  ClearText();
  text_ = s;
  if (!font_ || font_->is_loading()) return;

  font_->PrepareText(s);
  font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);
  UpdateTextRenderContext();
}

void Text::ClearText()
{
  text_render_ctx_.textvertex.clear();
  text_render_ctx_.vi.clear();
  text_render_ctx_.height = text_render_ctx_.width = 0;
  text_.clear();
}

// @warn
// Text position is set by this function, e.g. word warp
// So, If width/height changed after calling this function,
// text won't be displayed as expected.
void Text::UpdateTextRenderContext()
{
  text_render_ctx_.width = 0;
  text_render_ctx_.height = 0;
  text_render_ctx_.drawsize = Vector2(
    GetWidth(GetCurrentFrame().pos),
    GetHeight(GetCurrentFrame().pos)
  );

  // If no glyph, then maybe no glyph supported by this font.
  // XXX: Font must be loaded at Load(...) method. (synchronized)
  if (text_render_ctx_.textvertex.size() == 0)
    return;

  // copy calculated glyph vertices.
  for (const auto &tvi : text_render_ctx_.textvertex)
    for (unsigned i = 0; i < 4; ++i)
      text_render_ctx_.vi.push_back(tvi.vi[i]);

  // 1. calculate whole text width and height.
  // XXX: breaking loop if first cycle is over should be better?
  for (const auto &tvi : text_render_ctx_.textvertex)
  {
    text_render_ctx_.width = std::max(text_render_ctx_.width, tvi.vi[2].p.x);
    text_render_ctx_.height = std::max(text_render_ctx_.height, tvi.vi[2].p.y);
  }

  R_ASSERT(text_render_ctx_.width != 0 && text_render_ctx_.height != 0);

  // 2. resize textsize area (if necessary)
  if (use_height_as_font_height_)
  {
    text_render_ctx_.height = (float)font_->GetAttribute().height;
  }
  if (autosize_)
  {
    text_render_ctx_.drawsize.x = text_render_ctx_.width;
    text_render_ctx_.drawsize.y = text_render_ctx_.height;
    GetCurrentFrame().pos.z = GetCurrentFrame().pos.x + text_render_ctx_.drawsize.x;
    GetCurrentFrame().pos.w = GetCurrentFrame().pos.y + text_render_ctx_.drawsize.y;
  }

  Vector3 scale(1.0f, 1.0f, 1.0f);
  Vector3 centerpos(text_render_ctx_.width / 2.0f, text_render_ctx_.height / 2.0f, 0);
#if 0
  // text fitting/stretching
  switch (text_fitting_)
  {
  case TextFitting::kTextFitMaxSize:
    if (text_render_ctx_.drawsize.x != 0)
      scale.x = std::min(1.0f, text_render_ctx_.drawsize.x / text_render_ctx_.width);
    if (text_render_ctx_.drawsize.y != 0)
      scale.y = std::min(1.0f, text_render_ctx_.drawsize.y / text_render_ctx_.height);
    break;
  case TextFitting::kTextFitStretch:
    if (text_render_ctx_.drawsize.x != 0)
      scale.x = text_render_ctx_.drawsize.x / text_render_ctx_.width;
    if (text_render_ctx_.drawsize.y != 0)
      scale.y = text_render_ctx_.drawsize.y / text_render_ctx_.height;
    break;
  case TextFitting::kTextFitNone:
  default:
    break;
  }

  // 3. move text vertices (alignment)
  // consider newline if x == 0.
  // NOTE: default vertical alignment of text vertex is center.
  unsigned newline_idx = 0;
  if (text_alignment_.x != 0)
  {
    for (unsigned i = 0; i < text_render_ctx_.textvertex.size(); ++i)
    {
      bool end_of_line = false;
      if (i == text_render_ctx_.textvertex.size() - 1
        || text_render_ctx_.textvertex[i + 1].vi[0].p.x == 0)
        end_of_line = true;
      if (end_of_line)
      {
        float m =
          (text_render_ctx_.drawsize.x - text_render_ctx_.textvertex[i].vi[1].p.x)
          * text_alignment_.x;
        for (unsigned j = newline_idx; j <= i; ++j)
        {
          for (unsigned k = 0; k < 4; ++k)
            text_render_ctx_.vi[j*4+k].p.x += m;
        }
        newline_idx = i + 1;
      }
    }
  }
  if (text_alignment_.y != 0.5f)
  {
    float m =
      (text_render_ctx_.drawsize.y - text_render_ctx_.height)
      * (0.5f - text_alignment_.y);
    for (auto& vi : text_render_ctx_.vi)
      vi.p.y += m;
  }
#endif

  // 4. move (centering) and resize text vertices.
  for (auto& vi : text_render_ctx_.vi)
  {
    vi.p *= scale;
    vi.p -= centerpos;
  }
}

TextVertexInfo& Text::AddTextVertex(const TextVertexInfo &tvi)
{
  text_render_ctx_.textvertex.push_back(tvi);
  return text_render_ctx_.textvertex.back();
}

void Text::Refresh()
{
  if (res_id_)
    SetText(*res_id_);
  else
    UpdateTextRenderContext();
}

void Text::SetTextFitting(TextFitting fitting)
{
  text_fitting_ = fitting;
}

void Text::SetLineBreaking(bool enable_line_break)
{
  do_line_breaking_ = enable_line_break;
}

void Text::OnText(uint32_t codepoint)
{
  if (!editable_) return;

  if (codepoint == 0xFFFFFFFF)
  {
    // backspace; reserved codepoint
    // manually detect UTF8 start byte from end and delete.
    size_t pos = FindUtf8FirstByteReversed(text_);
    text_ = text_.substr(0, pos);
  }
  else
  {
    // convert between wchar <-> utf8
    char buf[6];
    unsigned size;
    ConvertUTF32ToUTF8(codepoint, buf, &size);
    for (unsigned i = 0; i < size; ++i)
      text_.push_back(buf[i]);
  }

  if (res_id_)
    *res_id_ = text_;

  // clear and update text vertex
  text_render_ctx_.textvertex.clear();
  text_render_ctx_.vi.clear();
  text_render_ctx_.height = text_render_ctx_.width = 0;
  font_->PrepareText(text_);
  font_->GetTextVertexInfo(text_, text_render_ctx_.textvertex, do_line_breaking_);
  UpdateTextRenderContext();
}

Font *Text::font() { return font_; }

void Text::OnAnimation(DrawProperty &frame)
{
  // simulate align center as topleft by making drawing size as zero
  if (set_xy_aligncenter_)
  {
    float dw = GetWidth(frame.pos) * text_alignment_.x;
    float dh = GetHeight(frame.pos) * text_alignment_.y;
    frame.pos.x -= dw;
    frame.pos.z -= dw;
    frame.pos.y -= dh;
    frame.pos.w -= dh;
  }
}

void Text::doUpdate(double delta)
{
  // hook animation position for LR2
  // (TODO)
}

void Text::doRender()
{
  if (!font_) return;
  const float width = GetWidth(GetCurrentFrame().pos);
  const float height = GetHeight(GetCurrentFrame().pos);
  float twidth = text_render_ctx_.width;
  float theight = text_render_ctx_.height;

  // calculate text scale (in case of stretch)
  Vector3 scale(1.0f, 1.0f, 1.0f);
  switch (text_fitting_)
  {
  case TextFitting::kTextFitMaxSize:
    scale.x = std::min(1.0f, width / text_render_ctx_.width);
    twidth = std::min(width, text_render_ctx_.width);
    break;
  case TextFitting::kTextFitStretch:
    scale.x = width / text_render_ctx_.width;
    twidth = width;
    break;
  case TextFitting::kTextFitNone:
  default:
    break;
  }
  if (scale.x != 1.0f)
    GRAPHIC->Scale(scale);

  // calculate text content position.
  Vector3 trans_content(0.0f, 0.0f, 0.0f);
  trans_content.x = (twidth - width) * (0.5f - text_alignment_.x);
  trans_content.y = (theight/2 - height) * (0.5f - text_alignment_.y) + theight/2;
  if (trans_content.x != 0 || trans_content.y != 0)
    GRAPHIC->Translate(trans_content);

  GRAPHIC->SetBlendMode(blending_);

  // Draw vertex by given quad.
  // optimize: draw glyph in group with same texid.
  float alpha = GetCurrentFrame().color.a;
  unsigned charsize = (unsigned)text_render_ctx_.textvertex.size();
  for (unsigned i = 0; i < charsize; )
  {
    auto &tvi = text_render_ctx_.textvertex[i];
    unsigned j = i;
    for (; j < charsize; ++j)
    {
      text_render_ctx_.vi[j * 4 + 0].c.a = alpha;
      text_render_ctx_.vi[j * 4 + 1].c.a = alpha;
      text_render_ctx_.vi[j * 4 + 2].c.a = alpha;
      text_render_ctx_.vi[j * 4 + 3].c.a = alpha;
      if (j == i) continue;
      // XXX: should compare texture id, or just pointer?
      if (text_render_ctx_.textvertex[i].tex != text_render_ctx_.textvertex[j].tex)
        break;
    }
    GRAPHIC->SetTexture(0, **text_render_ctx_.textvertex[i].tex);
    GRAPHIC->DrawQuads(&text_render_ctx_.vi[i * 4], (j - i) * 4);
    i = j;
  }
}

RHYTHMUS_NAMESPACE_END
