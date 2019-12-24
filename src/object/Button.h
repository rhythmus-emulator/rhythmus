#include "Sprite.h"

namespace rhythmus
{

class Button : public Sprite
{
public:
  Button();
  virtual ~Button();
  virtual void Load(const Metric &metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);
private:
  // XXX: for debugging?
  int panel_;
};

}