#pragma once

#include "Image.h"
#include "Sound.h"
#include "TaskPool.h"
#include <string>
#include <memory>
#include <vector>
#include <queue>
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
   * Load a song file into song list if file not exist in songlist.\
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

/* @brief A singleton class. Contains song and resources. */
class SongPlayer
{
public:
  SongPlayer();
  bool Load();
  void Play();
  void Stop();
  void Update(float delta);

  /* @brief Set course for play. */
  void SetCoursetoPlay(const std::string &coursepath);

  /* @brief Set single song for playing. */
  void SetSongtoPlay(const std::string &songpath, const std::string &chartpath);

  /* @brief Add song to play. */
  void AddSongtoPlaylist(const std::string &songpath, const std::string &chartpath);

  /* @brief returns currently playing or going to be played song play info. */
  const SongPlayinfo *GetSongPlayinfo() const;

  void PopSongFromPlaylist();
  void ClearPlaylist();

  rparser::Song* get_song();
  rparser::Chart* get_chart(const std::string& chartname);
  void set_load_bga(bool use_bga);
  int is_loaded() const;
  double get_progress() const;

  Sound* GetSound(const std::string& filename, int channel = -1);
  Image* GetImage(const std::string& filename);

  static SongPlayer& getInstance();
  friend class SongResourceLoadTask;
  friend class SongResourceLoadResourceTask;

private:
  rparser::Song* song_;

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

}