#include "Sprite.h"

namespace rhythmus
{

class Button : public Sprite
{
public:
  Button();
  virtual ~Button();
  virtual void Load(const MetricGroup &metric);

  virtual const char* type() const;

private:
  friend class Sprite;
  virtual void doRender();
};

}