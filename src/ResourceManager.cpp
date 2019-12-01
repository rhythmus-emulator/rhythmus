#include "ResourceManager.h"
#include "Image.h"
#include "Font.h"
#include "Timer.h"
#include "LR2/LR2Font.h"
#include <FreeImage.h>
#include <iostream>
#include <mutex>

namespace rhythmus
{

constexpr bool kUseCache = true;

template <typename T>
T* CreateObjectFromPath(const std::string &path);

template <>
Image *CreateObjectFromPath(const std::string &path)
{
  Image *img = new Image();
  img->LoadFromPath(path);
  img->CommitImage();
  return img;
}

template <>
Font *CreateObjectFromPath(const std::string &path)
{
  std::string fn, cmd, ext;
  Split(path, '|', fn, cmd);
  ext = GetExtension(fn);
  FontAttributes attr;
  Font *font;
  if (ext == "ttf")
  {
    font = new Font();
  }
  else if (ext == "dxa")
  {
    font = new LR2Font();
  }
  else
  {
    std::cerr << "Unknown Font file: " << path << std::endl;
  }

  attr.SetFromCommand(cmd);

  if (!font->LoadFont(fn.c_str(), attr))
  {
    std::cerr << "Invalid Font file: " << path << std::endl;
    delete font;
    return nullptr;
  }

  return font;
}

template <typename T>
class ObjectPoolWithName
{
private:
  struct ObjectWithSharedCnt
  {
    ObjectWithSharedCnt() = default;
    ObjectWithSharedCnt(size_t c, T* o)
    {
      count = c;
      obj.reset(o);
    }

    size_t count;
    std::unique_ptr<T> obj;
  };

  std::mutex mutex_;
  std::map<std::string, ObjectWithSharedCnt> objects_;

public:
  T* Load(const std::string &path)
  {
    // find for existing object
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = objects_.find(path);
      if (it != objects_.end())
      {
        ++it->second.count;
        return it->second.obj.get();
      }
    }

    // if not, create object using constructor.
    T* obj = CreateObjectFromPath<T>(path);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      objects_[path] = ObjectWithSharedCnt( 1, obj );
    }
    return obj;
  }

  void Unload(T *t)
  {
    if (!t) return;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = objects_.begin();
    while (it != objects_.end())
    {
      if (it->second.obj.get() == t)
        break;
      ++it;
    }
    if (it != objects_.end())
    {
      if (--it->second.count == 0)
      {
        // object will be automatically destroyed as it is unique_ptr
        objects_.erase(it);
      }
    }
  }

  bool Exist(const std::string &name) const
  {
    return objects_.find(name) != objects_.end();
  }

  typename std::map<std::string, ObjectWithSharedCnt>::iterator begin()
  { return objects_.begin(); }
  typename std::map<std::string, ObjectWithSharedCnt>::iterator end()
  { return objects_.end(); }
};

static ObjectPoolWithName<Image> gImgpool;
static ObjectPoolWithName<Font> gFontpool;

Image* ResourceManager::LoadImage(const std::string& path)
{
  std::string gamepath = getInstance().GetPath(path);
  return gImgpool.Load(gamepath);
}

void ResourceManager::UnloadImage(Image *img)
{
  gImgpool.Unload(img);
}

template <> Image*
ResourceManager::LoadObject(const std::string &path) { return LoadImage(path); }
template <> void
ResourceManager::UnloadObject(Image *img) { UnloadImage(img); }



Font* ResourceManager::LoadFont(const std::string& path)
{
  // TODO: FontAttributes& will be processed later
  // from file type or parameter in path.
  std::string gamepath = getInstance().GetPath(path);
  return gFontpool.Load(gamepath);
}

void ResourceManager::UnloadFont(Font *font)
{
  gFontpool.Unload(font);
}

Font* ResourceManager::LoadSystemFont()
{
  return LoadFont("../system/default.ttf|size:5;color:0xFFFFFFFF");
}

template <> Font*
ResourceManager::LoadObject(const std::string &path) { return LoadFont(path); }
template <> void
ResourceManager::UnloadObject(Font *img) { UnloadFont(img); }

void ResourceManager::UpdateMovie()
{
  // Update images
  float delta = (float)Timer::SystemTimer().GetDeltaTime();
  for (const auto& it : gImgpool)
    it.second.obj->Update(delta);
}

ResourceManager& ResourceManager::getInstance()
{
  static ResourceManager m;
  return m;
}

void ResourceManager::CacheSystemDirectory()
{
  auto &r = getInstance();

  /* cache system paths for resource. (not song path) */
  r.CacheDirectory("../themes/");
  r.CacheDirectory("../sound/");
  r.CacheDirectory("../bgm/"); /* for LR2 compatiblity */
}

void ResourceManager::CacheDirectory(const std::string& dir)
{
  // XXX: currently only files are cached.
  GetFilesFromDirectory(dir, path_cached_, 500 /* not so deep ...? */);
}

void ResourceManager::MakePathHigherPriority(const std::string& path)
{
  auto it = std::find(path_cached_.begin(), path_cached_.end(), path);
  if (it == path_cached_.end()) return;
  std::rotate(path_cached_.begin(), it, path_cached_.end());
}

std::string ResourceManager::GetPath(const std::string& masked_path, bool &is_found) const
{
  is_found = false;

  // attempt to find path replacement first ...
  auto it = path_replacement_.find(masked_path);
  if (it != path_replacement_.end())
  {
    is_found = true;
    return it->second;
  }

  // check masking first -- if not, return as it is.
  if (masked_path.find('*') == std::string::npos)
    return masked_path;

  // do path mask matching for cached paths
  std::string r = getInstance().ResolveMaskedPath(masked_path);
  if (r.empty())
  {
    is_found = false;
    return masked_path;
  }
  return r;
}

std::string ResourceManager::GetPath(const std::string& masked_path) const
{
  bool _;
  return GetPath(masked_path, _);
}

void ResourceManager::GetAllPaths(const std::string& masked_path, std::vector<std::string> &out) const
{
  // check masking first -- if not, return as it is.
  if (masked_path.find('*') == std::string::npos)
    return;

  // do path mask matching for cached paths
  for (const auto& path : path_cached_)
  {
    if (CheckMasking(path, masked_path))
    {
      out.push_back(path);
    }
  }
}

void ResourceManager::SetAlias(const std::string& path_from, const std::string& path_to)
{
  // check masked path. if so, resolve it.
  auto &inst = getInstance();
  inst.path_replacement_[path_from] = inst.ResolveMaskedPath(path_to);
}

std::string ResourceManager::ResolveMaskedPath(const std::string &masked_path)
{
  // check masking first -- if not, return as it is.
  if (masked_path.find('*') == std::string::npos)
    return masked_path;

  // do path mask matching for cached paths
  for (const auto& path : path_cached_)
  {
    if (CheckMasking(path, masked_path))
    {
      return path;
    }
  }

  // if not found, return empty string
  return std::string();
}

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

}