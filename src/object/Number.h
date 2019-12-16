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
  double number_;
  double disp_number_;
  float number_change_duration_;
  float number_change_remain_;
  bool need_update_;
  std::string number_format_;
  char num_str_[16];
};

/* @brief Number based on Sprite object */
class NumberSprite : public Sprite
{
public:
  NumberSprite();
  virtual ~NumberSprite();
  virtual void SetText(const std::string &num);
  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  virtual void Refresh();
  NumberFormatter &GetFormatter();

  void SetTextTableIndex(size_t idx);
  void SetLR2Alignment(int type);

  virtual void Load(const Metric& metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);

private:
  virtual void doUpdate(float);
  virtual void doRender();

  int align_;
  size_t data_index_;
  NumberFormatter formatter_;
  TextVertexInfo *tvi_glyphs_;
  std::vector<TextVertexInfo> tvi_;
};

/* @brief Number based on Text object */
class NumberText : public Text
{
public:
  NumberText();
  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  virtual void Refresh();
  NumberFormatter &GetFormatter();

  virtual void Load(const Metric& metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);

private:
  virtual void doUpdate(float);

  NumberFormatter formatter_;
};

}