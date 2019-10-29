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

struct SongListData
{
  std::string title;
  std::string subtitle;
  std::string artist;
  std::string subartist;
  std::string genre;
  std::string songpath;
  std::string chartpath;
  int level;
  int judgediff;
  int modified_date;
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

  /* @brief get currently selected SongInfo */
  SongListData* get_current_song_info();

  /* @brief trigger songlist selection */
  void select(int i);

  static SongList& getInstance();

private:
  // loaded songs
  std::vector<SongListData> songs_;

  // currently selected SongListData
  SongListData song_selected_;

  // songs to invalidate
  std::list<std::string> invalidate_list_;
  std::mutex invalidate_list_mutex_;
  std::string current_loading_file_;
  int total_inval_size_;
  std::atomic<int> load_count_;
  bool is_loaded_;

  std::string song_dir_;

  static int sql_dummy_callback(void*, int argc, char **argv, char **colnames);
  static int sql_songlist_callback(void*, int argc, char **argv, char **colnames);
};

/* @brief A singleton class. Contains song and resources. */
class SongResource
{
public:
  SongResource();
  void Load(const std::string& path);
  void LoadAsync(const std::string& path);
  void LoadResources();
  void UploadBitmaps();
  void Update(float delta);
  void CancelLoad();
  void Clear();

  rparser::Song* get_song();
  void set_load_bga(bool use_bga);
  int is_loaded() const;
  double get_progress() const;

  Sound* GetSound(const std::string& filename);
  Image* GetImage(const std::string& filename);

  static SongResource& getInstance();

private:
  rparser::Song* song_;
  std::vector<std::pair<std::string, Image*> > bgs_;
  std::vector<std::pair<std::string, Sound*> > sounds_;

  /**
   * Tasks (for cancel-waiting)
   */
  std::list<TaskAuto> tasks_;

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
  std::mutex loading_mutex_;

  /*
   * 0: not loaded
   * 1: loading
   * 2: load complete
   */
  int is_loaded_;

  bool load_bga_;
};

}