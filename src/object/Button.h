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
  virtual std::string toString() const;

private:
  // XXX: for debugging?
  int panel_, button_id_;
};

}