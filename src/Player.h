#pragma once

#include "Song.h"
#include "Setting.h"
#include "Event.h"
#include <string>
#include <list>

#include "rparser.h"

namespace rhythmus
{

constexpr size_t kMaxPlayerSlot = 16;
constexpr size_t kMaxTrackSize = 128;

enum PlayerTypes
{
  kPlayerNone,
  kPlayerGuest,
  kPlayerNormal,
  kPlayerAuto,
  kPlayerGhost,
  kPlayerNetwork,
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
struct PlayRecord
{
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

/* @brief context used for game playing */
class PlayContext
{
public:
  PlayContext(Player &player, rparser::Chart &chart);

  void StartPlay();
  void StopPlay();
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

  TrackIterator GetTrackIterator(size_t track_idx);

  Image* GetImage(size_t layer_idx) const;
  PlayRecord &GetPlayRecord();

private:
  Player &player_;
  rparser::TimingSegmentData *timing_seg_data_;

  /* unsaved playing context (for single stage) */

  BackgroundDataContext bga_context_[4]; /* up to 4 layers */
  BackgroundDataContext bgm_context_;
  TrackContext track_context_[kMaxTrackSize];
  void UpdateJudgeByRow(); /* for row-wise judgement update */

  double songtime_;
  double measure_;
  double beat_;

  double health_;
  int is_alive_;
  int combo_;
  int last_judge_type_;
  int passed_note_;
  PlayRecord playrecord_;

  Sound* keysounds_[2000]; // XXX: 2000?
  Image* bg_animations_[2000];
  bool is_play_bgm_;

  /* unsaved playing context (for course) */
  /* TODO: should moved to player object ...? */

  /* replay context */

  struct ReplayEvent
  {
    int event_type;
    int time;
    int value1;
    int value2;
  };

  struct Replay
  {
    std::vector<ReplayEvent> events;
  } replay_;

  void LoadRecord();
  void SaveRecord();
  void LoadReplay();
  void SaveReplay();
};

class Player
{
public:
  Player(PlayerTypes player_type, const std::string& player_name);
  virtual ~Player();
  void Load();
  void Save();

  const PlayRecord *GetPlayRecord(const std::string &chartname) const;
  void SetPlayRecord(const PlayRecord &playrecord);
  void SetPlayContext(const std::string &chartname);
  void ClearPlayContext();
  PlayContext *GetPlayContext();
  void AddChartnameToPlay(const std::string &chartname);
  void LoadNextChart();

  static Player& getMainPlayer();
  static void CreatePlayer(PlayerTypes playertype, const std::string& player_name);
  static void CreatePlayer(PlayerTypes playertype, const std::string& player_name, int player_slot);
  static Player* getPlayer(int player_slot);
  static void UnloadPlayer(int player_slot);
  static bool IsPlayerLoaded(int player_slot);
  static int GetLoadedPlayerCount();
  static bool IsAllPlayerFinished();

  static void Initialize();
  static void Cleanup();

private:
  std::string player_name_;
  PlayerTypes player_type_;
  OptionList config_;
  std::string config_path_;
  bool is_guest_;

#define USER_PROP(type, varname) \
public: \
  type get_##varname() const { return varname##_; } \
  void set_##varname(type v) { varname##_ = v; } \
private: \
  type varname##_; \

  USER_PROP(bool, use_lane_cover);
  USER_PROP(bool, use_hidden);
  USER_PROP(int, game_speed_type);
  USER_PROP(double, game_speed);
  USER_PROP(double, game_constant_speed);
  USER_PROP(double, lane_cover);
  USER_PROP(double, hidden);
  USER_PROP(int, option_chart);
  USER_PROP(int, option_chart_dp); /* only for BMS DP */
  USER_PROP(int, health_type);
  USER_PROP(int, assist);
  USER_PROP(int, pacemaker);
  USER_PROP(std::string, pacemaker_target);
  USER_PROP(int, bms_sp_class);
  USER_PROP(int, bms_dp_class);

#undef USER_PROP

  struct KeySetting
  {
    int keycode_per_track_[kMaxTrackSize][4];
  } default_keysetting_;

  /* Keysetting. */
  std::map<std::string, KeySetting> keysetting_per_gamemode_;
  KeySetting *curr_keysetting_;

  /* PlayRecord of this player. Filled in Load() method. */
  std::vector<PlayRecord> playrecords_;

  /* context for currently playing chart */
  PlayContext *play_context_;

  /* XXX: context for course play? */
  PlayRecord courserecord_;
  /* reserved charts to be played */
  std::list<std::string> play_chartname_;
  bool is_courseplay_;
  bool is_replay_;
  bool is_save_allowed_;

  friend class PlayContext;
};

#define FOR_EACH_PLAYER(p, i) \
{ Player *p; int i; \
for (p = nullptr, i = 0; i < kMaxPlayerSlot; p = Player::getPlayer(i), ++i) if (!p) continue; else

#define END_EACH_PLAYER() \
}

}