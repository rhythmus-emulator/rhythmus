#include "Sprite.h"

namespace rhythmus
{
  
class LR2FnArgs;

class Button : public Sprite
{
public:
  Button();
  virtual ~Button();
  virtual void Load(const MetricGroup &metric);
  using Sprite::RunCommand;
  virtual void RunCommand(const std::string& command, const LR2FnArgs& args);

  virtual const char* type() const;

private:
  friend class Sprite;
  virtual void doRender();
};

}