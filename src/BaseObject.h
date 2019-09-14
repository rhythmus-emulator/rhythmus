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
#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>

namespace rhythmus
{

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
class BaseObject
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

  bool IsAttribute(const std::string& key) const;
  void SetAttribute(const std::string& key, const std::string& value);
  void DeleteAttribute(const std::string& key);
  template <typename T>
  T GetAttribute(const std::string& key) const;

  /* @brief Load command(property) manually. */
  virtual void LoadProperty(const std::map<std::string, std::string>& properties);
  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

  void LoadDrawProperty(const BaseObject& other);
  void LoadDrawProperty(const DrawProperty& draw_prop);
  void LoadTween(const BaseObject& other);
  void LoadTween(const Tween& tween);
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
  void SetCenter(float x, float y);
  void Hide();
  void Show();
  void SetDrawOrder(int order);
  int GetDrawOrder() const;

  bool IsTweening() const;
  virtual bool IsVisible() const;

  void Update(float delta);
  void Render();

  /* @brief Load property(resource) by itself.
   * generally called when LoadScene() is called. */
  virtual void Load();

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

  // misc attributes
  std::map<std::string, std::string> attributes_;

  // Tween
  void UpdateTween(float delta);
  void SetTweenLoopTime(uint32_t loopstart_time_msec);

  virtual void doUpdate(float delta);
  virtual void doRender();
  virtual void doUpdateAfter(float delta);
  virtual void doRenderAfter();
};

void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  double r, int ease_type);

void MakeParamCountSafe(const std::string& in,
  std::vector<std::string> &vsOut, int required_size = -1, char sep = ',');

std::string GetFirstParam(const std::string& in, char sep = ',');

}