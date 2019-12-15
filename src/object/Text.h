#pragma once

#include "Font.h"
#include "Util.h"

RHYTHMUS_NAMESPACE_BEGIN

enum TextAlignments
{
  kTextAlignLeft,
  kTextAlignRight,
  kTextAlignCenter,
  kTextAlignFitMaxsize,
  kTextAlignCenterFitMaxsize,
  kTextAlignRightFitMaxsize,
  kTextAlignStretch,
};

class Text : public BaseObject
{
public:
  Text();
  virtual ~Text();

  void SetFontByPath(const std::string& path);
  void SetSystemFont();

  float GetTextWidth() const;
  void SetText(const std::string& s);
  void SetTextTableIndex(size_t idx);
  size_t GetTextTableIndex() const;
  void SetAlignment(TextAlignments align);
  void SetTextPosition(int position_attr);
  void SetLineBreaking(bool enable_line_break);
  void Clear();

  Font *font();

  virtual void Load(const Metric& metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);

protected:
  void SetFont(Font *font);
  virtual void SetTextFromTable();
  virtual void SetLR2Alignment(int alignment);
  virtual void doRender();
  virtual void doUpdate(float);
  virtual const CommandFnMap& GetCommandFnMap();

private:
  // Font.
  Font *font_;

  // text to be rendered
  std::string text_;

  // text_rendering related context
  struct {
    // glyph vertex to be rendered (generated by textglyph)
    std::vector<TextVertexInfo> textvertex;

    // calculated text width / height
    float width, height;
  } text_render_ctx_;

  // text alignment(stretch) option
  TextAlignments text_alignment_;

  // text position option
  // 0 : normal / 1 : offset -50% of text width / 2 : offset -100% of text width
  // (LR2 legacy option)
  int text_position_;

  // additional font attributes, which is set internally by font_alignment_ option.
  struct {
    // scale x / y
    float sx, sy;
    // translation x / y
    float tx, ty;
  } alignment_attrs_;

  // is line-breaking enabled?
  bool do_line_breaking_;

  // text table index
  size_t table_index_;

  virtual void SetLR2DSTCommandInternal(const CommandArgs &args);
};

RHYTHMUS_NAMESPACE_END