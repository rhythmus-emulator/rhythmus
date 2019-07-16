#pragma once
#include "Image.h"
#include <string>
#include <vector>

namespace rhythmus
{

/**
 * @brief
 * Contains cached resource(image, sound, etc.) objects
 */
class ResourceManager
{
public:
  ImageAuto LoadImage(const std::string& path);

  static ResourceManager& getInstance();
private:
  ResourceManager();
  ~ResourceManager();
};

}