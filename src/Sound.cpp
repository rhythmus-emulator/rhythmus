#include "Sound.h"
#include "Setting.h"
#include "Game.h"
#include "Error.h"
#include "SceneManager.h" /* GameSound::Load() */
#include "ResourceManager.h"
#include "Util.h"
#include "Logger.h"
#include "common.h"

/* OpenAL */
#include <AL/al.h>
#include <AL/alc.h>

/* Playback thread */
#include <thread>
#include <mutex>
#include <chrono>

namespace rhythmus
{

constexpr int kMixerBitsize = 16;
constexpr int kMixerSampleRate = 44100;
constexpr int kMixerChannel = 2;

// Maximum limited source count
constexpr size_t kMaxSourceCount = 256;

// Buffer count for each source (generally 2)
constexpr size_t kBufferCount = 2;

// Default buffer size, should be small to reduce sound delay.
// e.g.
// 16bit(2Byte) * 2ch = 4Byte for frame
// 1024 / 4 = 256 frame,
// nearly 256/44100 ~= 6msec delay
constexpr size_t kDefaultBufferSize = 1024;

// Sleeping time for sound body thread.
constexpr long long kSleepMilisecond = 100;

ALCdevice *device;
ALCcontext *context;
ALuint source_id[kMaxSourceCount];
ALuint buffer_id[kMaxSourceCount * kBufferCount];
std::thread sound_thread;

void SoundDriver::sound_thread_body()
{
  char* pData = (char*)calloc(1, buffer_size_);
  int iBuffersProcessed = 0;
  const ALsizei iSingleBufferSize = (ALsizei)(buffer_size_ / kBufferCount);
  const size_t uFrameSize = iSingleBufferSize / (sinfo_.channels * sinfo_.bitsize / 8);
  ALuint cur_source_id, cur_buffer_id;
  ALint state, buffers_queued;
  int err;

  // -- stream initialization --
  // set property of channel.
#if 0
  alSourcef(source_id, AL_PITCH, 1);
  alSourcef(source_id, AL_GAIN, 1);
  alSource3f(source_id, AL_POSITION, 0, 0, 0);
  alSourcei(source_id, AL_LOOPING, AL_FALSE);
#endif

  // fill empty data into buffer and start playing.
  for (size_t i = 0; i < source_count_; ++i)
  {
    for (size_t j = 0; j < kBufferCount; ++j)
    {
      alBufferData(buffer_id[i * kBufferCount + j], audio_format_, pData,
        iSingleBufferSize, sinfo_.rate);
      if ((err = (int)alGetError()) != AL_NO_ERROR)
      {
        Logger::Error("Error occured while buffering sound : code %d, audio cannot play.", err);
        is_running_ = false;
      }
    }
    alSourceQueueBuffers(source_id[i], 2, &buffer_id[i * kBufferCount]);
    if ((err = (int)alGetError()) != AL_NO_ERROR)
    {
      Logger::Error("Error occured while queueing sound : code %d, audio cannot play.", err);
      is_running_ = false;
    }
  }
  for (size_t i = 0; i < source_count_; ++i)
    alSourcePlay(source_id[i]);

  // -- main thread loop --
  while (is_running_)
  {
    for (size_t i = 0; i < source_count_; ++i)
    {
      cur_source_id = source_id[i];
      alGetSourcei(cur_source_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

      while (iBuffersProcessed)
      {
        alSourceUnqueueBuffers(cur_source_id, 1, &cur_buffer_id);
#if NOSOUND
        memset(pData, 0, iSingleBufferSize);
#else
        SoundDriver::getMixer().Copy(pData, uFrameSize, i);
#endif
        alBufferData(cur_buffer_id, audio_format_, pData, buffer_size_, kMixerSampleRate);
        alSourceQueueBuffers(cur_source_id, 1, &cur_buffer_id);
        iBuffersProcessed--;
      }

      // handle buffer underrun.
      alGetSourcei(cur_source_id, AL_SOURCE_STATE, &state);
      if (state != AL_PLAYING)
      {
        alGetSourcei(cur_source_id, AL_BUFFERS_QUEUED, &buffers_queued);
        if (buffers_queued > 0 && is_running_)
          alSourcePlay(cur_source_id);
      }
    }

    // CPU halt
    //Sleep(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMilisecond));
  }

  // -- finialize --
  free(pData);
}


// -------------------------------- class Mixer

SoundDriver::SoundDriver()
  : source_count_(kMaxSourceCount), buffer_size_(kDefaultBufferSize),
    channel_count_(4096), audio_format_(AL_FORMAT_STEREO16), is_running_(false),
    mixer_(nullptr)
{
  // S16, 44100, 2ch audio
  sinfo_.is_signed = 1;
  sinfo_.bitsize = kMixerBitsize;
  sinfo_.rate = kMixerSampleRate;
  sinfo_.channels = kMixerChannel;
}

SoundDriver::~SoundDriver()
{
  Destroy();
}

void SoundDriver::Initialize()
{
  using namespace rmixer;

  // Load settings from perference
  // TODO: Load SoundInfo setting
  //setting.GetValueSafe("AudioSourceCount", source_count_);
  //setting.GetValueSafe("SoundDevice", device_name_);
  const PrefValue<int> buffer_size("sound_buffer_size", kDefaultBufferSize);
  buffer_size_ = buffer_size.get();

  // Additional settings: set audio format.
  bool audio_format_set = true;
  if (sinfo_.is_signed == 1)
  {
    if (sinfo_.channels == 1)
    {
      switch (sinfo_.bitsize)
      {
      case 8:
        audio_format_ = AL_FORMAT_MONO8;
        break;
      case 16:
        audio_format_ = AL_FORMAT_MONO16;
        break;
      default:
        audio_format_set = false;
        break;
      }
    }
    else if (sinfo_.channels == 2)
    {
      switch (sinfo_.bitsize)
      {
      case 8:
        audio_format_ = AL_FORMAT_STEREO8;
        break;
      case 16:
        audio_format_ = AL_FORMAT_STEREO16;
        break;
      default:
        audio_format_set = false;
        break;
      }
    }
    else audio_format_set = false;
  }
  else audio_format_set = false;
  if (!audio_format_set)
    Logger::Error("Unknown sound format, use fallback.");

  // get output device from Game settings
  device = alcOpenDevice(
    device_name_.empty() ? nullptr : device_name_.c_str()
  );
  if (!device)
  {
    Logger::getInstance(LogMode::kLogError)
      << "Sound device initializing failed. Error: " << alGetError()
      << "(device name: " << device_name_ << ")" << Logger::endl;
    return;
  }

  // from here, device is must generated.
  context = alcCreateContext(device, 0);
  if (!alcMakeContextCurrent(context))
  {
    Logger::getInstance(LogMode::kLogError)
      << "Failed to make default audio context." << Logger::endl;
    return;
  }

  // check maximum audio source size
  unsigned numstereo;
  alcGetIntegerv(device, ALC_STEREO_SOURCES, 1, (ALCint*)&numstereo);
  if (source_count_ > numstereo)
    source_count_ = numstereo;

  // create audio channels and buffers
  alGenSources(source_count_, source_id);
  alGenBuffers(source_count_ * kBufferCount, buffer_id);

  // now mixer initialization
  mixer_ = new Mixer(sinfo_, channel_count_);

  // start audio stream
  is_running_ = true;
  sound_thread = std::thread(&SoundDriver::sound_thread_body, this);
  if (alGetError() != AL_NO_ERROR)
  {
    Logger::getInstance(LogMode::kLogError)
      << "Something wrong happened playing audio.. (" << alGetError() << ")" << Logger::endl;
    is_running_ = false;
  }

  Logger::Info("Audio initialized. "
               "Device name: %s, Mode: %uBit, %uHz, %uch. "
               "Channel %u",
    device_name_.c_str(),
    sinfo_.bitsize, sinfo_.rate, sinfo_.channels,
    source_count_);
}

void SoundDriver::Destroy()
{
  if (device)
  {
    for (unsigned i = 0; i < source_count_; ++i)
      alSourceStop(source_id[i]);

    is_running_ = false;
    sound_thread.join();

    alDeleteSources(source_count_, source_id);
    alDeleteBuffers(source_count_ * kBufferCount, buffer_id);
    alcMakeContextCurrent(0);
    alcDestroyContext(context);
    alcCloseDevice(device);
    device = 0;
  }
}

void SoundDriver::GetDevices(std::vector<std::string> &names)
{
  int enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
  if (enumeration == AL_FALSE)
    return; /* cannot enumerate */

  const ALCchar *devices = 0;
  if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_ALL_EXT") == AL_FALSE)
    devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
  else
    devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);

  const ALCchar *device = devices, *next = devices + 1;
  size_t len = 0;

  while (device && *device != '\0' && next && *next != '\0') {
    names.push_back(device);
    len = strlen(device);
    device += (len + 1);
    next += (len + 2);
  }
}

SoundDriver& SoundDriver::getInstance()
{
  static SoundDriver driver;
  return driver;
}

rmixer::Mixer& SoundDriver::getMixer()
{
  return *getInstance().mixer_;
}

// ---------------------------- class SoundData

SoundData::SoundData() : sound_(nullptr), result_(0) {}

SoundData::~SoundData()
{
  Unload();
}

/* @warn
 * This procedure is not async;
 * calling this procedure in main thread may stop rendering loop(hanging).
 * To load object without hang, use ResourceManager::LoadSound(...)
 * It will create async task which calls SoundData::Load(...) internally. */
bool SoundData::Load(const std::string &path)
{
  Unload();
  result_ = -1;
  if (sound_) return false;
  sound_ = new rmixer::Sound();
  if (!sound_->Load(path, SoundDriver::getMixer().GetSoundInfo()))
  {
    delete sound_;
    sound_ = nullptr;
  }
  else
  {
    result_ = 0;
  }
  return sound_ != nullptr;
}

bool SoundData::Load(const char *p, size_t len, const char *name_opt)
{
  Unload();
  result_ = -1;
  if (sound_) return false;
  sound_ = new rmixer::Sound();
  if (!sound_->Load(p, len, name_opt, SoundDriver::getMixer().GetSoundInfo()))
  {
    delete sound_;
    sound_ = nullptr;
  }
  else
  {
    result_ = 0;
  }
  return sound_ != nullptr;
}

bool SoundData::Load(const MetricGroup &m)
{
  return Load(m.get_str("path"));
}

void SoundData::Unload()
{
  if (sound_)
  {
    delete sound_;
    sound_ = nullptr;
  }
}

int SoundData::get_result() const
{
  return result_;
}

rmixer::Sound *SoundData::get_sound()
{
  return sound_;
}

bool SoundData::is_empty() const
{
  return sound_ == nullptr;
}


// -------------------------------- class Sound

Sound::Sound() : sounddata_(nullptr), channel_(nullptr), is_sound_from_rm_(false){}

Sound::~Sound()
{
  Unload();
}

/* @warn  This function instantly  */
bool Sound::Load(const std::string &path)
{
  Unload();
  sounddata_ = SOUNDMAN->Load(path);
  if (!sounddata_ || !sounddata_->get_sound())
    return false;
  is_sound_from_rm_ = true;
  channel_ = SoundDriver::getMixer().PlaySound(sounddata_->get_sound(), false);
  if (channel_) channel_->LockChannel();
  return channel_ != 0;
}

bool Sound::Load(const char *p, size_t len, const char *name_opt)
{
  Unload();
  sounddata_ = SOUNDMAN->Load(p, len, name_opt);
  if (!sounddata_ || !sounddata_->get_sound())
    return false;
  is_sound_from_rm_ = true;
  channel_ = SoundDriver::getMixer().PlaySound(sounddata_->get_sound(), false);
  if (channel_) channel_->LockChannel();
  return channel_ != 0;
}

bool Sound::Load(SoundData *sounddata)
{
  sounddata_ = sounddata;
  is_sound_from_rm_ = false;
  channel_ = SoundDriver::getMixer().PlaySound(sounddata_->get_sound(), false);
  if (channel_) channel_->LockChannel();
  return channel_ != 0;
}

void Sound::Unload()
{
  if (channel_)
  {
    channel_->Stop();
    channel_->UnlockChannel();
    channel_ = nullptr;
    if (sounddata_ && is_sound_from_rm_)
    {
      SOUNDMAN->Unload(sounddata_);
      sounddata_ = nullptr;
    }
  }
}

void Sound::Play()
{
  if (channel_)
    channel_->Play();
}

void Sound::Stop()
{
  if (channel_)
    channel_->Stop();
}

rmixer::Channel *Sound::get_channel()
{
  return channel_;
}

bool Sound::is_empty() const
{
  return channel_ == 0;
}

bool Sound::is_loading() const
{
  return (sounddata_ && sounddata_->is_loading());
}

}
