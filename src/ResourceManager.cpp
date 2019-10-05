#include "ResourceManager.h"
#include "LR2/LR2Font.h"
#include <FreeImage.h>
#include <iostream>

namespace rhythmus
{
  
constexpr bool kUseCache = true;

ImageAuto ResourceManager::LoadImage(const std::string& path)
{
  /* Most image won't be shared from scene to scene, so just load it now */
  ImageAuto img = std::make_shared<Image>();
  img->LoadFromPath(getInstance().GetPath(path));
  return img;
}

FontAuto ResourceManager::LoadFont(const std::string& path, FontAttributes& attrs)
{
  auto &r = getInstance();
  if (kUseCache)
  {
    for (const auto f : r.fonts_)
    {
      if (f->get_path() == path && f->is_ttf_font() && f->get_attribute() == attrs)
        return f;
    }
  }

  FontAuto f = std::make_shared<Font>();
  f->LoadFont(getInstance().GetPath(path).c_str(), attrs);
  r.fonts_.push_back(f);
  return f;
}

FontAuto ResourceManager::LoadLR2Font(const std::string& path)
{
  auto &r = getInstance();
  if (kUseCache)
  {
    for (const auto f : r.fonts_)
    {
      if (f->get_path() == path && !f->is_ttf_font())
        return f;
    }
  }

  FontAuto fa = std::make_shared<LR2Font>();
  ((LR2Font*)fa.get())->ReadLR2Font(path.c_str());
  r.fonts_.push_back(fa);
  return fa;
}

FontAuto ResourceManager::GetSystemFont()
{
  static FontAuto sys_font;
  if (!sys_font)
  {
    FontAttributes fnt_attr_;
    memset(&fnt_attr_, 0, sizeof(fnt_attr_));
    fnt_attr_.color = 0xFFFFFFFF;
    fnt_attr_.size = 5;
    sys_font = ResourceManager::LoadFont("../system/default.ttf", fnt_attr_);
    if (!sys_font)
    {
      std::cerr << "Failed to read system font." << std::endl;
      return nullptr;
    }
    // commit default glyphs
    sys_font->Commit();
  }
  return sys_font;
}

void ResourceManager::ReleaseImage(ImageAuto img)
{
}

void ResourceManager::ReleaseFont(const FontAuto& font)
{
  auto &r = getInstance();
  auto ii = r.fonts_.begin();
  for (; ii != r.fonts_.end(); ++ii)
  {
    if (*ii == font)
      break;
  }
  if (ii != r.fonts_.end())
    r.fonts_.erase(ii);
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
  for (const auto& path : path_cached_)
  {
    if (CheckMasking(path, masked_path))
    {
      is_found = true;
      return path;
    }
  }

  // not found; return as it is
  return masked_path;
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

void ResourceManager::AddPathReplacement(const std::string& path_from, const std::string& path_to)
{
  path_replacement_[path_from] = path_to;
}

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

}