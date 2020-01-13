#include "Player.h"
#include "SongPlayer.h"
#include "Timer.h"
#include "Util.h"
#include "Error.h"
#include <iostream>

#include "sqlite3.h"

namespace rhythmus
{

static Player *players_[kMaxPlaySession];
static int player_count;


// ------------------------------- class Player

Player::Player(PlayerTypes player_type, const std::string& player_name)
  : player_type_(player_type), player_name_(player_name), is_guest_(false),
  use_lanecover_(false), use_hidden_(false),
  game_speed_type_(GameSpeedTypes::kSpeedConstant),
  game_speed_(1.0), game_constant_speed_(1.0),
  lanecover_(0.0), hidden_(0.0),
  bms_sp_class_(0), bms_dp_class_(0),
  pr_db_(nullptr), is_network_(false), running_combo_(0), is_courseplay_(false)
{
  if (player_type_ == PlayerTypes::kPlayerGuest)
    is_guest_ = true;

  // TODO: make properties for each game mode.

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

  for (size_t i = 0; i < kMaxLaneCount; ++i)
  {
    // TODO: make keysetting option more solid
    config_.SetOption(format_string("Key%d", i), "!N");
  }
  config_.SetOption("use_hidden", "!N,0");
  config_.SetOption("use_lanecover", "!N,0");
  config_.SetOption("hidden", "!N,0");
  config_.SetOption("lanecover", "!N,0");
  config_.SetOption("speed", "!N,300");
  config_.SetOption("speed_option", "!N,0");
  config_.SetOption("chart_option", "!N,0");
  config_.SetOption("chart_option_2P", "!N,0");
  config_.SetOption("health_type", "!N,0");
  config_.SetOption("assist", "!N,0");
  config_.SetOption("pacemaker", "!N,0");
  config_.SetOption("pacemaker_target", "");
  config_.SetOption("sp_class", "!N,0");
  config_.SetOption("dp_class", "!N,0");

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

  // load keysetting
  for (size_t i = 0; i < kMaxLaneCount; ++i)
  {
    std::string keyname(format_string("Key%d", i));
    auto *opt = config_.GetOption(keyname);
    if (!opt) break;
    CommandArgs args(opt->value<std::string>(), 4);
    default_keysetting_.keycode_per_track_[i][0] = args.Get<int>(0);
    default_keysetting_.keycode_per_track_[i][1] = args.Get<int>(1);
    default_keysetting_.keycode_per_track_[i][2] = args.Get<int>(2);
    default_keysetting_.keycode_per_track_[i][3] = args.Get<int>(3);
  }
  use_hidden_ = config_.GetValue<int>("use_hidden");
  use_lanecover_ = config_.GetValue<int>("use_lanecover");
  hidden_ = config_.GetValue<int>("hidden");
  lanecover_ = config_.GetValue<int>("lanecover");
  game_speed_ = config_.GetValue<int>("speed");
  game_speed_type_ = config_.GetValue<int>("speed_option");
  option_chart_ = config_.GetValue<int>("chart_option");
  option_chart_dp_ = config_.GetValue<int>("chart_option_2P");
  health_type_ = config_.GetValue<int>("health_type");
  assist_ = config_.GetValue<int>("assist");
  pacemaker_ = config_.GetValue<int>("pacemaker");
  pacemaker_target_ = config_.GetValue<std::string>("pacemaker_target");
  bms_sp_class_ = config_.GetValue<int>("sp_class");
  bms_dp_class_ = config_.GetValue<int>("dp_class");

  // load playrecords
  LoadPlayRecords();
}

void Player::LoadPlayRecords()
{
  int rc = sqlite3_open("../system/song.db", &pr_db_);
  if (rc) {
    std::cout << "Cannot save song database." << std::endl;
    ClosePlayRecords();
  }
  else
  {
    char *errmsg;

    // create player record schema if not exists
    // TODO: schema check logic

    sqlite3_exec(pr_db_, "CREATE TABLE record("
      "id CHAR(128) PRIMARY KEY,"
      "name CHAR(512) NOT NULL,"
      "gamemode INT,"
      "timestamp INT,"
      "seed INT,"
      "speed INT,"
      "speed_type INT,"
      "clear_type INT,"
      "health_type INT,"
      "option INT,"
      "assist INT,"
      "total_note INT,"
      "miss INT, pr INT, bd INT, gd INT, gr INT, pg INT,"
      "maxcombo INT, score INT,"
      "playcount INT, clearcount INT, failcount INT"
      ");",
      &Player::CreateSchemaCallbackFunc, this, &errmsg);

    // load all records
    rc = sqlite3_exec(pr_db_,
      "SELECT id, name, gamemode, timestamp, seed, speed, speed_type, "
      "clear_type, health_type, option, assist, total_note, "
      "miss, pr, bd, gd, gr, pg, playcount, clearcount, failcount "
      "from songs;",
      &Player::PRQueryCallbackFunc, this, &errmsg);
    if (rc != SQLITE_OK)
    {
      std::cerr << "Failed to query song database, maybe corrupted? (" << errmsg << ")" << std::endl;
      ClosePlayRecords();
      return;
    }
  }
}

int Player::CreateSchemaCallbackFunc(void*, int argc, char **argv, char **colnames)
{
  return 0;
}

int Player::PRQueryCallbackFunc(void* _self, int argc, char **argv, char **colnames)
{
  Player *p = static_cast<Player*>(_self);
  PlayRecord pr;
  for (int i = 0; i < argc; ++i)
  {
    pr.id = argv[0];
    pr.chartname = argv[1];
    pr.gamemode = atoi(argv[2]);
    pr.timestamp = atoi(argv[2]);
    pr.seed = atoi(argv[2]);
    pr.speed= atoi(argv[2]);
    pr.speed_type = atoi(argv[2]);
    pr.clear_type = atoi(argv[2]);
    pr.health_type = atoi(argv[2]);
    pr.option = atoi(argv[2]);
    pr.assist = atoi(argv[2]);
    pr.total_note = atoi(argv[2]);
    pr.miss = atoi(argv[2]);
    pr.pr = atoi(argv[2]);
    pr.bd = atoi(argv[2]);
    pr.gd = atoi(argv[2]);
    pr.gr = atoi(argv[2]);
    pr.pg = atoi(argv[2]);
    pr.maxcombo = atoi(argv[2]);
    pr.score = atoi(argv[2]);
    pr.playcount = atoi(argv[2]);
    pr.clearcount = atoi(argv[2]);
    pr.failcount = atoi(argv[2]);
    p->playrecords_.push_back(pr);
  }
  return 0;
}

void Player::Save()
{
  if (is_guest_)
    return;

  for (size_t i = 0; i < kMaxLaneCount; ++i)
  {
  }

  config_.SaveToFile(config_path_);

  // save(close) playrecords
  ClosePlayRecords();
}

void Player::UpdatePlayRecord(const PlayRecord &pr)
{
  if (!pr_db_)
    return;

  char *errmsg;

  /* TODO: select query first, then check whether to update or insert. */

  sqlite3_exec(pr_db_, "",
    &Player::PRUpdateCallbackFunc, this, &errmsg);
}

int Player::PRUpdateCallbackFunc(void*, int argc, char **argv, char **colnames)
{
  return 0;
}

void Player::ClosePlayRecords()
{
  if (!pr_db_)
    return;

  sqlite3_close(pr_db_);
  pr_db_ = nullptr;
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