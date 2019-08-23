#pragma once

#include "Font.h"
#include "LR2Sprite.h"
#include "Event.h"
#include <vector>
#include <string>

namespace rhythmus
{

struct LR2FontSRC
{
  int fontidx, st, align, edit;
};

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
  LR2FontSRC& get_fontsrc();

  /* @brief upload LR2 SRC/DST data into tweens */
  void SetSpriteFromLR2Data();

  virtual void Update();

  virtual void Render();

private:
  /* Internally stored lr2 string ptr.
   * When this pointer changed, we need to invalidate text vertices. */
  const char* lr2_str_ptr_;

  LR2SprInfo spr_info_;

  /* Event handler for some sprite */
  class LR2EventReceiver : public EventReceiver
  {
  public:
    virtual bool OnEvent(const EventMessage &e);
    LR2Text* t_;
    int lr2_st_id_;
  } e_;
};

}