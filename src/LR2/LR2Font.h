#pragma once

#include "Font.h"
#include "LR2Sprite.h"
#include <vector>
#include <string>

namespace rhythmus
{

class LR2Font : public Font
{
public:
  LR2Font();
  virtual ~LR2Font();

  void ReadLR2Font(const char* path);

private:
  void UploadTextureFile(const char* p, size_t len);
};

class LR2Text : public Text
{
public:
  LR2Text();
  virtual ~LR2Text();

  LR2SprInfo& get_sprinfo();

  virtual void Update();

private:
  /* Internally stored lr2 string ptr.
   * When this pointer changed, we need to invalidate text vertices. */
  const char* lr2_str_ptr_;

  LR2SprInfo spr_info_;
};

}