#include "Sprite.h"

namespace rhythmus
{

/* @brief special object for LR2. */
class Bargraph : public BaseObject
{
public:
  Bargraph();
  virtual ~Bargraph();
  virtual void Load(const MetricGroup &metric);
private:
  // 0: horizontal, 1: vertical
  int direction_;

  float value_;

  float *val_ptr_;

  Sprite bar_;

  virtual void doUpdate(double delta);
  virtual void doRender();
};

}