#include "ResourceManager.h"
#include "Image.h"
#include <FreeImage.h>
#include <iostream>

namespace rhythmus
{

Image::Image()
  : bitmap_ctx_(0), data_ptr_(nullptr), width_(0), height_(0),
  textureID(0)
{
}

Image::~Image()
{
  Unload();
}

void Image::LoadFromPath(const std::string& path)
{
  Unload();

  FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(path.c_str());
  FIBITMAP *bitmap, *temp;
  temp = FreeImage_Load(fmt, path.c_str());
  bitmap = FreeImage_ConvertTo32Bits(temp);
  FreeImage_Unload(temp);

  width_ = FreeImage_GetWidth(bitmap);
  height_ = FreeImage_GetHeight(bitmap);
  data_ptr_ = FreeImage_GetBits(bitmap);
  bitmap_ctx_ = (void*)bitmap;
}

void Image::LoadFromData(uint8_t* p, size_t len)
{
  Unload();

  FIMEMORY *memstream = FreeImage_OpenMemory(p, len);
  FREE_IMAGE_FORMAT fmt = FreeImage_GetFileTypeFromMemory(memstream);
  FIBITMAP *bitmap, *temp;
  temp = FreeImage_LoadFromMemory(fmt, memstream);
  bitmap = FreeImage_ConvertTo32Bits(temp);
  FreeImage_Unload(temp);

  width_ = FreeImage_GetWidth(bitmap);
  height_ = FreeImage_GetHeight(bitmap);
  data_ptr_ = FreeImage_GetBits(bitmap);
  bitmap_ctx_ = (void*)bitmap;
}

void Image::CommitImage(bool delete_data)
{
  if (!bitmap_ctx_)
  {
    std::cerr << "Cannot commit image as it's not loaded." << std::endl;
    return;
  }

  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)data_ptr_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  if (delete_data)
  {
    Unload();
  }
}

void Image::Unload()
{
  if (bitmap_ctx_)
  {
    free(bitmap_ctx_);
    bitmap_ctx_ = 0;
  }
  if (textureID)
  {
    glDeleteTextures(1, &textureID);
    textureID = 0;
  }
}

GLuint Image::GetTextureID()
{
  return textureID;
}

}