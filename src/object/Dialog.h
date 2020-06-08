#pragma once
#include "BaseObject.h"
#include "Sprite.h"
#include "Text.h"

namespace rhythmus
{

enum class DialogType
{
  kOkDialog,
  kYesNoDialog,
  kRetryDialog
};

class Dialog : public Sprite
{
public:
  Dialog();
  virtual ~Dialog();
  virtual void Load(const MetricGroup &metric);
  void SetDialogType(DialogType type);
  void SetTitle(const std::string &text);
  void SetText(const std::string &text);

private:
  /* type of dialog */
  DialogType dialog_type_;

  /* padding attribute */
  Vector4 padding_;
  float title_height_;
  float button_height_;

  /* Text */
  Text title_, text_;

  virtual void doUpdate(float delta);
};

}