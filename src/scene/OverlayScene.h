#pragma once
#include "Scene.h"

namespace rhythmus
{

class Dialog;

/* @brief Scene used for system message */
class OverlayScene : public Scene
{
public:
  OverlayScene();
  virtual ~OverlayScene() = default;

  virtual void LoadScene();
  virtual void StartScene();
  virtual void Load(const Metric& metric);
  virtual void CloseScene(bool next);
  virtual void ProcessInputEvent(const InputEvent& e);

  void AlertMessageBox(const std::string &title, const std::string &text);

private:
  Dialog *alert_dialog_;
  Dialog *log_dialog_;

  struct DialogMessages
  {
    std::string title;
    std::string message;
  };
  std::list<DialogMessages> dialog_msg_;

  virtual void doUpdate(float delta);
};

}