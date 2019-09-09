#include "Sound.h"
#include "Game.h"
#include <iostream>
#include <algorithm>

/* rencoder mixer */
#include "rmixer.h"

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
constexpr size_t kDefaultMixSize = 4096;

ALCdevice *device;
ALCcontext *context;
ALuint source_id;
ALuint buffer_id[kBufferCount];
int buffer_index;
int buffer_size;
std::thread sound_thread;
bool sound_thread_finish;

// software mixer which allocates channel virtually
Mixer* mixer;

void sound_thread_body()
{
  char* pData = (char*)malloc(buffer_size);
  int iBuffersProcessed = 0;
  ALuint cur_buffer_id;
  int audio_fmt = 0;
  switch (kMixerBitsize)
  {
  case 8:
    audio_fmt = AL_FORMAT_STEREO8;
    break;
  case 16:
    audio_fmt = AL_FORMAT_STEREO16;
    break;
  default:
    // SHOULD not happen
    // ASSERT
    break;
  }

  while (!sound_thread_finish)
  {
    // TODO: just for buffering test
    alGetSourcei(source_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

    while (iBuffersProcessed)
    {
      alSourceUnqueueBuffers(source_id, 1, &cur_buffer_id);
      mixer->Mix(pData, buffer_size);
      alBufferData(cur_buffer_id, audio_fmt, pData, buffer_size, kMixerSampleRate);
      alSourceQueueBuffers(source_id, 1, &cur_buffer_id);
      iBuffersProcessed--;
    }

    _sleep(1);
  }
  free(pData);
}

// -------------------------------- class Mixer

GameMixer::GameMixer() : buffer_size_(kDefaultMixSize)
{}

GameMixer::~GameMixer()
{
  Destroy();
}

void GameMixer::Initialize()
{
  // get output device from Game settings
  auto& setting = Game::getInstance().getSetting();
  if (setting.Exist("SoundDevice"))
  {
    std::string device_name;
    setting.Load("SoundDevice", device_name);
    device = alcOpenDevice(device_name.c_str());
  }

  if (!device)
  {
    std::cerr << "Cannot find sound device or specified device is wrong, use default device." <<
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
  alcMakeContextCurrent(context);

  alGenSources(1, &source_id);
  alSourcef(source_id, AL_PITCH, 1);
  alSourcef(source_id, AL_GAIN, 1);
  alSource3f(source_id, AL_POSITION, 0, 0, 0);
  alSource3f(source_id, AL_VELOCITY, 0, 0, 0);
  alSourcei(source_id, AL_LOOPING, AL_FALSE);

  alGenBuffers(kBufferCount, buffer_id);
  buffer_size = buffer_size_;
  sound_thread_finish = false;
  sound_thread = std::thread(sound_thread_body);

  // now mixer initialization
  SoundInfo mixinfo;
  mixinfo.bitsize = kMixerBitsize;
  mixinfo.rate = kMixerSampleRate;
  mixinfo.channels = kMixerChannel;
  mixer = new Mixer(mixinfo);

  // okay start stream
  alSourceQueueBuffers(source_id, 4, buffer_id);
  alSourcePlay(source_id);
}

void GameMixer::Destroy()
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

    delete mixer;
  }
}

void GameMixer::GetDevices(std::vector<std::string> &names)
{
  int enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
  if (enumeration == AL_FALSE)
    return; /* cannot enumerate */

  const ALCchar *devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
  const ALCchar *device = devices, *next = devices + 1;
  size_t len = 0;

  while (device && *device != '\0' && next && *next != '\0') {
    names.push_back(device);
    len = strlen(device);
    device += (len + 1);
    next += (len + 2);
  }
}

GameSoundAuto GameMixer::LoadSound(const std::string& path)
{
  GameSoundAuto s = std::make_unique<GameSound>();
  //s->is_loaded_ = true;
  return s;
}

GameSoundAuto GameMixer::LoadSound(const char* ptr, size_t len)
{
  GameSoundAuto s = std::make_unique<GameSound>();
  //s->is_loaded_ = true;
  return s;
}

GameMixer& GameMixer::getInstance()
{
  static GameMixer mixer;
  return mixer;
}

void GameMixer::set_buffer_size(int bytes)
{
  buffer_size_ = bytes;
}


// -------------------------------- class Sound


GameSound::GameSound()
  : channel_id_(-1)
{
}

GameSound::~GameSound()
{
  Unload();
}

void GameSound::Load(const std::string& path)
{
  Unload();
  int ch = allocate_channel();
  if (!mixer->LoadSound(ch, path))
  {
    std::cerr << "Failed to load audio: " << path << std::endl;
    return;
  }
  channel_id_ = ch;
}

void GameSound::LoadFromMemory(const char* p, size_t len)
{
  rutil::FileData fd;
  fd.p = (uint8_t*)p;
  fd.len = len;
  LoadFromMemory(fd);
}

void GameSound::LoadFromMemory(const rutil::FileData &fd)
{
  Unload();
  int ch = allocate_channel();
  if (!mixer->LoadSound(ch, fd))
  {
    std::cerr << "Failed to load audio from memory ..." << std::endl;
    return;
  }
  channel_id_ = ch;
}

void GameSound::Unload()
{
  if (channel_id_ == -1)
    return;
  mixer->FreeSound(channel_id_);
}

void GameSound::Play()
{
  if (channel_id_ < 0) return;
  mixer->Play(channel_id_);
}

bool GameSound::is_loaded() const
{
  return channel_id_ >= 0;
}

int GameSound::allocate_channel()
{
  static int channel_idx = 0;
  return channel_idx++;
}


}