#pragma once

#include <set>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <functional>

/* Input ID lists borrowed from glfw.h */
#define RI_KEY_SPACE              32
#define RI_KEY_APOSTROPHE         39  /* ' */
#define RI_KEY_COMMA              44  /* , */
#define RI_KEY_MINUS              45  /* - */
#define RI_KEY_PERIOD             46  /* . */
#define RI_KEY_SLASH              47  /* / */
#define RI_KEY_0                  48
#define RI_KEY_1                  49
#define RI_KEY_2                  50
#define RI_KEY_3                  51
#define RI_KEY_4                  52
#define RI_KEY_5                  53
#define RI_KEY_6                  54
#define RI_KEY_7                  55
#define RI_KEY_8                  56
#define RI_KEY_9                  57
#define RI_KEY_SEMICOLON          59  /* ; */
#define RI_KEY_EQUAL              61  /* = */
#define RI_KEY_A                  65
#define RI_KEY_B                  66
#define RI_KEY_C                  67
#define RI_KEY_D                  68
#define RI_KEY_E                  69
#define RI_KEY_F                  70
#define RI_KEY_G                  71
#define RI_KEY_H                  72
#define RI_KEY_I                  73
#define RI_KEY_J                  74
#define RI_KEY_K                  75
#define RI_KEY_L                  76
#define RI_KEY_M                  77
#define RI_KEY_N                  78
#define RI_KEY_O                  79
#define RI_KEY_P                  80
#define RI_KEY_Q                  81
#define RI_KEY_R                  82
#define RI_KEY_S                  83
#define RI_KEY_T                  84
#define RI_KEY_U                  85
#define RI_KEY_V                  86
#define RI_KEY_W                  87
#define RI_KEY_X                  88
#define RI_KEY_Y                  89
#define RI_KEY_Z                  90
#define RI_KEY_LEFT_BRACKET       91  /* [ */
#define RI_KEY_BACKSLASH          92  /* \ */
#define RI_KEY_RIGHT_BRACKET      93  /* ] */
#define RI_KEY_GRAVE_ACCENT       96  /* ` */
#define RI_KEY_WORLD_1            161 /* non-US #1 */
#define RI_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define RI_KEY_ESCAPE             256
#define RI_KEY_ENTER              257
#define RI_KEY_TAB                258
#define RI_KEY_BACKSPACE          259
#define RI_KEY_INSERT             260
#define RI_KEY_DELETE             261
#define RI_KEY_RIGHT              262
#define RI_KEY_LEFT               263
#define RI_KEY_DOWN               264
#define RI_KEY_UP                 265
#define RI_KEY_PAGE_UP            266
#define RI_KEY_PAGE_DOWN          267
#define RI_KEY_HOME               268
#define RI_KEY_END                269
#define RI_KEY_CAPS_LOCK          280
#define RI_KEY_SCROLL_LOCK        281
#define RI_KEY_NUM_LOCK           282
#define RI_KEY_PRINT_SCREEN       283
#define RI_KEY_PAUSE              284
#define RI_KEY_F1                 290
#define RI_KEY_F2                 291
#define RI_KEY_F3                 292
#define RI_KEY_F4                 293
#define RI_KEY_F5                 294
#define RI_KEY_F6                 295
#define RI_KEY_F7                 296
#define RI_KEY_F8                 297
#define RI_KEY_F9                 298
#define RI_KEY_F10                299
#define RI_KEY_F11                300
#define RI_KEY_F12                301
#define RI_KEY_F13                302
#define RI_KEY_F14                303
#define RI_KEY_F15                304
#define RI_KEY_F16                305
#define RI_KEY_F17                306
#define RI_KEY_F18                307
#define RI_KEY_F19                308
#define RI_KEY_F20                309
#define RI_KEY_F21                310
#define RI_KEY_F22                311
#define RI_KEY_F23                312
#define RI_KEY_F24                313
#define RI_KEY_F25                314
#define RI_KEY_KP_0               320
#define RI_KEY_KP_1               321
#define RI_KEY_KP_2               322
#define RI_KEY_KP_3               323
#define RI_KEY_KP_4               324
#define RI_KEY_KP_5               325
#define RI_KEY_KP_6               326
#define RI_KEY_KP_7               327
#define RI_KEY_KP_8               328
#define RI_KEY_KP_9               329
#define RI_KEY_KP_DECIMAL         330
#define RI_KEY_KP_DIVIDE          331
#define RI_KEY_KP_MULTIPLY        332
#define RI_KEY_KP_SUBTRACT        333
#define RI_KEY_KP_ADD             334
#define RI_KEY_KP_ENTER           335
#define RI_KEY_KP_EQUAL           336
#define RI_KEY_LEFT_SHIFT         340
#define RI_KEY_LEFT_CONTROL       341
#define RI_KEY_LEFT_ALT           342
#define RI_KEY_LEFT_SUPER         343
#define RI_KEY_RIGHT_SHIFT        344
#define RI_KEY_RIGHT_CONTROL      345
#define RI_KEY_RIGHT_ALT          346
#define RI_KEY_RIGHT_SUPER        347
#define RI_KEY_MENU               348

#define RI_MOUSE_BUTTON_1         0x200
#define RI_MOUSE_BUTTON_2         0x201
#define RI_MOUSE_BUTTON_3         0x202
#define RI_MOUSE_BUTTON_4         0x203
#define RI_MOUSE_BUTTON_5         0x204
#define RI_MOUSE_BUTTON_6         0x205
#define RI_MOUSE_BUTTON_7         0x206
#define RI_MOUSE_BUTTON_8         0x207
#define RI_MOUSE_BUTTON_LAST      RI_MOUSE_BUTTON_8
#define RI_MOUSE_BUTTON_LEFT      RI_MOUSE_BUTTON_1
#define RI_MOUSE_BUTTON_RIGHT     RI_MOUSE_BUTTON_2
#define RI_MOUSE_BUTTON_MIDDLE    RI_MOUSE_BUTTON_3

#define RI_JOYSTICK_1             0x300
#define RI_JOYSTICK_2             0x301
#define RI_JOYSTICK_3             0x302
#define RI_JOYSTICK_4             0x303
#define RI_JOYSTICK_5             0x304
#define RI_JOYSTICK_6             0x305
#define RI_JOYSTICK_7             0x306
#define RI_JOYSTICK_8             0x307
#define RI_JOYSTICK_9             0x308
#define RI_JOYSTICK_10            0x309
#define RI_JOYSTICK_11            0x310
#define RI_JOYSTICK_12            0x311
#define RI_JOYSTICK_13            0x312
#define RI_JOYSTICK_14            0x313
#define RI_JOYSTICK_15            0x314
#define RI_JOYSTICK_16            0x315
#define RI_JOYSTICK_LAST          RI_JOYSTICK_16

#define RI_GAMEPAD_BUTTON_A               0x400
#define RI_GAMEPAD_BUTTON_B               0x401
#define RI_GAMEPAD_BUTTON_X               0x402
#define RI_GAMEPAD_BUTTON_Y               0x403
#define RI_GAMEPAD_BUTTON_LEFT_BUMPER     0x404
#define RI_GAMEPAD_BUTTON_RIGHT_BUMPER    0x405
#define RI_GAMEPAD_BUTTON_BACK            0x406
#define RI_GAMEPAD_BUTTON_START           0x407
#define RI_GAMEPAD_BUTTON_GUIDE           0x408
#define RI_GAMEPAD_BUTTON_LEFT_THUMB      0x409
#define RI_GAMEPAD_BUTTON_RIGHT_THUMB     0x410
#define RI_GAMEPAD_BUTTON_DPAD_UP         0x411
#define RI_GAMEPAD_BUTTON_DPAD_RIGHT      0x412
#define RI_GAMEPAD_BUTTON_DPAD_DOWN       0x413
#define RI_GAMEPAD_BUTTON_DPAD_LEFT       0x414
#define RI_GAMEPAD_BUTTON_LAST            RI_GAMEPAD_BUTTON_DPAD_LEFT

#define RI_GAMEPAD_BUTTON_CROSS       RI_GAMEPAD_BUTTON_A
#define RI_GAMEPAD_BUTTON_CIRCLE      RI_GAMEPAD_BUTTON_B
#define RI_GAMEPAD_BUTTON_SQUARE      RI_GAMEPAD_BUTTON_X
#define RI_GAMEPAD_BUTTON_TRIANGLE    RI_GAMEPAD_BUTTON_Y

#define RI_GAMEPAD_AXIS_LEFT_X        0x420
#define RI_GAMEPAD_AXIS_LEFT_Y        0x421
#define RI_GAMEPAD_AXIS_RIGHT_X       0x422
#define RI_GAMEPAD_AXIS_RIGHT_Y       0x423
#define RI_GAMEPAD_AXIS_LEFT_TRIGGER  0x424
#define RI_GAMEPAD_AXIS_RIGHT_TRIGGER 0x425
#define RI_GAMEPAD_AXIS_LAST          RI_GAMEPAD_AXIS_RIGHT_TRIGGER

#define RI_MIDI_KEYSTART   0x500

namespace rhythmus
{

/**
 * @brief
 * General game event types (including input event)
 */
enum InputEvents
{
  kOnKeyDown,
  kOnKeyPress,  /* key event repeating due to pressing */
  kOnKeyUp,
  kOnText,
  kOnCursorMove,
  kOnCursorDown,
  kOnCursorClick,
  kOnJoystick,

  kInputEventLast    /* unused event; just for last index */
};

class InputEventManager
{
public:
  static void Flush();
};

/* @brief Only for Input related event */
class InputEvent
{
public:
  InputEvent();
  InputEvent(int type);

  /* type index of enum InputEvents */
  int type() const;

  /* time in second. unsynced from gametime (means exact time) */
  double time() const;

  void SetKeyCode(int v);
  void SetPosition(int x, int y);
  void SetButton(int button);
  void SetCodepoint(uint32_t codepoint);

  int KeyCode() const;
  int GetX() const;
  int GetY() const;
  int GetButton() const;
  uint32_t Codepoint() const;

private:
  double time_;
  int type_;
  int argv_[4];
};

class InputEventReceiver
{
public:
  InputEventReceiver();
  ~InputEventReceiver();
  virtual void OnInputEvent(const InputEvent &e) = 0;
};

/**
 * @brief
 * Events related to game 
 * (Not for input event;
 *  input event is processed in InputEvent / InputManager class)
 */
class EventMessage
{
public:
  EventMessage() = default;
  EventMessage(const std::string &name);
  void SetEventName(const std::string &name);
  const std::string &GetEventName() const;

private:
  std::string name_;
};

/* @brief Base EventReceiver class, invoked when event occured. */
class EventReceiver
{
public:
  virtual ~EventReceiver();

  void SubscribeTo(const std::string &name);
  void UnsubscribeAll();

  virtual bool OnEvent(const EventMessage& msg);
  friend class EventManager;

private:
  /* to what events it subscribed. */
  std::vector<std::string> subscription_;
};

typedef std::map<std::string, std::function<void()> > EventFnMap;

/* @brief EventReceiver with EventMap. */
class EventReceiverMap : public EventReceiver
{
public:
  virtual ~EventReceiverMap();
  void AddEvent(const std::string &event_name, const std::function<void()> &func);
  void Clear();
private:
  virtual bool OnEvent(const EventMessage& msg);
  EventFnMap eventFnMap_;
};

class EventManager
{
public:
  static void Initialize();
  static void Cleanup();

  void Subscribe(EventReceiver& e, const std::string &name);
  void Unsubscribe(EventReceiver& e);
  bool IsSubscribed(EventReceiver& e, const std::string &name);

  /* broadcast event to whole subscriber of it. */
  //static void SendEvent(int event_id);
  void SendEvent(const std::string& event_name);
  void SendEvent(const EventMessage &msg);
  void SendEvent(EventMessage &&msg);

  /* Peek status of input id */
  int GetStatus(unsigned input_id) const;

  /* Flush all events for each rendering. */
  void Flush();

private:
  EventManager();
  ~EventManager();

  /* subscribers are stored in it */
  std::map<std::string, std::set<EventReceiver*> > event_subscribers_;

  int current_evtidx_;

  friend class EventReceiver;
};

/* @brief simple utility to use delayed events */
class QueuedEventCache
{
public:
  QueuedEventCache();
  void QueueEvent(const std::string &name, float timeout_msec);
  float GetRemainingTime(const std::string& queue_name);
  bool IsQueued(const std::string& queue_name);
  void Update(float delta);
  void SetQueueable(bool allow_queue);

  /* @brief process all queued events regardless of remaining time */
  void FlushAll();

  void DeleteAll();
private:
  struct QueuedEvent
  {
    std::string name;   // event to trigger
    float time_delta;   // remain time of this event
  };
  std::list<QueuedEvent> events_;
  bool allow_queue_;
};

extern EventManager *EVENTMAN;

}