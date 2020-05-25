#include "ResourceManager.h"
#include "Image.h"
#include "Timer.h"
#include "Setting.h"
#include "Util.h"
#include "common.h"
#include <FreeImage.h>

/* ffmpeg */
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#define FFMPEG_REGISTER_CODEC 0
#define FFMPEG_USE_DEPRECIATED 0
#define FFMPEG_USE_DECODE_DEPRECIATED 1

/* rutil for some string functions */
#include "rparser.h"

namespace rhythmus
{

/* ffmpeg context declaration */
class FFmpegContext
{
public:
  FFmpegContext();
  ~FFmpegContext();

  bool Open(const std::string& path);
  int DecodePacket(float target_time);
  int ReadPacket();
  void Unload();
  void Rewind();

  int get_width() { return width_; }
  int get_height() { return height_; }
  bool is_eof() { return is_eof_ && is_eof_packet_; }
  bool is_image() { return is_image_; }
  double get_duration() { return duration_; }
  AVFrame* get_frame() { return frame; }

  int get_error_code() const { return error_code_; }
  const char *get_error_msg() const { return error_msg_.c_str(); }

private:
  AVCodec* codec;
  AVCodecContext* context;
  AVFormatContext* formatctx;
  AVStream *stream;
  AVPacket *packet;
  AVFrame *frame;
  int video_stream_idx;

  // microsecond duration
  double duration_;

  // current video frame time
  double time_;

  // timestamp offset for current movie
  int time_offset_;

  // current packet offset
  // if -1, packet is not ready.
  int packet_offset;

  int frame_no_;

  // EOF of packet
  bool is_eof_packet_;

  // EOF of video stream
  bool is_eof_;

  // is file image actually, not video.
  bool is_image_;

  // video width / height
  int width_, height_;

  int error_code_;
  std::string error_msg_;
};

FFmpegContext::FFmpegContext()
  : codec(0), context(0), formatctx(0), stream(0), packet(0), frame(0), video_stream_idx(0),
    duration_(0), time_(0), time_offset_(0), packet_offset(-1), frame_no_(0),
    is_eof_(false), is_eof_packet_(false), is_image_(false),
    width_(0), height_(0)
{
#if FFMPEG_REGISTER_CODEC
  /* ffmpeg initialization */
  static bool is_ffmpeg_initialized_ = false;
  if (!is_ffmpeg_initialized_)
  {
    av_register_all();
    avcodec_register_all();
    is_ffmpeg_initialized_ = true;
  }
#endif
}

FFmpegContext::~FFmpegContext()
{
  Unload();
}

void FFmpegContext::Unload()
{
  if (frame)
  {
    av_frame_free(&frame);
    frame = 0;
  }

  if (packet)
  {
    av_packet_unref(packet);
    av_packet_free(&packet);
    packet = 0;
  }

  if (context)
  {
    avcodec_close(context);
    context = 0;
  }

  if (formatctx)
  {
    avformat_close_input(&formatctx);
    formatctx = 0;
  }

  stream = 0;
}

bool FFmpegContext::Open(const std::string& path)
{
  // should not open it again ...
  R_ASSERT(formatctx == 0);
  int ret = 0;
  error_msg_.clear();
  error_code_ = 0;

  if ((ret = avformat_open_input(&formatctx, path.c_str(), 0, 0)) != 0)
  {
    error_msg_ = "Movie file open failed.";
    error_code_ = ret;
    return false;
  }

  ret = avformat_find_stream_info(formatctx, NULL);
  if (ret < 0)
  {
    error_msg_ = "Movie stream search failed.";
    error_code_ = ret;
    Unload();
    return false;
  }

  video_stream_idx = av_find_best_stream(formatctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

  stream = formatctx->streams[video_stream_idx];
  if (stream->codecpar)
    codec = avcodec_find_decoder(stream->codecpar->codec_id);
  else
  {
    std::string ext = GetExtension(path);
    codec = avcodec_find_decoder_by_name(ext.c_str());
  }

  if (!codec)
  {
    error_msg_ = "Movie stream codec failed.";
    error_code_ = -1;
    Unload();
    return false;
  }

  // mark for image
  switch (formatctx->streams[video_stream_idx]->codecpar->codec_id)
  {
  case AV_CODEC_ID_BMP:
  case AV_CODEC_ID_JPEGLS:
  case AV_CODEC_ID_JPEG2000:
  case AV_CODEC_ID_GIF:
  case AV_CODEC_ID_PNG:
    is_image_ = true;
  }

  context = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(context, stream->codecpar);
  int codec_open_code = avcodec_open2(context, codec, NULL);
  if (codec_open_code < 0)
  {
    error_msg_ = "Movie codec open failed - code: " + std::to_string(codec_open_code);
    error_code_ = -1;
    Unload();
    return false;
  }

  // fetch width / height / duration of video
#if FFMPEG_USE_DEPRECIATED
  AVCodecContext* pCodecCtx = stream->codec;
  width_ = pCodecCtx->width;
  height_ = pCodecCtx->height;
#else
  width_ = context->width;
  height_ = context->height;
#endif
  duration_ = formatctx->duration / 1000.0;
  packet_offset = -1;
  is_eof_ = false;
  time_ = 0;
  frame = av_frame_alloc();
  packet = av_packet_alloc();
  frame_no_ = 0;


  return true;
}

/* may multiple frame reside in same packet. */
int FFmpegContext::DecodePacket(float target_time)
{
  /* No packet. Read packet and attempt again. */
  if (!is_eof_ && packet_offset == -1)
    return 0;

  /* zero packet size might exist at first, skip it. */
  if (frame_no_ == 0 && packet->size == 0)
    return 0;

  /* too early time - already decoded, don't need to decode, and don't retry */
  if (target_time < time_ - time_offset_)
    return 2;

  double pts, last_frame_duration;
  bool skip_this_frame = true;
  int ret;
#if FFMPEG_USE_DECODE_DEPRECIATED
  int decode_len, got_frame;
  while (packet_offset < packet->size)
  {
    skip_this_frame = true;

    /* Give nullptr data to finish loop (to set is_eof_packet)
     * in case of last frame */
    packet->data = packet->size ? packet->data : nullptr;
    decode_len = avcodec_decode_video2(context, frame, &got_frame, packet);
    if (decode_len < 0)
    {
      /* XXX: unknown. decoding failed. */
      return 0;
    }
    packet_offset += decode_len;

    // if no frame, packet is end, maybe.
    // XXX: should retry ?
    if (!got_frame)
    {
      is_eof_packet_ = true;
      return 0;
    }

    // calculate time
    if (frame->pkt_dts != AV_NOPTS_VALUE)
      pts = (double)frame->best_effort_timestamp;
    else
      pts = 0;
    if (pts != 0)
    {
      /* XXX: Not use CodecContext time base! (It means FPS) */
      time_ = (pts * av_q2d(stream->time_base)) * 1000;
    }
    else
    {
      last_frame_duration = av_q2d(stream->time_base);
      last_frame_duration += frame->repeat_pict * (last_frame_duration * 0.5f);
      time_ += last_frame_duration * 1000;
    }

    /* I don't know this code is necessary... */
#if 0
    // some video's first timestamp is not zero. if so, shift timestamp offset.
    if (frame_no_ == 0)
    {
      time_offset_ = time_;
    }
#endif

    frame_no_++;

    // now, should we need to skip this frame?
    if (target_time <= time_)
    {
      skip_this_frame = false;
      break;
    }
  }
#else
  ret = avcodec_send_packet(context, packet);
  if (ret < 0)
  {
    is_eof_packet_ = true;
    return 0;
  }
  while ((ret = avcodec_receive_frame(context, frame)) >= 0)
  {
    // calculate time
    if (frame->pkt_dts != AV_NOPTS_VALUE)
      pts = (double)frame->best_effort_timestamp;
    else
      pts = 0;
    if (pts != 0)
    {
      /* XXX: Not use CodecContext time base! (It means FPS) */
      time_ = (pts * av_q2d(stream->time_base)) * 1000;
    }
    else
    {
      last_frame_duration = av_q2d(stream->time_base);
      last_frame_duration += frame->repeat_pict * (last_frame_duration * 0.5f);
      time_ += last_frame_duration * 1000;
    }

    frame_no_++;

    // now, should we need to skip this frame?
    if (target_time <= time_)
    {
      // XXX: exiting here will result in bad-quality frame.
      // how to get 'complete' frame?
      skip_this_frame = false;
      break;
    }
  }
  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
  {
    // try again with new packet.
    is_eof_packet_ = true;
    return 0;
  }
#endif

  /* return: skip this frame(0) or decode this frame(1) */
  return skip_this_frame ? 0 : 1;
}

int FFmpegContext::ReadPacket()
{
  // EOF
  if (is_eof_)
    return 0;

  // clear previous packet if necessary.
  av_packet_unref(packet);
  packet_offset = -1;

  int ret;
  while ((ret = av_read_frame(formatctx, packet)) >= 0)
  {
    if (packet->stream_index == video_stream_idx)
    {
      // found good stream. exit searching.
      is_eof_packet_ = false;
      packet_offset = 0;
      break;
    }

    // other stream is not for our video stream
    // clear it.
    av_packet_unref(packet);
  }

  // EOF
  if (ret < 0)
  {
    is_eof_ = true;
    //packet->size = 0;
    return 1; /* anyway, we read packet. */
  }

  /* Packet reading done. */
  return 1;
}

void FFmpegContext::Rewind()
{
  //av_seek_frame(formatctx, -1, 0, 0);
  av_seek_frame(formatctx, -1, 0, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);
  avcodec_flush_buffers(context);

  packet_offset = -1;
  is_eof_ = false;
  is_eof_packet_ = false;
  time_ = 0;
  frame_no_ = 0;
}

/* check whether open file as movie or image */
bool IsMovieFile(const std::string& path)
{
  std::string ext = rutil::upper(rutil::GetExtension(path));
  return ext == "MPEG" || ext == "AVI" || ext == "MP4" || ext == "MPG";
}



// -------------------------------- class Image

Image::Image()
  : bitmap_ctx_(0), data_ptr_(nullptr), width_(0), height_(0),
    textureID_(0), ffmpeg_ctx_(0), video_time_(.0f), loop_movie_(true),
    is_invalid_(true)
{
}

Image::~Image()
{
  Unload();
}

std::string Image::get_path() const
{
  return path_;
}

void Image::LoadImageFromPath(const std::string& path)
{
  FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(path.c_str());
  if (fmt == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
    return;

  FIBITMAP *bitmap, *temp;
  temp = FreeImage_Load(fmt, path.c_str());
  // I don't know why, but image need to be flipped
  FreeImage_FlipVertical(temp);
  bitmap = FreeImage_ConvertTo32Bits(temp);
  FreeImage_Unload(temp);

  width_ = FreeImage_GetWidth(bitmap);
  height_ = FreeImage_GetHeight(bitmap);
  data_ptr_ = FreeImage_GetBits(bitmap);
  bitmap_ctx_ = (void*)bitmap;
  path_ = path;
}

/* Return boolean value: whether is image file */
bool Image::LoadImageFromMemory(const char* p, size_t len)
{
  FIMEMORY *memstream = FreeImage_OpenMemory((uint8_t*)p, len);
  FREE_IMAGE_FORMAT fmt = FreeImage_GetFileTypeFromMemory(memstream);
  if (fmt == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
  {
    FreeImage_CloseMemory(memstream);
    return false;
  }

  FIBITMAP *bitmap, *temp;
  temp = FreeImage_LoadFromMemory(fmt, memstream);
  // I don't know why, but image need to be flipped
  FreeImage_FlipVertical(temp);
  bitmap = FreeImage_ConvertTo32Bits(temp);
  FreeImage_Unload(temp);

  width_ = FreeImage_GetWidth(bitmap);
  height_ = FreeImage_GetHeight(bitmap);
  data_ptr_ = FreeImage_GetBits(bitmap);
  bitmap_ctx_ = (void*)bitmap;
  path_ = "(memory)";

  FreeImage_CloseMemory(memstream);
  return true;
}

void Image::LoadMovieFromPath(const std::string& path)
{
  if (ffmpeg_ctx_)
  {
    FFmpegContext *f = (FFmpegContext*)ffmpeg_ctx_;
    delete f;
  }

  FFmpegContext *ffmpeg_ctx = new FFmpegContext();
  if (!ffmpeg_ctx->Open(path))
  {
    error_msg_buf_ = ffmpeg_ctx->get_error_msg();
    error_msg_ = error_msg_buf_.c_str();
    error_code_ = -1;

    delete ffmpeg_ctx;
    ffmpeg_ctx = 0;
    return;
  }
  ffmpeg_ctx_ = ffmpeg_ctx;

  width_ = ffmpeg_ctx->get_width();
  height_ = ffmpeg_ctx->get_height();
  path_ = path;

  // create empty bitmap
  data_ptr_ = (uint8_t*)malloc(width_ * height_ * 4);
  for (int i = 0; i < width_ * height_ * 4; i += 4)
  {
    // all black bitmap with no transparency (alpha 0xff)
    *(uint32_t*)(data_ptr_ + i) = 0xff000000;
  }
}

void Image::LoadMovieFromMemory(const char* p, size_t len)
{
  // TODO
  R_ASSERT(0);
}

void Image::Load(const std::string& path)
{
  Unload();
  error_code_ = 0;
  error_msg_ = 0;

  if (IsMovieFile(path))
    LoadMovieFromPath(path);
  else
    LoadImageFromPath(path);
}

void Image::Load(const char* p, size_t len, const char *ext_hint_opt)
{
  Unload();
  error_code_ = 0;
  error_msg_ = 0;

  if (ext_hint_opt)
  {
    bool is_image = false;
    static const char *ext_img_hints[] = {
      "jpg", "png", "gif", "jpeg", "bmp", "tga", 0
    };
    const char *ext = ext_hint_opt;
    for (const char *p = ext; *p; ++p)
      if (*p == '.') ext = p + 1;
    for (const char **ext_img = ext_img_hints; !is_image && *ext_img; ++ext_img)
      if (stricmp(*ext_img, ext) == 0)
        is_image = true;
    if (!is_image)
      LoadMovieFromMemory(p, len);
    else
      LoadImageFromMemory(p, len);
  }
  else
  {
    if (!LoadImageFromMemory(p, len))
      LoadMovieFromMemory(p, len);
  }
}

void Image::Load(const MetricGroup &m)
{
  Load(m.get_str("path"));
}

void Image::Commit()
{
  if (!data_ptr_)
  {
    error_msg_ = "Cannot commit image as it's not loaded.";
    error_code_ = -2;
    return;
  }

  textureID_ = GRAPHIC->CreateTexture(data_ptr_, width_, height_);

  if (textureID_ == 0)
  {
    error_msg_ = "Allocating textureID failed";
    error_code_ = -2;
    return;
  }

  /* if movie type -- won't remove bitmap (need to update continously) */
  if (ffmpeg_ctx_ == 0)
  {
    UnloadBitmap();
  }
}

void Image::Update(double delta)
{
  /* upload texture first. */
  if (data_ptr_ && is_invalid_)
  {
    Commit();
    is_invalid_ = false;
  }

  /* update movie */
  if (ffmpeg_ctx_)
  {
    FFmpegContext *fctx = (FFmpegContext*)ffmpeg_ctx_;

    // If EOF and not image, restart (if necessary)
    if (loop_movie_ && !fctx->is_image() && fctx->is_eof())
    {
      fctx->Rewind();
      video_time_ = 0;
    }
    video_time_ += delta;

    // Decode first, Read later.
    // If both failed, video stream is completely end. exit loop.
    int ret = 0; /* ret of decoding */
    while (!fctx->is_eof())
    {
      // DecodePacket == 0 --> EOF or decoding failure.
      // We read next packet in this case.
      // If successfully decode packet, exit loop.
      if (ret = fctx->DecodePacket((float)video_time_))
        break;

      // ReadPacket() might fail, But we can retry.
      // If success, retry decoding. otherwise exit loop.
      if (fctx->ReadPacket() == 0) break;
    }

    // Convert decoded image into image (if necessary)
    if (ret == 1)
    {
      AVFrame *frame = fctx->get_frame();
      AVFrame *frame_conv = av_frame_alloc();
      R_ASSERT(frame->pict_type != AV_PICTURE_TYPE_NONE);
#if FFMPEG_USE_DEPRECIATED
      avpicture_fill((AVPicture*)frame_conv, data_ptr_, AV_PIX_FMT_RGBA, width_, height_);
#else
      av_image_fill_arrays(frame_conv->data, frame_conv->linesize, data_ptr_,
        AV_PIX_FMT_RGBA, width_, height_, 1);
#endif

      // convert frame to RGB24
      SwsContext* mod_ctx;
      mod_ctx = sws_getContext(
        frame->width, frame->height, (AVPixelFormat)frame->format,
        width_, height_, AV_PIX_FMT_BGRA,
        SWS_BICUBIC, 0, 0, 0);
      sws_scale(mod_ctx, frame->data, frame->linesize, 0, frame->height,
        frame_conv->data, frame_conv->linesize);
      sws_freeContext(mod_ctx);

      // XXX: need to flip but I don't know why. should check about this problem

      // Upload texture
      GRAPHIC->UpdateTexture(textureID_, data_ptr_, 0, 0, width_, height_);

      // Free mem
      av_frame_free(&frame_conv);
    }
  }
}

void Image::Unload()
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
    FFmpegContext *f = (FFmpegContext*)ffmpeg_ctx_;
    delete f;
    ffmpeg_ctx_ = 0;
  }
}

void Image::UnloadTexture()
{
  if (textureID_)
  {
    GRAPHIC->DeleteTexture(textureID_);
    textureID_ = 0;
  }
}

/* @brief Is texture loaded into graphic memory? */
bool Image::is_loaded() const
{
  return textureID_ > 0;
}

void Image::RestartMovie()
{
  if (ffmpeg_ctx_)
    static_cast<FFmpegContext*>(ffmpeg_ctx_)->Rewind();
}

void Image::Invalidate()
{
  is_invalid_ = true;
}

unsigned Image::get_texture_ID() const
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

const uint8_t *Image::get_ptr() const
{
  return data_ptr_;
}

void Image::SetLoopMovie(bool loop)
{
  loop_movie_ = loop;
}

}
