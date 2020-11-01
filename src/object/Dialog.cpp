#include "Dialog.h"
#include "common.h"

namespace rhythmus
{

Dialog::Dialog() :
  dialog_type_(DialogType::kOkDialog),
  title_height_(0), button_height_(0)
{
  memset(&padding_, 0, sizeof(padding_));
}

Dialog::~Dialog() {}

void Dialog::Load(const MetricGroup &metric)
{
  // Add childs with fixed member
  AddChild(&background_);
  AddChild(&title_);
  AddChild(&text_);

  BaseObject::Load(metric);

  // Load background image
  if (metric.exist("path"))
    background_.SetImage(metric.get_str("path"));
  else if (metric.exist("src"))
    background_.SetImage(metric.get_str("src"));
  /*if (metric.exist("crop"))
    background_.SetImageCoord(metric.get_str("crop"));*/

  // Load fonts for text/title
  if (metric.exist("title_font"))
    title_.SetFont(metric.get_str("title_font"));
  if (metric.exist("text_font"))
    text_.SetFont(metric.get_str("text_font"));
  if (metric.exist("title"))
    title_.SetText(metric.get_str("title"));
  if (metric.exist("text"))
    text_.SetText(metric.get_str("text"));

  // check for additional attribute for text/title
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

void Dialog::doUpdate(double delta)
{
  if (IsVisible())
  {
    // update size of bg / title / text
    float width = rhythmus::GetWidth(GetCurrentFrame().pos) - padding_[1] - padding_[3];
    float height = rhythmus::GetHeight(GetCurrentFrame().pos) - padding_[0] - padding_[2];
    background_.SetSize(
      rhythmus::GetWidth(GetCurrentFrame().pos),
      rhythmus::GetHeight(GetCurrentFrame().pos)
    );
    title_.SetPos(Vector4{ padding_[1], padding_[0],
      width, title_height_ });
    text_.SetPos(Vector4{ padding_[1], padding_[0] + title_height_,
      width, height - title_height_ - button_height_ });
  }

  // TODO: set pos, visibility of dialog_type
}

// TODO: need to check InputEvent per BaseObject.
// If it receives, return true.
// Dialog should receive event, and set focus for itself.

}