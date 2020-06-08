/**
 * @brief
 *
 * This file declare BaseObject, which is common interface
 * to all renderable objects.
 *
 * It includes attribute with child/parent, coordinate value
 * just as DOM object, but without Texture attributes.
 * (Texture attribute is implemented in Sprite class)
 *
 * BaseObject itself is AST tree, which parses command to
 * fill rendering semantic.
 * TODO: detach parser from BaseObject and change input like
 * xml tree, instead of string command.
 *
 * Also, coordinate related utility functions declared in
 * this file.
 */

#pragma once

#include "Graphic.h"
#include "Event.h"
#include "Setting.h"

#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <functional>

namespace rhythmus
{

class BaseObject;
class CommandArgs;

/** @brief Drawing properties of Object */
struct DrawProperty
{
  Vector4 pos;    // x1, y1, x2, y2
  Vector4 color;  // a, r, g, b
  Vector3 rotate; // rotation
  Point align;    // center(align) of object (.0f ~ .1f)
  Point scale;    // object scale
};

/** @brief Command function type for object. */
typedef std::function<void(void*, CommandArgs&, const std::string&)> CommandFn;

/** @brief Command mapping for object. */
typedef std::map<std::string, CommandFn> CommandFnMap;

/** @brief Tweens' ease type */
enum EaseTypes
{
  kEaseNone,
  kEaseLinear,
  kEaseIn,
  kEaseOut,
  kEaseInOut,
  kEaseInOutBack,
};

void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  float r, int ease_type);

/**
 * @brief
 * Declaration for state of current tween
 * State includes:
 * - Drawing state (need to be animated)
 * - Tween itself information: Easetype, Time (duration), ...
 * - Loop
 * - Tween ease type
 * - Event to be called (kind of triggering)
 */
struct AnimationFrame
{
  DrawProperty draw_prop;
  double time;              // timepoint of this frame
  int ease_type;            // tween ease type
};

class Animation
{
public:
  Animation(const DrawProperty *initial_state);
  void Clear();
  void DuplicateFrame(double duration);
  void AddFrame(const AnimationFrame &frame);
  void AddFrame(AnimationFrame &&frame);
  void AddFrame(const DrawProperty &draw_prop, double time, int ease_type);
  void SetCommand(const std::string &cmd);
  void Update(double &ms, std::string &command_to_invoke, DrawProperty *out);
  void GetDrawProperty(DrawProperty &out);
  void SetEaseType(int ease_type);
  void SetLoop(unsigned repeat_start_time);
  void DeleteLoop();
  const DrawProperty &LastFrame() const;
  DrawProperty &LastFrame();
  double GetTweenLength() const;
  size_t size() const;
  bool empty() const;
  bool is_finished() const;

  // used if currently tweening.
  // ex) if 500~1500 tween (not starting from zero),
  //     then if tween time is
  // 50ms   : false
  // 700ms  : true
  // 1600ms : false (actually, animation is already finished)
  bool is_tweening() const;

private:
  std::vector<AnimationFrame> frames_;
  int current_frame_;         // current frame. if -1, then time is yet to first frame.
  double current_frame_time_; // current frame eclipsed time
  double frame_time_;         // eclipsed time of whole animation
  bool is_finished_;
  bool repeat_;
  unsigned repeat_time_;
  std::string command;        // commands to be triggered when this tween starts
};

/**
 * @brief
 * Common interface to all renderable objects.
 * - which indicates containing its own frame coordinate value.
 */
class BaseObject : public EventReceiver
{
public:
  BaseObject();
  BaseObject(const BaseObject& obj);
  virtual ~BaseObject();

  void set_name(const std::string& name);
  const std::string& get_name() const;

  // Add child to be updated / rendered.
  void AddChild(BaseObject* obj);

  // Remove child.
  void RemoveChild(BaseObject *obj);
  void RemoveAllChild();

  // Find child by name.
  BaseObject* FindChildByName(const std::string& name);

  void set_parent(BaseObject* obj);
  BaseObject* get_parent();
  BaseObject* GetLastChild();
  BaseObject *GetChildAtPosition(float x, float y);
  void SetOwnChildren(bool v);

  // Load object property from metric info.
  virtual void Load(const MetricGroup &m);

  // Load object property from metric file.
  void LoadFromFile(const std::string &metric_file);

  // Load object property from ThemeMetric using name of this object.
  void LoadFromName();

  /**
   * Commands can modify attributes(mainly animation) of objects during runtime.
   * Metric, on the other hand, is expect to set object's initial state,
   * and all objects are expected to have general command set.
   * So, commands has more limitation than metric property.
   */
  void RunCommandByName(const std::string &name);
  void RunCommand(std::string command);
  void ClearCommand(const std::string &name);
  void DeleteAllCommand();
  void QueueCommand(const std::string &command);
  void AddCommand(const std::string &name, const std::string &command);
  void LoadCommand(const MetricGroup& metric);
  void LoadCommandWithPrefix(const std::string &prefix, const MetricGroup& metric);

  /**
   * @brief
   * Run single command which mainly changes mutable attribute(e.g. tween)
   * of the object.
   * Types of executable command and function are mapped
   * in GetCommandFnMap() function, which is refered by this procedure.
   */
  void RunCommand(const std::string &commandname, const std::string& value);

  /* @brief Inherited from EventReceiver */
  virtual bool OnEvent(const EventMessage& msg);

  DrawProperty& GetLastFrame();
  DrawProperty& GetCurrentFrame();
  void SetX(int x);
  void SetY(int y);
  void SetWidth(int w);
  void SetHeight(int h);
  void SetOpacity(float opa);
  void SetClip(bool clip);
  void SetPos(int x, int y);
  void SetPos(const Vector4& pos);    // x, y, w, h
  void MovePos(int x, int y);
  void SetSize(int w, int h);
  void SetAlpha(unsigned a);
  void SetAlpha(float a);
  void SetRGB(unsigned r, unsigned g, unsigned b);
  void SetRGB(float r, float g, float b);
  void SetScale(float x, float y);
  void SetRotation(float x, float y, float z);
  void SetRotationAsDegree(float x, float y, float z);
  void SetRepeat(bool repeat);
  void SetLoop(unsigned loop_start_time);
  float GetX() const;
  float GetY() const;
  void SetLR2DST(const std::string &cmd);

  /**
   * 0: Center
   * 1: Bottomleft
   * 2: Bottomcenter
   * ... (Same as numpad position)
   */
  void SetCenter(int type);

  /* local coord (0.0 ~ 1.0) */
  void SetCenter(float x, float y);

  /**
   * refer: enum EaseTypes
   */
  void SetAcceleration(int acc);
  void SetVisibleFlag(const std::string& group0, const std::string& group1,
    const std::string& group2, const std::string& group3);
  void UnsetVisibleFlag();
  void Hide();
  void Show();
  void SetDrawOrder(int order);
  int GetDrawOrder() const;
  virtual void SetText(const std::string &value);
  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  /* Refresh object value. */
  virtual void Refresh();

  void SetFocusable(bool is_focusable);
  void SetDraggable(bool is_draggable);
  bool IsEntered(float x, float y);
  void SetHovered(bool is_hovered);
  void SetFocused(bool is_focused);
  bool IsHovered() const;
  bool IsFocused() const;
  bool IsDragging() const;
  bool IsFocusable() const;
  bool IsDraggable() const;
  virtual void Click();

  virtual void OnDrag(float dx, float dy);
  virtual void OnText(uint32_t codepoint);
  virtual void OnAnimation(DrawProperty &frame);

  void SetDeltaTime(double time);
  void Stop();
  double GetTweenLength() const;
  bool IsTweening() const;
  bool IsVisible() const;

  void Update(double delta);
  void Render();

  bool operator==(const BaseObject& o) {
    return o.get_name() == get_name();
  }

private:

protected:
  std::string name_;

  BaseObject *parent_;

  // children for rendering (not released when this object is destructed)
  std::vector<BaseObject*> children_;

  // is this object owns children? if so, delete them when Removed.
  bool own_children_;

  // drawing order
  int draw_order_;

  // position property of object
  // 0: inherited/relative (default)
  // 1: absolute
  int position_prop_;

  // set translation center by xy pos.
  // if true - xy is the center of object.
  // if false - xy is the topleft of object.
  bool set_xy_as_center_;

  // queued animation list
  std::list<Animation> ani_;

  // current drawing state
  DrawProperty frame_;

  bool visible_;

  // hide if current object is not tweening.
  // (LR2 only)
  bool hide_if_not_tweening_;

  // group for visibility
  // @warn 4th group: for special use (e.g. panel visibility of onmouse)
  const int *visible_flag_[4];

  // ignoring visible group
  bool ignore_visible_group_;

  // is object is draggable?
  bool is_draggable_;

  // is this object focusable?
  bool is_focusable_;

  // is this object currently focused?
  bool is_focused_;

  // is object is currently hovered?
  bool is_hovered_;

  // do clipping when rendering object, including children?
  bool do_clipping_;

  // background color.
  Vector4 bg_color_;

  // commands to be called
  std::map<std::string, std::string> commands_;

  // information for debugging
  std::string debug_;

  void FillVertexInfo(VertexInfo *vi);
  virtual void doUpdate(double delta);
  virtual void doRender();
  virtual void doUpdateAfter();
  virtual void doRenderAfter();

  virtual const CommandFnMap& GetCommandFnMap();
};


/**
 * @brief
 * An util function for creating general object from metric.
 */
BaseObject* CreateObject(const MetricGroup &m);

}