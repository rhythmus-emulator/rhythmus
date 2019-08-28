#include "Song.h"
#include "Timer.h"

namespace rhythmus
{

void SongList::Load()
{
}

void SongList::Save()
{
}

bool Song::Load(const std::string& path)
{
  return false;
}

void SongPlayable::Play()
{
}

void SongPlayable::Stop()
{
}

bool SongPlayable::IsPlaying()
{
  return false;
}

double SongPlayable::GetSongStartTime()
{
  return 0;
}

double SongPlayable::GetSongEclipsedTime()
{
  return 0;
}

}