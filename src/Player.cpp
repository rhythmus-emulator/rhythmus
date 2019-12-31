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
  if (is_judge_finished())
    return judgement_;

  auto *notedesc = get_curr_notedesc();
  int judgement;

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

  if (subtype() == rparser::NoteTypes::kMineNote)
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
static int player_count;


// ------------------------- class TrackContext

void TrackContext::Initialize(rparser::Track &track)
{
  curr_keysound_idx_ = 0;
  curr_judge_idx_ = 0;
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
    else break;
  }
}

NoteWithJudging *TrackContext::get_curr_judged_note()
{ return &objects_[curr_judge_idx_]; }

NoteWithJudging *TrackContext::get_curr_sound_note()
{ return &objects_[curr_keysound_idx_]; }

bool TrackContext::is_all_judged() const
{
  return curr_keysound_idx_ >= objects_.size();
}

bool NoteWithJudging::is_judgable() const
{
  return (subtype() != rparser::NoteTypes::kInvisibleNote);
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


// ---------------- class BackgroundDataContext

BackgroundDataContext::BackgroundDataContext()
  : curr_idx_(0), curr_process_idx_(0) {}

void BackgroundDataContext::Initialize(rparser::TrackData &data)
{
  auto iter = data.GetAllTrackIterator();
  while (!iter.is_end())
  {
    objects_.push_back(&*iter);
    ++iter;
  }
  curr_idx_ = 0;
  curr_process_idx_ = 0;
}

void BackgroundDataContext::Initialize(rparser::Track &track)
{
  for (auto *n : track)
    objects_.push_back(n);
}

void BackgroundDataContext::Clear()
{
  objects_.clear();
  curr_process_idx_ = 0;
  curr_idx_ = 0;
}

void BackgroundDataContext::Update(uint32_t songtime)
{
  while (curr_idx_ < objects_.size())
  {
    if (objects_[curr_idx_]->time_msec > songtime)
      break;
    curr_idx_++;
  }
}

rparser::NotePos* BackgroundDataContext::get_current()
{
  if (curr_idx_ == 0)
    return nullptr;
  return objects_[curr_idx_ - 1];
}

const rparser::NotePos* BackgroundDataContext::get_current() const
{
  return const_cast<BackgroundDataContext*>(this)->get_current();
}

rparser::NotePos* BackgroundDataContext::get_stack()
{
  if (curr_process_idx_ >= curr_idx_)
    return nullptr;
  return objects_[curr_process_idx_];
}

const rparser::NotePos* BackgroundDataContext::get_stack() const
{
  return const_cast<BackgroundDataContext*>(this)->get_stack();
}

void BackgroundDataContext::pop_stack()
{
  if (curr_process_idx_ < curr_idx_)
    curr_process_idx_++;
}

void BackgroundDataContext::clear_stack()
{
  curr_process_idx_ = curr_idx_;
}

// --------------------------------- PlayRecord

double PlayRecord::rate() const
{
  return (double)exscore() / (total_note * 2);
}

int PlayRecord::exscore() const
{
  return pg * 2 + gr * 1;
}


// -------------------------- class PlayContext

PlayContext::PlayContext(Player &player, rparser::Chart &c)
  : player_(player), songtime_(0), measure_(0), beat_(0), timing_seg_data_(nullptr),
    is_alive_(0), health_(0.), combo_(0),
    last_judge_type_(JudgeTypes::kJudgeNone), is_play_bgm_(true)
{
  timing_seg_data_ = &c.GetTimingSegmentData();

  // set note / track data from notedata
  auto &nd = c.GetNoteData();
  size_t i = 0;
  for (; i < nd.get_track_count(); ++i)
    track_context_[i].Initialize(nd.get_track(i));
  for (; i < kMaxTrackSize; ++i)
    track_context_[i].Clear();
  bgm_context_.Initialize(c.GetBgmData());
  for (size_t i = 0; i < 4; ++i)
    bga_context_[i].Initialize(c.GetBgaData().get_track(i));

  // Fetch sound/bg data from SongResource.
  // These resources are managed by SongResource instance,
  // so do not release them here.
  memset(keysounds_, 0, sizeof(keysounds_));
  memset(bg_animations_, 0, sizeof(bg_animations_));
  for (auto &f : c.GetMetaData().GetSoundChannel()->fn)
    keysounds_[f.first] = SongResource::getInstance().GetSound(f.second, f.first);
  for (auto &f : c.GetMetaData().GetBGAChannel()->bga)
    bg_animations_[f.first] = SongResource::getInstance().GetImage(f.second.fn);

  // Set PlayRecord / Replay context
  memset(&playrecord_, 0, sizeof(playrecord_));
  playrecord_.timestamp = 0;  // TODO: get system timestamp from Util
  playrecord_.seed = 0;   // TODO
  playrecord_.speed = player_.game_speed_;
  playrecord_.speed_type = player_.game_speed_type_;
  playrecord_.health_type = player_.health_type_;
  playrecord_.score = 0;
  playrecord_.total_note = 0; // TODO: use song class?
  playrecord_.option = player_.option_chart_;
  playrecord_.option_dp = player_.option_chart_dp_;
  playrecord_.assist = player_.assist_;
  replay_.events.clear();
  RecordPlay(ReplayEventTypes::kReplaySong, 0, 0);

  /* Check play record saving allowed, e.g. assist option */
  bool check = (playrecord_.assist == 0);

  /* create random play_id */
  playrecord_.id.resize(32, '0');
  time_t rndseed = time(0);
  for (size_t i = 0; i < 32; ++i)
  {
    char c = (static_cast<int>(rndseed % 16) + rand()) % 16;
    if (c < 10) c += '0';
    else c += 'a';
    rndseed = rndseed >> 1;
    playrecord_.id[i] = c;
  }
}

void PlayContext::LoadPlay(const std::string &play_id)
{
  // TODO: load play record
  // XXX: integrate with LoadChart?

  playrecord_.id = play_id;
}

void PlayContext::SavePlay()
{
  SaveRecord();
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
    playrecord_.miss++;
    combo_ = 0;
    break;
  case ReplayEventTypes::kReplayPR:
    playrecord_.pr++;
    combo_ = 0;
    break;
  case ReplayEventTypes::kReplayBD:
    playrecord_.bd++;
    combo_ = 0;
    break;
  case ReplayEventTypes::kReplayGD:
    playrecord_.gd++;
    combo_++;
    break;
  case ReplayEventTypes::kReplayGR:
    playrecord_.gr++;
    combo_++;
    break;
  case ReplayEventTypes::kReplayPG:
    playrecord_.pg++;
    combo_++;
    break;
  default:
    break;
  }

  if (event_type >= ReplayEventTypes::kReplayMiss &&
    event_type <= ReplayEventTypes::kReplayPG)
    last_judge_type_ = (int)event_type;
}

const std::string &PlayContext::GetPlayId() const
{
  return playrecord_.id;
}

double PlayContext::get_beat() const { return beat_; }
double PlayContext::get_measure() const { return measure_; }
double PlayContext::get_time() const { return songtime_; }

double PlayContext::get_rate() const
{
  return (double)(playrecord_.pg * 2 + playrecord_.gr) / playrecord_.total_note;
}

double PlayContext::get_current_rate() const
{
  return (double)(playrecord_.pg * 2 + playrecord_.gr) / passed_note_;
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

void PlayContext::ProcessInputEvent(const InputEvent& e)
{
  // get track from keycode setting
  int track_no = -1;
  for (size_t i = 0; i < kMaxTrackSize; ++i)
  {
    for (size_t j = 0; j < 4; ++j)
    {
      if (player_.curr_keysetting_->keycode_per_track_[i][j] == 0)
        break;
      if (player_.curr_keysetting_->keycode_per_track_[i][j] == e.KeyCode())
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
    double judgetime =
      (e.time() - Timer::SystemTimer().GetTime()) * 1000 + songtime_;
    int event_type = JudgeEventTypes::kJudgeEventDown;
    if (e.type() == InputEvents::kOnKeyUp)
      event_type = JudgeEventTypes::kJudgeEventUp;

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
  songtime_ += delta;
  measure_ = timing_seg_data_->GetMeasureFromTime(songtime_);
  beat_ = timing_seg_data_->GetBeatFromMeasure(measure_);

  // 1. update each track for missed / mine note
  for (size_t i = 0; i < 128 /* XXX: get & set lane count? */; ++i)
  {
    track_context_[i].Update(songtime_);
  }

  // 2. for row-wise game mode (e.g. guitarfreaks),
  //    check notes of whole row to decide judgement.
  UpdateJudgeByRow();

  // 3. BGA / BGM process
  if (is_play_bgm_)
  {
    bgm_context_.Update(songtime_);
    while (bgm_context_.get_stack())
    {
      auto *obj = static_cast<rparser::BgmObject*>(bgm_context_.get_stack());
      bgm_context_.pop_stack();
      if (!obj) continue;
      auto *s = keysounds_[obj->channel()];
      if (s) s->Play();
    }
  }
  for (size_t i = 0; i < 4; ++i)
    bga_context_[i].Update(songtime_);
}

void PlayContext::UpdateJudgeByRow()
{
  // TODO
}

TrackIterator PlayContext::GetTrackIterator(size_t track_idx)
{
  return TrackIterator(track_context_[track_idx]);
}

Image* PlayContext::GetImage(size_t layer_idx) const
{
  const auto *obj = static_cast<const rparser::BgaObject*>(
    bga_context_[std::min(layer_idx, 3u)].get_current()
    );
  if (!obj) return nullptr;
  return bg_animations_[obj->channel()];
}

PlayRecord &PlayContext::GetPlayRecord()
{
  return playrecord_;
}


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

void Player::SetPlayRecord(const PlayRecord &playrecord)
{
  for (size_t i = 0; i < playrecords_.size(); ++i)
  {
    if (playrecords_[i].chartname == playrecord.chartname)
    {
      playrecords_[i] = playrecord;
      return;
    }
  }
  // TODO: use SQL insert using SQLwrapper
  playrecords_.push_back(playrecord);
}

/* @warn: must load proper song to SongResource instance before this function. */
void Player::SetPlayContext(const std::string& chartname)
{
  ASSERT(play_context_ == nullptr);
  auto *chart = SongResource::getInstance().get_chart(chartname);
  if (!chart)
    return;

  chart->Invalidate();
  play_context_ = new PlayContext(*this, *chart);
}

void Player::ClearPlayContext()
{
  // 1. if not courseplay
  // bring setting & save records before removing play context
  //
  // 2. if courseplay
  // 2-1. if courseplaying
  // append play info to courseplay context.
  // 2-2. if course finish
  // save course play info.

  if (is_save_allowed_ && !is_replay_ && play_context_)
  {
    if (!is_courseplay_)
    {
      SetPlayRecord(play_context_->GetPlayRecord());
      // TODO: decide to save record by 'what' policy ..?
      play_context_->SavePlay();
    }
    else
    {
      if (!play_chartname_.empty())
      {
        courserecord_.pg += play_context_->GetPlayRecord().pg;
        courserecord_.gr += play_context_->GetPlayRecord().gr;
        courserecord_.gd += play_context_->GetPlayRecord().gd;
        courserecord_.pr += play_context_->GetPlayRecord().pr;
        courserecord_.bd += play_context_->GetPlayRecord().bd;
        courserecord_.maxcombo = std::max(
          courserecord_.maxcombo, play_context_->GetPlayRecord().maxcombo);
        // TODO: append replay info to coursecontext
      }
      else
      {
        SetPlayRecord(courserecord_);
      }
    }
  }

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
  SetPlayContext(chartname);
}


// ------- static methods --------

Player& Player::getMainPlayer()
{
  for (int i = 0; i < kMaxPlayerSlot; ++i)
    if (players_[i])
      return *players_[i];

  /* This shouldn't happened */
  ASSERT(false);
  return *players_[0];
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

void Player::Initialize()
{
  /* Set Guest player in first slot by default */
  players_[0] = new Player(PlayerTypes::kPlayerGuest, "GUEST");
}

void Player::Cleanup()
{
  /* Clear all player */
  for (int i = 0; i < kMaxPlayerSlot; ++i) if (players_[i])
  {
    delete players_[i];
    players_[i] = nullptr;
  }
}

}