#pragma once

#include "Sprite.h"
#include "Font.h"

namespace rhythmus
{

class LR2FnArgs;
constexpr size_t kMaxNumberVertex = 16;

/* @brief Optimized for displaying number. */
class Number : public BaseObject
{
public:
  Number();
  virtual ~Number();

  virtual void Load(const MetricGroup &metric);
  virtual void OnReady();
  using BaseObject::RunCommandLR2;
  virtual void RunCommandLR2(const std::string& command, const LR2FnArgs& args);

  virtual void SetGlyphFromFont(const MetricGroup &m);
  virtual void SetGlyphFromImage(const std::string &path, const Rect &clip,
                                 int divx, int divy, int digitcount);

  void SetAlignment(int align);
  void SetDigit(int count);
  void SetResizeToBox(bool v);
  void SetBlending(int blending);
  void SetResourceId(const std::string &resourceid);
  void SetLoopCycle(int cycle);
  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  virtual void SetText(const std::string &s);
  virtual void Refresh();

  virtual const char* type() const;
  virtual std::string toString() const;

private:
  Image *img_;
  Font *font_;

  int blending_;

  /* 0 ~ 9 : positive number glyphs
   * 10 : positive zero(empty) number glyph 
   * 11 : plus glyph
   * 12 ~ 21 : negative number glyphs
   * 22 : negative zero(empty) number glyph
   * 23 : minus glyph
   * multiply by cycle_count. */
  TextVertexInfo *tvi_glyphs_;

  /* number glyphs to render. */
  VertexInfo render_glyphs_[kMaxNumberVertex][4];
  const Texture* tex_[kMaxNumberVertex];
  float text_width_;
  float text_height_;

  /* number glyphs vertex size. */
  unsigned render_glyphs_count_;

  // cycle count for number sprite
  // @warn  must modified by AllocNumberGlyph(...)
  unsigned cycle_count_;

  // time per cycle (zero is non-cycle)
  unsigned cycle_time_;
  double cycle_curr_time_;

  // number characters to display
  char num_chrs[256];

  // reference to value (if necessary)
  int *val_ptr_;

  // width multiply (for LR2 specification)
  int keta_;

  // resize to drawing box
  bool resize_to_box_;

  // displaying value
  struct {
    int start;
    int end;
    int curr;
    double time;
    double rollingtime;   // number rolling time
    int max_string;       // maximum size of whole glyph
    int max_decimal;      // maximum size of decimal area
    int fill_empty_zero;  // fill empty zero when string is less than max_string
                          // (0: none, 1: left, 2: center, 3: right)
  } value_params_;

  void SetNumberInternal(int number);
  void AllocNumberGlyph(unsigned cycles);
  void ClearAll();
  void UpdateNumberStr();
  void UpdateVertex();
  virtual void OnAnimation(DrawProperty &frame);
  virtual void doUpdate(double);
  virtual void doRender();
};

}
