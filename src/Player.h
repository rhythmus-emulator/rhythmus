#pragma once

#include "SongPlayer.h" /* PlayRecord */
#include "Setting.h"
#include "Event.h"
#include "Sound.h"
#include "Image.h"
#include <string>
#include <list>

#include "rparser.h"

struct sqlite3;

namespace rhythmus
{

class PlayRecord;
class PlayContext;

enum PlayerTypes
{
  kPlayerNone,
  kPlayerGuest,
  kPlayerNormal,
  kPlayerAuto,
  kPlayerGhost,
  kPlayerNetwork,
};

class Player
{
public:
  Player(PlayerTypes player_type, const std::string& player_name);
  virtual ~Player();
  void Load();
  void Save();
  void Sync();

  const PlayRecord *GetPlayRecord(const std::string &chartname) const;
  void GetReplayList(const std::vector<std::string> &replay_names);
  void SetRunningCombo(int combo);
  int GetRunningCombo() const;

  /* @brief set current playdata for saving (not actually saved) */
  void SetCurrentPlay(const PlayRecord &playrecord, const ReplayData &replaydata);

  /* @brief save current play record / replay. */
  void SaveCurrentPlay();

  /* @brief clear out current play record / replay data. */
  void ClearCurrentPlay();

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

  USER_PROP(bool, use_lanecover);
  USER_PROP(bool, use_hidden);
  USER_PROP(int, game_speed_type);
  USER_PROP(double, game_speed);
  USER_PROP(double, game_constant_speed);
  USER_PROP(double, lanecover);
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

  /* Keysetting. */
  KeySetting default_keysetting_;
  std::map<std::string, KeySetting> keysetting_per_gamemode_;
  KeySetting *curr_keysetting_;

  /* PlayRecord of this player. Filled in Load() method. */
  std::vector<PlayRecord> playrecords_;

  /* PlayRecord DB */
  sqlite3 *pr_db_;

  /* Accumulated PlayRecord. Updated in SavePlayRecord() method.
   * Used for courseplay recording. */
  PlayRecord playrecord_;

  /* context for current play */
  bool is_network_;
  int running_combo_;

  /* XXX: context for course play? */
  PlayRecord courserecord_;
  bool is_courseplay_;

  friend class PlayContext;
  void LoadPlayRecords();
  void UpdatePlayRecord(const PlayRecord &pr);
  void ClosePlayRecords();
  static int CreateSchemaCallbackFunc(void*, int argc, char **argv, char **colnames);
  static int PRQueryCallbackFunc(void*, int argc, char **argv, char **colnames);
  static int PRUpdateCallbackFunc(void*, int argc, char **argv, char **colnames);
};

class PlayerManager
{
public:
  /* @brief Get any available player (must not be null) */
  static Player* GetPlayer();

  /* @brief Get player at the slot. */
  static Player* GetPlayer(int player_slot);

  /* @brief Create Player object.
   * the player is automatically Sync & Loaded.
   * @param playertype  player type (GUEST, LOCAL, NETWORK)
   * @param player_name the id of the player.
   * @param passwd      hashed password of the player.
   * @param slot_no     Create player at the slot (optional).
   *                    Set to any left slot if empty.
   */
  static void CreatePlayer(PlayerTypes playertype, const std::string& player_name, const std::string &passwd);
  static void CreatePlayer(PlayerTypes playertype, const std::string& player_name, const std::string &passwd, int slot_no);

  /* @brief Create guest player if no player loaded.
   * used for safe logic of SelectScreen() */
  static void CreateGuestPlayerIfEmpty();

  /* @brief Create guest player if no player loaded.
   * used for safe logic of PlayScreen() */
  static void CreateNonePlayerIfEmpty();

  /* @brief Unload Player object, automatically saved. */
  static void UnloadPlayer(int player_slot);

  static bool IsAvailable(int player_slot);
  static int GetLoadedPlayerCount();
  static void Initialize();
  static void Cleanup();
};

#define FOR_EACH_PLAYER(p, i) \
{ Player *p; int i; \
for (p = nullptr, i = 0; i < kMaxPlaySession; ++i) { \
p = PlayerManager::GetPlayer(i); \
if (!p) continue; else

#define END_EACH_PLAYER() \
} }

}