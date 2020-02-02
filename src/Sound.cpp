#include "Sound.h"
#include "Game.h"
#include "Error.h"
#include "SceneManager.h" /* GameSound::Load() */
#include "Util.h"
#include "common.h"

/* OpenAL */
#include <AL/al.h>
#include <AL/alc.h>

/* Playback thread */
#include <thread>
#include <mutex>

namespace rhythmus
{

constexpr int kMixerBitsize = 16;
constexpr int kMixerSampleRate = 44100;
constexpr int kMixerChannel = 2;
constexpr size_t kBufferCount = 4;
constexpr size_t kDefaultMixSize = 2048;

ALCdevice *device;
ALCcontext *context;
ALuint source_id;
ALuint buffer_id[kBufferCount];
std::thread sound_thread;
bool sound_thread_finish;

int get_audio_fmt()
{
  switch (kMixerBitsize)
  {
  case 8:
    return AL_FORMAT_STEREO8;
    break;
  case 16:
    return AL_FORMAT_STEREO16;
    break;
  default:
    // SHOULD not happen
    // ASSERT
    break;
  }
  return -1;
}

void SoundDriver::sound_thread_body()
{
  char* pData = (char*)calloc(1, buffer_size_);
  int iBuffersProcessed = 0;
  ALuint cur_buffer_id;
  int audio_fmt = get_audio_fmt();

  // stream initialization
  for (int i = 0; i < kBufferCount; ++i)
  {
    /* Don't know why but always need to cache dummy data into buffer ... */
    alBufferData(buffer_id[i], get_audio_fmt(), pData, buffer_size_, 44100);
    alSourceQueueBuffers(source_id, 1, &buffer_id[i]);
    ASSERT(alGetError() == 0);
  }
  alSourcePlay(source_id);

  while (!sound_thread_finish)
  {
    // TODO: just for buffering test
    alGetSourcei(source_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

    while (iBuffersProcessed)
    {
      alSourceUnqueueBuffers(source_id, 1, &cur_buffer_id);
      memset(pData, 0, buffer_size_);
      SoundDriver::getMixer().Mix(pData, buffer_size_);
      alBufferData(cur_buffer_id, audio_fmt, pData, buffer_size_, kMixerSampleRate);
      alSourceQueueBuffers(source_id, 1, &cur_buffer_id);
      iBuffersProcessed--;
    }

#if 1
    ALint state, errcode;
    alGetSourcei(source_id, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING)
    {
      ASSERT((errcode = alGetError()) == 0);
      /**
       * This logic shouldn't reached too much due to flickering sound.
       * If reached this logic, then might problem with alSourceQueueBuffers() ...
       * Or just program is stopped (e.g. debugging)
       */
      alSourcePlay(source_id);
    }
#endif

    Sleep(1);
  }
  free(pData);
}

// ---------------------------- class GameSound

void GameSound::set_name(const std::string &name)
{
  name_ = name;
}

void GameSound::Load()
{
  std::string path;
  if (!Setting::GetThemeMetricList().exist("Sound", name_))
    return;
  if (rmixer::Sound::Load(Setting::GetThemeMetricList().get<std::string>("Sound", name_)))
    RegisterToMixer(&SoundDriver::getMixer());
}

void GameSound::Load(const std::string &path)
{
  // XXX: returning value?
  rmixer::Sound::Load(path);
}

void GameSound::LoadWithName(const std::string &name)
{
  set_name(name);
  Load();
}


// -------------------------------- class Mixer

SoundDriver::SoundDriver() : buffer_size_(kDefaultMixSize)
{}

SoundDriver::~SoundDriver()
{
  Destroy();
}

void SoundDriver::Initialize()
{
  using namespace rmixer;

  // get output device from Game settings
  auto& setting = Setting::GetSystemSetting();
  std::string device_name;
  if (setting.GetOption("SoundDevice"))
  {
    device_name = setting.GetOption("SoundDevice")->value<std::string>();
    device = alcOpenDevice(device_name.c_str());
  }

  // load sound buffer size (fallback: use default value)
  buffer_size_ = setting.GetOption("SoundBufferSize")->value<size_t>();

  if (!device)
  {
    std::cerr << "Cannot find sound device or specified device is wrong, use NULL device (warn: no sound)." <<
      std::endl;
    device = alcOpenDevice(0);
    if (!device)
    {
      std::cerr << "Sound device initializing failed. Error: " << alGetError() << std::endl;
      return;
    }
  }

  // from here, device is must generated.
  context = alcCreateContext(device, 0);
  if (!alcMakeContextCurrent(context))
  {
    std::cerr << "Failed to make default context audio" << std::endl;
  }

  alGenSources(1, &source_id);
  alSourcef(source_id, AL_PITCH, 1);
  alSourcef(source_id, AL_GAIN, 1);
  alSource3f(source_id, AL_POSITION, 0, 0, 0);
  alSourcei(source_id, AL_LOOPING, AL_FALSE);

  alGenBuffers(kBufferCount, buffer_id);

  // now mixer initialization
  SoundInfo mixinfo;
  mixinfo.bitsize = kMixerBitsize;
  mixinfo.rate = kMixerSampleRate;
  mixinfo.channels = kMixerChannel;
  mixer_.SetSoundInfo(mixinfo);

  // okay start stream
  sound_thread_finish = false;
  sound_thread = std::thread(&SoundDriver::sound_thread_body, this);
  if (alGetError() != AL_NO_ERROR)
  {
    std::cerr << "Something wrong happened playing audio.." << std::endl;
  }
}

void SoundDriver::Destroy()
{
  if (device)
  {
    alSourceStop(source_id);

    sound_thread_finish = true;
    sound_thread.join();

    alDeleteSources(1, &source_id);
    alDeleteBuffers(kBufferCount, buffer_id);
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
  return getInstance().mixer_;
}

}
