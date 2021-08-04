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

}