#pragma once

#include "TaskPool.h"
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <list>
#include <vector>

namespace rparser { class Song; class Chart; }

namespace rhythmus
{

enum Gamemode
{
  kGamemodeNone,
  kGamemode4Key,
  kGamemode5Key,
  kGamemode6Key,
  kGamemode7Key,
  kGamemode8Key,
  kGamemodeIIDXSP,
  kGamemodeIIDXDP,
  kGamemodeIIDX5Key,
  kGamemodeIIDX10Key,
  kGamemodePopn,
  kGamemodeEZ2DJ,
  kGamemodeDDR,
  kGamemodeEnd,
};
const char* GamemodeToString(int gamemode);
int StringToGamemode(const char* s);

enum Difficulty
{
  kDifficultyNone,
  kDifficultyBeginner,
  kDifficultyEasy,
  kDifficultyNormal,
  kDifficultyHard,
  kDifficultyEx,
  kDifficultyInsane,
  kDifficultyEnd,
};
const char* DifficultyToString(int gamemode);
int StringToDifficulty(const char* s);

enum Sorttype
{
  kNoSort,
  kSortByTitle,
  kSortByLevel,
  kSortByClear,
  kSortByRate,
  kSortEnd,
};
const char* SorttypeToString(int gamemode);
int StringToSorttype(const char* s);

/* @brief Brief song data for caching in database */
struct SongListData
{
  std::string id;
  std::string title;
  std::string subtitle;
  std::string artist;
  std::string subartist;
  std::string genre;
  std::string songpath;
  std::string chartpath;
  int type;
  int level;
  int difficulty;
  int judgediff;
  int modified_date;
  int notecount;
  int length_ms;
  int bpm_max;
  int bpm_min;
  int is_longnote;
  int is_backspin;
};

/* @brief song, charts (per player) to play */
struct SongPlayinfo
{
  std::string songpath;
  std::string chartpaths[10];
};

/* @brief A singleton class which contains currently loaded Song DB */
class SongList
{
public:
  SongList();

  void Load();
  void Save();
  void Clear();

  /* @brief invalidate song entry. used for multi-threading. */
  void LoadInvalidationList();

  /* @brief clear song invalidation entry
   * (for canceling songlist loading) */
  void ClearInvalidationList();

  /**
   * @brief
   * Load a song file into song list if file not exist in songlist.
   */
  void LoadFileIntoSongList(
    const std::string& songpath,
    const std::string& chartname = std::string()
  );

  double get_progress() const;
  bool is_loaded() const;
  std::string get_loading_filename() const;

  size_t size();
  std::vector<SongListData>::iterator begin();
  std::vector<SongListData>::iterator end();
  const SongListData& get(int i) const;
  SongListData get(int i);

  static SongList& getInstance();

private:
  // loaded songs
  std::vector<SongListData> songs_;

  // songs to invalidate
  std::list<std::string> invalidate_list_;
  std::mutex invalidate_list_mutex_, songlist_mutex_;
  std::string current_loading_file_;
  int total_inval_size_;
  std::atomic<int> load_count_;
  bool is_loaded_;

  std::string song_dir_;

  static int sql_dummy_callback(void*, int argc, char **argv, char **colnames);
  static int sql_songlist_callback(void*, int argc, char **argv, char **colnames);
};

}