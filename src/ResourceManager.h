#pragma once

#include "Image.h"
#include "Font.h"
#include <string>
#include <vector>
#include <list>

namespace rhythmus
{

/**
 * @brief
 * Contains cached resource(image, sound, etc.) objects
 */
class ResourceManager
{
public:
  static ImageAuto LoadImage(const std::string& path);
  static FontAuto LoadFont(const std::string& path, FontAttributes& attrs);
  static FontAuto LoadLR2Font(const std::string& path);
  static FontAuto GetSystemFont();
  static void ReleaseImage(ImageAuto img);
  static void ReleaseFont(const FontAuto& font);

  static ResourceManager& getInstance();
private:
  ResourceManager();
  ~ResourceManager();

  std::list<FontAuto> fonts_;
};

}