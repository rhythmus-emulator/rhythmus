#pragma once
#include "Graphic.h"
#include "ResourceManager.h"
#include <memory>
#include <string>

namespace rhythmus
{

enum ImageErrorCode
{
  kImageNoError,
  kImageNoFile,
  kImageFileReadFailed,
  kImageTextureUploadFailed,
  kImageMovieFailed,
};

/**
 * @brief Contains loaded bitmap image / texture.
 * @warn After Commit(), image data will be deleted to save memory space.
 * @warn All bitmap is stored in 32bit.
 */
class Image : public ResourceElement
{
public:
  Image();
  virtual ~Image();
  std::string get_path() const;
  void Load(const std::string& path);
  void Load(const char* p, size_t len, const char *ext_hint_opt);
  void Update(float delta_ms);
  void Unload();
  bool is_loaded() const;
  int get_error_code() const;
  const char* get_error_msg() const;
  unsigned get_texture_ID() const;
  uint16_t get_width() const;
  uint16_t get_height() const;
  const uint8_t *Image::get_ptr() const;
  void SetLoopMovie(bool loop = true);
  void RestartMovie();

private:
  std::string path_;

  void* bitmap_ctx_;
  uint8_t *data_ptr_;
  uint16_t width_, height_;
  unsigned textureID_;
  int error_code_;
  const char *error_msg_;
  std::string error_msg_buf_;

  /*
   * Context used for ffmpeg playing.
   * Must call Update() method to update movie.
   */
  void *ffmpeg_ctx_;

  /* video target time */
  float video_time_;

  /* loop in case of movie */
  bool loop_movie_;

  void Commit();
  void UnloadTexture();
  void UnloadBitmap();
  void UnloadMovie();
  void LoadImageFromPath(const std::string& path);
  void LoadMovieFromPath(const std::string& path);
  bool LoadImageFromMemory(const char* p, size_t len);
  void LoadMovieFromMemory(const char* p, size_t len);
};

using ImageAuto = std::shared_ptr<Image>;

}