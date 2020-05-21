#include "SongPlayer.h"
#include "ResourceManager.h"
#include "Song.h" /* SongPlayinfo class */
#include "PlaySession.h"
#include "Player.h"
#include "Timer.h"
#include "Util.h"
#include "Logger.h"
#include "common.h"

namespace rhythmus
{

// ------------------------- class SongResource

SongPlayer::SongPlayer()
  : song_(nullptr), playlist_index_(0), state_(SongPlayerState::STOPPED),
    load_progress_(.0), play_progress_(.0),
    load_bga_(true), play_after_loading_(false), action_after_playing_(1)
{
  memset(sessions_, 0, sizeof(sessions_));
}

void SongPlayer::SetCoursetoPlay(const std::string &coursepath)
{
  // TODO
  R_ASSERT(0 && "CoursePlayNotImplemented");
  Clear();
}

void SongPlayer::SetSongtoPlay(const std::string &songpath, const std::string &chartpath)
{
  Clear();
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

bool SongPlayer::LoadNext()
{
  /* if already playing, then do nothing. */
  if (state_ != SongPlayerState::STOPPED)
    return true;

  /* if nothing in playlist (unable to load), then reset and return false */
  if (playlist_.empty())
  {
    Stop();
    return false;
  }

  /*
   * Now loading starts from here.
   * reset variables and fetch play info.
   */
  state_ = SongPlayerState::LOADING;
  load_progress_ = 0;
  auto *pctx = GetSongPlayinfo();
  ++playlist_index_;

  /* load song file without resource. */
  if (!pctx)
  {
    Stop();
    Logger::Error("Attempt to play song but no song queued for playing.");
    return false;
  }
  std::string path = pctx->songpath;
  song_ = new rparser::Song();
  if (!song_->Open(path))
  {
    Stop();
    Logger::Error("Song loading failed: %s", path.c_str());
    delete song_;
    song_ = nullptr;
    return false;
  }

  /* Create empty player if none exists
   * XXX: should delete when play is over? */
  PlayerManager::CreateNonePlayerIfEmpty();

  FOR_EACH_PLAYER(p, i)
  {
    /* Create PlayContext for playing */
    rparser::Chart *c = song_->GetChart(pctx->chartpaths[i]);
    if (!c)
    {
      Logger::Error("Attempt to play chart %s but not existing.",
        pctx->chartpaths[0].c_str());
      continue;
    }
    c->Update();
    sessions_[i] = new PlaySession(i, p, *c);

    /* load resource (expecting async mode) */
    LoadResourceFromChart(*c, i);
  }
  END_EACH_PLAYER();

  return true;
}

void SongPlayer::Play()
{
  switch (state_)
  {
  case SongPlayerState::STOPPED:
    /* call loading method, and play after loading */
    LoadNext();
  case SongPlayerState::LOADING:
    /* play if loaded. otherwise, enable play after loading. */
    if (load_progress_ < 1.0)
    {
      play_after_loading_ = true;
      return;
    }
    break;
  case SongPlayerState::PAUSED:
    state_ = SongPlayerState::PLAYING;
    return;
  default:
    Logger::Warn("SongPlayer::Play method recalled while it's already playing.");
    return;
  }

  /* Start play all sessions */
  for (size_t i = 0; i < kMaxPlaySession; ++i)
  {
    if (!sessions_[i]) continue;
    if (is_course() && playlist_index_ == 0)
      sessions_[i]->OnCourseStart();
    sessions_[i]->OnSongStart();
  }
}

void SongPlayer::Stop()
{
  // Release PlaySession.
  for (unsigned i = 0; i < kMaxPlaySession; ++i)
  {
    if (sessions_[i])
    {
      delete sessions_[i];
      sessions_[i] = nullptr;
    }
  }

  // Cancel loading & release resources
  // Sound playing is also canceled.
  for (auto *img : image_arr_)
    IMAGEMAN->Unload(img);
  for (auto *snd : sound_arr_)
    snd->Unload();  // automatically calls SOUNDMAN->Unload(sounddata)
  image_arr_.clear();
  sound_arr_.clear();

  // Go back to first song of the playlist
  playlist_index_ = 0;

  // Reset state
  for (size_t s = 0; s < kMaxPlaySession; ++s)
  {
    for (size_t i = 0; i < kMaxChannelCount; ++i)
    {
      bga_fn_[s][i].clear();
      bgm_fn_[s][i].clear();
      bgm_[s][i] = 0;
      bga_[s][i] = 0;
    }
  }
  state_ = SongPlayerState::STOPPED;
  load_progress_ = 0;
  play_progress_ = 0;
  song_ = nullptr;
}

void SongPlayer::Clear()
{
  // Stop currently playing song
  Stop();

  // Clear playlist
  playlist_.clear();
}

void SongPlayer::Update(float delta)
{
  switch (state_)
  {
  case SongPlayerState::STOPPED:
    // nothing to do
    break;
  case SongPlayerState::LOADING:
    // update load progress
    {
    size_t load_count = 0;
    size_t total_load_count = image_arr_.size() + sound_arr_.size();
    for (auto *img : image_arr_)
      if (!img->is_loading()) load_count++;
    for (auto *snd : sound_arr_)
      if (!snd->is_loading()) load_count++;
    if (total_load_count == 0)
      load_progress_ = 1.0;
    else
      load_progress_ = load_count / (double)total_load_count;
    }
    // if done, check play immediate after loading.
    if (load_progress_ >= 1.0)
    {
      if (play_after_loading_)
        Play();
      else
        state_ = SongPlayerState::PAUSED;
    }
    break;
  case SongPlayerState::PLAYING:
    // update play progress
    {
    double progress = 1.0;
    for (size_t i = 0; i < kMaxPlaySession; ++i) if (sessions_[i])
    {
      progress = std::min(sessions_[i]->get_progress(), progress);
    }
    play_progress_ = progress;
    }
    // if finished, do presetted behavior.
    if (play_progress_ >= 1.0)
    {
      switch (action_after_playing_)
      {
      case 0:
        state_ = SongPlayerState::FINISHED;
        break;
      case 1:
        Stop();
        break;
      case 2:
        if (playlist_index_ + 1 < playlist_.size())
          LoadNext();
        else
          Stop();
        break;
      }
    }
    // update play sessions
    for (size_t i = 0; i < kMaxPlaySession; ++i) if (sessions_[i])
    {
      sessions_[i]->Update(delta);
    }
    break;
  case SongPlayerState::PAUSED:
    // nothing to do
    break;
  case SongPlayerState::FINISHED:
    // nothing to do
    break;
  }

  /* @warn we don't update resouces(image / sound) here.
   * All resources are automatically updated by ResourceManager module. */
}

const SongPlayinfo *SongPlayer::GetSongPlayinfo() const
{
  if (playlist_.empty() || playlist_index_ >= playlist_.size())
    return nullptr;
  return &playlist_[playlist_index_];
}

void SongPlayer::ProcessInputEvent(const InputEvent& e)
{
  for (size_t i = 0; i < kMaxPlaySession; ++i)
  {
    if (!sessions_[i]) continue;
    sessions_[i]->ProcessInputEvent(e);
  }
}

PlaySession* SongPlayer::GetPlaySession(int session)
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

void SongPlayer::LoadResourceFromChart(rparser::Chart &c, unsigned session)
{
  auto *dir = song_->GetDirectory();
  if (!dir)
  {
    /* some types of song (e.g. MIDI based like VOS) has no resource directory. */
    return;
  }

  auto &bga_map = c.GetMetaData().GetBGAChannel()->bga;
  for (auto it = bga_map.begin(); it != bga_map.end(); ++it)
    bga_fn_[session][it->first] = it->second.fn;
  auto &bgm_map = c.GetMetaData().GetSoundChannel()->fn;
  for (auto it = bgm_map.begin(); it != bgm_map.end(); ++it)
    bgm_fn_[session][it->first] = it->second;

  /* read resources */
  const char *fn, *p;
  size_t len;
  for (unsigned i = 0; i < kMaxChannelCount; ++i)
  {
    if (!bga_fn_[session][i].empty())
    {
      Image *img;
      fn = bga_fn_[session][i].c_str();
      if (dir->GetFile(bga_fn_[session][i], &p, len) && len > 0)
      {
        img = new Image();
        img->Load(p, len, fn);
        bga_[session][i] = img;
        image_arr_.push_back(img);
      }
    }
    if (!bgm_fn_[session][i].empty())
    {
      Sound *s;
      fn = bgm_fn_[session][i].c_str();
      if (dir->GetFile(bgm_fn_[session][i], &p, len) && len > 0)
      {
        s = new Sound();
        s->Load(p, len, fn);
        bgm_[session][i] = s;
        sound_arr_.push_back(s);
      }
    }
  }
}

#if 0
void SongPlayer::PrepareResourceListFromSong()
{
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
#endif

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

bool SongPlayer::is_course() const
{
  return playlist_.size() > 1;
}

size_t SongPlayer::get_course_index() const
{
  return playlist_index_;
}

SongPlayerState SongPlayer::get_state() const
{
  return state_;
}

bool SongPlayer::is_loading() const
{
  return state_ == SongPlayerState::LOADING;
}

bool SongPlayer::is_loaded() const
{
  return state_ > SongPlayerState::LOADING;
}

bool SongPlayer::is_play_finished() const
{
  return state_ > SongPlayerState::FINISHED;
}

double SongPlayer::get_load_progress() const
{
  return load_progress_;
}

double SongPlayer::get_play_progress() const
{
  return play_progress_;
}

void SongPlayer::PlaySound(int channel, unsigned session)
{
  auto *sound = bgm_[session][(unsigned)channel];
  if (sound) sound->Play();
}

void SongPlayer::StopSound(int channel, unsigned session)
{
  auto *sound = bgm_[session][(unsigned)channel];
  if (sound) sound->Stop();
}

void SongPlayer::SetImage(int channel, unsigned session, unsigned layer)
{
  bga_selected_[session][layer] = (int)channel;
}

Image* SongPlayer::GetImage(unsigned session, unsigned layer)
{
  int ch = bga_selected_[session][layer];
  if (ch < 0) return nullptr;
  return bga_[session][(unsigned)ch];
}

void SongPlayer::set_play_after_loading(bool v)
{
  play_after_loading_ = v;
}

SongPlayer& SongPlayer::getInstance()
{
  static SongPlayer r;
  return r;
}

#if 0
// TODO: going to be used for general judgement system.

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

// TRACKCONTEXT

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
#endif

}
