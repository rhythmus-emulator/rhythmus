#include "SongPlayer.h"
#include "Song.h" /* SongPlayinfo class */
#include "Player.h"
#include "Timer.h"
#include "Util.h"
#include "Logger.h"

namespace rhythmus
{

// ----------------- class SongResourceLoadTask

class SongResourceLoadTask : public Task
{
public:
  SongResourceLoadTask(SongPlayer *res) : res_(res) {}
  virtual void run() { res_->Load(); }
  virtual void abort() {}
private:
  SongPlayer *res_;
};

class SongResourceLoadResourceTask : public Task
{
public:
  SongResourceLoadResourceTask(SongPlayer *res) : res_(res) {}
  virtual void run() { res_->LoadResources(); }
  virtual void abort() { res_->Stop(); }
private:
  SongPlayer *res_;
};

// ------------------------- class SongResource

SongPlayer::SongPlayer()
  : song_(nullptr), is_courseplay_(false),
  loadfile_count_total_(0), wthr_count_(0), loaded_file_count_(0),
  is_loaded_(0), load_async_(true), load_bga_(true)
{
  memset(sessions_, 0, sizeof(sessions_));
  memset(prev_playcombo_, 0, sizeof(prev_playcombo_));
}

bool SongPlayer::Load()
{
  bool r;

  /* if started loading, then do nothing. */
  if (is_loaded_ != 0)
    return true;

  /* load song (sync) */
  auto *pctx = GetSongPlayinfo();
  if (!pctx)
  {
    Logger::Error("Attempt to play song but no song queued for playing.");
    return false;
  }
  std::string path = pctx->songpath;
  song_ = new rparser::Song();
  r = song_->Open(path);
  if (!r)
  {
    Logger::Error("Song loading failed: %s", path.c_str());
    delete song_;
    return false;
  }
  is_loaded_ = 1;

  /* Create empty player if none exists
   * XXX: should delete when play is over? */
  PlayerManager::CreateNonePlayerIfEmpty();

  /* Initialize each player for chart */
  FOR_EACH_PLAYER(p, i)
  {
    rparser::Chart *c = song_->GetChart(pctx->chartpaths[0]);
    if (!c)
    {
      Logger::Error("Attempt to play chart %s but not existing.",
        pctx->chartpaths[0].c_str());
      continue;
    }
    c->Invalidate();
    sessions_[i] = new PlayContext(p, *c);
  }
  END_EACH_PLAYER()

  /* prepare resource list to load */
  PrepareResourceListFromSong();

  /* load image & audio */
  if (load_async_)
  {
    LoadResourcesAsync();
  }
  else
  {
    wthr_count_ = 1;
    LoadResources();
    is_loaded_ = 2;
  }

  return true;
}

void SongPlayer::Play()
{
  /* if not loaded, then do loading first. */
  if (is_loaded_ == 0)
    Load();
  else if (is_loaded_ != 2)
    return;

  /* Start play all sessions */
  for (size_t i = 0; i < kMaxPlaySession; ++i)
  {
    if (!sessions_[i]) continue;
    sessions_[i]->StartPlay();
  }
}

void SongPlayer::Stop(bool interrupted)
{
  if (is_loaded_ == 0)
    return;

  /* is this the last song of the course? */
  bool is_course_finished = (playlist_.size() == 1);

  /* stop loading thread */
  CancelLoad();

  /* Clear out all sessions. */
  for (size_t i = 0; i < kMaxPlaySession; ++i)
  {
    if (!sessions_[i]) continue;
    sessions_[i]->FinishPlay();
    if (is_course_finished && !interrupted)
      sessions_[i]->SavePlay();
    delete sessions_[i];
    sessions_[i] = 0;
  }

  /* lock and clear all resources. */
  std::lock_guard<std::mutex> lock(loading_mutex_);
  files_to_load_.clear();

  for (auto &bg : bg_animations_)
    delete bg.second;
  for (auto &sound : sounds_)
    delete sound.second;
  bg_animations_.clear();
  sounds_.clear();
  if (song_)
  {
    song_->Close();
    delete song_;
    song_ = nullptr;
  }
  is_loaded_ = 0;

  /* Pop current song info. */
  PopSongFromPlaylist();

  /* Unload all NONE player (only for SongPlayer use) */
  PlayerManager::UnloadNonePlayer();
}

void SongPlayer::Update()
{
  float delta = Timer::SystemTimer().GetDeltaTime() * 1000;

  /* upload bitmap if not uploaded
   * XXX: heavy? */
  {
    std::lock_guard<std::mutex> lock(res_mutex_);
    UploadBitmaps();
  }

  /* update movie */
  for (auto &bg : bg_animations_) if (bg.second->is_loaded())
    bg.second->Update(delta);

  /* Update all players */
  for (size_t i = 0; i < kMaxPlaySession; ++i)
  {
    if (!sessions_[i]) continue;
    sessions_[i]->Update(delta);
  }
}

void SongPlayer::SetCoursetoPlay(const std::string &coursepath)
{
  // TODO
  R_ASSERT(0, "CoursePlayNotImplemented");
  is_courseplay_ = true;
}

void SongPlayer::SetSongtoPlay(const std::string &songpath, const std::string &chartpath)
{
  ClearPlaylist();
  // TODO: if songpath is course, then call SetCoursetoPlay()
  AddSongtoPlaylist(songpath, chartpath);
}

void SongPlayer::AddSongtoPlaylist(const std::string &songpath, const std::string &chartpath)
{
  // if songpath contains chartpath, then automatically fill chartpath.
  SongPlayinfo playinfo;
  if (chartpath.empty() && IsChartPath(songpath))
  {
    std::string songpath_new, chartpath_new;
    auto p = songpath.find_last_of('/');
    if (p != std::string::npos)
    {
      songpath_new = songpath.substr(0, p);
      chartpath_new = songpath.substr(p + 1);
    }
    playinfo.songpath = songpath_new;
    playinfo.chartpaths[0] = chartpath_new;
  }
  else
  {
    playinfo.songpath = songpath;
    playinfo.chartpaths[0] = chartpath;
  }
  playlist_.push_back(playinfo);
}

const SongPlayinfo *SongPlayer::GetSongPlayinfo() const
{
  if (!playlist_.empty())
    return &playlist_.front();
  return nullptr;
}

void SongPlayer::ProcessInputEvent(const InputEvent& e)
{
  for (size_t i = 0; i < kMaxPlaySession; ++i)
  {
    if (!sessions_[i]) continue;
    sessions_[i]->ProcessInputEvent(e);
  }
}

void SongPlayer::PopSongFromPlaylist()
{
  playlist_.pop_front();
}

void SongPlayer::ClearPlaylist()
{
  playlist_.clear();
  is_courseplay_ = false;
}

PlayContext* SongPlayer::GetPlayContext(int session)
{
  return sessions_[session];
}

#if 0
void SongPlayer::LoadSongAsync(const std::string& path)
{
  TaskAuto t = std::make_shared<SongResourceLoadTask>(this, path);
  tasks_.push_back(t);
  TaskPool::getInstance().EnqueueTask(t);
}
#endif

void SongPlayer::PrepareResourceListFromSong()
{
  auto *dir = song_->GetDirectory();
  if (!dir)
  {
    /* some types of song has no resource directory. */
    return;
  }
  FileToLoad load;
  for (auto *file : *dir)
  {
    load.filename = file->filename;
    std::string ext = GetExtension(file->filename);
    if (ext == "wav" || ext == "ogg" || ext == "flac" || ext == "mp3")
    {
      load.type = 1;
    }
    else if (ext == "bmp" || ext == "gif" || ext == "jpg" || ext == "jpeg" ||
      ext == "tiff" || ext == "tga" || ext == "png" || ext == "avi" || ext == "mpg" ||
      ext == "mpeg" || ext == "mp4")
    {
      if (!load_bga_) continue;
      load.type = 0;
    }
    else continue;
    files_to_load_.push_back(load);
  }
}

/* internally called from Load() and LoadAsync() */
void SongPlayer::LoadResources()
{
  if (!song_) return;

  /* If no resources are designated to be loaded,
   * Automatically create load list */
  if (is_loaded_ == 0)
  {
    std::lock_guard<std::mutex> lock(loading_mutex_);
    if (files_to_load_.empty())
    {
      PrepareResourceListFromSong();
    }
    is_loaded_ = 1;
  }

  /* Main resource loading loop */
  while (is_loaded_ == 1)
  {
    FileToLoad file_to_load;
    std::string fn;
    {
      std::lock_guard<std::mutex> lock(loading_mutex_);
      if (files_to_load_.empty())
        break;
      file_to_load = files_to_load_.front();
      files_to_load_.pop_front();
      loaded_file_count_++;
    }
    fn = file_to_load.filename;

    const char* file;
    size_t len;
    if (song_->GetDirectory()->GetFile(fn, &file, len))
    {
      if (file_to_load.type == 0)
      {
        // image
        Image *img = new Image();
        img->LoadFromData((uint8_t*)file, len);
        {
          std::lock_guard<std::mutex> lock(res_mutex_);
          bg_animations_.push_back({ { fn }, img });
        }
      }
      else if (file_to_load.type == 1)
      {
        // sound
        Sound *snd = new Sound();
        snd->get_buffer()->Resample(
          SoundDriver::getInstance().getMixer().GetSoundInfo()
        );
        snd->Load(file, len);
        {
          std::lock_guard<std::mutex> lock(res_mutex_);
          sounds_.push_back({ { fn, -1 }, snd });
        }
      }
    }
  }

  ASSERT(wthr_count_ > 0);
  if (--wthr_count_ == 0)
  {
    /* last loading thread does after-loading tasks. */
    for (size_t i = 0; i < kMaxPlaySession; ++i)
      if (sessions_[i]) sessions_[i]->UpdateResource();
  }

  is_loaded_ = 2;
}

void SongPlayer::LoadResourcesAsync()
{
  loadfile_count_total_ = files_to_load_.size();
  loaded_file_count_ = 0;
  size_t load_thread_count
    = wthr_count_
    = TaskPool::getInstance().GetPoolSize();
  for (int i = 0; i < load_thread_count; ++i)
  {
    TaskAuto t = std::make_shared<SongResourceLoadResourceTask>(this);
    tasks_.push_back(t);
    TaskPool::getInstance().EnqueueTask(t);
  }
}

/* This function should be called after all song resources are loaded */
void SongPlayer::UploadBitmaps()
{
  for (auto &bg : bg_animations_) if (bg.second->is_loaded())
    bg.second->CommitImage();
}

void SongPlayer::CancelLoad()
{
  {
    std::lock_guard<std::mutex> lock(loading_mutex_);
    files_to_load_.clear();
  }
  for (auto& t : tasks_)
    t->wait();
  tasks_.clear();
}

bool SongPlayer::IsChartPath(const std::string &path)
{
  // TODO: use rparser library later.
  std::string path_lower = Lower(path);
  return (endsWith(path_lower, ".bms")
    || endsWith(path_lower, ".bme")
    || endsWith(path_lower, ".bml")
    || endsWith(path_lower, ".osu")
    || endsWith(path_lower, ".sm"));
}

rparser::Song* SongPlayer::get_song()
{
  return song_;
}

rparser::Chart* SongPlayer::get_chart(const std::string& chartname)
{
  if (!song_)
    return nullptr;
  return song_->GetChart(chartname);
}

void SongPlayer::set_load_bga(bool use_bga)
{
  load_bga_ = use_bga;
}

int SongPlayer::is_loaded() const
{
  return is_loaded_;
}

double SongPlayer::get_progress() const
{
  if (loadfile_count_total_ == 0) return .0;
  return (double)loaded_file_count_ / loadfile_count_total_;
}

bool SongPlayer::IsFinished() const
{
  for (int i = 0; i < kMaxPlaySession; ++i)
  {
    if (!sessions_[i]) continue;
    if (!sessions_[i]->is_finished())
      return false;
  }
  return true;
}

Sound* SongPlayer::GetSound(const std::string& filename, int channel)
{
  Sound *sound_found = nullptr;
  Sound *new_sound = nullptr;

  for (auto &snd : sounds_)
  {
    if (CompareFilename(snd.first.name, filename))
    {
      sound_found = snd.second;

      // -1 channel value: nobody assigned, so take it.
      if (snd.first.channel == -1)
      {
        snd.first.channel = channel;
        snd.second->RegisterToMixer(&SoundDriver::getInstance().getMixer());
      }

      // if sound found with same channel, return it directly.
      if (snd.first.channel == channel)
      {
        return snd.second;
      }
    }
  }

  // not found; return nullptr
  if (!sound_found) return nullptr;

  // sound found, but different channel.
  // therefore, create new sound by shallow_clone.
  new_sound = sound_found->shallow_clone();
  new_sound->RegisterToMixer(&SoundDriver::getInstance().getMixer());
  sounds_.push_back({ { filename, channel }, new_sound });
  return new_sound;
}

Image* SongPlayer::GetImage(const std::string& filename)
{
  for (auto &bg : bg_animations_)
    if (bg.first.name == filename) return bg.second;
  return nullptr;
}

SongPlayer& SongPlayer::getInstance()
{
  static SongPlayer r;
  return r;
}

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
{
  return &objects_[curr_judge_idx_];
}

NoteWithJudging *TrackContext::get_curr_sound_note()
{
  return &objects_[curr_keysound_idx_];
}

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

PlayContext::PlayContext(Player *player, rparser::Chart &c)
  : player_(player), keysetting_(nullptr), track_count_(0),
  songtime_(0), measure_(0), beat_(0), timing_seg_data_(nullptr), metadata_(nullptr),
  is_alive_(0), health_(0.), combo_(0), running_combo_(0), passed_note_(0),
  last_judge_type_(JudgeTypes::kJudgeNone), is_autoplay_(true), is_play_bgm_(true)
{
  if (player_)
    keysetting_ = &player->GetPlayOption().GetKeysetting();
  timing_seg_data_ = &c.GetTimingSegmentData();
  metadata_ = &c.GetMetaData();

  // set note / track data from notedata
  auto &nd = c.GetNoteData();
  track_count_ = nd.get_track_count();
  size_t i = 0;
  for (; i < track_count_; ++i)
    track_context_[i].Initialize(nd.get_track(i));
  for (; i < kMaxLaneCount; ++i)
    track_context_[i].Clear();
  bgm_context_.Initialize(c.GetBgmData());
  for (size_t i = 0; i < 4; ++i)
    bga_context_[i].Initialize(c.GetBgaData().get_track(i));

  // Set PlayRecord / Replay context
  memset(&playrecord_, 0, sizeof(playrecord_));
  if (player_)
  {
    playrecord_.timestamp = 0;  // TODO: get system timestamp from Util
    playrecord_.seed = 0;   // TODO
    playrecord_.speed = player_->GetPlayOption().get_speed();
    playrecord_.speed_type = player_->GetPlayOption().get_speed_type();
    playrecord_.health_type = player_->GetPlayOption().get_health_type();
    playrecord_.score = 0;
    playrecord_.total_note = 0; // TODO: use song class?
    playrecord_.option = player_->GetPlayOption().get_option_chart();
    playrecord_.option_dp = player_->GetPlayOption().get_option_chart_dp();
    playrecord_.assist = player_->GetPlayOption().get_assist();
    running_combo_ = player_->GetRunningCombo();
  }
  replaydata_.events.clear();
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

void PlayContext::FinishPlay()
{
  // Upload playrecord & replay data to current player.
  if (player_)
  {
    player_->SetRunningCombo(combo_);
    player_->SetCurrentPlay(playrecord_, replaydata_);
  }
}

void PlayContext::SavePlay()
{
  // Only play is saved when
  // - Player is valid (not guest or autoplay, bgm)
  // - Not assist nor network user
  // - Courseplay is end (is_course_finished)
  if (player_ && !playrecord_.assist && !is_autoplay_)
  {
    player_->SaveCurrentPlay();
  }
  player_->ClearCurrentPlay();
}

void PlayContext::LoadReplay(const std::string &replay_id)
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
  replaydata_.events.emplace_back(ReplayData::ReplayEvent{
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
  if (playrecord_.total_note == 0) return .0;
  return (double)(playrecord_.pg * 2 + playrecord_.gr) / playrecord_.total_note;
}

double PlayContext::get_current_rate() const
{
  if (passed_note_ == 0) return .0;
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
  for (size_t i = 0; i < kMaxLaneCount; ++i) /* TODO: change to track size */
    if (!track_context_[i].is_all_judged())
      return false;
  return true;
}

void PlayContext::ProcessInputEvent(const InputEvent& e)
{
  if (is_autoplay_ || !keysetting_)
    return;

  // get track from keycode setting
  int track_no = -1;
  for (size_t i = 0; i < kMaxLaneCount; ++i)
  {
    for (size_t j = 0; j < 4; ++j)
    {
      if (keysetting_->keycode_per_track_[i][j] == 0)
        break;
      if (keysetting_->keycode_per_track_[i][j] == e.KeyCode())
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
  if (!is_alive_)
    return;

  songtime_ += delta;
  measure_ = timing_seg_data_->GetMeasureFromTime(songtime_);
  beat_ = timing_seg_data_->GetBeatFromMeasure(measure_);

  // 1. update each track for missed / mine note
  for (size_t i = 0; i < kMaxLaneCount /* XXX: get & set lane count? */; ++i)
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
      /* TODO: access keysound of SongPlayer. */
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

size_t PlayContext::GetTrackCount() const
{
  return track_count_;
}

TrackIterator PlayContext::GetTrackIterator(size_t track_idx)
{
  return TrackIterator(track_context_[track_idx]);
}

void PlayContext::UpdateResource()
{
  // This method is manually called by SongPlayer
  // when resource loading is over.
  memset(keysounds_, 0, sizeof(keysounds_));
  memset(bg_animations_, 0, sizeof(bg_animations_));
  for (auto &f : metadata_->GetSoundChannel()->fn)
    keysounds_[f.first] = SongPlayer::getInstance().GetSound(f.second, f.first);
  for (auto &f : metadata_->GetBGAChannel()->bga)
    bg_animations_[f.first] = SongPlayer::getInstance().GetImage(f.second.fn);
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

}