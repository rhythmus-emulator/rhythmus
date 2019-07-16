#include "ResourceManager.h"
#include <FreeImage.h>
#include <iostream>

namespace rhythmus
{
  
ImageAuto ResourceManager::LoadImage(const std::string& path)
{
  Image* img = new Image();
  img->LoadFromPath(path);
  return ImageAuto(img);
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