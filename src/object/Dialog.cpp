#include "Dialog.h"
#include "common.h"

namespace rhythmus
{

Dialog::Dialog() : title_height_(0), dialog_type_(DialogType::kOkDialog)
{
  memset(&padding_, 0, sizeof(padding_));
}

Dialog::~Dialog() {}

void Dialog::Load(const MetricGroup &metric)
{
  // Add childs with fixed member
  SetOwnChildren(false);
  AddChild(&title_);
  AddChild(&text_);

  Sprite::Load(metric);

  // Load fonts
  if (metric.exist("title_font"))
    title_.SetFont(metric.get_str("title_font"));
  if (metric.exist("text_font"))
    text_.set_name("text_font");

  // check for additional attribute
  const MetricGroup *title_m = metric.get_group("title");
  if (title_m) title_.Load(*title_m);
  const MetricGroup *text_m = metric.get_group("text");
  if (text_m) text_.Load(*text_m);

  // Load other metrics
  if (metric.exist("padding"))
  {
    CommandArgs args(metric.get_str("padding"), 4, true);
    padding_ = Vector4{ args.Get<float>(0), args.Get<float>(1),
                        args.Get<float>(2), args.Get<float>(3) };
  }
  if (metric.exist("dialog_type"))
  {
    std::string dtype;
    metric.get_safe("dialog_type", dtype);
    if (dtype == "yesno") {
      SetDialogType(DialogType::kYesNoDialog);
    }
    else if (dtype == "retry") {
      SetDialogType(DialogType::kRetryDialog);
    }
    else /*if (dtype == "ok")*/ {
      SetDialogType(DialogType::kOkDialog);
    }
  }
  metric.get_safe("title_height", title_height_);
  metric.get_safe("button_height", button_height_);
}

void Dialog::SetDialogType(DialogType type)
{
  // TODO: hide/show yes,no,retry button
  dialog_type_ = type;
}

void Dialog::SetTitle(const std::string &text)
{
  title_.SetText(text);
}

void Dialog::SetText(const std::string &text)
{
  text_.SetText(text);
}

void Dialog::doUpdate(float delta)
{
  // update size of title / text
  float width = GetWidth(GetCurrentFrame().pos) - padding_[1] - padding_[3];
  float height = GetHeight(GetCurrentFrame().pos) - padding_[0] - padding_[2];
  title_.SetPos(Vector4{ padding_[1], padding_[0],
    width, title_height_ });
  text_.SetPos(Vector4{ padding_[1], padding_[0] + title_height_,
    width, height - title_height_ - button_height_ });

  // TODO: set pos, visibility of dialog_type
}

// TODO: need to check InputEvent per BaseObject.
// If it receives, return true.
// Dialog should receive event, and set focus for itself.

}