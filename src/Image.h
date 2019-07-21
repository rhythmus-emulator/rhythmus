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
  void UnloadAll();
  void UnloadTexture();
  void UnloadBitmap();
  GLuint get_texture_ID() const;
  uint16_t get_width() const;
  uint16_t get_height() const;

private:
  void* bitmap_ctx_;
  uint8_t *data_ptr_;
  uint16_t width_, height_;
  GLuint textureID_;
};

using ImageAuto = std::shared_ptr<Image>;

}