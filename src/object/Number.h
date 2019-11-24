#pragma once

#include "Sprite.h"
#include "Text.h"

namespace rhythmus
{

class NumberInterface
{
public:
  NumberInterface();
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
  char num_str_[128];
};

/* @brief Number based on Sprite object */
class NumberSprite : public Sprite, public NumberInterface
{
public:
  NumberSprite();
  void SetNumberText(const char* num);
  void SetTextTableIndex(size_t idx);
  void SetLR2Alignment(int type);

  virtual void Load(const Metric& metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);

private:
  virtual void SetTextFromTable();
  virtual void CreateCommandFnMap();
  virtual void doUpdate(float);
  virtual void doRender();

  int align_;
  size_t data_index_;
  std::vector<TextVertexInfo> tvi_;
};

/* @brief Number based on Text object */
class NumberText : public Text, public NumberInterface
{
public:
  NumberText();
  virtual void Load(const Metric& metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);

private:
  virtual void SetTextFromTable();
  virtual void CreateCommandFnMap();
  virtual void doUpdate(float);
};

}