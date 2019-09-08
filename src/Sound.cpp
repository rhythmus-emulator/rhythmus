#include "Sound.h"
#include "Game.h"
#include <iostream>
#include <algorithm>

/* OpenAL */
#include <AL/al.h>
#include <AL/alc.h>

/* Playback thread */
#include <thread>
#include <mutex>

namespace rhythmus
{

ALCdevice *device;
ALCcontext *context;
ALuint source_id;
ALuint buffer_id;
int buffer_index;
int buffer_size;
std::thread sound_thread;
bool sound_thread_finish;

void sound_thread_body()
{
  char* pData = (char*)malloc(buffer_size);
  int iBuffersProcessed = 0;

  while (!sound_thread_finish)
  {
    // TODO: just for buffering test
    alGetSourcei(source_id, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

    while (iBuffersProcessed)
    {
      alSourceUnqueueBuffers(source_id, 1, &buffer_id);
      alBufferData(buffer_id, AL_FORMAT_STEREO16, pData, buffer_size, 44100);
      alSourceQueueBuffers(source_id, 1, &buffer_id);
      iBuffersProcessed--;
    }

    _sleep(1);
  }
  free(pData);
}

// -------------------------------- class Mixer

Mixer::Mixer() {}

Mixer::~Mixer()
{
  Destroy();
}

void Mixer::Initialize()
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

  //alGenBuffers(1, &buffer_id);
  buffer_size = 4096;
  sound_thread_finish = false;
  sound_thread = std::thread(sound_thread_body);
}

void Mixer::Destroy()
{
  if (device)
  {
    sound_thread_finish = true;
    sound_thread.join();

    alDeleteSources(1, &source_id);
    //alDeleteBuffers(2, buffer_id);
    alcMakeContextCurrent(0);
    alcDestroyContext(context);
    alcCloseDevice(device);
    device = 0;
  }
}

void Mixer::GetDevices(std::vector<std::string> &names)
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

SoundAuto Mixer::LoadSound(const std::string& path)
{
  SoundAuto s = std::make_unique<Sound>();
  s->is_loaded_ = true;
  return s;
}

SoundAuto Mixer::LoadSound(const char* ptr, size_t len)
{
  SoundAuto s = std::make_unique<Sound>();
  s->is_loaded_ = true;
  return s;
}

/* @warn Sound is automatically unloaded when destructed. */
void Mixer::UnloadSound(Sound* s)
{
  if (!s->is_loaded_)
    return;

  s->is_loaded_ = false;
  sounds_.erase(std::find(sounds_.begin(), sounds_.end(), s));
}

Mixer& Mixer::getInstance()
{
  static Mixer mixer;
  return mixer;
}


// -------------------------------- class Sound


Sound::Sound()
  : is_loaded_(false)
{
}

Sound::~Sound()
{
  Mixer::getInstance().UnloadSound(this);
}


}