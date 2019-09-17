#include "Player.h"
#include "Util.h"
#include "Error.h"
#include <iostream>

namespace rhythmus
{

static Player *players_[kMaxPlayerSlot];
static Player guest_player(PlayerTypes::kPlayerGuest, "GUEST");
static int player_count;

Player::Player(PlayerTypes player_type, const std::string& player_name)
  : player_type_(player_type), player_name_(player_name),
    use_lane_cover_(false), use_hidden_(false),
    game_speed_type_(GameSpeedTypes::kSpeedConstant),
    game_speed_(1.0), game_constant_speed_(1.0),
    lane_cover_(0.0), hidden_(0.0),
    bms_sp_class_(0), bms_dp_class_(0),
    is_save_allowed_(true),is_save_record_(true), is_save_replay_(true),
    is_guest_(false)
{
  if (player_type_ == PlayerTypes::kPlayerGuest)
    is_guest_ = true;
}

Player::~Player()
{
  Save();
}

void Player::Load()
{
  if (!setting_.Open(format_string(
    "../player/%s.xml",
    player_name_.c_str())))
  {
    std::cerr << "Cannot load player preference (maybe default setting will used)"
      << std::endl;
    return;
  }
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
}

void Player::Save()
{
  if (!is_save_allowed_)
    return;

  setting_.Save();
}

void Player::SetPlayId(const std::string& play_id)
{
  ClearPlay();
  play_id_ = play_id;
}

void Player::LoadPlay(bool load_replay)
{
  ClearPlay();
  is_save_allowed_ = false;
  is_save_record_ = false;
  is_save_replay_ = false;
}

void Player::SavePlay()
{
  if (!is_save_allowed_ || is_guest_)
    return;

  // update final result

  if (is_save_record_)
    SaveRecord();
  if (is_save_replay_)
    SaveReplay();
}

void Player::LoadRecord()
{
  // TODO: use sqlite
}

void Player::SaveRecord()
{
  // TODO: use sqlite
}

void Player::LoadReplay()
{
  // TODO: use sqlite
}

void Player::SaveReplay()
{
  // TODO: use sqlite
}

void Player::ClearPlay()
{
}

void Player::StartPlay()
{
  // Set record context
  replay_.events.clear();
  replay_.timestamp = 0;  // TODO: get system timestamp from Util
  replay_.seed = 0;   // TODO
  replay_.speed = game_speed_;
  replay_.speed_type = game_speed_type_;
  replay_.health_type = health_type_;
  replay_.rate = 0;
  replay_.score = 0;
  replay_.total_note = 0; // TODO: use song class?
  replay_.option = option_chart_;
  replay_.option_dp = option_chart_dp_;
  replay_.assist = assist_;
  memset(&course_judge_, 0, sizeof(Judge));

  is_alive_ = 1;
  health_ = 1.0;
  combo_ = 0;
  last_judge_type_ = JudgeTypes::kJudgeNone;

  // other is general song context clear
  StartNextSong();

  /* Check play record saving allowed, e.g. assist option */
  bool check = (assist_ == 0);
  is_save_record_ = check;
  is_save_replay_ = check;
}

void Player::StartNextSong()
{
  memset(&judge_, 0, sizeof(Judge));
  RecordPlay(ReplayEventTypes::kReplaySong, 0, 0);
}

void Player::RecordPlay(ReplayEventTypes event_type, int time, int value1, int value2)
{
  replay_.events.emplace_back(ReplayEvent{
    (int)event_type, time, value1, value2
    });

  switch (event_type)
  {
  case ReplayEventTypes::kReplayMiss:
    judge_.miss++;
    course_judge_.miss++;
    combo_ = 0;
    break;
  case ReplayEventTypes::kReplayPR:
    judge_.pr++;
    course_judge_.pr++;
    combo_ = 0;
    break;
  case ReplayEventTypes::kReplayBD:
    judge_.bd++;
    course_judge_.bd++;
    combo_ = 0;
    break;
  case ReplayEventTypes::kReplayGD:
    judge_.gd++;
    course_judge_.gd++;
    combo_++;
    break;
  case ReplayEventTypes::kReplayGR:
    judge_.gr++;
    course_judge_.gr++;
    combo_++;
    break;
  case ReplayEventTypes::kReplayPG:
    judge_.pg++;
    course_judge_.pg++;
    combo_++;
    break;
  default:
    break;
  }

  if (event_type >= ReplayEventTypes::kReplayMiss &&
      event_type <= ReplayEventTypes::kReplayPG)
    last_judge_type_ = (int)event_type;
}

double Player::get_rate() const
{
  return (double)(judge_.pg * 2 + judge_.gr) / total_note_;
}

double Player::get_current_rate() const
{
  return (double)(judge_.pg * 2 + judge_.gr) / passed_note_;
}

double Player::get_score() const
{
  return get_rate() * 200000.0;
}

double Player::get_health() const
{
  return health_;
}

bool Player::is_alive() const
{
  return is_alive_;
}

void Player::Update(float delta)
{
  // fetch some playing context from song ... ?
  //passed_note_++; // TODO
}


// ------- static methods --------

Player& Player::getMainPlayer()
{
  for (int i = 0; i < kMaxPlayerSlot; ++i)
    if (players_[i])
      return *players_[i];

  /* This shouldn't happened */
  ASSERT(false);
  return guest_player;
}

void Player::CreatePlayer(PlayerTypes playertype, const std::string& player_name)
{
  for (int i = 0; i < kMaxPlayerSlot; ++i)
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
  ASSERT(player_slot < kMaxPlayerSlot);
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
  ASSERT(player_slot < kMaxPlayerSlot);

  // if no player created & first player accessed,
  // then just return guest player
  // (always existing, ensured.)
  if (player_count == 0)
  {
    return &guest_player;
  }

  return players_[player_slot];
}

void Player::UnloadPlayer(int player_slot)
{
  ASSERT(player_slot < kMaxPlayerSlot);
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
  ASSERT(player_slot < kMaxPlayerSlot);
  return getPlayer(player_slot) != nullptr;
}

int Player::GetLoadedPlayerCount()
{
  return player_count;
}

}