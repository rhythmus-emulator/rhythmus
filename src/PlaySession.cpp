#include "PlaySession.h"
#include "SongPlayer.h"
#include "Player.h"
#include "Event.h"
#include <algorithm>

namespace rhythmus
{

// --------------------------------- PlayRecord

double PlayRecord::rate() const
{
  return (double)exscore() / (total_note * 2);
}

int PlayRecord::exscore() const
{
  return pg * 2 + gr * 1;
}

// -------------------------- class PlaySession

PlaySession::PlaySession(unsigned session, Player *player, rparser::Chart &c)
  : player_(player), timing_seg_data_(nullptr), metadata_(nullptr),
    session_(session), songtime_(0), measure_(0), beat_(0), track_count_(0),
    is_alive_(0), health_(0.), combo_(0), running_combo_(0), passed_note_(0),
    last_judge_type_(JudgeTypes::kJudgeNone), is_autoplay_(true), is_play_bgm_(true)
{
  if (player)
    LoadFromPlayer(*player);
  LoadFromChart(c);
#if 0
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
#endif
}

void PlaySession::LoadFromChart(rparser::Chart &c)
{
  /* MUST be called only once. */
  R_ASSERT(total_songtime_ == 0);

  /* get timing segment and metadata from chart */
  timing_seg_data_ = &c.GetTimingSegmentData();
  metadata_ = &c.GetMetaData();

  /* load general note data.
   * TODO: change longnote data form in rparser */
  {
    auto &nd = c.GetNoteData();
    track_count_ = nd.get_track_count();
    size_t i = 0;
    for (; i < track_count_; ++i)
    {
      Note n;
      for (auto &ne : nd.get_track(i))
      {
        n.note = &ne;
        n.time = ne.time();
        n.measure = ne.measure();
        n.channel = ne.get_value_u();
        n.status = NoteStatus::kNoteStatusNone;
        n.judge = JudgeTypes::kJudgeNone;
        n.judgetime = 0;
        track_[i].notes.push_back(n);
      }
      track_[i].index = 0;
    }
  }

  /* load bgm data */
  {
    auto &nd = c.GetBgmData();
    unsigned trkcnt = (unsigned)nd.get_track_count();
    for (unsigned i = 0; i < trkcnt; ++i)
    {
      auto &trk = nd[i];
      for (auto &ne : trk)
      {
        BgmNote n;
        n.note = &ne;
        n.time = ne.time();
        n.channel = ne.get_value_u();
        bgm_track_.v.push_back(n);
      }
    }
    std::sort(bgm_track_.v.begin(), bgm_track_.v.end(),
      [](BgmNote &a, BgmNote &b) { return a.time < b.time; });
  }

  /* load bga data
   * NOTE: layer 0 is miss layer. */
  {
    auto &nd = c.GetCommandData();
    const static int track_idx[] = {
      rparser::CommandTrackTypes::kBgaMiss,
      rparser::CommandTrackTypes::kBgaMain,
      rparser::CommandTrackTypes::kBgaLayer1,
      rparser::CommandTrackTypes::kBgaLayer2,
    };
    const static unsigned layer_idx[] = { 0, 1, 2, 3 };
    for (unsigned i = 0; i < 4; ++i)
    {
      auto &trk = nd[track_idx[i]];
      for (auto &ne : trk)
      {
        BgaNote n;
        n.note = &ne;
        n.time = ne.time();
        n.layer = layer_idx[i];
        n.channel = ne.get_value_u();
        bga_track_.v.push_back(n);
      }
    }
    std::sort(bga_track_.v.begin(), bga_track_.v.end(),
      [](BgaNote &a, BgaNote &b) { return a.time < b.time; });
  }

  /* TODO: load mine / invisible notes. */

  /* total_songtime (TODO: add proper sound length time) */
  total_songtime_ = c.GetSongLastObjectTime() + 3000;
}

/* set key matching and play option from player object
 * XXX: const Player type */
void PlaySession::LoadFromPlayer(Player &player)
{
  // Set PlayRecord / Replay context
  player.InitPlayRecord(playrecord_);

  // Check play record saving allowed, e.g. assist option
  bool check = (playrecord_.assist == 0);
}

void PlaySession::LoadReplay(const std::string &replay_path)
{
  /* TODO: read replaydata and fill it into track note objects. */
}

void PlaySession::SaveReplay(const std::string &replay_path)
{

}

void PlaySession::Save()
{
  if (!player_) return;
  /**
  TODO: Call these method at the end of the PlayScreen,
  or at the beginning of the ResultScreen.
  sessions_[i]->OnSongEnd();
  if (is_course_finished)
    sessions_[i]->OnCourseEnd();
  */

  /* Upload playrecord & replay data to player. */
  player_->PostReplayData(replay_);
  player_->PostPlayRecord(playrecord_);
}

double PlaySession::get_beat() const { return beat_; }
double PlaySession::get_measure() const { return measure_; }
double PlaySession::get_time() const { return songtime_; }
double PlaySession::get_total_time() const { return total_songtime_; }
double PlaySession::get_progress() const { return songtime_ / total_songtime_; }

double PlaySession::get_rate() const
{
  if (playrecord_.total_note == 0) return .0;
  return (double)(playrecord_.pg * 2 + playrecord_.gr) / playrecord_.total_note;
}

double PlaySession::get_current_rate() const
{
  if (passed_note_ == 0) return .0;
  return (double)(playrecord_.pg * 2 + playrecord_.gr) / passed_note_;
}

double PlaySession::get_score() const
{
  return get_rate() * 200000.0;
}

double PlaySession::get_health() const
{
  return health_;
}

double PlaySession::get_running_combo() const { return running_combo_; }

bool PlaySession::is_alive() const
{
  return is_alive_;
}

bool PlaySession::is_finished() const
{
#if 0
  // died, or no remain note to play.
  if (!is_alive()) return true;
  for (size_t i = 0; i < kMaxLaneCount; ++i)
  {
    if (track_[i].notes.size() > track_[i].index)
      return false;
  }
  return true;
#endif
  return get_progress() >= 1.0;
}

void PlaySession::ProcessInputEvent(const InputEvent& e)
{
  if (is_autoplay_ || !player_)
    return;

  // get track from keycode setting
  int track_no = player_->GetTrackFromKeycode(e.KeyCode());

  if (track_no == -1)
    return;

  // sound first before judgement
  SongPlayer::getInstance().PlaySound(track_no, session_);

  // check if note is in range of judgement. if so, trigger event.
  // TODO: trigger for OnNoteUp
  if (!track_[track_no].is_finished())
  {
    auto &obj = *track_[track_no].current_note();
    double delta =
      (e.time() - Timer::SystemTimer().GetTime()) * 1000 + songtime_;
    obj.judgetime = delta;
    OnNoteDown(obj);
  }
}

void PlaySession::Update(float delta)
{
  if (!is_alive_)
    return;

  songtime_ += delta;
  measure_ = timing_seg_data_->GetMeasureFromTime(songtime_);
  beat_ = timing_seg_data_->GetBeatFromMeasure(measure_);

  // 1. update index of track and send event
  //    for time-passing note
  for (size_t i = 0; i < track_count_ /* XXX: get & set lane count? */; ++i)
  {
    while (!track_[i].is_finished())
    {
      auto &n = *track_[i].current_note();
      if (n.status == NoteStatus::kNoteStatusNone && n.time < songtime_)
      {
        OnNoteAutoplay(n);
        n.status = NoteStatus::kNoteStatusJudgelinePassed;
      }
      else if (n.status == NoteStatus::kNoteStatusJudgelinePassed && n.time + note_deltatime_ < songtime_)
      {
        OnNotePassed(n);
        n.judge = JudgeTypes::kJudgeMiss;
        n.status = NoteStatus::kNoteStatusJudged;
        track_[i].index++;
      }
      else /* note is already judged by something - like touch event */
      {
        track_[i].index++;
      }
      if (n.status != NoteStatus::kNoteStatusJudged) break;
    }
  }

  // 2. update for special object note
  // (e.g. Mine object, Keysound object, etc)
  // TODO

  // 3. update for Bga / Bgm notes
  while (bga_track_.i < bga_track_.v.size())
  {
    auto &n = bga_track_.v[bga_track_.i];
    if (n.time > songtime_) break;
    SongPlayer::getInstance().SetImage(n.channel, session_, n.layer);
  }
  while (bgm_track_.i < bgm_track_.v.size())
  {
    auto &n = bgm_track_.v[bgm_track_.i];
    if (n.time > songtime_) break;
    SongPlayer::getInstance().PlaySound(n.channel, session_);
  }

  // 4. trigger OnNoteDrag event
  // TODO: tap-event-table is necessary

  // 5. update synchronous-note-table for some game mode (e.g. guitarfreaks, DDR).
  // TODO
}

size_t PlaySession::GetTrackCount() const
{
  return track_count_;
}

PlayRecord &PlaySession::GetPlayRecord()
{
  return playrecord_;
}

bool PlaySession::Track::is_finished() const { return notes.size() <= index; }
Note* PlaySession::Track::current_note() { return &notes[index]; }

void PlaySession::OnSongStart()
{
  is_alive_ = 1;
  health_ = 1.0;
  combo_ = 0;
  songtime_ = 0;
  last_judge_type_ = JudgeTypes::kJudgeNone;
}

void PlaySession::OnSongEnd()
{
}

void PlaySession::OnCourseStart()
{
}

void PlaySession::OnCourseEnd()
{
}

void PlaySession::OnNoteAutoplay(Note &n)
{
}

/* missing note goes here. */
void PlaySession::OnNotePassed(Note &n)
{
}

void PlaySession::OnNoteDown(Note &n)
{
}

void PlaySession::OnNoteUp(Note &n)
{
}

void PlaySession::OnNoteDrag(Note &n)
{
}

void PlaySession::OnBgaNote(BgaNote &n)
{
}

void PlaySession::OnBgmNote(BgmNote &n)
{
}

}
