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
 *
 * Also, coordinate related utility functions declared in
 * this file.
 */

#pragma once

#include "Graphic.h"
#include "Animation.h"
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
class LR2FnArgs;


struct ChildData
{
  BaseObject* p;
  bool is_static;
};

/** @brief Command function type for object. */
typedef std::function<void(void*, CommandArgs&, const std::string&)> CommandFn;

/** @brief Command mapping for object. */
typedef std::map<std::string, CommandFn> CommandFnMap;

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
  virtual BaseObject *clone();

  void set_name(const std::string& name);
  const std::string& get_name() const;

  // Add child to be updated / rendered.
  void AddChild(BaseObject* obj);
  void AddStaticChild(BaseObject* obj);

  // Remove child.
  void RemoveChild(BaseObject *obj);
  void RemoveAllChild();

  // Find child by name.
  BaseObject* FindChildByName(const std::string& name);

  void set_parent(BaseObject* obj);
  BaseObject* get_parent();
  BaseObject* GetLastChild();
  BaseObject* GetLastChildWithName(const std::string& name);
  BaseObject *GetChildAtPosition(float x, float y);
  
  // Load object property from metric info.
  virtual void Load(const MetricGroup &metric);

  // Load object property from ThemeMetric using name of this object.
  void LoadFromName();

  // Called after all object tree is created.
  virtual void OnReady();

  /**
   * Commands can modify attributes(mainly animation) of objects during runtime.
   * Metric, on the other hand, is expect to set object's initial state,
   * and all objects are expected to have general command set.
   * So, commands has more limitation than metric property.
   */
  void RunCommandByName(const std::string &name);
  void ClearCommand(const std::string &name);
  void DeleteAllCommand();
  void QueueCommand(const std::string &command);
  void AddCommand(const std::string &name, const std::string &command);
  void LoadCommand(const MetricGroup& metric);

  /**
   * @brief
   * Run single command which mainly changes mutable attribute(e.g. tween)
   * of the object.
   * Types of executable command and function are mapped
   * in GetCommandFnMap() function, which is refered by this procedure.
   */
  void RunCommand(const std::string &commandname, const std::string& value);
  void RunCommand(std::string command);

  /* Execute LR2 command for this object
   * @comment implementation is written in macro. */
  void RunCommandLR2(const LR2FnArgs& args);
  virtual void RunCommandLR2(const std::string& command, const LR2FnArgs& args);

  /* @brief Inherited from EventReceiver */
  virtual bool OnEvent(const EventMessage& msg);

  DrawProperty& GetLastFrame();
  DrawProperty& GetCurrentFrame();
  Animation& GetAnimation();
  QueuedAnimation& GetQueuedAnimation();
  void SetX(float x);
  void SetY(float y);
  void SetWidth(float w);
  void SetHeight(float h);
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
  float GetWidth() const;
  float GetHeight() const;
  void SetDebug(const std::string &debug_msg);
  void BringToTop();

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

  void Stop();
  void Replay();
  void Pause();
  bool IsTweening() const;
  bool IsVisible() const;

  void Update(double delta);
  void Render();

  bool operator==(const BaseObject& o) {
    return o.get_name() == get_name();
  }

  virtual const char* type() const;
  virtual std::string toString() const;

  /**
   * @brief Creates new object by given type.
   */
  static BaseObject* CreateObject(const std::string &type);

private:

protected:
  std::string name_;

  BaseObject *parent_;

  // child objects which are going to be rendered.
  std::vector<ChildData> children_;

  // propagate event(command) to children?
  bool propagate_event_;

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

  // framed animation
  Animation ani_;

  // queued animation
  QueuedAnimation qani_;

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

  void SortChildren();
  void FillVertexInfo(VertexInfo *vi);
  virtual void doUpdate(double delta);
  virtual void doRender();
  virtual void doUpdateAfter();
  virtual void doRenderAfter();

  virtual const CommandFnMap& GetCommandFnMap();
};

}