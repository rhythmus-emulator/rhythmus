#include "Player.h"
#include "SongPlayer.h"
#include "Timer.h"
#include "Util.h"
#include "Error.h"
#include "common.h"

#include "sqlite3.h"

namespace rhythmus
{

static Player *players_[kMaxPlaySession];
static int player_count;


// ------------------------------- class Player

Player::Player(PlayerTypes player_type, const std::string& player_name)
  : player_type_(player_type), player_name_(player_name),
  is_guest_(false), is_network_(false), gamemode_(0), running_combo_(0), course_combo_(0),
  pr_db_(nullptr)
{
  if (player_type_ == PlayerTypes::kPlayerGuest)
    is_guest_ = true;

  config_name_ = format_string("player/%s", player_name_.c_str());
}

Player::~Player()
{
  Save();
  ClosePlayRecords();
}

Player::PlayOption::PlayOption()
  : use_lanecover(false), use_hidden(false),
    speed(1.0f), speed_type(GameSpeedTypes::kSpeedConstant),
    lanecover(0.0), hidden(0.0), pclass(0) {}

void Player::Load()
{
  // Unload if property already loaded
  ClosePlayRecords();

  std::string filepath = config_name_ + ".xml";
  MetricGroup config;
  if (!config.Load(filepath))
  {
    std::cerr << "Cannot load player preference (maybe default setting will used)"
      << std::endl;
  }

  // TODO: load specific setting metric suitable for play config.
  MetricGroup &curr_config = config;
  curr_config.get_safe("running_combo", running_combo_);
  curr_config.get_safe("use_hidden", option_.use_hidden);
  curr_config.get_safe("use_lanecover", option_.use_lanecover);
  curr_config.get_safe("hidden", option_.hidden);
  curr_config.get_safe("lanecover", option_.lanecover);
  curr_config.get_safe("speed", option_.speed);
  curr_config.get_safe("speed_option", option_.speed_type);
  curr_config.get_safe("chart_option", option_.option_chart);
  curr_config.get_safe("chart_option_2P", option_.option_chart_dp);
  curr_config.get_safe("health_type", option_.health_type);
  curr_config.get_safe("assist", option_.assist);
  curr_config.get_safe("pacemaker", option_.pacemaker);
  curr_config.get_safe("pacemaker_target", option_.pacemaker_target);
  curr_config.get_safe("class", option_.pclass);
  keysetting_.Load(curr_config);

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

  std::string filepath = config_name_ + ".xml";
  MetricGroup curr_config;

  curr_config.set("running_combo", running_combo_);
  curr_config.set("use_hidden", option_.use_hidden);
  curr_config.set("use_lanecover", option_.use_lanecover);
  curr_config.set("hidden", option_.hidden);
  curr_config.set("lanecover", option_.lanecover);
  curr_config.set("speed", option_.speed);
  curr_config.set("speed_option", option_.speed_type);
  curr_config.set("chart_option", option_.option_chart);
  curr_config.set("chart_option_2P", option_.option_chart_dp);
  curr_config.set("health_type", option_.health_type);
  curr_config.set("assist", option_.assist);
  curr_config.set("pacemaker", option_.pacemaker);
  curr_config.set("pacemaker_target", option_.pacemaker_target);
  curr_config.set("class", option_.pclass);
  keysetting_.Save(curr_config);

  curr_config.Save(filepath);

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
  //R_ASSERT(0, "Player::Sync() is not implemented.");
}

int Player::player_type() const
{
  return player_type_;
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

void Player::SetCurrentPlay(const PlayRecord &playrecord)
{
  // TODO: accumulate playrecord.
}

void Player::InitPlayRecord(PlayRecord &pr)
{
  memset(&playrecord_, 0, sizeof(playrecord_));
  playrecord_.timestamp = 0;  // TODO: get system timestamp from Util
  playrecord_.seed = 0;   // TODO
  playrecord_.speed = (int)(option_.speed * 100);
  playrecord_.speed_type = option_.speed_type;
  playrecord_.health_type = option_.health_type;
  playrecord_.score = 0;
  playrecord_.total_note = 0; // TODO: use song class?
  playrecord_.option = option_.option_chart;
  playrecord_.option_dp = option_.option_chart_dp;
  playrecord_.assist = option_.assist;
  running_combo_ = 0; /* TODO: in case of courseplay? */
}

/* @brief store given playrecord. */
void Player::PostPlayRecord(PlayRecord &pr)
{
  // TODO: only store replaydata of current player.


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

/* @brief store given replay data. */
void Player::PostReplayData(ReplayData &replay)
{
  // TODO: only store replaydata of current player.
  replaydata_.push_back(replay);
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

int Player::GetTrackFromKeycode(int keycode) const
{
  return keysetting_.GetTrackFromKeycode(keycode);
}

// ------- static methods --------

Player* PlayerManager::GetPlayer()
{
  Player *r = nullptr;
  for (size_t i = 0; !r && i < kMaxPlaySession; ++i) r = players_[i];
  R_ASSERT(r);
  return r;
}

Player* PlayerManager::GetPlayer(int player_slot)
{
  R_ASSERT(player_slot < kMaxPlaySession);

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
  R_ASSERT(slot_no < kMaxPlaySession);
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

void PlayerManager::UnloadNonePlayer()
{
  if (players_[0] && players_[0]->player_type() == PlayerTypes::kPlayerNone)
  {
    UnloadPlayer(0);
  }
}

void PlayerManager::UnloadPlayer(int player_slot)
{
  R_ASSERT(player_slot < kMaxPlaySession);
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
  R_ASSERT(player_slot < kMaxPlaySession);

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
