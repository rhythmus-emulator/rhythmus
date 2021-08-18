#pragma once

#include "BaseObject.h"

namespace rhythmus
{

class Wheel : public BaseObject
{
public:
  virtual void MoveDown() = 0;
  virtual void MoveUp() = 0;
  virtual void Expand() = 0;
  virtual void Collapse() = 0;
  virtual void OnSelectChange(const void* data, int direction) = 0;
  virtual void OnSelectChanged() = 0;
  virtual const std::string& GetSelectedItemId() const = 0;
  virtual void SetSelectedItemId(const std::string& id) = 0;
};

}