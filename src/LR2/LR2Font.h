#pragma once

#include "object/Text.h"
#include <vector>
#include <string>

namespace rhythmus
{

class LR2Font : public Font
{
public:
  LR2Font();
  virtual ~LR2Font();
  virtual bool LoadFont(const char* path, const FontAttributes&);

private:
  void ReadLR2Font(const char* path);
  void UploadTextureFile(const char* p, size_t len);
};

}