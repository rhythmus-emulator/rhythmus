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
  img->LoadFromPath(path);
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
  f->LoadFont(path.c_str(), attrs);
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

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

}