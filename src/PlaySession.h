#pragma once

/* use rparser::Note. @TODO copy note definition to Note object. */
#include "rparser.h"
#include "config.h"

namespace rhythmus
{

class InputEvent;
class Player;

enum ReplayEventTypes
{
  kReplayNone,
  kReplayMiss,
  kReplayPR,
  kReplayBD,
  kReplayGD,
  kReplayGR,
  kReplayPG,
  kReplayMine,
  kReplaySong, /* Need in course mode: song change trigger */
};

enum JudgeEventTypes
{
  kJudgeEventDown,
  kJudgeEventUp,
  kJudgeEventMove
};

enum ClearTypes
{
  kClearNone,
  kClearFailed,
  kClearAssist,
  kClearEasy,
  kClearNormal,
  kClearHard,
  kClearExHard,
  kClearFullcombo,
  kClearPerfect,
};

enum JudgeTypes
{
  kJudgeNone,
  kJudgeMiss,
  kJudgePR,
  kJudgeBD,
  kJudgeGD,
  kJudgeGR,
  kJudgePG,
  kJudgeOK, /* mine, LN */
};

enum GameSpeedTypes
{
  kSpeedNormal,
  kSpeedConstant,
};

enum NoteStatus
{
  kNoteStatusNone,
  kNoteStatusJudgelinePassed,
  kNoteStatusJudged
};

/**
 * @brief Note data
 */
struct Note
{
  /* pointer to original note objects */
  rparser::NoteElement *note;

  /* event time */
  double time;

  /* measure position */
  double measure;

  /* channel */
  unsigned channel;

  /* a status for a note (is processed or going to be processed) */
  int status;

  /* judgement for a note */
  int judge;

  /* judgement delta time for a note event */
  double judgetime;
};

/**
 * @brief Mine note
 */


/**
 * @brief Bga note
 */
struct BgaNote
{
  rparser::NoteElement *note;

  double time;

  unsigned channel;

  unsigned layer;
};

/**
 * @brief Bgm note
 */
struct BgmNote
{
  rparser::NoteElement *note;

  double time;

  unsigned channel;
};


/* @brief Brief status for play status */
class PlayRecord
{
public:
  std::string id;
  std::string chartname;
  int gamemode;
  int timestamp;
  int seed;
  int speed;
  int speed_type;
  int clear_type;
  int health_type;
  int option;
  int option_dp;
  int assist;
  int total_note;
  int miss, pr, bd, gd, gr, pg;
  int maxcombo;
  int score;
  int playcount;
  int clearcount;
  int failcount;

  double rate() const;
  int exscore() const;
};

class ReplayData
{
};

/* @brief game play context used for playing song */
class PlaySession
{
public:
  PlaySession(unsigned session_idx, Player *player, rparser::Chart &chart);

  /* @brief save play information to player (if exists) */
  void Save();

  double get_beat() const;
  double get_measure() const;
  double get_time() const;
  double get_total_time() const;
  double get_progress() const;

  double get_rate() const;
  double get_current_rate() const;
  double get_score() const;
  double get_health() const;
  double get_running_combo() const;
  bool is_alive() const;
  bool is_finished() const;

  void ProcessInputEvent(const InputEvent& e);

  void Update(float delta);

  size_t GetTrackCount() const;

  PlayRecord &GetPlayRecord();

  virtual void OnSongStart();
  virtual void OnSongEnd();
  virtual void OnCourseStart();
  virtual void OnCourseEnd();

  /**
   * @brief called when a note just passed a judgeline(time),
   * which might be assisted or not.
   */
  virtual void OnNoteAutoplay(Note &n);

  /**
   * @brief called when a note is not tapped(missed).
   */
  virtual void OnNotePassed(Note &n);

  /**
   * @brief called when a note is pressed.
   */
  virtual void OnNoteDown(Note &n);

  /**
   * @brief called when a note is released.
   */
  virtual void OnNoteUp(Note &n);

  /**
   * @brief called when a note is dragged.
   * \warn  delta time is not updated when this method is called.
   * \info  Hell charge note also need to be processed using this method.
   */
  virtual void OnNoteDrag(Note &n);

  /**
   * @brief called when Bga note passed.
   */
  virtual void OnBgaNote(BgaNote &n);

  /**
   * @brief called when Bgm note passed.
   */
  virtual void OnBgmNote(BgmNote &n);

private:
  Player *player_;
  rparser::TimingSegmentData *timing_seg_data_;
  rparser::MetaData *metadata_;
  unsigned session_;

  /* Track context */
  struct Track
  {
    std::vector<Note> notes;
    size_t index;
    bool is_finished() const;
    Note *current_note();
  } track_[kMaxLaneCount];
  size_t track_count_;

  /* Bgm */
  struct BgmTrack {
    std::vector<BgmNote> v;
    size_t i;
  } bgm_track_;

  /* Bga */
  struct BgaTrack {
    std::vector<BgaNote> v;
    size_t i;
  } bga_track_;

  double songtime_;
  double total_songtime_;
  double measure_;
  double beat_;

  /* delta time of note judgement
   * (note won't be judged if it is too early or late compared to this deltatime) */
  double note_deltatime_;

  double health_;
  int is_alive_;
  int combo_;         // combo of this chart
  int running_combo_; // currently displaying combo
  int passed_note_;
  int last_judge_type_;
  bool is_autoplay_;
  PlayRecord playrecord_;
  ReplayData replay_;

  bool is_play_bgm_;

  /* unsaved playing context (for course) */
  /* TODO: should moved to player object ...? */

  void LoadFromChart(rparser::Chart &chart);
  void LoadFromPlayer(Player &player);
  void LoadReplay(const std::string &replay_path);
  void SaveReplay(const std::string &replay_path);
};

/**
 * @brief Interface of a handlers required for gameplay.
 */
class PlayContextInterface
{
public:

  /* @brief get mode name of this playcontext. */
  virtual const std::string& get_mode() const;
};

}