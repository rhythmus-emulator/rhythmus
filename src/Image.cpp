#include "ResourceManager.h"
#include "Image.h"
#include <FreeImage.h>
#include <iostream>

/* ffmpeg */
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

/* rutil for some string functions */
#include "rutil.h"

namespace rhythmus
{

/* ffmpeg context declaration */
struct FFmpegContext
{
  AVCodec* codec;
  AVCodecContext* context;
  AVFormatContext* formatctx;
};

/* check whether open file as movie or image */
bool IsFileMovie(const std::string& path)
{
  std::string ext = rutil::upper(rutil::GetExtension(path));
  return ext == "MPEG" || ext == "AVI" || ext == "MP4" || ext == "MPG";
}

// -------------------------------- class Image

Image::Image()
  : bitmap_ctx_(0), data_ptr_(nullptr), width_(0), height_(0),
    textureID_(0), ffmpeg_ctx_(0)
{
  /* ffmpeg initialization */
  static bool is_ffmpeg_initialized_ = false;
  if (!is_ffmpeg_initialized_)
  {
    av_register_all();
    is_ffmpeg_initialized_ = true;
  }
}

Image::~Image()
{
  UnloadAll();
}

void Image::LoadFromPath(const std::string& path)
{
  UnloadAll();

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
  UnloadAll();

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

  glGenTextures(1, &textureID_);
  if (textureID_ == 0)
  {
    GLenum err = glGetError();
    std::cerr << "Allocating textureID failed: " << (int)err << std::endl;
    return;
  }
  glBindTexture(GL_TEXTURE_2D, textureID_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)data_ptr_);

  /* do not render outside texture, clamp it. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  /* prevent font mumbling when minimized, prevent cracking when magnified. */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (delete_data)
  {
    UnloadBitmap();
  }
}

void Image::UnloadAll()
{
  UnloadBitmap();
  UnloadTexture();
}

void Image::UnloadBitmap()
{
  if (bitmap_ctx_)
  {
    FreeImage_Unload((FIBITMAP*)bitmap_ctx_);
    bitmap_ctx_ = 0;
  }
}

void Image::UnloadTexture()
{
  if (textureID_)
  {
    glDeleteTextures(1, &textureID_);
    textureID_ = 0;
  }
}

GLuint Image::get_texture_ID() const
{
  return textureID_;
}

uint16_t Image::get_width() const
{
  return width_;
}

uint16_t Image::get_height() const
{
  return height_;
}

}