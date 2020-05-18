#include "Sprite.h"

namespace rhythmus
{

class Button : public Sprite
{
public:
  Button();
  virtual ~Button();
  virtual void Load(const MetricGroup &metric);
private:
  // XXX: for debugging?
  int panel_, button_id_;
};

}