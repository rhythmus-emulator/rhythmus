#pragma once

#include "Sprite.h"
#include "LR2Flag.h"
#include <vector>

namespace rhythmus
{

class LR2Sprite : public Sprite, public LR2BaseObject
{
public:
  LR2Sprite();
  virtual void LoadProperty(const std::string& prop_name, const std::string& value);
  virtual bool IsVisible() const;

private:
  int loop_;
  int src_timer_id_;

  // to make specific attribute is only loaded for once
  bool attr_loaded_;

  virtual bool IsUpdatable();
};

}