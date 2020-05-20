#include "OverlayScene.h"
#include "SceneManager.h"
#include "object/Dialog.h"

namespace rhythmus
{

OverlayScene::OverlayScene() : alert_dialog_(nullptr), log_dialog_(nullptr)
{
  set_name("OverlayScene");
}

void OverlayScene::LoadScene()
{
  alert_dialog_ = new Dialog();
  log_dialog_ = new Dialog();
  alert_dialog_->set_name("dialog");
  log_dialog_->set_name("log_dialog");
  AddChild(alert_dialog_);
  AddChild(log_dialog_);
  alert_dialog_->Hide();
  log_dialog_->Hide();

  // invoke LoadByName() internally
  Scene::LoadScene();
}

void OverlayScene::StartScene()
{
  // do nothing here (overlayscene don't use fadein effect)
}

void OverlayScene::Load(const MetricGroup& metric)
{
  Scene::Load(metric);
}

void OverlayScene::CloseScene(bool next)
{
  Scene::CloseScene(next);
}

void OverlayScene::ProcessInputEvent(const InputEvent& e)
{
  // if dialog is running, then close it
  if (alert_dialog_->IsVisible())
    alert_dialog_->RunCommandByName("Close");
}

void OverlayScene::doUpdate(float delta)
{
  Scene::doUpdate(delta);

  // if dialog is hidden and message exists, then display it
  if (!alert_dialog_->IsVisible())
  {
    auto *msg = MessageBoxData::Peek();
    if (msg)
    {
      alert_dialog_->SetTitle(msg->title);
      alert_dialog_->SetText(msg->text);
      MessageBoxData::Pop();
      alert_dialog_->RunCommandByName("Open");
      alert_dialog_->Show();
    } else
    {
      GAME->Pause(false);
    }
  }
  else
    GAME->Pause(true);
}


void MessageBoxData::Add(int type, const std::string &title, const std::string &text)
{
  getInstance().data_.push_back({ type, title, text });
}

const MessageBoxData::Data *MessageBoxData::Peek()
{
  const auto &i = getInstance();
  if (i.data_.empty()) return nullptr;
  return &i.data_.front();
}

void MessageBoxData::Pop()
{
  auto &i = getInstance();
  if (!i.data_.empty())
    i.data_.pop_front();
}

MessageBoxData &MessageBoxData::getInstance()
{
  static MessageBoxData d;
  return d;
}

}