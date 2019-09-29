#pragma once

#include "Image.h"
#include "Font.h"
#include <string>
#include <vector>
#include <map>
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

  void AddPathReplacement(const std::string& path_from, const std::string& path_to);
private:
  ResourceManager();
  ~ResourceManager();

  // cached font
  std::list<FontAuto> fonts_;

  // path replacement
  std::map<std::string, std::string> path_replacement_;

  std::string GetFinalPath(const std::string& path);
};

}