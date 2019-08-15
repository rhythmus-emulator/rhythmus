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
  void LoadFromPath(const std::string& path);
  void LoadFromData(uint8_t* p, size_t len);
  void CommitImage(bool delete_data = true);
  void Update();
  void UnloadAll();
  void UnloadTexture();
  void UnloadBitmap();
  void UnloadMovie();
  GLuint get_texture_ID() const;
  uint16_t get_width() const;
  uint16_t get_height() const;
  void SetLoopMovie(bool loop = true);
  void RestartMovie();

private:
  void* bitmap_ctx_;
  uint8_t *data_ptr_;
  uint16_t width_, height_;
  GLuint textureID_;

  /* movie timer related */
  int movie_start_time;

  /*
   * Context used for ffmpeg playing.
   * Must call Update() method to update movie.
   */
  void *ffmpeg_ctx_;

  /* loop in case of movie */
  bool loop_movie_;

  void LoadImageFromPath(const std::string& path);
  void LoadMovieFromPath(const std::string& path);
  bool LoadImageFromMemory(uint8_t* p, size_t len);
  void LoadMovieFromMemory(uint8_t* p, size_t len);
};

using ImageAuto = std::shared_ptr<Image>;

}