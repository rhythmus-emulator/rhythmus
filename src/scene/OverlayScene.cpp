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
  RegisterChild(alert_dialog_);
  RegisterChild(log_dialog_);
  alert_dialog_->Hide();
  log_dialog_->Hide();

  // invoke LoadByName() internally
  Scene::LoadScene();
}

void OverlayScene::StartScene()
{
  // do nothing here (overlayscene don't use fadein effect)
}

void OverlayScene::Load(const Metric& metric)
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

void OverlayScene::AlertMessageBox(const std::string &title, const std::string &text)
{
  dialog_msg_.push_back({ title, text });
}

void OverlayScene::doUpdate(float delta)
{
  Scene::doUpdate(delta);

  // if dialog is hidden and message exists, then display it
  if (!alert_dialog_->IsVisible())
  {
    if (!dialog_msg_.empty())
    {
      {
        DialogMessages &m = dialog_msg_.front();
        alert_dialog_->SetTitle(m.title);
        alert_dialog_->SetTitle(m.message);
        dialog_msg_.pop_front();
      }
      alert_dialog_->RunCommandByName("Open");
      alert_dialog_->Show();
    } else
    {
      SceneManager::getInstance().PauseScene(false);
    }
  }
  else
    SceneManager::getInstance().PauseScene(true);
}

}