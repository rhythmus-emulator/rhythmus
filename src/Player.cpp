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
  is_courseplay_(false)
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

  // load playrecords
  LoadPlayRecords();
}

void Player::LoadPlayRecords()
{
}

void Player::Save()
{
  if (is_guest_)
    return;

  config_.SaveToFile(config_path_);

  // save playrecords
  SavePlayRecords();
}

void Player::SavePlayRecords()
{

}

void Player::Sync()
{
  // TODO: sync data from network
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

void Player::SetRunningCombo(int combo) { running_combo_ = combo; }
int Player::GetRunningCombo() const { return running_combo_; }

// ------- static methods --------

Player* PlayerManager::GetPlayer()
{
  Player *r = nullptr;
  for (size_t i = 0; !r && i < kMaxPlaySession; ++i) r = players_[i];
  ASSERT(r);
  return r;
}

Player* PlayerManager::GetPlayer(int player_slot)
{
  ASSERT(player_slot < kMaxPlaySession);

  return players_[player_slot];
}

void PlayerManager::CreatePlayer(PlayerTypes playertype,
  const std::string &player_name,
  const std::string &passwd)
{
  for (int i = 0; i < kMaxPlaySession; ++i)
  {
    if (!players_[i])
    {
      CreatePlayer(playertype, player_name, passwd, i);
      return;
    }
  }
  std::cerr << "Failed to add new player: Player slot is full" << std::endl;
}

void PlayerManager::CreatePlayer(PlayerTypes playertype,
  const std::string &player_name,
  const std::string &passwd,
  int slot_no)
{
  ASSERT(slot_no < kMaxPlaySession);
  if (IsAvailable(slot_no))
    return;

  // TODO: need switch ~ case later
  players_[slot_no] = new Player(playertype, player_name);

  // TODO: use wthr for sync ~ load player.
  players_[slot_no]->Sync();
  players_[slot_no]->Load();
  player_count++;
}

void PlayerManager::CreateGuestPlayerIfEmpty()
{
  if (GetLoadedPlayerCount() == 0)
  {
    CreatePlayer(PlayerTypes::kPlayerGuest, "GUEST", "");
  }
}

void PlayerManager::CreateNonePlayerIfEmpty()
{
  if (GetLoadedPlayerCount() == 0)
  {
    CreatePlayer(PlayerTypes::kPlayerNone, "NONE", "");
  }
}

void PlayerManager::UnloadPlayer(int player_slot)
{
  ASSERT(player_slot < kMaxPlaySession);
  if (players_[player_slot])
  {
    players_[player_slot]->Save();
    delete players_[player_slot];
    players_[player_slot] = nullptr;
    --player_count;
  }
}

bool PlayerManager::IsAvailable(int player_slot)
{
  ASSERT(player_slot < kMaxPlaySession);

  // @warn if player is guest, then it

  return GetPlayer(player_slot) != nullptr;
}

int PlayerManager::GetLoadedPlayerCount()
{
  return player_count;
}

void PlayerManager::Initialize()
{
}

void PlayerManager::Cleanup()
{
  for (int i = 0; i < kMaxPlaySession; ++i)
  {
    UnloadPlayer(i);
  }
}

}