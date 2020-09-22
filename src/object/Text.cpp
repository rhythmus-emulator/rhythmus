#include "Text.h"
#include "ResourceManager.h"
#include "Script.h"
#include "KeyPool.h"
#include "Logger.h"
#include "Util.h"
#include "common.h"
#include "config.h"
#include <sstream>

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
    MetricGroup *fontmetric = METRIC->get_group(m.get_str("name"));
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
  MetricGroup *fontmetric = METRIC->get_group(path);
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

void Text::SetTextResource(const std::string &resname)
{
  KeyData<std::string> kdata = KEYPOOL->GetString(resname);
  res_id_ = &*kdata;
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

void Text::SetTextAlignment(float x, float y)
{
  text_alignment_.x = x;
  text_alignment_.y = y;
}

void Text::SetLR2StyleText(bool v)
{
  set_xy_aligncenter_ = v;
  use_height_as_font_height_ = v;
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

const char* Text::type() const { return "Text"; }

std::string Text::toString() const
{
  std::stringstream ss;
  ss << "text: " << text_ << std::endl;
  ss << "text width/height: " << text_render_ctx_.width << "," << text_render_ctx_.height << std::endl;
  ss << "vertex count: " << text_render_ctx_.vi.size() << std::endl;
  ss << "text fitting: " << text_fitting_ << std::endl;
  ss << "alignment: " << text_alignment_.x << "," << text_alignment_.y << std::endl;
  ss << "scale: " << alignment_attrs_.sx << "," << alignment_attrs_.sy << std::endl;
  ss << "transition: " << alignment_attrs_.tx << "," << alignment_attrs_.ty << std::endl;
  ss << "editable: " << editable_ << std::endl;
  ss << "autosize: " << autosize_ << std::endl;
  ss << "blending: " << blending_ << std::endl;
  ss << "set_xy_aligncenter: " << set_xy_aligncenter_ << std::endl;
  ss << "use_height_as_font_height: " << use_height_as_font_height_ << std::endl;
  ss << "do_line_breaking: " << do_line_breaking_ << std::endl;
  return BaseObject::toString();
}

// ------------------------------------------------------------------ Loader/Helper

class LR2CSVTextandlers
{
public:
  static void src_text(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *o = _this ? (Text*)_this : (Text*)BaseObject::CreateObject("text");
    loader->set_object("text", o);

    o->SetFont(format_string("font%s", ctx->get_str(2)));

    /* track change of text table */
    std::string eventname = format_string("Text%s", ctx->get_str(3));
    o->AddCommand(eventname, "refresh");
    o->SubscribeTo(eventname);
    std::string resname = format_string("S%s", ctx->get_str(3));
    o->SetTextResource(resname);
    o->Refresh();   // manually refresh to fill text vertices

    /* alignment */
    const int lr2align = ctx->get_int(4);
    switch (lr2align)
    {
    case 0:
      // topleft
      o->SetTextFitting(TextFitting::kTextFitMaxSize);
      o->SetTextAlignment(0.0f, 0.0f);
      break;
    case 1:
      // topcenter
      o->SetTextFitting(TextFitting::kTextFitMaxSize);
      o->SetTextAlignment(0.5f, 0.0f);
      break;
    case 2:
      // topright
      o->SetTextFitting(TextFitting::kTextFitMaxSize);
      o->SetTextAlignment(1.0f, 0.0f);
      break;
    }
    o->SetLR2StyleText(true);

    /* editable (focusable) */
    if (ctx->get_int(5) > 0)
      o->SetFocusable(true);

    /* TODO: panel */
  }

  static void dst_text(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    const char *args[21];
    auto *o = _this ? (Text*)_this : (Text*)loader->get_object("text");
    if (!o)
    {
      Logger::Warn("Invalid #DST_TEXT command.");
      return;
    }

    for (unsigned i = 0; i < 21; ++i) args[i] = ctx->get_str(i);
    o->AddFrameByLR2command(args + 1);

    // these attributes are only affective for first run
    if (loader->get_command_index() == 0)
    {
      const int timer = ctx->get_int(17);

      // LR2 needs to keep its animation queue, so don't use stop.
      o->AddCommand(format_string("LR%d", timer), "replay");
      o->AddCommand(format_string("LR%dOff", timer), "hide");
      //o->SetBlending(ctx->get_int(12));
      //o->SetFiltering(ctx->get_int(13));
    }

    // TODO: load blending from LR2DST
    // TODO: fetch font size
  }

  LR2CSVTextandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_TEXT", (LR2CSVCommandHandler*)&src_text);
    LR2CSVExecutor::AddHandler("#DST_TEXT", (LR2CSVCommandHandler*)&dst_text);
  }
};

// register handler
LR2CSVTextandlers _LR2CSVTextandlers;

RHYTHMUS_NAMESPACE_END
