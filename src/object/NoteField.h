#pragma once

#include "BaseObject.h"
#include "Sprite.h"

namespace rhythmus
{

struct NoteRenderContext {
  int player;
  int lane;
  double pos_start;
  double pos_end;
  int status;
};

class NoteLane;
class Player;

class NoteField : public BaseObject
{
public:
  NoteField();
  virtual ~NoteField();

  void set_player(int player_idx);
  virtual void Load(const Metric &metric);
  virtual void doUpdate(float delta);

private:
  int player_idx_;
  Player *player_;
  std::vector<NoteLane*> lanes_;
  std::vector<NoteRenderContext> nctxs_;

  void FillNoteRenderContext(int player_idx);
};

class NoteLane : public BaseObject
{
public:
  NoteLane(int player_idx, int track_idx);
  virtual ~NoteLane();
  virtual void Load(const Metric &metric);

  /* @warn
   * This method borrows NoteRenderContext object address from other vector for rendering,
   * So, borrowed vector MUST NOT BE CHANGED before this NoteLane is rendered.
   * (otherwise, it will crash)
   * We can copy value for more safety, but it is not necessary logically... maybe. */
  void UpdateNoteRenderContext(std::vector<NoteRenderContext> &nctxs);
private:
  struct NoteSprite
  {
    Sprite tap;
    Sprite ln_head, ln_body, ln_tail;
    Sprite mine;
  };
  NoteSprite note_spr_normal_;
  NoteSprite note_spr_auto_;
  std::vector<NoteRenderContext*> nctxs_;

  int player_idx_;
  int track_idx_;

  virtual void doRender();
};

}