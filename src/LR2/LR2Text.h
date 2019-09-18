#include "LR2Flag.h"
#include "LR2Font.h"
#include "Event.h"

RHYTHMUS_NAMESPACE_BEGIN

class LR2Text : public Text, public LR2BaseObject, public EventReceiver
{
public:
  LR2Text();
  virtual ~LR2Text();

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);
  virtual bool IsVisible() const;

  virtual void Load();

  virtual bool OnEvent(const EventMessage &e);

private:
  // lr2 text code
  int lr2_source_id_;
  int align_;
  int editable_;
};

// TODO: LR2NumberText
// - Inherites NumberText, LR2BaseObject, EventReceiver
// - internally has its own Font object & its glyph list

RHYTHMUS_NAMESPACE_END