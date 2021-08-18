#pragma once

#include "Image.h"
#include "Sound.h"
#include "TaskPool.h"
#include "Event.h"
#include "config.h"
#include "rparser.h"
#include <list>
#include <mutex>
#include <atomic>

namespace rhythmus
{

struct SongPlayinfo;
class PlaySession;
class Player;
class NoteRenderData;

/* @brief current state of SongPlayer */
enum class SongPlayerState
{
  STOPPED,
  LOADING,
  PLAYING,
  PAUSED,
  FINISHED
};

class SongResourceLoadCallback : public ITaskCallback
{
public:
  SongResourceLoadCallback(std::atomic<unsigned>& v) : v_(&v) {};
  virtual void run() { (*v_)++; }
private:
  std::atomic<unsigned>* v_;
};

/* @brief A singleton class. Contains song and resources. */
class SongPlayer
{
public:
  SongPlayer();

  /* @brief Set course for play. */
  void SetCoursetoPlay(const std::string &coursepath);

  /* @brief Set single song for playing. */
  void SetSongtoPlay(const std::string &songpath, const std::string &chartpath);

  /* @brief Add song to play. */
  void AddSongtoPlaylist(const std::string &songpath, const std::string &chartpath);

  void AddSongtoPlaylistFromId(const std::string& id);

  /* @brief Load next song to play. return if next song exists.
   * @warn If PlaySession is not created, then autoplay playsession is created. */
  bool LoadNext();

  /* @brief Play current song. If not loaded, then it automatically invokes LoadNext(). */
  void Play();

  /* @brief stop current song & resource */
  void Stop();

  /* @brief stop current song and clear playlist. */
  void Clear();

  /* @brief called before rendering */
  void Update(float ms);

  /* @brief returns currently playing or going to be played song play info. */
  const SongPlayinfo *GetSongPlayinfo() const;

  void ProcessInputEvent(const InputEvent& e);

  /* @brief Get notes to display
   * @param lane lane for filtering (-1 for all lanes) */
  //void GetNoteForRendering(int session, int lane, std::vector<NoteRenderData> &out);

  /* @brief get PlaySession */
  PlaySession* GetPlaySession(int session_index);

  rparser::Song* get_song();
  rparser::Chart* get_chart(const std::string& chartname);
  void set_load_bga(bool use_bga);
  bool is_course() const;
  size_t get_course_index() const;

  /**
   * @warn
   * below functions return state of SongPlayer.
   * Update() must be called before execution to get proper result.
   */

  SongPlayerState get_state() const;
  bool is_loading() const;
  bool is_loaded() const;
  bool is_play_finished() const;
  double get_load_progress() const;
  double get_play_progress() const;

  void PlaySound(int channel, unsigned session);
  void StopSound(int channel, unsigned session);
  void SetImage(int channel, unsigned session, unsigned layer);
  Image* GetImage(unsigned session, unsigned layer);

  /* settings */
  
  void set_play_after_loading(bool v);

  static SongPlayer& getInstance();

private:
  rparser::Song *song_;

  /* currently playing sessions. */
  PlaySession *sessions_[kMaxPlaySession];

  /* BGM resource filenames for each sessions / channels */
  std::string bgm_fn_[kMaxPlaySession][kMaxChannelCount];

  /* BGA resource filenames for each sessions / channels */
  std::string bga_fn_[kMaxPlaySession][kMaxChannelCount];

  /* BGM resource for each session / channels */
  Sound* bgm_[kMaxPlaySession][kMaxChannelCount];

  /* BGA resource for each session / channels */
  Image* bga_[kMaxPlaySession][kMaxChannelCount];

  /* currently selected BGA index for each player / layer
   * @warn negative value means : no BGA selected */
  int bga_selected_[kMaxPlaySession][64];

  /* list of songs to play (multiple in case of courseplay) */
  std::vector<SongPlayinfo> playlist_;

  /* currently playing song index */
  size_t playlist_index_;

  /* image objects to load */
  std::vector<Image*> image_arr_;

  /* sound objects to load */
  std::vector<Sound*> sound_arr_;

  /* current state of SongPlayer */
  SongPlayerState state_;

  /* loaded object count */
  std::atomic<unsigned> load_count_;
  unsigned total_count_;
  SongResourceLoadCallback load_callback_;

  /* load progress */
  double load_progress_;

  /* play progress */
  double play_progress_;

  /* load bga file */
  bool load_bga_;

  /* play after loading (internal flag) */
  bool play_after_loading_;

  /* behaviour after playing.
   * 0: do nothing
   * 1: play next song (if none, then stop)
   * 2: Stop()
   */
  int action_after_playing_;

  void LoadResourceFromChart(rparser::Chart &c, unsigned session);
  bool IsChartPath(const std::string &path);
};

}