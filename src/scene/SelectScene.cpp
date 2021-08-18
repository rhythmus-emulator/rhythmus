#include "SelectScene.h"
#include "Event.h"
#include "SceneManager.h"
#include "Player.h"
#include "Song.h"
#include "SongPlayer.h"
#include "object/LR2MusicWheel.h"
#include "ScriptLR2.h"

namespace rhythmus
{


// -------------------------- class SelectScene

SelectScene::SelectScene() : wheel_(nullptr)
{
  set_name("SelectScene");
  next_scene_ = "DecideScene";
  prev_scene_ = "Exit";
}

void SelectScene::LoadScene()
{
  MetricValue<std::string> bgm_path("SelectSceneBgm");

  // Before starting, unload song and create player instance.
  SongPlayer::getInstance().Stop();
  PlayerManager::CreateGuestPlayerIfEmpty();

  // load scene script and resources
  Scene::LoadScene();

  // load remaining resources
  bgm_.Load(*bgm_path);
}

void SelectScene::StartScene()
{
  Scene::StartScene();
  bgm_.Play();
  EVENTMAN->SendEvent("SelectSceneLoad");
}

void SelectScene::CloseScene(bool next)
{
  if (next)
  {
    // push selected song info to SongResource & Players for playing.

  }

  Scene::CloseScene(next);
}

void SelectScene::ProcessInputEvent(const InputEvent& e)
{
  if (e.type() == InputEvents::kOnKeyUp && e.KeyCode() == RI_KEY_ESCAPE)
  {
    // Exit program instantly.
    CloseScene(false);
    return;
  }

  Scene::ProcessInputEvent(e);

  if (e.type() == InputEvents::kOnKeyDown || e.type() == InputEvents::kOnKeyPress) {
    switch (e.KeyCode()) {
    case RI_KEY_UP:
      wheel_->MoveUp();
      break;
    case RI_KEY_DOWN:
      wheel_->MoveDown();
      break;
    case RI_KEY_RIGHT:
      wheel_->Expand();
      break;
    case RI_KEY_LEFT:
      wheel_->Collapse();
      break;
    }
  }

  if (e.type() == InputEvents::kOnKeyUp)
  {
    if (e.KeyCode() == RI_KEY_ENTER)
    {
      EVENTMAN->SendEvent("MusicWheelDecide");

      // Register select song / course into Game / Player state.
      // XXX: can we preload selected song from here before PlayScene...?
      // XXX: what about course selection? select in gamemode?
      std::string id = wheel_->GetSelectedItemId();
      SongPlayer::getInstance().AddSongtoPlaylistFromId(id);
#if 0
      Game::getInstance().push_song(d.info.songpath);
      FOR_EACH_PLAYER(p, i)
      {
        // TODO: different chart selection for each player
        p->AddChartnameToPlay(d.name);
      }
      END_EACH_PLAYER()
#endif

      // Song selection - immediately change scene mode
      CloseScene(true);
    }
  }
}

Wheel* SelectScene::GetWheelObject()
{
  if (wheel_) return wheel_;
  wheel_ = new LR2MusicWheel();
  AddChild(wheel_);
  return wheel_;
}

LR2MusicWheel* SelectScene::GetLR2WheelObject()
{
  if (wheel_ == nullptr) {
    wheel_ = new LR2MusicWheel();
    AddChild(wheel_);
  }
  return dynamic_cast<LR2MusicWheel*>(wheel_);
}


#define HANDLERLR2_OBJNAME SelectScene
REGISTER_LR2OBJECT(SelectScene);

class SelectSceneLR2Handler : public LR2FnClass {
public:
  static void PropagateToMusicWheel(SelectScene *scene, const LR2FnArgs &args) {
    auto* wheel = scene->GetLR2WheelObject();
    wheel->RunCommand(args);
  }

  HANDLERLR2(SRC_BAR_BODY) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_BAR_BODY_OFF) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_BAR_BODY_ON) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(BAR_CENTER) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(BAR_AVAILABLE) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(SRC_BAR_TITLE) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_BAR_TITLE) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(SRC_BAR_FLASH) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_BAR_FLASH) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(SRC_BAR_LEVEL) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_BAR_LEVEL) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(SRC_BAR_LAMP) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_BAR_LAMP) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(SRC_MY_BAR_LAMP) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_MY_BAR_LAMP) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(SRC_RIVAL_BAR_LAMP) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_RIVAL_BAR_LAMP) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(SRC_RIVAL_LAMP) {
    PropagateToMusicWheel(o, args);
  }
  HANDLERLR2(DST_RIVAL_LAMP) {
    PropagateToMusicWheel(o, args);
  }
  SelectSceneLR2Handler() : LR2FnClass(
    GetTypename<SelectScene>(), GetTypename<Scene>()
  ) {
    ADDSHANDLERLR2(SRC_BAR_BODY);
    ADDSHANDLERLR2(DST_BAR_BODY_OFF);
    ADDSHANDLERLR2(DST_BAR_BODY_ON);
    ADDSHANDLERLR2(BAR_CENTER);
    ADDSHANDLERLR2(BAR_AVAILABLE);
    ADDSHANDLERLR2(SRC_BAR_TITLE);
    ADDSHANDLERLR2(DST_BAR_TITLE);
    ADDSHANDLERLR2(SRC_BAR_FLASH);
    ADDSHANDLERLR2(DST_BAR_FLASH);
    ADDSHANDLERLR2(SRC_BAR_LEVEL);
    ADDSHANDLERLR2(DST_BAR_LEVEL);
    ADDSHANDLERLR2(SRC_BAR_LAMP);
    ADDSHANDLERLR2(DST_BAR_LAMP);
    ADDSHANDLERLR2(SRC_MY_BAR_LAMP);
    ADDSHANDLERLR2(DST_MY_BAR_LAMP);
    ADDSHANDLERLR2(SRC_RIVAL_BAR_LAMP);
    ADDSHANDLERLR2(DST_RIVAL_BAR_LAMP);
    ADDSHANDLERLR2(SRC_RIVAL_LAMP);
    ADDSHANDLERLR2(DST_RIVAL_LAMP);
  }
};

SelectSceneLR2Handler _SelectSceneLR2Handler;

}