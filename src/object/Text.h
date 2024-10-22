#pragma once

#include "Font.h"
#include "Util.h"

RHYTHMUS_NAMESPACE_BEGIN

class LR2FnArgs;

enum TextFitting
{
  kTextFitNone,
  kTextFitMaxSize,
  kTextFitStretch,
};

class Text : public BaseObject
{
public:
  Text();
  Text(const Text &text);
  virtual ~Text();
  virtual BaseObject *clone();

  virtual void Load(const MetricGroup &metric);
  virtual void OnReady();
  using BaseObject::RunCommandLR2;
  virtual void RunCommandLR2(const std::string& command, const LR2FnArgs& args);

  void SetFont(const std::string& path);
  void SetFont(const MetricGroup &m);
  void ClearFont();

  float GetTextWidth() const;
  virtual void SetText(const std::string& s);
  void ClearText();
  void SetTextResource(const std::string &resourcename);
  virtual void Refresh();
  void SetTextFitting(TextFitting fitting);
  void SetLineBreaking(bool enable_line_break);
  void SetTextAlignment(float x, float y);
  void SetLR2StyleText(bool v);

  virtual void OnText(uint32_t codepoint);

  Font *font();
  virtual const char* type() const;
  virtual std::string toString() const;

protected:
  virtual void OnAnimation(DrawProperty &frame);
  virtual void doRender();
  virtual void doUpdate(double);
  void UpdateTextRenderContext();
  TextVertexInfo& AddTextVertex(const TextVertexInfo &tvi);

private:
  // Font.
  Font *font_;

  // text to be rendered
  std::string text_;

  // text_rendering related context
  struct {
    // glyph vertex generated by textglyph
    std::vector<TextVertexInfo> textvertex;

    // glyph vertex to be rendered
    std::vector<VertexInfo> vi;

    // calculated text width / height
    float width, height;

    // targeted draw size
    // used for scaling when actually rendered
    // (set when UpdateTextRenderContext() called)
    Vector2 drawsize;
  } text_render_ctx_;

  TextFitting text_fitting_;
  Vector2 text_alignment_;

  // LR2 text has special coordination system; its x,y coord
  // means 'Center' of the alignment, not topleft point.
  // To enable it, set this to true.
  bool set_xy_aligncenter_;

  // Use height value as font height (used for LR2)
  bool use_height_as_font_height_;

  // additional font attributes, which is set internally by font_alignment_ option.
  struct {
    // scale x / y
    float sx, sy;
    // translation x / y
    float tx, ty;
  } alignment_attrs_;

  bool editable_;

  bool autosize_;

  int blending_;

  // internal counter for updating Text object intervally.
  unsigned counter_;

  std::string *res_id_;

  // is line-breaking enabled?
  bool do_line_breaking_;
};

RHYTHMUS_NAMESPACE_END
