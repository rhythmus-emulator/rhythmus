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
  ImageAuto LoadImage(const std::string& path);
  FontAuto LoadFont(const std::string& path, FontAttributes& attrs);
  FontAuto LoadLR2Font(const std::string& path);
  void ReleaseImage(ImageAuto img);
  void ReleaseFont(FontAuto font);

  static ResourceManager& getInstance();
private:
  ResourceManager();
  ~ResourceManager();

  std::list<FontAuto> fonts_;
};

}