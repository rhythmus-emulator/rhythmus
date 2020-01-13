#pragma once

#include "Image.h"
#include "Sound.h"
#include "TaskPool.h"
#include "Event.h"
#include "Setting.h"  /* for kMaxLaneCount */
#include "rparser.h"
#include <list>
#include <mutex>

namespace rhythmus
{

class SongPlayinfo;
class PlayContext;
class Player;
class NoteRenderData;
constexpr size_t kMaxChannelCount = 2048;

/* @brief A singleton class. Contains song and resources. */
class SongPlayer
{
public:
  SongPlayer();
  bool Load();
  void Play();
  void Stop(bool interrupted = true);
  void Update(float delta);

  /* @brief Set course for play. */
  void SetCoursetoPlay(const std::string &coursepath);

  /* @brief Set single song for playing. */
  void SetSongtoPlay(const std::string &songpath, const std::string &chartpath);

  /* @brief Add song to play. */
  void AddSongtoPlaylist(const std::string &songpath, const std::string &chartpath);

  /* @brief returns currently playing or going to be played song play info. */
  const SongPlayinfo *GetSongPlayinfo() const;

  void ProcessInputEvent(const InputEvent& e);
  void PopSongFromPlaylist();
  void ClearPlaylist();

  /* @brief Get notes to display
   * @param lane lane for filtering (-1 for all lanes) */
  //void GetNoteForRendering(int session, int lane, std::vector<NoteRenderData> &out);

  /* @brief get PlayContext */
  PlayContext* GetPlayContext(int session);

  rparser::Song* get_song();
  rparser::Chart* get_chart(const std::string& chartname);
  void set_load_bga(bool use_bga);
  int is_loaded() const;
  double get_progress() const;
  bool IsFinished() const;

  Sound* GetSound(const std::string& filename, int channel = -1);
  Image* GetImage(const std::string& filename);

  static SongPlayer& getInstance();
  friend class SongResourceLoadTask;
  friend class SongResourceLoadResourceTask;

private:
  rparser::Song *song_;

  /* for each session playing */
  PlayContext *sessions_[kMaxPlaySession];

  /* for combo (retaining previous playing combo) */
  int prev_playcombo_[kMaxPlaySession];

  struct SoundInfo
  {
    std::string name;
    int channel;
  };
  struct ImageInfo
  {
    std::string name;
  };

  std::vector<std::pair<ImageInfo, Image*> > bg_animations_;
  std::vector<std::pair<SoundInfo, Sound*> > sounds_;

  /**
   * Tasks (for cancel-waiting)
   */
  std::list<TaskAuto> tasks_;

  /* @brief list of songs to play */
  std::list<SongPlayinfo> playlist_;

  /* @brief is currently playing as course mode
   * (multiple songs in playlist) */
  bool is_courseplay_;

  /*
   * @brief files to load
   */
  struct FileToLoad
  {
    int type; // 0: image, 1: sound
    std::string filename;
  };
  std::list<FileToLoad> files_to_load_;
  size_t loadfile_count_total_;
  size_t wthr_count_;
  size_t loaded_file_count_;
  std::mutex loading_mutex_, res_mutex_;

  /*
   * 0: not loaded
   * 1: loading
   * 2: load complete
   */
  int is_loaded_;

  /* load(play) as async */
  bool load_async_;

  bool load_bga_;

  void CancelLoad();
  void LoadResources();
  void LoadResourcesAsync();
  void UploadBitmaps();
  void PrepareResourceListFromSong();
  bool IsChartPath(const std::string &path);
};

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

/* @brief judge context */
class JudgementContext
{
public:
  JudgementContext();
  JudgementContext(int pg, int gr, int gd, int bd, int pr);
  int get_pr_time() const;
  void setJudgementRatio(double r);
  int judge(int delta_time);
  static JudgementContext& getDefaultJudgementContext();

private:
  int judge_time_[6];
};

/* @brief Note object with judge context
 * TODO: what about chain note?
 * TODO: what about guitarfreaks type note? (insert invisible mine note?) */
class NoteWithJudging : public rparser::Note
{
public:
  NoteWithJudging(rparser::Note *note);
  virtual ~NoteWithJudging() = default;
  int judge(uint32_t songtime, int event_type);
  int judge_with_pos(uint32_t songtime, int event_type, int x, int y, int z);
  int judge_check_miss(uint32_t songtime);
  bool is_judge_finished() const;
  bool is_judgable() const;

private:
  /* current chain index of judgement */
  size_t chain_index_;

  /**
   * 0: not judged
   * 1: judging
   * 2: judge finished
   */
  int judge_status_;

  /* value of JudgeTypes */
  int judgement_;

  /* for invisible note object */
  bool invisible_;

  rparser::NoteDesc *get_curr_notedesc();
  JudgementContext *get_judge_ctx();
  int judge_only_timing(uint32_t songtime);
  JudgementContext *judge_ctx_;
};

class Player;
class TrackIterator;

/* @brief Contains current keysound, note with judgement of a track */
class TrackContext
{
public:
  void Initialize(rparser::Track &track);
  void SetInvisibleMineNote(double beat, uint32_t time); /* used for guitarfreaks */
  void Clear();
  void Update(uint32_t songtime);
  NoteWithJudging *get_curr_judged_note();
  NoteWithJudging *get_curr_sound_note();
  bool is_all_judged() const;
  friend class TrackIterator;

private:
  std::vector<NoteWithJudging> objects_;
  size_t curr_keysound_idx_;
  size_t curr_judge_idx_;
};

class TrackIterator
{
public:
  TrackIterator(TrackContext& track);
  bool is_end() const;
  void next();
  NoteWithJudging& operator*();
private:
  std::vector<NoteWithJudging*> notes_;
  size_t curr_idx_;
};

/* @brief Track for background objects (automatic, without judgement) */
class BackgroundDataContext
{
public:
  BackgroundDataContext();
  void Initialize(rparser::TrackData &data);
  void Initialize(rparser::Track &track);
  void Clear();
  void Update(uint32_t songtime);
  rparser::NotePos* get_current();
  const rparser::NotePos* get_current() const;
  rparser::NotePos* get_stack();
  const rparser::NotePos* get_stack() const;
  void pop_stack();
  void clear_stack();

private:
  std::vector<rparser::NotePos*> objects_;
  size_t curr_idx_;
  size_t curr_process_idx_;
};

/* @brief Brief status for play status */
class PlayRecord
{
public:
  std::string id;
  std::string chartname;
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
public:
  struct ReplayEvent
  {
    int event_type;
    int time;
    int value1;
    int value2;
  };

  std::vector<ReplayEvent> events;
};

/* @brief context used for playing song */
class PlayContext
{
public:
  PlayContext(Player *player, rparser::Chart &chart);

  void StartPlay();
  void StopPlay();
  void FinishPlay();
  void SavePlay();
  void LoadPlay(const std::string &play_id);

  void RecordPlay(ReplayEventTypes event_type, int time, int value1, int value2 = 0);
  const std::string &GetPlayId() const;

  double get_beat() const;
  double get_measure() const;
  double get_time() const;

  double get_rate() const;
  double get_current_rate() const;
  double get_score() const;
  double get_health() const;
  bool is_alive() const;
  bool is_finished() const;

  void ProcessInputEvent(const InputEvent& e);

  void Update(float delta);

  size_t GetTrackCount() const;
  TrackIterator GetTrackIterator(size_t track_idx);

  void UpdateResource();
  Image* GetImage(size_t layer_idx) const;
  PlayRecord &GetPlayRecord();

private:
  /* Player for current session. (may be null if autoplay) */
  Player *player_;

  rparser::TimingSegmentData *timing_seg_data_;
  rparser::MetaData *metadata_;
  size_t track_count_;

  /* unsaved playing context (for single stage) */

  BackgroundDataContext bga_context_[4]; /* up to 4 layers */
  BackgroundDataContext bgm_context_;
  TrackContext track_context_[kMaxLaneCount];
  void UpdateJudgeByRow(); /* for row-wise judgement update */

  double songtime_;
  double measure_;
  double beat_;

  double health_;
  int is_alive_;
  int combo_;         // combo in this chart
  int running_combo_; // currently displaying combo
  int passed_note_;
  int last_judge_type_;
  bool is_autoplay_;
  int course_index_;
  PlayRecord playrecord_;

  Sound* keysounds_[kMaxChannelCount];
  Image* bg_animations_[kMaxChannelCount];
  bool is_play_bgm_;

  /* unsaved playing context (for course) */
  /* TODO: should moved to player object ...? */

  /* replay context */
  ReplayData replaydata_;

  void LoadReplay(const std::string &replay_id);
};


}