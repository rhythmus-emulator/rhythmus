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

#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>

namespace rhythmus
{

class ThemeMetrics;

/** @brief Drawing properties of Object */
struct DrawProperty
{
  float x, y, w, h;         // general object position
  float r, g, b;            // alpha masking
  float aTL, aTR, aBR, aBL; // alpha fade
  float sx, sy, sw, sh;     // texture src crop
  ProjectionInfo pi;
  bool display;             // display: show or hide
};

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
struct TweenState
{
  DrawProperty draw_prop;
  uint32_t time_duration;   // original duration time of this loop
  uint32_t time_loopstart;  // time to start when looping
  uint32_t time_eclipsed;   // current tween's eclipsed time
  bool loop;                // should this tween reused?
  int ease_type;            // tween ease type
  std::string event_name;   // event to be triggered when this tween starts
};

using Tween = std::list<TweenState>;

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

  /* Add child to be updated / rendered. */
  void AddChild(BaseObject* obj);

  /* Remove child to be updated / rendered. */
  void RemoveChild(BaseObject* obj);

  void RemoveAllChild();

  /**
   * @brief
   * Make cacsading relationship to object.
   * If parent object is deleted, then registered children are also deleted.
   */
  void RegisterChild(BaseObject* obj);

  void set_parent(BaseObject* obj);
  BaseObject* get_parent();
  BaseObject* FindChildByName(const std::string& name);
  BaseObject* FindRegisteredChildByName(const std::string& name);
  BaseObject* GetLastChild();

  void LoadCommandByName(const std::string &name);
  void LoadCommand(const std::string &command);
  void AddCommand(const std::string &name, const std::string &command);
  void DeleteAllCommand();

  virtual void RunCommand(const std::string &command, const std::string& value);

  /* @brief Load property(resource). */
  virtual void Load(const ThemeMetrics& metric);

  /* @brief Initialize object from metric information. */
  virtual void Initialize();

  /* @brief Inherited from EventReceiver */
  virtual bool OnEvent(const EventMessage& msg);

  void AddTweenState(const DrawProperty &draw_prop, uint32_t time_duration,
    int ease_type = EaseTypes::kEaseOut, bool loop = false);
  void SetTweenTime(int time_msec);
  void SetDeltaTweenTime(int time_msec);
  void StopTween();
  uint32_t GetTweenLength() const;

  DrawProperty& GetDestDrawProperty();
  DrawProperty& get_draw_property();
  void SetPos(int x, int y);
  void MovePos(int x, int y);
  void SetSize(int w, int h);
  void SetAlpha(unsigned a);
  void SetAlpha(float a);
  void SetRGB(unsigned r, unsigned g, unsigned b);
  void SetRGB(float r, float g, float b);
  void SetScale(float x, float y);
  void SetRotation(float x, float y, float z);
  void SetRotationAsRadian(float x, float y, float z);
  /**
   * -1: Use absolute coord
   * 0: Center
   * 1: Bottomleft
   * 2: Bottomcenter
   * ... (Same as numpad position)
   */
  void SetRotationCenter(int rot_center);
  void SetRotationCenterCoord(float x, float y);
  /**
   * refer: enum EaseTypes
   */
  void SetAcceleration(int acc);
  void Hide();
  void Show();
  void SetDrawOrder(int order);
  int GetDrawOrder() const;

  bool IsTweening() const;
  virtual bool IsVisible() const;

  void Update(float delta);
  void Render();

  bool operator==(const BaseObject& o) {
    return o.get_name() == get_name();
  }

protected:
  std::string name_;

  BaseObject *parent_;

  // children for rendering (not released when this object is destructed)
  std::vector<BaseObject*> children_;

  // owned children list (released when its destruction)
  std::vector<BaseObject*> owned_children_;

  int draw_order_;

  Tween tween_;

  // current drawing state
  DrawProperty current_prop_;

  // rotation center property
  int rot_center_;

  // commands to be called
  std::map<std::string, std::string> commands_;

  // Tween
  void UpdateTween(float delta);
  void SetTweenLoopTime(uint32_t loopstart_time_msec);

  virtual void doUpdate(float delta);
  virtual void doRender();
  virtual void doUpdateAfter(float delta);
  virtual void doRenderAfter();
  virtual bool IsUpdatable();
};

void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  double r, int ease_type);

}