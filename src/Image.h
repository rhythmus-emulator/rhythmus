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

class MetricGroup;

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
  void Load(const MetricGroup &m);
  void Update(double delta_ms);
  void Unload();
  bool is_loaded() const;
  unsigned get_texture_ID() const;
  const Texture* get_texture() const;
  uint16_t get_width() const;
  uint16_t get_height() const;
  const uint8_t *get_ptr() const;
  void SetLoopMovie(bool loop = true);
  void RestartMovie();
  void Invalidate();

private:
  std::string path_;

  void* bitmap_ctx_;
  uint8_t *data_ptr_;
  uint16_t width_, height_;
  Texture tex_;
  std::string error_msg_buf_;

  /*
   * Context used for ffmpeg playing.
   * Must call Update() method to update movie.
   */
  void *ffmpeg_ctx_;

  /* video target time */
  double video_time_;

  /* loop in case of movie */
  bool loop_movie_;

  /* is invalid? (should be committed?) */
  bool is_invalid_;

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
