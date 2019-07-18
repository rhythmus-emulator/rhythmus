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
  void Unload();
  GLuint GetTextureID();

private:
  void* bitmap_ctx_;
  uint8_t *data_ptr_;
  uint16_t width_, height_;
  uint16_t sx_, sy_, sw_, sh_;
  uint16_t dx_, dy_, dw_, dh_;
  GLuint textureID;
};

using ImageAuto = std::shared_ptr<Image>;

}