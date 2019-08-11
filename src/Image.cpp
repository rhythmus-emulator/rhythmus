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
  int video_stream_idx;

  // microsecond duration
  uint32_t duration;

  // video width / height
  int width, height;
};

/* check whether open file as movie or image */
bool IsMovieFile(const std::string& path)
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

void Image::LoadImageFromPath(const std::string& path)
{
  FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(path.c_str());
  if (fmt == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
    return;

  FIBITMAP *bitmap, *temp;
  temp = FreeImage_Load(fmt, path.c_str());
  bitmap = FreeImage_ConvertTo32Bits(temp);
  FreeImage_Unload(temp);

  width_ = FreeImage_GetWidth(bitmap);
  height_ = FreeImage_GetHeight(bitmap);
  data_ptr_ = FreeImage_GetBits(bitmap);
  bitmap_ctx_ = (void*)bitmap;
}

/* Return boolean value: whether is image file */
bool Image::LoadImageFromMemory(uint8_t* p, size_t len)
{
  FIMEMORY *memstream = FreeImage_OpenMemory(p, len);
  FREE_IMAGE_FORMAT fmt = FreeImage_GetFileTypeFromMemory(memstream);
  if (fmt == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
  {
    FreeImage_CloseMemory(memstream);
    return false;
  }

  FIBITMAP *bitmap, *temp;
  temp = FreeImage_LoadFromMemory(fmt, memstream);
  bitmap = FreeImage_ConvertTo32Bits(temp);
  FreeImage_Unload(temp);

  width_ = FreeImage_GetWidth(bitmap);
  height_ = FreeImage_GetHeight(bitmap);
  data_ptr_ = FreeImage_GetBits(bitmap);
  bitmap_ctx_ = (void*)bitmap;

  FreeImage_CloseMemory(memstream);
  return true;
}

void Image::LoadMovieFromPath(const std::string& path)
{
  FFmpegContext *ffmpeg_ctx = (FFmpegContext*)malloc(sizeof(FFmpegContext));
  memset(ffmpeg_ctx, 0, sizeof(FFmpegContext));
  
  if (avformat_open_input(&ffmpeg_ctx->formatctx, path.c_str(), 0, 0) != 0)
  {
    std::cerr << "Movie file open failed: " << path << std::endl;
    free(ffmpeg_ctx);
    return;
  }

  ffmpeg_ctx_ = ffmpeg_ctx;

  int sinfo_ret = avformat_find_stream_info(ffmpeg_ctx->formatctx, NULL);
  if (sinfo_ret < 0)
  {
    std::cerr << "Movie stream search failed (code " << sinfo_ret << "): " << path << std::endl;
    UnloadMovie();
    return;
  }

  int video_stream_idx = av_find_best_stream(
    ffmpeg_ctx->formatctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  ffmpeg_ctx->video_stream_idx = video_stream_idx;

  ffmpeg_ctx->codec = avcodec_find_decoder(
    ffmpeg_ctx->formatctx->streams[video_stream_idx]->codecpar->codec_id);
  if (!ffmpeg_ctx->codec)
  {
    std::cerr << "Movie stream codec failed: " << path << std::endl;
    UnloadMovie();
    return;
  }

  ffmpeg_ctx->context = avcodec_alloc_context3(ffmpeg_ctx->codec);
  int codec_open_code = avcodec_open2(ffmpeg_ctx->context, ffmpeg_ctx->codec, NULL);
  if (codec_open_code < 0)
  {
    std::cerr << "Movie codec open failed - code: " << codec_open_code << ". " << path << std::endl;
    UnloadMovie();
    return;
  }

  // fetch width / height / duration of video
  AVCodecContext* codecctx = ffmpeg_ctx->formatctx->streams[video_stream_idx]->codec;
  ffmpeg_ctx->duration = ffmpeg_ctx->formatctx->duration / 1000;
  width_ = ffmpeg_ctx->width = codecctx->width;
  height_ = ffmpeg_ctx->height = codecctx->height;

  // create empty bitmap
  data_ptr_ = (uint8_t*)malloc(width_ * height_ * 4);
  for (int i = 0; i < width_ * height_ * 4; i += 4)
  {
    // all black bitmap with no transparency (alpha 0xff)
    *(uint32_t*)(data_ptr_ + i) = 0xff000000;
  }
}

void Image::LoadMovieFromMemory(uint8_t* p, size_t len)
{
  // TODO
}

void Image::LoadFromPath(const std::string& path)
{
  UnloadAll();

  if (IsMovieFile(path))
    LoadMovieFromPath(path);
  else
    LoadImageFromPath(path);
}

void Image::LoadFromData(uint8_t* p, size_t len)
{
  UnloadAll();

  if (!LoadImageFromMemory(p, len))
    LoadMovieFromMemory(p, len);
}

void Image::CommitImage(bool delete_data)
{
  if (!data_ptr_)
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

void Image::Update()
{
  if (ffmpeg_ctx_)
  {
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    FFmpegContext *fctx = (FFmpegContext*)ffmpeg_ctx_;

    while (av_read_frame(fctx->formatctx, &packet) == 0)
    {
      // only decode video packet here ...
      if (packet.stream_index != fctx->video_stream_idx)
        continue;

      // TODO
    }

    av_frame_free(&frame);
    av_packet_unref(&packet);
  }
}

void Image::UnloadAll()
{
  UnloadMovie();
  UnloadBitmap();
  UnloadTexture();
}

void Image::UnloadBitmap()
{
  if (bitmap_ctx_)
  {
    FreeImage_Unload((FIBITMAP*)bitmap_ctx_);
    bitmap_ctx_ = 0;
    data_ptr_ = 0;
  }

  /* only called when data is create by ffmpeg (when not using freeimage) */
  if (data_ptr_)
  {
    free(data_ptr_);
    data_ptr_ = 0;
  }
}

void Image::UnloadMovie()
{
  if (ffmpeg_ctx_)
  {
    FFmpegContext *fctx = (FFmpegContext*)ffmpeg_ctx_;
    if (fctx->formatctx)
      avformat_close_input(&fctx->formatctx);

    free(ffmpeg_ctx_);
    ffmpeg_ctx_ = 0;
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