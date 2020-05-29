#pragma once

#include "Sprite.h"
#include "Font.h"

namespace rhythmus
{

class NumberFormatter
{
public:
  NumberFormatter();
  void SetNumber(int number);
  void SetNumber(double number);
  template <typename T> T GetNumber() const;
  void SetNumberChangeTime(float msec);

  /**
   * Set number format
   * e.g. %04d : show number always 4 digits
   *      %3d : show number at least 3 digits
   *      +%03d : show number always 3 digits with plus/minus sign always
   *      %03f : show number always 3 digits + decimal (if decimal exists)
   *      %02.2f : show number in form of 2 digits + 2 decimal always.
   *      %f : show number in full digits
   */
  void SetFormat(const std::string& number_format);

  /* @brief Update number text. return true if text is updated. */
  bool UpdateNumber(float delta);

  const char* numstr() const;

private:
  double number_, from_number_;
  double disp_number_;
  float number_change_duration_;
  float number_change_remain_;
  bool need_update_;
  std::string number_format_;
  char num_str_[16];
};

constexpr size_t kMaxNumberVertex = 64;

/* @brief Optimized for displaying number. */
class Number : public BaseObject
{
public:
  Number();
  virtual ~Number();

  virtual void Load(const MetricGroup& metric);

  virtual void SetGlyphFromFont(const MetricGroup &m);
  virtual void SetGlyphFromLR2SRC(const std::string &lr2src);

  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  virtual void SetText(const std::string &s);
  virtual void Refresh();

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

  /* number glyphs vertex size. */
  unsigned render_glyphs_count_;

  // cycle count for number sprite
  // @warn  must modified by AllocNumberGlyph(...)
  unsigned cycle_count_;

  // time per cycle (zero is non-cycle)
  unsigned cycle_time_;

  // number characters to display
  char num_chrs[256];

  // reference to value (if necessary)
  int *val_ptr_;

  // font display style (for LR2 alignment)
  int display_style_;

  // width multiply (for LR2 specification)
  int keta_;

  // displaying value
  struct {
    double start;
    double end;
    double curr;
    double time;
    double rollingtime; // number rolling time
    int max_int;        // maximum size of integer area
    int max_decimal;    // maximum size of decimal area
  } value_params_;

  void AllocNumberGlyph(unsigned cycles);
  void ClearAll();
  void UpdateVertex();
  virtual void doUpdate(double);
  virtual void doRender();
  virtual void UpdateRenderingSize(Vector2 &d, Vector3 &p);
};

}
