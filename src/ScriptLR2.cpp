#include "Script.h"
#include "Logger.h"

// objects
#include "BaseObject.h"
#include "Sprite.h"
#include "object/Text.h"
#include "object/Number.h"
#include "object/Button.h"
#include "object/Slider.h"
#include "object/Bargraph.h"
#include "object/OnMouse.h"

#include "scene/SelectScene.h"
#include "object/MusicWheel.h"

namespace rhythmus
{

// ---------------------------------------------------------------------- LR2Object

namespace LR2Object
{
  BaseObject* Create(LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    BaseObject* o = nullptr;
    const std::string& type = ctx->get_str(0);
    // create new object
    if (type == "#SRC_IMAGE") {
      o = BaseObject::CreateObject("sprite");
      loader->set_object("#DST_IMAGE", o);
    }
    else if (type == "#SRC_TEXT") {
      o = BaseObject::CreateObject("text");
      loader->set_object("#DST_TEXT", o);
    }
    else if (type == "#SRC_NUMBER") {
      o = BaseObject::CreateObject("number");
      loader->set_object("#DST_NUMBER", o);
    }
    else if (type == "#SRC_SLIDER") {
      o = BaseObject::CreateObject("slider");
      loader->set_object("#DST_SLIDER", o);
    }
    else if (type == "#SRC_BARGRAPH") {
      o = BaseObject::CreateObject("bargraph");
      loader->set_object("#DST_BARGRAPH", o);
    }
    else if (type == "#SRC_ONMOUSE") {
      o = BaseObject::CreateObject("onmouse");
      loader->set_object("#DST_ONMOUSE", o);
    }
    else if (type == "#SRC_BUTTON") {
      o = BaseObject::CreateObject("button");
      loader->set_object("#DST_BUTTON", o);
    }
    else if (type == "#BAR_") {
      /* TODO: starts with BAR_ && is_select_scene
       * then fetch MusicWheel if exists.
       * If not exists, then create and get one.
       * and set command to all related objects. */
    }

    if (o == nullptr) {
      Logger::Warn("Invalid command in LR2Object::Create (%s)", type.c_str());
      return nullptr;
    }

    auto* scene = ((BaseObject*)loader->get_object("scene"));
    R_ASSERT(scene != nullptr);
    scene->AddChild(o);

    return o;
  }

  void SRC_OBJECT(BaseObject *o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    // set debug text if exists
    o->SetDebug(format_string("LR2SRC-%u", ctx->get_str(21)));
  }

  void DST_OBJECT(BaseObject* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    const char* args[21];

    for (unsigned i = 0; i < 21; ++i) args[i] = ctx->get_str(i);
    o->AddFrameByLR2command(args + 1);

    // these attributes are only affective for first run
    if (loader->get_command_index() == 0) {
      const int loop = ctx->get_int(16);
      const int timer = ctx->get_int(17);

      // LR2 needs to keep its animation queue, so don't use stop.
      o->AddCommand(format_string("LR%d", timer), "replay");
      o->AddCommand(format_string("LR%dOff", timer), "hide");
      if (loop >= 0)
        o->SetLoop(loop);

      // XXX: move this flag to Sprite,
      // as LR2_TEXT object don't work with this..?
      o->SetVisibleFlag(
        format_string("F%s", ctx->get_str(18)),
        format_string("F%s", ctx->get_str(19)),
        format_string("F%s", ctx->get_str(20)),
        std::string()
      );
    }
  }

  void SRC_IMAGE(Sprite* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    Vector4 r{
      ctx->get_int(3), ctx->get_int(4), ctx->get_int(5), ctx->get_int(6)
    };
    // Use whole image if width/height is zero.
    o->SetImage(format_string("image%s", ctx->get_str(2)));
    if (r.z < 0 || r.w < 0)
      o->SetTextureCoord(Vector4{ 0.0f, 0.0f, 1.0f, 1.0f });
    else
      o->SetImageCoord(Vector4{ r.x, r.y, r.x + r.z, r.y + r.w });
    o->SetAnimatedTexture(ctx->get_int(7), ctx->get_int(8), ctx->get_int(9));
  }

  void DST_IMAGE(Sprite* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    if (loader->get_command_index() == 0) {
      o->SetBlending(ctx->get_int(12));
      o->SetFiltering(ctx->get_int(13));
    }
  }

  void SRC_TEXT(Text* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o->SetFont(format_string("font%s", ctx->get_str(2)));

    /* track change of text table */
    std::string eventname = format_string("Text%s", ctx->get_str(3));
    o->AddCommand(eventname, "refresh");
    o->SubscribeTo(eventname);
    std::string resname = format_string("S%s", ctx->get_str(3));
    o->SetTextResource(resname);
    o->Refresh();   // manually refresh to fill text vertices

    /* alignment */
    const int lr2align = ctx->get_int(4);
    switch (lr2align)
    {
    case 0:
      // topleft
      o->SetTextFitting(TextFitting::kTextFitMaxSize);
      o->SetTextAlignment(0.0f, 0.0f);
      break;
    case 1:
      // topcenter
      o->SetTextFitting(TextFitting::kTextFitMaxSize);
      o->SetTextAlignment(0.5f, 0.0f);
      break;
    case 2:
      // topright
      o->SetTextFitting(TextFitting::kTextFitMaxSize);
      o->SetTextAlignment(1.0f, 0.0f);
      break;
    }
    o->SetLR2StyleText(true);

    /* editable (focusable) */
    if (ctx->get_int(5) > 0)
      o->SetFocusable(true);

    /* TODO: panel */
  }

  void DST_TEXT(Text* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    // these attributes are only affective for first run
    if (loader->get_command_index() == 0)
    {
      const int loop = ctx->get_int(16);
      const int timer = ctx->get_int(17);

      // LR2 needs to keep its animation queue, so don't use stop.
      o->AddCommand(format_string("LR%d", timer), "replay");
      o->AddCommand(format_string("LR%dOff", timer), "hide");
      //o->SetBlending(ctx->get_int(12));
      //o->SetFiltering(ctx->get_int(13));
      if (loop >= 0)
        o->SetLoop(loop);
    }

    // TODO: load blending from LR2DST
    // TODO: fetch font size
  }

  void SRC_NUMBER(Number* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    // (null),(image),(x),(y),(w),(h),(divx),(divy),(cycle),(timer),(num),(align),(keta)
    std::string imgname = std::string("image") + ctx->get_str(2);
    Rect clip(ctx->get_int(3), ctx->get_int(4),
      ctx->get_int(5),
      ctx->get_int(6)); /* x, y, w, h */
    const int divx = ctx->get_int(7);
    const int divy = ctx->get_int(8);
    int digitcount = 10;
    if ((divx * divy) % 24 == 0) digitcount = 24;
    else if ((divx * divy) % 11 == 0) digitcount = 11;

    o->SetGlyphFromImage(imgname, clip, divx, divy, digitcount);

    /* alignment (not use LR2 alignment here) */
    o->SetAlignment(ctx->get_int(12));

    /* digit (keta) */
    o->SetDigit(ctx->get_int(13));

    o->SetLoopCycle(ctx->get_int(9));
    o->SetResizeToBox(true);

    /* track change of number table */
    std::string eventname = format_string("Number%s", ctx->get_str(11));
    o->AddCommand(eventname, "refresh");
    o->SubscribeTo(eventname);
    std::string resname = format_string("N%s", ctx->get_str(11));
    o->SetResourceId(resname);
  }

  void DST_NUMBER(Number* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    // these attributes are only affective for first run
    if (loader->get_command_index() == 0)
    {
      const int timer = ctx->get_int(17);

      // LR2 needs to keep its animation queue, so don't use stop.
      o->AddCommand(format_string("LR%d", timer), "replay");
      o->AddCommand(format_string("LR%dOff", timer), "hide");
      o->SetBlending(ctx->get_int(12));
      //o->SetFiltering(ctx->get_int(13));
    }
  }

  void SRC_BUTTON(Button* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o->SetFocusable(ctx->get_int(11));

    /* XXX: change clickable by panel opening */
    int panel = ctx->get_int(12);
    if (panel >= 0)
    {
      o->AddCommand("Panel" + std::to_string(panel), "focusable:1");
      o->AddCommand("Panel" + std::to_string(panel) + "Off", "focusable:0");
    }

    /**
     * create command to execute when clicked
     * e.g.
     * Click10  : LR2 click event with name 10
     * Click10R : LR2 click event with name 10, reverse.
     */
    std::string minus;
    int button_id = ctx->get_int(10);
    if (ctx->get_int(13) == -1)
      minus = "R";
    o->AddCommand("click",
      format_string("sendevent:Click%d%s", button_id, minus.c_str())
    );
    o->SetResourceId("button" + std::to_string(button_id));
    o->AddCommand(format_string("Number%d", button_id + 1000), "refresh");

    /* Set sprite duration to zero to prevent unexpected sprite animation */
    o->SetDuration(0);
  }

  void DST_BUTTON(Button* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {

  }

  void SRC_SLIDER(Slider* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    // TODO: sprite 'src' attribute
    /* (null),imgname,sx,sy,sw,sh,divx,divy,cycle,timer */

    // only load texture path & texture coord for cursor
    // TODO: set cycle for image
    o->cursor()->SetImage(std::string("image") + ctx->get_str(2));
    o->cursor()->SetImageCoord(Rect{
      ctx->get_float(3), ctx->get_float(4),
      ctx->get_float(3) + ctx->get_float(5),
      ctx->get_float(4) + ctx->get_float(6) });

    int direction = ctx->get_int(11);
    float range = ctx->get_float(12);
    o->SetRange(direction, range);

    /* track change of text table */
    int eventid = ctx->get_int(13) + 1500;
    std::string eventname = "Number" + std::to_string(eventid);
    o->AddCommand(eventname, "refresh");
    o->SubscribeTo(eventname);

    /* set ref value for update */
    o->SetResource("slider" + std::to_string(eventid));
    o->Refresh();

    /* disabled? */
    o->SetEditable(ctx->get_int(14) == 0);
  }

  void DST_SLIDER(Slider* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    // these attributes are only affective for first run
    if (loader->get_command_index() == 0)
    {
      const int timer = ctx->get_int(17);

      // LR2 needs to keep its animation queue, so don't use stop.
      o->AddCommand(format_string("LR%d", timer), "replay");
      o->AddCommand(format_string("LR%dOff", timer), "hide");
      //o->SetBlending(ctx->get_int(12));
      //o->SetFiltering(ctx->get_int(13));

      // XXX: move this flag to Sprite,
      // as LR2_TEXT object don't work with this..?
      o->SetVisibleFlag(
        format_string("F%s", ctx->get_str(18)),
        format_string("F%s", ctx->get_str(19)),
        format_string("F%s", ctx->get_str(20)),
        std::string()
      );
    }
  }

  void SRC_BARGRAPH(Bargraph* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o->SetDirection(ctx->get_int(9));
    std::string resname = "bargraph";
    resname += ctx->get_str(11);
    o->SetResourceId(resname);

    // Load SRC information to bar_ Sprite.
    LR2Object::SRC_OBJECT(o->sprite(), loader, ctx);
    LR2Object::SRC_IMAGE(o->sprite(), loader, ctx);
    o->sprite()->DeleteAllCommand();
  }

  void SRC_ONMOUSE(OnMouse* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    int panel = ctx->get_int(10);
    if (panel > 0 || panel == -1)
    {
      if (panel == -1) panel = 0;
      o->AddCommand("Panel" + std::to_string(panel), "focusable:1");
      o->AddCommand("Panel" + std::to_string(panel) + "Off", "focusable:0");
      o->SetVisibleFlag("", "", "", std::to_string(20 + panel));
    }

    Rect r = Vector4(ctx->get_int(11), ctx->get_int(12),
      ctx->get_int(13), ctx->get_int(14));
    o->SetOnmouseRect(r);
  }
};


class LR2CSVSpriteHandlers
{
public:
  static void src_image(Sprite* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o = (Sprite*)LR2Object::Create(loader, ctx);
    LR2Object::SRC_OBJECT(o, loader, ctx);
    LR2Object::SRC_IMAGE(o, loader, ctx);
  }

  static void dst_image(Sprite* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    R_ASSERT(o != nullptr);
    LR2Object::DST_OBJECT(o, loader, ctx);
    LR2Object::DST_IMAGE(o, loader, ctx);
  }

  LR2CSVSpriteHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_IMAGE", (LR2CSVHandlerFunc)& src_image);
    LR2CSVExecutor::AddHandler("#DST_IMAGE", (LR2CSVHandlerFunc)& dst_image);
  }
};

class LR2CSVTextandlers
{
public:
  static void src_text(Text* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o = (Text*)LR2Object::Create(loader, ctx);
    LR2Object::SRC_OBJECT(o, loader, ctx);
    LR2Object::SRC_TEXT(o, loader, ctx);
  }

  static void dst_text(Text* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    R_ASSERT(o != nullptr);
    LR2Object::DST_OBJECT(o, loader, ctx);
    LR2Object::DST_TEXT(o, loader, ctx);
  }

  LR2CSVTextandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_TEXT", (LR2CSVHandlerFunc)& src_text);
    LR2CSVExecutor::AddHandler("#DST_TEXT", (LR2CSVHandlerFunc)& dst_text);
  }
};

class LR2CSVNumberHandlers
{
public:
  static void src_number(Number* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o = (Number*)LR2Object::Create(loader, ctx);
    LR2Object::SRC_OBJECT(o, loader, ctx);
    LR2Object::SRC_NUMBER(o, loader, ctx);
  }
  static void dst_number(Number* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    R_ASSERT(o != nullptr);
    LR2Object::DST_OBJECT(o, loader, ctx);
    LR2Object::DST_NUMBER(o, loader, ctx);
  }
  LR2CSVNumberHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_NUMBER", (LR2CSVHandlerFunc)& src_number);
    LR2CSVExecutor::AddHandler("#DST_NUMBER", (LR2CSVHandlerFunc)& dst_number);
  }
};


// ------------------------------------------------------------------ Loader/Helper

class LR2CSVButtonHandlers
{
public:
  static void src_button(Button* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o = (Button*)LR2Object::Create(loader, ctx);
    LR2Object::SRC_OBJECT(o, loader, ctx);
    LR2Object::SRC_BUTTON(o, loader, ctx);
  }
  static void dst_button(Button* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    LR2Object::DST_OBJECT(o, loader, ctx);
  }
  LR2CSVButtonHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BUTTON", (LR2CSVHandlerFunc)& src_button);
    LR2CSVExecutor::AddHandler("#DST_BUTTON", (LR2CSVHandlerFunc)& dst_button);
  }
};

class LR2CSVSliderHandlers
{
public:
  static void src_slider(Slider* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o = (Slider*)LR2Object::Create(loader, ctx);
    LR2Object::SRC_OBJECT(o, loader, ctx);
    LR2Object::SRC_SLIDER(o, loader, ctx);
  }

  static void dst_slider(Slider* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    R_ASSERT(o != nullptr);
    LR2Object::DST_OBJECT(o, loader, ctx);
    LR2Object::DST_SLIDER(o, loader, ctx);
  }

  LR2CSVSliderHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_SLIDER", (LR2CSVHandlerFunc)& src_slider);
    LR2CSVExecutor::AddHandler("#DST_SLIDER", (LR2CSVHandlerFunc)& dst_slider);
  }
};

class LR2CSVBargraphHandlers
{
public:
  static void src_bargraph(Bargraph* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o = (Bargraph*)LR2Object::Create(loader, ctx);
    LR2Object::SRC_OBJECT(o, loader, ctx);
    LR2Object::SRC_BARGRAPH(o, loader, ctx);
  }
  static void dst_bargraph(Bargraph* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    LR2Object::DST_OBJECT(o, loader, ctx);
  }
  LR2CSVBargraphHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BARGRAPH", (LR2CSVHandlerFunc)& src_bargraph);
    LR2CSVExecutor::AddHandler("#DST_BARGRAPH", (LR2CSVHandlerFunc)& dst_bargraph);
  }
};

class LR2CSVOnMouseHandlers
{
public:
  static void src_onmouse(OnMouse* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    o = (OnMouse*)LR2Object::Create(loader, ctx);
    LR2Object::SRC_OBJECT(o, loader, ctx);
    LR2Object::SRC_IMAGE(o, loader, ctx);
    LR2Object::SRC_ONMOUSE(o, loader, ctx);
  }
  static void dst_onmouse(OnMouse* o, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    LR2Object::DST_OBJECT(o, loader, ctx);
  }
  LR2CSVOnMouseHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_ONMOUSE", (LR2CSVHandlerFunc)& src_onmouse);
    LR2CSVExecutor::AddHandler("#DST_ONMOUSE", (LR2CSVHandlerFunc)& dst_onmouse);
  }
};

LR2CSVSpriteHandlers _LR2CSVSpriteHandlers;
LR2CSVTextandlers _LR2CSVTextandlers;
LR2CSVNumberHandlers _LR2CSVNumberHandlers;
LR2CSVButtonHandlers _LR2CSVButtonHandlers;
LR2CSVSliderHandlers _LR2CSVSliderHandlers;
LR2CSVBargraphHandlers _LR2CSVBargraphHandlers;
LR2CSVOnMouseHandlers _LR2CSVOnMouseHandlers;


// ---------------------------------------------------- Special handler: MusicWheel

class LR2CSVMusicWheelHandlers
{
public:
  static MusicWheel* get_musicwheel(LR2CSVExecutor* loader)
  {
    auto* wheel = (MusicWheel*)loader->get_object("musicwheel");
    if (loader->get_object("musicwheel") == NULL)
    {
      /* for first-appearence */
      auto* scene = (Scene*)loader->get_object("scene");
      if (!scene) return nullptr;
      wheel = (MusicWheel*)scene->FindChildByName("MusicWheel");
      if (!wheel) return nullptr;
      wheel->InitializeLR2();
      wheel->BringToTop();
      wheel->SetWheelWrapperCount(30, "LR2");
      wheel->SetWheelPosMethod(WheelPosMethod::kMenuPosFixed);
      loader->set_object("musicwheel", wheel);
    }
    return wheel;
  }

  static void src_bar_body(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    auto* wheel = get_musicwheel(loader);
    unsigned bgtype = 0;
    if (!wheel) return;
    bgtype = (unsigned)ctx->get_int(1);
    for (unsigned i = 0; i < wheel->GetMenuItemWrapperCount(); ++i) {
      auto* item = static_cast<MusicWheelItem*>(wheel->GetMenuItemWrapperByIndex(i));
      auto* bg = item->get_background(bgtype);
      LR2Object::SRC_OBJECT(bg, loader, ctx);
      LR2Object::SRC_IMAGE(bg, loader, ctx);
    }
  }

  static void dst_bar_body_off(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    auto* wheel = get_musicwheel(loader);
    unsigned itemindex = 0;
    std::string itemname;
    if (!wheel) return;
    itemindex = (unsigned)ctx->get_int(1);
    itemname = format_string("musicwheelitem%u", itemindex);
    // XXX: Sprite casting is okay?
    auto* item = (Sprite*)wheel->GetMenuItemWrapperByIndex(itemindex);
    LR2Object::DST_OBJECT(item, loader, ctx);
    LR2Object::DST_IMAGE(item, loader, ctx);
  }

  static void dst_bar_body_on(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    // TODO: implement. currently ignored.
  }

  static void bar_center(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    auto* wheel = get_musicwheel(loader);
    unsigned center_idx = 0;

    center_idx = (unsigned)ctx->get_int(1);
    wheel->set_item_center_index(center_idx);
    wheel->set_item_min_index(center_idx);
    wheel->set_item_max_index(center_idx);
  }

  static void bar_available(void*, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    // TODO
    auto* wheel = get_musicwheel(loader);
    loader->set_object("musicwheel", wheel);
    //wheel->set_item_min_index(0);
    //wheel->set_item_max_index((unsigned)ctx->get_int(1));
  }

  static void src_bar_title(void*, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    auto* wheel = get_musicwheel(loader);
    unsigned itemindex = 0;
    itemindex = (unsigned)ctx->get_int(1);
    if (itemindex != 0) return; // TODO: bar title for index 1
    for (unsigned i = 0; i < wheel->GetMenuItemWrapperCount(); ++i) {
      auto* item = (MusicWheelItem*)wheel->GetMenuItemWrapperByIndex(i);
      auto* title = item->get_title();
      LR2Object::SRC_OBJECT(title, loader, ctx);
      LR2Object::SRC_TEXT(title, loader, ctx);
    }
  }

  static void dst_bar_title(void*, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
    auto* wheel = get_musicwheel(loader);
    unsigned itemindex = 0;
    itemindex = (unsigned)ctx->get_int(1);
    if (itemindex != 0) return; // TODO: bar title for index 1
    for (unsigned i = 0; i < wheel->GetMenuItemWrapperCount(); ++i) {
      auto* item = (MusicWheelItem*)wheel->GetMenuItemWrapperByIndex(i);
      auto* title = item->get_title();
      LR2Object::DST_OBJECT(title, loader, ctx);
      LR2Object::DST_TEXT(title, loader, ctx);
    }
  }

  static void src_bar_flash(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void dst_bar_flash(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void src_bar_level(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void dst_bar_level(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void src_bar_lamp(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void dst_bar_lamp(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void src_my_bar_lamp(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void dst_my_bar_lamp(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void src_rival_bar_lamp(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void dst_rival_bar_lamp(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void src_rival_lamp(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  static void dst_rival_lamp(void* _this, LR2CSVExecutor* loader, LR2CSVContext* ctx)
  {
  }

  LR2CSVMusicWheelHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BAR_BODY", (LR2CSVHandlerFunc)& src_bar_body);
    LR2CSVExecutor::AddHandler("#DST_BAR_BODY_OFF", (LR2CSVHandlerFunc)& dst_bar_body_off);
    LR2CSVExecutor::AddHandler("#DST_BAR_BODY_ON", (LR2CSVHandlerFunc)& dst_bar_body_on);
    LR2CSVExecutor::AddHandler("#BAR_CENTER", (LR2CSVHandlerFunc)& bar_center);
    LR2CSVExecutor::AddHandler("#BAR_AVAILABLE", (LR2CSVHandlerFunc)& bar_available);
    LR2CSVExecutor::AddHandler("#SRC_BAR_TITLE", (LR2CSVHandlerFunc)& src_bar_title);
    LR2CSVExecutor::AddHandler("#DST_BAR_TITLE", (LR2CSVHandlerFunc)& dst_bar_title);
    LR2CSVExecutor::AddHandler("#SRC_BAR_FLASH", (LR2CSVHandlerFunc)& src_bar_flash);
    LR2CSVExecutor::AddHandler("#DST_BAR_FLASH", (LR2CSVHandlerFunc)& dst_bar_flash);
    LR2CSVExecutor::AddHandler("#SRC_BAR_LEVEL", (LR2CSVHandlerFunc)& src_bar_level);
    LR2CSVExecutor::AddHandler("#DST_BAR_LEVEL", (LR2CSVHandlerFunc)& dst_bar_level);
    LR2CSVExecutor::AddHandler("#SRC_BAR_LAMP", (LR2CSVHandlerFunc)& src_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_BAR_LAMP", (LR2CSVHandlerFunc)& dst_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_MY_BAR_LAMP", (LR2CSVHandlerFunc)& src_my_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_MY_BAR_LAMP", (LR2CSVHandlerFunc)& dst_my_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_RIVAL_BAR_LAMP", (LR2CSVHandlerFunc)& src_rival_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_RIVAL_BAR_LAMP", (LR2CSVHandlerFunc)& dst_rival_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_RIVAL_LAMP", (LR2CSVHandlerFunc)& src_rival_lamp);
    LR2CSVExecutor::AddHandler("#DST_RIVAL_LAMP", (LR2CSVHandlerFunc)& dst_rival_lamp);
    LR2CSVExecutor::AddHandler("#BAR_CENTER", (LR2CSVHandlerFunc)& bar_center);
  }
};

LR2CSVMusicWheelHandlers _LR2CSVMusicWheelHandlers;


// ------------------------------------------------------------------ LR2SSHandlers
class LR2SSHandlers
{
public:
  static void INFORMATION(const char *path, LR2CSVContext *ctx)
  {
    METRIC->set("SelectSceneBgm", FilePath(path).get());
  }
  LR2SSHandlers()
  {
    LR2SSExecutor::AddHandler("#SELECT", (LR2SSHandlerFunc)INFORMATION);
  }
private:
};

LR2SSHandlers _LR2SSHandlers;

}