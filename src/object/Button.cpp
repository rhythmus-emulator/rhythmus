#include "Button.h"
#include "Util.h"
#include "Script.h"
#include "config.h"
#include "ResourceManager.h"  /* SystemFont */
#include "Font.h" /* DrawText */
#include <sstream>

namespace rhythmus
{

Button::Button() {}

Button::~Button() {}

void Button::Load(const MetricGroup &metric)
{
  Sprite::Load(metric);
}

const char* Button::type() const { return "Button"; }

void Button::doRender()
{
  Sprite::doRender();
}

// ------------------------------------------------------------------ Loader/Helper

class LR2CSVButtonHandlers
{
public:
  static bool src_button(Button* const& o, LR2CSVExecutor *loader, LR2CSVContext *ctx)
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

    return true;
  }
  LR2CSVButtonHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BUTTON", (LR2CSVHandlerFunc)&src_button);
    LR2CSVExecutor::AddTrigger("#SRC_BUTTON", "#SRC_IMAGE");
    LR2CSVExecutor::AddTrigger("#DST_BUTTON", "#DST_BASE_");
  }
};

// register handler
LR2CSVButtonHandlers _LR2CSVButtonHandlers;

}