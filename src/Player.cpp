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


// --------------------------- class PlayOption

PlayOption::PlayOption()
  : use_lanecover_(false), use_hidden_(false),
  speed_(1.0), speed_type_(GameSpeedTypes::kSpeedConstant),
  lanecover_(0.0), hidden_(0.0), class_(0)
{
  memset(keysetting_.keycode_per_track_, 0, sizeof(KeySetting));
}

KeySetting &PlayOption::GetKeysetting()
{
  return keysetting_;
}

void PlayOption::SetDefault(int gamemode)
{
  memset(keysetting_.keycode_per_track_, 0, sizeof(KeySetting));

  // TODO: provide different properties for each game mode.
  keysetting_.keycode_per_track_[0][0] = GLFW_KEY_A;
  keysetting_.keycode_per_track_[1][0] = GLFW_KEY_S;
  keysetting_.keycode_per_track_[2][0] = GLFW_KEY_D;
  keysetting_.keycode_per_track_[3][0] = GLFW_KEY_SPACE;
  keysetting_.keycode_per_track_[4][0] = GLFW_KEY_J;
  keysetting_.keycode_per_track_[5][0] = GLFW_KEY_K;
  keysetting_.keycode_per_track_[6][0] = GLFW_KEY_L;
  keysetting_.keycode_per_track_[7][0] = GLFW_KEY_LEFT_SHIFT;
  keysetting_.keycode_per_track_[7][1] = GLFW_KEY_SEMICOLON;
}

// ------------------------------- class Player

Player::Player(PlayerTypes player_type, const std::string& player_name)
  : player_type_(player_type), player_name_(player_name),
  is_guest_(false), is_network_(false), is_courseplay_(false),
  gamemode_(0), running_combo_(0), pr_db_(nullptr)
{
  if (player_type_ == PlayerTypes::kPlayerGuest)
    is_guest_ = true;

  // TODO: set current gamemode from current gamemode ?
  current_playoption_ = &playoptions_[gamemode_];

  // make properties for player common.
  config_.SetOption("running_combo", "!T0");

  // make properties for each gamemode.
  for (size_t i = 0; i < Gamemode::kGamemodeEnd; ++i)
  {
    std::string ns = GamemodeToString(i);
    ns.push_back('_');
    for (size_t i = 0; i < kMaxLaneCount; ++i)
    {
      config_.SetOption(ns + format_string("Key%d", i), "!T0");
    }
    config_.SetOption(ns + "use_hidden", "!T0");
    config_.SetOption(ns + "use_lanecover", "!T0");
    config_.SetOption(ns + "hidden", "!T0");
    config_.SetOption(ns + "lanecover", "!T0");
    config_.SetOption(ns + "speed", "!T300");
    config_.SetOption(ns + "speed_option", "!T0");
    config_.SetOption(ns + "chart_option", "!T0");
    config_.SetOption(ns + "chart_option_2P", "!T0");
    config_.SetOption(ns + "health_type", "!T0");
    config_.SetOption(ns + "assist", "!T0");
    config_.SetOption(ns + "pacemaker", "!T0");
    config_.SetOption(ns + "pacemaker_target", "!T0");
    config_.SetOption(ns + "class", "!T0");
  }

  config_name_ = format_string("../player/%s", player_name_.c_str());
}

Player::~Player()
{
  Save();
  ClosePlayRecords();
}

void Player::Load()
{
  // Unload if property already loaded
  ClosePlayRecords();

  std::string filepath = config_name_ + ".xml";
  if (!config_.ReadFromFile(filepath))
  {
    std::cerr << "Cannot load player preference (maybe default setting will used)"
      << std::endl;
  }

  running_combo_ = config_.GetValue<int>("running_combo");

  for (size_t i = 0; i < Gamemode::kGamemodeEnd; ++i)
  {
    std::string ns = GamemodeToString(i);
    ns.push_back('_');
    for (size_t k = 0; k < kMaxLaneCount; ++k)
    {
      std::string keyname(format_string(ns + "Key%d", k));
      auto *opt = config_.GetOption(keyname);
      if (!opt) break;
      CommandArgs args(opt->value<std::string>(), 4);
      playoptions_[i].GetKeysetting().keycode_per_track_[k][0] = args.Get<int>(0);
      playoptions_[i].GetKeysetting().keycode_per_track_[k][1] = args.Get<int>(1);
      playoptions_[i].GetKeysetting().keycode_per_track_[k][2] = args.Get<int>(2);
      playoptions_[i].GetKeysetting().keycode_per_track_[k][3] = args.Get<int>(3);
    }
    playoptions_[i].set_use_hidden(config_.GetValue<int>(ns + "use_hidden"));
    playoptions_[i].set_use_lanecover(config_.GetValue<int>(ns + "use_lanecover"));
    playoptions_[i].set_hidden(config_.GetValue<int>(ns + "hidden"));
    playoptions_[i].set_lanecover(config_.GetValue<int>(ns + "lanecover"));
    playoptions_[i].set_speed(config_.GetValue<int>(ns + "speed"));
    playoptions_[i].set_speed_type(config_.GetValue<int>(ns + "speed_option"));
    playoptions_[i].set_option_chart(config_.GetValue<int>(ns + "chart_option"));
    playoptions_[i].set_option_chart_dp(config_.GetValue<int>(ns + "chart_option_2P"));
    playoptions_[i].set_health_type(config_.GetValue<int>(ns + "health_type"));
    playoptions_[i].set_assist(config_.GetValue<int>(ns + "assist"));
    playoptions_[i].set_pacemaker(config_.GetValue<int>(ns + "pacemaker"));
    playoptions_[i].set_pacemaker_target(config_.GetValue<std::string>(ns + "pacemaker_target"));
    playoptions_[i].set_class(config_.GetValue<int>(ns + "class"));
  }

  // load playrecords
  LoadPlayRecords();
}

void Player::LoadPlayRecords()
{
  std::string filepath = config_name_ + ".db";
  int rc = sqlite3_open(filepath.c_str(), &pr_db_);
  if (rc) {
    std::cout << "Cannot save song database." << std::endl;
    ClosePlayRecords();
  }
  else
  {
    char *errmsg;

    // create player record schema
    sqlite3_exec(pr_db_,
      "CREATE TABLE IF NOT EXISTS record("
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
    pr.timestamp = atoi(argv[3]);
    pr.seed = atoi(argv[4]);
    pr.speed = atoi(argv[5]);
    pr.speed_type = atoi(argv[6]);
    pr.clear_type = atoi(argv[7]);
    pr.health_type = atoi(argv[8]);
    pr.option = atoi(argv[9]);
    pr.assist = atoi(argv[10]);
    pr.total_note = atoi(argv[11]);
    pr.miss = atoi(argv[12]);
    pr.pr = atoi(argv[13]);
    pr.bd = atoi(argv[14]);
    pr.gd = atoi(argv[15]);
    pr.gr = atoi(argv[16]);
    pr.pg = atoi(argv[17]);
    pr.maxcombo = atoi(argv[18]);
    pr.score = atoi(argv[19]);
    pr.playcount = atoi(argv[20]);
    pr.clearcount = atoi(argv[21]);
    pr.failcount = atoi(argv[22]);
    p->playrecords_.push_back(pr);
  }
  return 0;
}

void Player::Save()
{
  if (is_guest_)
    return;

  for (size_t i = 0; i < Gamemode::kGamemodeEnd; ++i)
  {
    std::string ns = GamemodeToString(i);
    ns.push_back('_');
    for (size_t k = 0; k < kMaxLaneCount; ++k)
    {
      std::string keyname(format_string(ns + "Key%d", k));
      std::string v(format_string("%d,%d,%d,%d",
        playoptions_[i].GetKeysetting().keycode_per_track_[k][0],
        playoptions_[i].GetKeysetting().keycode_per_track_[k][1],
        playoptions_[i].GetKeysetting().keycode_per_track_[k][2],
        playoptions_[i].GetKeysetting().keycode_per_track_[k][3]));
      config_.SetValue(keyname, v);
    }
    config_.SetValue(ns + "use_hidden", playoptions_[i].get_use_hidden());
    config_.SetValue(ns + "use_lanecover", playoptions_[i].get_use_lanecover());
    config_.SetValue(ns + "hidden", playoptions_[i].get_hidden()); /* TODO: set_option in case of double */
    config_.SetValue(ns + "lanecover", playoptions_[i].get_lanecover());
    config_.SetValue(ns + "speed", playoptions_[i].get_speed());
    config_.SetValue(ns + "speed_option", playoptions_[i].get_speed_type());
    config_.SetValue(ns + "chart_option", playoptions_[i].get_option_chart());
    config_.SetValue(ns + "chart_option_2P", playoptions_[i].get_option_chart_dp());
    config_.SetValue(ns + "health_type", playoptions_[i].get_health_type());
    config_.SetValue(ns + "assist", playoptions_[i].get_assist());
    config_.SetValue(ns + "pacemaker", playoptions_[i].get_pacemaker());
    config_.SetValue(ns + "pacemaker_target", playoptions_[i].get_pacemaker_target());
    config_.SetValue(ns + "class", playoptions_[i].get_class());
  }

  std::string filepath = config_name_ + ".xml";
  config_.SaveToFile(filepath);

  // save(close) playrecords
  //ClosePlayRecords();
}

void Player::UpdatePlayRecord(const PlayRecord &pr)
{
  if (!pr_db_)
    return;

  int rc;
  char *errmsg;
  std::string sql;
  sql = format_string(
    "INSERT OR REPLACE INTO record VALUES ("
    "'%s', '%s', %d, %d, %d, %d, %d, %d, %d, %d, "
    "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
    "%d, %d, %d"
    ");", pr.id, pr.chartname, pr.gamemode, pr.timestamp, pr.seed,
    pr.speed, pr.speed_type, pr.clear_type, pr.health_type, pr.option,
    pr.assist, pr.total_note, pr.miss, pr.pr, pr.bd,
    pr.gd, pr.gr, pr.pg, pr.maxcombo, pr.score,
    pr.playcount, pr.clearcount, pr.failcount);

  rc = sqlite3_exec(pr_db_, sql.c_str(),
    &Player::PRUpdateCallbackFunc, this, &errmsg);

  if (rc != SQLITE_OK)
  {
    std::cerr << "Error while saving playrecord " << pr.id << " (" << errmsg << ")" << std::endl;
    return;
  }
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

PlayOption &Player::GetPlayOption()
{
  return *current_playoption_;
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