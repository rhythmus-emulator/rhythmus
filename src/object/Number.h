#pragma once

#include "Sprite.h"
#include "Text.h"

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

/* @brief Number based on Text object
 * TODO: separate it with NumberSprite */
class Number : public Text
{
public:
  Number();
  virtual ~Number();

  virtual void Load(const MetricGroup& metric);

  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  virtual void SetText(const std::string &s);
  virtual void Refresh();
  NumberFormatter &GetFormatter();

private:
  void AllocNumberGlyph(size_t cycles);
  void CreateTextVertexFromSprite();
  void CreateTextVertexFromFont();
  virtual void doUpdate(double);

  Sprite numbersprite_;
  NumberFormatter formatter_;
  int *res_ptr_;

  /* 0 ~ 9 : positive number glyphs
   * 10 : positive zero(empty) number glyph 
   * 11 : plus glyph
   * 12 ~ 21 : negative number glyphs
   * 22 : negative zero(empty) number glyph
   * 23 : minus glyph */
  TextVertexInfo *tvi_glyphs_;
};

}
