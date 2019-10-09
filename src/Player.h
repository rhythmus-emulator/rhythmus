#pragma once

#include "Song.h"
#include "Setting.h"
#include <string>

namespace rhythmus
{

constexpr size_t kMaxPlayerSlot = 16;

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

enum JudgeTypes
{
  kJudgeNone,
  kJudgeMiss,
  kJudgePR,
  kJudgeBD,
  kJudgeGD,
  kJudgeGR,
  kJudgePG,
  kJudgeMine,
};

enum GameSpeedTypes
{
  kSpeedNormal,
  kSpeedConstant,
};

class Player
{
public:
  Player(PlayerTypes player_type, const std::string& player_name);
  virtual ~Player();
  void Load();
  void Save();
  void SetPlayId(const std::string& play_id);
  void LoadPlay(bool load_replay = false);
  void SavePlay();
  void StopPlay();
  void ClearPlay();

  /* Should be called after song is loaded */
  void StartPlay();

  /**
   * Difference from StartPlay:
   * Only changes song; Don't clear course context.
   */
  void StartNextSong();

  void RecordPlay(ReplayEventTypes event_type, int time, int value1, int value2 = 0);

  double get_rate() const;
  double get_current_rate() const;
  double get_score() const;
  double get_health() const;
  bool is_alive() const;
  void set_chartname(const std::string& chartname);
  ChartPlayer& get_chart_player();

  void Update(float delta);

  static Player& getMainPlayer();
  static void CreatePlayer(PlayerTypes playertype, const std::string& player_name);
  static void CreatePlayer(PlayerTypes playertype, const std::string& player_name, int player_slot);
  static Player* getPlayer(int player_slot);
  static void UnloadPlayer(int player_slot);
  static bool IsPlayerLoaded(int player_slot);
  static int GetLoadedPlayerCount();

private:
  std::string player_name_;
  PlayerTypes player_type_;
  Setting setting_;

  /* saved playing context */

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

  /* unsaved playing context (for single stage) */

  std::string play_id_;
  ChartPlayer chartplayer_;
  std::string chartname_;

  double health_;
  int is_alive_;
  int combo_;
  int last_judge_type_;
  int total_note_;
  int passed_note_;
  struct Judge {
    int miss, pr, bd, gd, gr, pg;
  };
  Judge judge_;

  /* unsaved playing context (for course) */

  Judge course_judge_;

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
    int timestamp;
    int seed;
    int speed;
    int speed_type;
    int health_type;
    double rate;
    int score;
    int total_note;
    int option;
    int option_dp;
    int assist;
    std::vector<ReplayEvent> events;
  } replay_;

  /* unsaved context (internal) */

  bool is_save_allowed_;
  bool is_save_record_;
  bool is_save_replay_;
  bool is_guest_;

  void LoadRecord();
  void SaveRecord();
  void LoadReplay();
  void SaveReplay();
};

#define FOR_EACH_PLAYER(p, i) \
for (p = nullptr, i = 0; i < kMaxPlayerSlot; p = Player::getPlayer(i), ++i) if (!p) continue; else

}