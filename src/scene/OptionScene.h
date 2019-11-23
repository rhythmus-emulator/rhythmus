#pragma once

#include "Scene.h"
#include "object/OptionMenu.h"

namespace rhythmus
{

class OptionScene : public Scene
{
public:
  OptionScene();
  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene(bool next);
  virtual void ProcessInputEvent(const InputEvent& e);

private:
  OptionMenu menu_;

  // which option menu should built?
  int option_mode_;

  // keyconfig palatte ?
  // TODO - maybe Keyconfig menu?

  void EnterOptionItem();
  void EscapeOptionItem();

  /* @brief Build option items from OptionEntry */
  static void BuildOptionItemData();

  /* @brief Apply changed option to game. */
  void ApplyOptions();
};

}