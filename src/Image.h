#pragma once
#include "Graphic.h"
#include <string>

namespace rhythmus
{

class Image
{
public:
  Image();
  ~Image();
  void set_name(const std::string& name);
  std::string get_name() const;
  std::string get_path() const;
  void LoadFromPath(const std::string& path);
  void LoadFromData(uint8_t* p, size_t len);
  void CommitImage(bool delete_data = true);
  void Update(float delta_ms);
  void UnloadAll();
  void UnloadTexture();
  void UnloadBitmap();
  void UnloadMovie();
  bool is_loaded() const;
  GLuint get_texture_ID() const;
  uint16_t get_width() const;
  uint16_t get_height() const;
  void SetLoopMovie(bool loop = true);
  void RestartMovie();

private:
  std::string name_;
  std::string path_;

  void* bitmap_ctx_;
  uint8_t *data_ptr_;
  uint16_t width_, height_;
  GLuint textureID_;

  /*
   * Context used for ffmpeg playing.
   * Must call Update() method to update movie.
   */
  void *ffmpeg_ctx_;

  /* video target time */
  float video_time_;

  /* loop in case of movie */
  bool loop_movie_;

  void LoadImageFromPath(const std::string& path);
  void LoadMovieFromPath(const std::string& path);
  bool LoadImageFromMemory(uint8_t* p, size_t len);
  void LoadMovieFromMemory(uint8_t* p, size_t len);
};

using ImageAuto = std::shared_ptr<Image>;

}