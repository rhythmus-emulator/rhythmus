#pragma once

#include "PlaySession.h"  /* for playrecord */
#include "Game.h"
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
  int player_type() const;

  const PlayRecord *GetPlayRecord(const std::string &chartname) const;
  void GetReplayList(const std::vector<std::string> &replay_names);
  void SetRunningCombo(int combo);
  int GetRunningCombo() const;
  int GetTrackFromKeycode(int keycode) const;

  /* @brief set current playdata for saving (not actually saved) */
  void SetCurrentPlay(const PlayRecord &playrecord);

  /* @brief Initialize playrecord with current player option. */
  void InitPlayRecord(PlayRecord &pr);

  /* @brief store given playrecord. */
  void PostPlayRecord(PlayRecord &pr);

  /* @brief store given replay data. */
  void PostReplayData(ReplayData &replay);

  /* @brief clear out current play record / replay data. */
  void ClearCurrentPlay();

private:
  std::string player_name_;
  PlayerTypes player_type_;
  std::string config_name_;

  /* context for current play */
  int gamemode_;
  bool is_guest_;
  bool is_network_;
  int running_combo_;
  int course_combo_;

  /* PlayOption for current gamemode. */
  struct PlayOption {
    bool use_lanecover;
    bool use_hidden;
    int speed_type;
    float speed;
    float lanecover;
    float hidden;
    int option_chart;
    int option_chart_dp; /* only for BMS DP */
    int health_type;
    int assist;
    int pacemaker;
    std::string pacemaker_target;
    int pclass;

    PlayOption();
  } option_;
  KeySetting keysetting_;

  /* PlayRecord of this player. */
  std::vector<PlayRecord> playrecords_;

  /* ReplayData of this player. */
  std::vector<ReplayData> replaydata_;

  /* PlayRecord DB */
  sqlite3 *pr_db_;

  /* Accumulated PlayRecord. Updated in SavePlayRecord() method.
   * Used for courseplay recording. */
  PlayRecord playrecord_;

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
  static void UnloadNonePlayer();

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