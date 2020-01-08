#pragma once

#include "Image.h"
#include "Sound.h"
#include "TaskPool.h"
#include "rparser.h"
#include <list>
#include <mutex>

namespace rhythmus
{

class SongPlayinfo;

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

  /* @brief   Assign a song session to a player.
   * @detail  with this method,
   * * Indicated chart of the song session became invalid
   *   and don't autoplay anymore.
   * * Player owns chart info and need to call play() method
   *   of the session. */
  void AssignSessionToPlayer(int session_id, int player_id);

  /* @brief play keysound of the channel of the session. */
  void PlaySound(int session, int channel);

  /* @brief stop keysound of the channel of the session */
  void StopSound(int session, int channel);

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