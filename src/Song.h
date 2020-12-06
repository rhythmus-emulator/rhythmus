#pragma once

#include "TaskPool.h"
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <list>
#include <vector>

namespace rparser { class Song; class Chart; }

struct sqlite3;

namespace rhythmus
{

struct ChartMetaData;
struct SongMetaData;

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

/* @brief song data cached in database */
struct SongMetaData
{
  std::string path;
  int type;
  int64_t modified_time;

  // in-memory attributes

  unsigned count;         // chart count
  ChartMetaData* chart;   // any chart data
};

/* @brief chart data cached in database */
struct ChartMetaData
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
  int64_t modified_time;

  // in-memory attributes

  SongMetaData* song;
  ChartMetaData *next, *prev;
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
  ~SongList();

  static void Initialize();
  static void Cleanup();

  void Load();
  void Update();
  void Save();
  void Clear();

  SongMetaData* FindSong(const std::string& path);
  ChartMetaData* FindChart(const std::string& id);

  const std::vector<SongMetaData*> &GetSongList() const;
  const std::vector<ChartMetaData *> &GetChartList() const;

  /**
   * @brief
   * Load a song file into song list if file not exist in songlist.
   */
  void LoadFileIntoChartList(
    const std::string& songpath,
    const std::string& chartname = std::string()
  );

  double get_progress() const;
  bool is_loaded() const;
  std::string get_loading_filename() const;

  size_t song_count() const;
  size_t chart_count() const;

  friend class SongListUpdateTask;

private:
  std::vector<SongMetaData*> songs_;
  std::vector<ChartMetaData*> charts_;

  // songs to load
  std::mutex loading_mutex_;
  std::string current_loading_file_;
  int total_inval_size_;
  int load_count_;
  bool is_loaded_;

  // song directory
  std::string song_dir_;

  // song db directory
  std::string song_db_;

  // sqlite handler
  static int sql_dummy_callback(void*, int argc, char **argv, char **colnames);
  static int sql_chartlist_callback(void*, int argc, char** argv, char** colnames);
  static int sql_songlist_callback(void*, int argc, char** argv, char** colnames);

  bool LoadFromDatabase(const std::string& path);
  bool CreateDatabase(sqlite3 *db);
  bool PushChart(ChartMetaData* p);
  void PushSong(SongMetaData* p);
  void StartSongLoading(const std::string &name);
  void FinishSongLoading();
};

extern SongList *SONGLIST;

}