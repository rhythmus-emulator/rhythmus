#include "BaseObject.h"
#include "LR2/LR2SceneLoader.h"   // XXX: atoi_op
#include "rutil.h"

namespace rhythmus
{

BaseObject::BaseObject()
  : parent_(nullptr), draw_order_(0)
{
  memset(&current_prop_, 0, sizeof(DrawProperty));
  current_prop_.sw = current_prop_.sh = 1.0f;
  current_prop_.display = true;
  SetRGB(1.0f, 1.0f, 1.0f);
  SetAlpha(1.0f);
  SetScale(1.0f, 1.0f);
}

BaseObject::BaseObject(const BaseObject& obj)
{
  name_ = obj.name_;
  parent_ = obj.parent_;
  // XXX: childrens won't copy by now
  tween_ = obj.tween_;
  current_prop_ = obj.current_prop_;
}

BaseObject::~BaseObject()
{
  for (auto* p : owned_children_)
    delete p;
}

void BaseObject::set_name(const std::string& name)
{
  name_ = name;
}

const std::string& BaseObject::get_name() const
{
  return name_;
}

void BaseObject::AddChild(BaseObject* obj)
{
  children_.push_back(obj);
}

void BaseObject::RemoveChild(BaseObject* obj)
{
  auto it = std::find(children_.begin(), children_.end(), obj);
  if (it != children_.end())
    children_.erase(it);
}

void BaseObject::RemoveAllChild()
{
  children_.clear();
}

void BaseObject::RegisterChild(BaseObject* obj)
{
  owned_children_.push_back(obj);
}

void BaseObject::set_parent(BaseObject* obj)
{
  parent_ = obj;
}

BaseObject* BaseObject::get_parent()
{
  return parent_;
}

BaseObject* BaseObject::FindChildByName(const std::string& name)
{
  for (const auto& spr : children_)
    if (spr->get_name() == name) return spr;
  return nullptr;
}

BaseObject* BaseObject::FindRegisteredChildByName(const std::string& name)
{
  for (const auto& spr : owned_children_)
    if (spr->get_name() == name) return spr;
  return nullptr;
}

BaseObject* BaseObject::GetLastChild()
{
  if (children_.size() > 0)
    return children_.back();
  else return nullptr;
}

bool BaseObject::IsAttribute(const std::string& key) const
{
  auto ii = attributes_.find(key);
  return (ii != attributes_.end());
}

void BaseObject::SetAttribute(const std::string& key, const std::string& value)
{
  attributes_[key] = value;
}

void BaseObject::DeleteAttribute(const std::string& key)
{
  auto ii = attributes_.find(key);
  if (ii != attributes_.end())
    attributes_.erase(ii);
}

template <>
std::string BaseObject::GetAttribute(const std::string& key) const
{
  const auto ii = attributes_.find(key);
  if (ii != attributes_.end())
    return ii->second;
  else
    return std::string();
}

template <>
int BaseObject::GetAttribute(const std::string& key) const
{
  return atoi_op( GetAttribute<std::string>(key).c_str() );
}

void BaseObject::LoadProperty(const std::map<std::string, std::string>& properties)
{
  for (const auto& ii : properties)
  {
    LoadProperty(ii.first, ii.second);
  }
}

// return true if property is set
void BaseObject::LoadProperty(const std::string& prop_name, const std::string& value)
{
  if (prop_name == "pos")
  {
    const auto it = std::find(value.begin(), value.end(), ',');
    if (it != value.end())
    {
      // TODO: change into 'setting'
      std::string v1 = value.substr(0, it - value.begin());
      const char *v2 = &(*it);
      SetPos(atoi(v1.c_str()), atoi(v2));
    }
  }
  /* Below is LR2 type commands */
  else if (strnicmp(prop_name.c_str(), "#DST_", 5) == 0)
  {
    std::vector<std::string> params;
    MakeParamCountSafe(value, params, 20);
    int attr = atoi(params[0].c_str());
    int time = atoi(params[1].c_str());
    int x = atoi(params[2].c_str());
    int y = atoi(params[3].c_str());
    int w = atoi(params[4].c_str());
    int h = atoi(params[5].c_str());
    int acc_type = atoi(params[6].c_str());
    int a = atoi(params[7].c_str());
    int r = atoi(params[8].c_str());
    int g = atoi(params[9].c_str());
    int b = atoi(params[10].c_str());
    int blend = atoi(params[11].c_str());
    int filter = atoi(params[12].c_str());
    int angle = atoi(params[13].c_str());
    int center = atoi(params[14].c_str());
#if 0
    int loop = atoi(params[15].c_str());
    int timer = atoi(params[16].c_str());
    int op1 = atoi_op(params[17].c_str());
    int op2 = atoi_op(params[18].c_str());
    int op3 = atoi_op(params[19].c_str());
#endif

    // DST specifies time information as the time of motion
    // So we need to calculate 'duration' of that motion.

    // calculate previous tween's duration
    if (!tween_.empty())
    {
      auto cur_motion_length = GetTweenLength();
      // to prevent overflow bug
      if (time < cur_motion_length)
        time = cur_motion_length;
      tween_.back().time_duration = time - cur_motion_length;
    }

    // If first tween and starting time is not zero,
    // then add dummy tween (invisible).
    if (tween_.empty() && time > 0)
    {
      // dummy tween should invisible
      Hide();
      SetDeltaTweenTime(time);
    }

    // make current tween
    SetDeltaTweenTime(0);
    
    // Now specify current tween.
    GetDestDrawProperty().display = true;
    SetPos(x, y);
    SetSize(w, h);
    SetRGB((unsigned)r, (unsigned)g, (unsigned)b);
    SetAlpha((unsigned)a);
    SetRotation(0, 0, angle);

    // these attribute will be processed in LR2Objects
    if (!IsAttribute("loop")) SetAttribute("loop", params[15]);
    if (!IsAttribute("timer")) SetAttribute("timer", params[16]);
    if (!IsAttribute("op1")) SetAttribute("op1", params[17]);
    if (!IsAttribute("op2")) SetAttribute("op2", params[18]);
    if (!IsAttribute("op3")) SetAttribute("op3", params[19]);
  }
}

void BaseObject::LoadDrawProperty(const BaseObject& other)
{
  LoadDrawProperty(other.current_prop_);
}

void BaseObject::LoadDrawProperty(const DrawProperty& draw_prop)
{
  current_prop_ = draw_prop;
}

void BaseObject::LoadTween(const BaseObject& other)
{
  LoadTween(other.tween_);
}

void BaseObject::LoadTween(const Tween& tween)
{
  tween_ = tween;
}

void BaseObject::AddTweenState(const DrawProperty &draw_prop, uint32_t time_duration, int ease_type, bool loop)
{
  tween_.emplace_back(TweenState{
    draw_prop, time_duration, 0, 0, loop, ease_type
    });
}

void BaseObject::SetTweenTime(int time_msec)
{
  auto tween_length = GetTweenLength();
  if (tween_length > time_msec) return;
  SetDeltaTweenTime(time_msec - tween_length);
}

void BaseObject::SetDeltaTweenTime(int time_msec)
{
  tween_.push_back({
    GetDestDrawProperty(),
    (uint32_t)time_msec, 0, 0, false, EaseTypes::kEaseOut
    });
}

void BaseObject::StopTween()
{
  tween_.clear();
}

uint32_t BaseObject::GetTweenLength() const
{
  uint32_t r = 0;
  for (auto &t : tween_)
    r += t.time_duration - t.time_eclipsed;
  return r;
}

DrawProperty& BaseObject::GetDestDrawProperty()
{
  if (tween_.empty())
    return current_prop_;
  else
    return tween_.back().draw_prop;
}

DrawProperty& BaseObject::get_draw_property()
{
  return current_prop_;
}

void BaseObject::SetPos(int x, int y)
{
  auto& p = GetDestDrawProperty();
  p.pi.x = x;
  p.pi.y = y;
}

void BaseObject::MovePos(int x, int y)
{
  auto& p = GetDestDrawProperty();
  p.pi.x += x;
  p.pi.y += y;
}

void BaseObject::SetSize(int w, int h)
{
  auto& p = GetDestDrawProperty();
  p.w = w;
  p.h = h;
}

void BaseObject::SetAlpha(unsigned a)
{
  SetAlpha(a / 255.0f);
}

void BaseObject::SetAlpha(float a)
{
  auto& p = GetDestDrawProperty();
  p.aBL = p.aBR = p.aTL = p.aTR = a;
}

void BaseObject::SetRGB(unsigned r, unsigned g, unsigned b)
{
  SetRGB(r / 255.0f, g / 255.0f, b / 255.0f);
}

void BaseObject::SetRGB(float r, float g, float b)
{
  auto& p = GetDestDrawProperty();
  p.r = r;
  p.g = g;
  p.b = b;
}

void BaseObject::SetScale(float x, float y)
{
  auto& p = GetDestDrawProperty();
  p.pi.sx = x;
  p.pi.sy = y;
}

void BaseObject::SetRotation(float x, float y, float z)
{
  auto& p = GetDestDrawProperty();
  p.pi.rotx = x;
  p.pi.roty = y;
  p.pi.rotz = z;
}

void BaseObject::SetCenter(float x, float y)
{
  auto& p = GetDestDrawProperty();
  p.pi.tx = x;
  p.pi.ty = y;
}

void BaseObject::Hide()
{
  current_prop_.display = false;
}

void BaseObject::Show()
{
  current_prop_.display = true;
}

void BaseObject::SetDrawOrder(int order)
{
  draw_order_ = order;
}

int BaseObject::GetDrawOrder() const
{
  return draw_order_;
}

// milisecond
void BaseObject::UpdateTween(float delta_ms)
{
  if (!IsTweening())
    return;

  // time process
  while (1)
  {
    //
    // kind of trick: if single tween with loop,
    // It's actually useless. turn off loop attr.
    //
    // kind of trick: if current tween is last one,
    // Do UpdateTween here. We expect last tween state
    // should be same as current tween in that case.
    //
    // By this logic, two or more tweens exist
    // if we exit this loop with delta_ms <= 0.
    //
    if (tween_.size() == 1)
    {
      current_prop_ = tween_.front().draw_prop;
      tween_.clear();
      return;
    }

    //
    // actual loop condition
    //
    if (delta_ms <= 0)
      break;

    TweenState &t = tween_.front();
    if (delta_ms + t.time_eclipsed >= t.time_duration)  // tween end
    {
      delta_ms -= t.time_duration - t.time_eclipsed;

      // trigger event if exist
      if (!t.event_name.empty())
        EventManager::SendEvent(t.event_name);

      // loop tween by push it again.
      // -> use more efficient method splice
      if (t.loop)
      {
        t.time_eclipsed = t.time_loopstart;
        tween_.splice(tween_.end(), tween_, tween_.begin());
      }
      else tween_.pop_front();
    }
    else  // ongoing tween
    {
      t.time_eclipsed += delta_ms;
      delta_ms = 0;  // actually same as exiting tween
    }
  }

  //
  // Now calculate DrawState
  //
  DrawProperty& ti = current_prop_;
  const TweenState &t1 = tween_.front();
  const TweenState &t2 = *std::next(tween_.begin());

  ti.display = t1.draw_prop.display;

  // If not display, we don't need to calculate further away.
  if (ti.display)
  {
    float r = (float)t1.time_eclipsed / t1.time_duration;
    MakeTween(ti, t1.draw_prop, t2.draw_prop, r, t1.ease_type);
  }
}

/* This method should be called just after all fixed tween is appended. */
void BaseObject::SetTweenLoopTime(uint32_t time_msec)
{
  // all tweens should loop after a tween is marked to be looped.
  bool loopstart = false;

  for (auto& t : tween_)
  {
    if (loopstart || time_msec < t.time_duration)
    {
      t.time_loopstart = time_msec;
      t.loop = true;
      time_msec = 0;
      loopstart = true;
    }
    else {
      t.time_loopstart = 0;
      t.loop = false;
      time_msec -= t.time_duration;
    }
  }
}

bool BaseObject::IsVisible() const
{
  return current_prop_.display &&
    current_prop_.aBL > 0 &&
    current_prop_.aBR > 0 &&
    current_prop_.aTL > 0 &&
    current_prop_.aTR > 0;
}

bool BaseObject::IsTweening() const
{
  return tween_.size() > 0;
}


// milisecond
void BaseObject::Update(float delta)
{
  UpdateTween(delta);
  doUpdate(delta);
  for (auto* p : children_)
    p->Update(delta);
  doUpdateAfter(delta);
}

void BaseObject::Render()
{
  if (!current_prop_.display)
    return;

  Graphic::PushMatrix();

  // set matrix
  Graphic::getInstance().SetMatrix(get_draw_property().pi);

  // render vertices
  doRender();

  for (auto* p : children_)
    p->Render();

  doRenderAfter();

  Graphic::PopMatrix();
}

void BaseObject::doUpdate(float delta) {}
void BaseObject::doRender() {}
void BaseObject::doUpdateAfter(float delta) {}
void BaseObject::doRenderAfter() {}


#define TWEEN_ATTRS \
  TWEEN(x) \
  TWEEN(y) \
  TWEEN(w) \
  TWEEN(h) \
  TWEEN(r) \
  TWEEN(g) \
  TWEEN(b) \
  TWEEN(aTL) \
  TWEEN(aTR) \
  TWEEN(aBR) \
  TWEEN(aBL) \
  TWEEN(sx) \
  TWEEN(sy) \
  TWEEN(sw) \
  TWEEN(sh) \
  TWEEN(pi.rotx) \
  TWEEN(pi.roty) \
  TWEEN(pi.rotz) \
  TWEEN(pi.tx) \
  TWEEN(pi.ty) \
  TWEEN(pi.x) \
  TWEEN(pi.y) \
  TWEEN(pi.sx) \
  TWEEN(pi.sy) \


void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  double r, int ease_type)
{
  switch (ease_type)
  {
  case EaseTypes::kEaseLinear:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseIn:
  {
    // use cubic function
    r = r * r * r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseOut:
  {
    // use cubic function
    r = 1 - r;
    r = r * r * r;
    r = 1 - r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseInOut:
  {
    // use cubic function
    r = 2 * r - 1;
    r = r * r * r;
    r = 0.5f + r / 2;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseNone:
  default:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  }
}

void MakeParamCountSafe(const std::string& in,
  std::vector<std::string> &vsOut, int required_size, char sep)
{
  rutil::split(in, sep, vsOut);
  if (required_size < 0) return;
  for (auto i = vsOut.size(); i < required_size; ++i)
    vsOut.emplace_back(std::string());
}

std::string GetFirstParam(const std::string& in, char sep)
{
  return in[0] != sep ? in.substr(0, in.find(',')) : std::string();
}

}