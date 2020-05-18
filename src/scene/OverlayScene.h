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
  virtual void Load(const MetricGroup& metric);
  virtual void CloseScene(bool next);
  virtual void ProcessInputEvent(const InputEvent& e);

private:
  Dialog *alert_dialog_;
  Dialog *log_dialog_;

  virtual void doUpdate(float delta);
};

enum MessageBoxTypes
{
  kMbNone,
  kMbInformation,
  kMbWarning,
  kMbCritical,
};

/* @brief Container of data of dialog messages. */
class MessageBoxData
{
public:
  struct Data
  {
    int type;
    std::string title;
    std::string text;
  };

  static void Add(int type, const std::string &title, const std::string &text);
  static const Data *Peek();
  static void Pop();
  static MessageBoxData &getInstance();

private:
  std::list<Data> data_;
};

}