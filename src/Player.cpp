#include "Player.h"
#include "Timer.h"
#include "Util.h"
#include "Error.h"
#include <iostream>

namespace rhythmus
{

// --------------------- class JudgementContext

JudgementContext::JudgementContext()
{
  memset(judge_time_, 0, sizeof(judge_time_));
}

JudgementContext::JudgementContext(int pg, int gr, int gd, int bd, int pr)
{
  judge_time_[0] = 0; /* not in use */
  judge_time_[1] = pr;
  judge_time_[2] = bd;
  judge_time_[3] = gd;
  judge_time_[4] = gr;
  judge_time_[5] = pg;
}

int JudgementContext::get_pr_time() const
{
  return judge_time_[1];
}

int JudgementContext::judge(int delta_time)
{
  int dt = abs(delta_time);
  for (int i = 5; i > 0; --i)
    if (dt < judge_time_[i])
      return i;
  return JudgeTypes::kJudgeNone;
}

void JudgementContext::setJudgementRatio(double r)
{
  for (size_t i = 0; i <= 5; ++i) judge_time_[i] *= r;
}

JudgementContext& JudgementContext::getDefaultJudgementContext()
{
  // DDR: 16.7
  // IIDX: 20 (17 ~ 18?)
  // Drummania: 27
  // Guitarfreaks: 33
  // jubeat: 42
  static JudgementContext jctx(18, 36, 100, 150, 250);
  return jctx;
}

// ---------------------- class NoteWithJudging

NoteWithJudging::NoteWithJudging(rparser::Note *note)
  : rparser::Note(*note), chain_index_(0),
    judge_status_(0), judgement_(JudgeTypes::kJudgeNone), invisible_(false),
    judge_ctx_(nullptr) {}

int NoteWithJudging::judge(uint32_t songtime, int event_type)
{
  auto *notedesc = get_curr_notedesc();
  int judgement;

  if (is_judge_finished())
    return judgement_;

  if ((judgement = judge_only_timing(songtime)) != JudgeTypes::kJudgeNone)
  {
    if (event_type == JudgeEventTypes::kJudgeEventDown)
    {
      judgement_ = judgement;
      // if miss, finish judge
      if (judgement_ <= JudgeTypes::kJudgeBD)
      {
        judge_status_ = 2;
        return judgement_;
      }
    }

    // judge success, go to next chain if available.
    // if no chain remains, finish judging.
    if (chain_index_ >= chainsize())
    {
      judge_status_ = 2;
    }
    else chain_index_++;
  }

  // XXX: is this necessary?
  if (judge_status_ == 0)
    judge_status_ = 1;

  return judgement_;
}

int NoteWithJudging::judge_with_pos(uint32_t songtime, int event_type, int x, int y, int z)
{
  if (is_judge_finished())
    return judgement_;

  auto *notedesc = get_curr_notedesc();
  int ax, ay, az;
  int judgement;
  notedesc->get_pos(ax, ay, az);

  /* TODO: need range of position check? */
  if (ax != x || ay != y || az != z)
  {
    return JudgeTypes::kJudgeNone;
  }

  return judge(songtime, event_type);
}

int NoteWithJudging::judge_check_miss(uint32_t songtime)
{
  if (is_judge_finished())
    return judgement_;

  if (get_track() == rparser::NoteTypes::kMineNote)
  {
    /* in case of mine note */
    if (time_msec > songtime)
    {
      judgement_ = JudgeTypes::kJudgeOK;
      judge_status_ = 2;
    }
  }
  else {
    /* in case of normal note */
    if (chain_index_ == 0 && this->time_msec + get_judge_ctx()->get_pr_time() < songtime)
    {
      judgement_ = JudgeTypes::kJudgeMiss;
      judge_status_ = 2;
    }
    /* in case of LN note (bms type) */
    else if (chain_index_ > 0 && get_curr_notedesc()->time_msec < songtime)
    {
      if (chain_index_ >= chainsize())
      {
        judge_status_ = 2;
      }
      else chain_index_++;
    }
  }
  return judgement_;
}

JudgementContext *NoteWithJudging::get_judge_ctx()
{
  if (!judge_ctx_)
    return &JudgementContext::getDefaultJudgementContext();
  else
    return judge_ctx_;
}

int NoteWithJudging::judge_only_timing(uint32_t songtime)
{
  return get_judge_ctx()->judge(songtime);
}

bool NoteWithJudging::is_judge_finished() const
{
  return judge_status_ >= 2;
}

rparser::NoteDesc *NoteWithJudging::get_curr_notedesc()
{
  return this->get_chain(chain_index_);
}

static Player *players_[kMaxPlayerSlot];
static Player guest_player(PlayerTypes::kPlayerGuest, "GUEST");
static int player_count;


// ------------------------- class TrackContext

void TrackContext::Initialize(rparser::Track &track)
{
  // TODO: check duplicated object, and replace it if so.
  for (auto *n : track)
  {
    objects_.emplace_back(NoteWithJudging(static_cast<rparser::Note*>(n)));
  }
}

void TrackContext::SetInvisibleMineNote(double beat, uint32_t time)
{
  // TODO: check duplicated object, and replace it if so.
  rparser::Note n;
  n.SetBeatPos(beat);
  n.SetTime(time);
  objects_.emplace_back(NoteWithJudging(&n));
}

void TrackContext::Clear()
{
  objects_.clear();
}

void TrackContext::Update(uint32_t songtime)
{
  // check miss timing if general note
  while (curr_judge_idx_ < objects_.size())
  {
    auto &currobj = objects_[curr_judge_idx_];
    // if it is not judgable (transparent object), go to next object
    if (!currobj.is_judgable())
      curr_judge_idx_++;
    currobj.judge_check_miss(songtime);
    if (currobj.is_judge_finished())
      curr_judge_idx_++;
    else break;
  }

  // update keysound index
  while (curr_keysound_idx_ < objects_.size())
  {
    auto &currobj = objects_[curr_keysound_idx_];
    // if it is not judgable (transparent object), wait until time is done
    if (!currobj.is_judgable())
    {
      if (currobj.time_msec < songtime)
        curr_keysound_idx_++;
      continue;
    }
    if (currobj.is_judge_finished())
      curr_keysound_idx_++;
  }
}

NoteWithJudging *TrackContext::get_curr_judged_note()
{ return &objects_[curr_judge_idx_]; }

NoteWithJudging *TrackContext::get_curr_sound_note()
{ return &objects_[curr_keysound_idx_]; }

bool TrackContext::is_all_judged() const
{
  return curr_judge_idx_ >= objects_.size();
}

bool NoteWithJudging::is_judgable() const
{
  // TODO
  return true;
}

constexpr size_t kMaxNotesToDisplay = 200;

TrackIterator::TrackIterator(TrackContext& track)
  : curr_idx_(0)
{
  // go back few notes to display previously judged notes (if necessary)
  size_t idx = std::max(track.curr_judge_idx_, 5u) - 5;
  for (; idx < track.objects_.size() && notes_.size() < kMaxNotesToDisplay; ++idx)
    notes_.push_back(&track.objects_[idx]);
}

bool TrackIterator::is_end() const
{
  return notes_.size() <= curr_idx_;
}

void TrackIterator::next()
{
  ++curr_idx_;
}

NoteWithJudging& TrackIterator::operator*()
{
  return *notes_[curr_idx_];
}


// -------------------------- class PlayContext

PlayContext::PlayContext(Player &player, rparser::Chart &c)
  : player_(player),
    is_save_allowed_(true), is_save_record_(true), is_save_replay_(true),
    is_alive_(0), health_(0.), combo_(0), songtime_(0),
    last_judge_type_(JudgeTypes::kJudgeNone)
{
  // set note / track data from notedata
  auto &nd = c.GetNoteData();
  size_t i = 0;
  for (; i < nd.get_track_count(); ++i)
    track_context_[i].Initialize(nd.get_track(i));
  for (; i < kMaxTrackSize; ++i)
    track_context_[i].Clear();

  // Fetch sound/bg data from SongResource.
  // These resources are managed by SongResource instance,
  // so do not release them here.
  // TODO: need to pass channel information to GetSound() param.
  for (auto &f : c.GetMetaData().GetSoundChannel()->fn)
    keysounds_[f.first] = SongResource::getInstance().GetSound(f.second);
  for (auto &f : c.GetMetaData().GetBGAChannel()->bga)
    bgs_[f.first] = SongResource::getInstance().GetImage(f.second.fn);

  // Set record context
  replay_.events.clear();
  replay_.timestamp = 0;  // TODO: get system timestamp from Util
  replay_.seed = 0;   // TODO
  replay_.speed = player_.game_speed_;
  replay_.speed_type = player_.game_speed_type_;
  replay_.health_type = player_.health_type_;
  replay_.rate = 0;
  replay_.score = 0;
  replay_.total_note = 0; // TODO: use song class?
  replay_.option = player_.option_chart_;
  replay_.option_dp = player_.option_chart_dp_;
  replay_.assist = player_.assist_;
  memset(&course_judge_, 0, sizeof(Judge));

  // initialize judgement and record
  memset(&judge_, 0, sizeof(Judge));
  RecordPlay(ReplayEventTypes::kReplaySong, 0, 0);

  /* Check play record saving allowed, e.g. assist option */
  bool check = (replay_.assist == 0);
  is_save_record_ = check;
  is_save_replay_ = check;
}

void PlayContext::LoadPlay()
{
  // TODO: load play record
  // XXX: integrate with LoadChart?
}

void PlayContext::SavePlay()
{
  if (!is_save_allowed_)
    return;

  // update final result

  if (is_save_record_)
    SaveRecord();
  if (is_save_replay_)
    SaveReplay();
}

void PlayContext::LoadRecord()
{
  // TODO: use sqlite
}

void PlayContext::SaveRecord()
{
  // TODO: use sqlite
}

void PlayContext::LoadReplay()
{
  // TODO: use sqlite
}

void PlayContext::SaveReplay()
{
  // TODO: use sqlite
}

void PlayContext::StartPlay()
{
  is_alive_ = 1;
  health_ = 1.0;
  combo_ = 0;
  songtime_ = 0;
  last_judge_type_ = JudgeTypes::kJudgeNone;
}

void PlayContext::StopPlay()
{
  // set as dead player
  is_alive_ = 0;
}

void PlayContext::RecordPlay(ReplayEventTypes event_type, int time, int value1, int value2)
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

double PlayContext::get_rate() const
{
  return (double)(judge_.pg * 2 + judge_.gr) / total_note_;
}

double PlayContext::get_current_rate() const
{
  return (double)(judge_.pg * 2 + judge_.gr) / passed_note_;
}

double PlayContext::get_score() const
{
  return get_rate() * 200000.0;
}

double PlayContext::get_health() const
{
  return health_;
}

bool PlayContext::is_alive() const
{
  return is_alive_;
}

bool PlayContext::is_finished() const
{
  // died, or no remain note to play.
  if (!is_alive()) return true;
  for (size_t i = 0; i < kMaxTrackSize; ++i) /* TODO: change to track size */
    if (!track_context_[i].is_all_judged())
      return false;
  return true;
}

void PlayContext::ProcessInputEvent(const EventMessage& e)
{
  if (!e.IsInput())
    return;

  // get track from keycode setting
  int track_no = -1;
  for (size_t i = 0; i < kMaxTrackSize; ++i)
  {
    for (size_t j = 0; j < 4; ++j)
    {
      if (player_.curr_keysetting_->keycode_per_track_[i][j] == 0)
        break;
      if (player_.curr_keysetting_->keycode_per_track_[i][j] == e.GetKeycode())
      {
        track_no = i;
        break;
      }
    }
  }

  if (track_no == -1)
    return;

  // make judgement
  // - re-calculate judgetime to fetch exact judging time
  auto *obj = track_context_[track_no].get_curr_judged_note();
  if (obj)
  {
    double judgetime = e.GetTime() - Timer::GetGameTime() + songtime_;
    int event_type = JudgeEventTypes::kJudgeEventDown;
    if (e.IsKeyUp()) event_type = JudgeEventTypes::kJudgeEventUp;

    // sound first before judgement
    // TODO: get sounding object before judgement
    auto *ksound = keysounds_[obj->channel()];
    if (ksound) ksound->Play();

    // make judgement & afterwork
    // TODO: update sounding index after judgement
    obj->judge(judgetime, event_type);
  }
}

void PlayContext::Update(float delta)
{
  // fetch some playing context from song ... ?
  //passed_note_++; // TODO
  songtime_ += delta;
  UpdateJudgeByRow();
}

void PlayContext::UpdateJudgeByRow()
{
  // 1. update each track for missed / mine note
  for (size_t i = 0; i < 128; ++i)
  {
    track_context_[i].Update(songtime_);
  }

  // 2. for row-wise game mode (e.g. guitarfreaks),
  //    check notes of whole row to decide judgement.
  // TODO
}

TrackIterator PlayContext::GetTrackIterator(size_t track_idx)
{
  return TrackIterator(track_context_[track_idx]);
}


// ------------------------------- class Player

Player::Player(PlayerTypes player_type, const std::string& player_name)
  : player_type_(player_type), player_name_(player_name), is_guest_(false),
  use_lane_cover_(false), use_hidden_(false),
  game_speed_type_(GameSpeedTypes::kSpeedConstant),
  game_speed_(1.0), game_constant_speed_(1.0),
  lane_cover_(0.0), hidden_(0.0),
  bms_sp_class_(0), bms_dp_class_(0),
  play_context_(nullptr)
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

  // TODO: load keysetting
}

void Player::Save()
{
  if (is_guest_)
    return;

  setting_.Save();
}

/* @warn: must load proper song to SongResource instance before this function. */
void Player::SetPlayContext(const std::string& chartname)
{
  ASSERT(play_context_ == nullptr);
  auto *chart = SongResource::getInstance().get_chart(chartname);
  if (!chart)
    return;

  play_context_ = new PlayContext(*this, *chart);
}

void Player::ClearPlayContext()
{
  // TODO: bring setting & save records before removing play context
  delete play_context_;
  play_context_ = nullptr;
}

PlayContext *Player::GetPlayContext()
{
  return play_context_;
}

void Player::AddChartnameToPlay(const std::string& chartname)
{
  play_chartname_.push_back(chartname);
}

void Player::LoadNextChart()
{
  ClearPlayContext();
  if (play_chartname_.empty())
    return;
  std::string chartname = play_chartname_.front();
  play_chartname_.pop_front();
  AddChartnameToPlay(chartname);
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

bool Player::IsAllPlayerFinished()
{
  for (int i = 0; i < kMaxPlayerSlot; ++i) if (players_[i])
  {
    auto *pctx = players_[i]->play_context_;
    if (!pctx) continue;
    if (!pctx->is_finished())
      return false;
  }
  return true;
}

}