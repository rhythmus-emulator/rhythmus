#pragma once

#include "Font.h"
#include "LR2Sprite.h"
#include "Event.h"
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

class LR2Text : public Text, EventReceiver
{
public:
  LR2Text();
  virtual ~LR2Text();

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);
  virtual bool IsVisible() const;

  virtual bool OnEvent(const EventMessage &e);

private:
  // lr2 text code
  int lr2_st_id_;
  int align_;
  int editable_;

  int op_[3];
  int timer_id_;
};

}