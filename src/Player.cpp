#include "Player.h"
#include "SongPlayer.h"
#include "Timer.h"
#include "Util.h"
#include "Error.h"
#include <iostream>

namespace rhythmus
{

static Player *players_[kMaxPlaySession];
static int player_count;


// ------------------------------- class Player

Player::Player(PlayerTypes player_type, const std::string& player_name)
  : player_type_(player_type), player_name_(player_name), is_guest_(false),
  use_lane_cover_(false), use_hidden_(false),
  game_speed_type_(GameSpeedTypes::kSpeedConstant),
  game_speed_(1.0), game_constant_speed_(1.0),
  lane_cover_(0.0), hidden_(0.0),
  bms_sp_class_(0), bms_dp_class_(0),
  play_context_(nullptr),
  is_courseplay_(false), is_replay_(false), is_save_allowed_(true)
{
  if (player_type_ == PlayerTypes::kPlayerGuest)
    is_guest_ = true;

  // XXX: default keysetting should be here?
  memset(default_keysetting_.keycode_per_track_, 0, sizeof(KeySetting));
  default_keysetting_.keycode_per_track_[0][0] = GLFW_KEY_A;
  default_keysetting_.keycode_per_track_[1][0] = GLFW_KEY_S;
  default_keysetting_.keycode_per_track_[2][0] = GLFW_KEY_D;
  default_keysetting_.keycode_per_track_[3][0] = GLFW_KEY_SPACE;
  default_keysetting_.keycode_per_track_[4][0] = GLFW_KEY_J;
  default_keysetting_.keycode_per_track_[5][0] = GLFW_KEY_K;
  default_keysetting_.keycode_per_track_[6][0] = GLFW_KEY_L;
  default_keysetting_.keycode_per_track_[7][0] = GLFW_KEY_LEFT_SHIFT;
  default_keysetting_.keycode_per_track_[7][1] = GLFW_KEY_SEMICOLON;
  curr_keysetting_ = &default_keysetting_;

  // TODO: use preset options like game setting does
#if 0
  setting_.Load<double>("speed", game_speed_);
  setting_.Load<bool>("use_hidden", use_hidden_);
  setting_.Load<bool>("use_lane_cover", use_lane_cover_);
  setting_.Load<double>("hidden", hidden_);
  setting_.Load<double>("lanecover", lane_cover_);
  setting_.Load<double>("game_speed", game_speed_);
  setting_.Load<double>("game_constant_speed", game_constant_speed_);
  setting_.Load<int>("option_chart", option_chart_);
  setting_.Load<int>("option_chart_dp", option_chart_dp_);
  setting_.Load<int>("health_type", health_type_);
  setting_.Load<int>("assist", assist_);
  setting_.Load<int>("pacemaker", pacemaker_);
  setting_.Load<std::string>("pacemaker_target", pacemaker_target_);
  setting_.Load<int>("bms_sp_class", bms_sp_class_);
  setting_.Load<int>("bms_dp_class", bms_dp_class_);
#endif

  config_path_ = format_string("../player/%s.xml", player_name_.c_str());
}

Player::~Player()
{
  Save();
}

void Player::Load()
{
  if (!config_.ReadFromFile(config_path_))
  {
    std::cerr << "Cannot load player preference (maybe default setting will used)"
      << std::endl;
    return;
  }

  // TODO: load keysetting

  // TODO: load playrecords
}

void Player::Save()
{
  if (is_guest_)
    return;

  config_.SaveToFile(config_path_);

  // TODO: save playrecords
}

/* XXX: unsafe if playrecords is modified */
const PlayRecord *Player::GetPlayRecord(const std::string &chartname) const
{
  for (auto &r : playrecords_)
    if (r.chartname == chartname)
      return &r;
  return nullptr;
}

void Player::GetReplayList(const std::vector<std::string> &replay_names)
{
  // TODO
}

void Player::SetCurrentPlay(const PlayRecord &playrecord, const ReplayData &replaydata)
{
  // TODO: accumulate playrecord & playrecord.
}

void Player::SaveCurrentPlay()
{
  if (playrecord_.chartname.empty()) return;
  for (size_t i = 0; i < playrecords_.size(); ++i)
  {
    if (playrecords_[i].chartname == playrecord_.chartname)
    {
      playrecords_[i] = playrecord_;
      return;
    }
  }
  // TODO: save replay file as data, or as hash64 in db?
  // TODO: use SQL insert using SQLwrapper
  playrecords_.push_back(playrecord_);
}

void Player::ClearCurrentPlay()
{
  playrecord_.chartname.clear();
  playrecord_.pg =
    playrecord_.gr =
    playrecord_.gd =
    playrecord_.bd =
    playrecord_.pr = 0;
}

// ------- static methods --------

Player& Player::getMainPlayer()
{
  for (int i = 0; i < kMaxPlaySession; ++i)
    if (players_[i])
      return *players_[i];

  /* This shouldn't happened */
  ASSERT(false);
  return *players_[0];
}

void Player::CreatePlayer(PlayerTypes playertype, const std::string& player_name)
{
  for (int i = 0; i < kMaxPlaySession; ++i)
  {
    if (!players_[i])
    {
      CreatePlayer(playertype, player_name, i);
      return;
    }
  }
  std::cerr << "Failed to add new player: Player slot is full" << std::endl;
}

void Player::CreatePlayer(PlayerTypes playertype, const std::string& player_name, int player_slot)
{
  ASSERT(player_slot < kMaxPlaySession);
  if (players_[player_slot])
  {
    // already player created at the slot - skip
    return;
  }

  // TODO: need switch ~ case later

  players_[player_slot] = new Player(playertype, player_name);
  player_count++;
}

Player* Player::getPlayer(int player_slot)
{
  ASSERT(player_slot < kMaxPlaySession);

  return players_[player_slot];
}

void Player::UnloadPlayer(int player_slot)
{
  ASSERT(player_slot < kMaxPlaySession);
  if (players_[player_slot])
  {
    delete players_[player_slot];
    players_[player_slot] = nullptr;
    --player_count;
  }
  ASSERT(player_count >= 0);
}

bool Player::IsPlayerLoaded(int player_slot)
{
  ASSERT(player_slot < kMaxPlaySession);
  return getPlayer(player_slot) != nullptr;
}

int Player::GetLoadedPlayerCount()
{
  return player_count;
}

bool Player::IsAllPlayerFinished()
{
  for (int i = 0; i < kMaxPlaySession; ++i) if (players_[i])
  {
    auto *pctx = players_[i]->play_context_;
    if (!pctx) continue;
    if (!pctx->is_finished())
      return false;
  }
  return true;
}

void Player::Initialize()
{
  /* Set Guest player in first slot by default */
  players_[0] = new Player(PlayerTypes::kPlayerGuest, "GUEST");
}

void Player::Cleanup()
{
  /* Clear all player */
  for (int i = 0; i < kMaxPlaySession; ++i) if (players_[i])
  {
    delete players_[i];
    players_[i] = nullptr;
  }
}

void Player::SetRunningCombo(int combo) { running_combo_ = combo; }
int Player::GetRunningCombo() const { return running_combo_; }

}