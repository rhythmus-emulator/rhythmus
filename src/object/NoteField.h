#pragma once

#include "Sprite.h"

namespace rhythmus
{

class NoteField : public Sprite
{
public:
  NoteField();
  virtual ~NoteField();

  void set_player_id(int player_id);
  void set_track_idx(int track_idx);

  virtual void doUpdate(float delta);
  virtual void doRender();

private:
  int player_id_;
  int track_idx_;

  struct NoteRenderContext
  {
    VertexInfo vi[4];
  };

  std::vector<NoteRenderContext> note_render_ctx_;
};

}