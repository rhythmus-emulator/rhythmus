#include "NoteField.h"
#include "Player.h"

namespace rhythmus
{


NoteField::NoteField() : player_id_(0), track_idx_(0)
{
}

NoteField::~NoteField() {}

void NoteField::set_player_id(int player_id) { player_id_ = player_id; }
void NoteField::set_track_idx(int track_idx) { track_idx_ = track_idx; }

void NoteField::doUpdate(float delta)
{
  Player *player = nullptr;
  PlayContext *pctx = nullptr;

  /* clear previous rendering context */
  note_render_ctx_.clear();

  player = Player::getPlayer(player_id_);
  if (!player) return;
  pctx = player->GetPlayContext();
  if (!pctx) return;

  NoteRenderContext nctx;
  nctx.vi[0].Clear();
  nctx.vi[1].Clear();
  nctx.vi[2].Clear();
  nctx.vi[3].Clear();

  auto iter = pctx->GetTrackIterator(track_idx_);
  while (!iter.is_end())
  {
    auto &n = *iter;
    double ny = (1.0 - (pctx->get_measure() - n.measure))
      * get_draw_property().y;

    /* TODO: use beat instead of measure */
    nctx.vi[0].x = get_draw_property().x;
    nctx.vi[0].y = ny;
    nctx.vi[1].x = get_draw_property().x + get_draw_property().w;
    nctx.vi[1].y = ny;
    nctx.vi[2].x = get_draw_property().x + get_draw_property().w;
    nctx.vi[2].y = ny + get_draw_property().h;
    nctx.vi[3].x = get_draw_property().x;
    nctx.vi[3].y = ny + get_draw_property().h;

    note_render_ctx_.push_back(nctx);
    iter.next();
  }
}

void NoteField::doRender()
{
  if (!img_)
    return;

  /* 1. draw longnote body */

  /* 2. draw longnote head / foot */

  /* 3. draw normal note */
  Graphic::SetTextureId(img_->get_texture_ID());
  for (const auto& n : note_render_ctx_)
  {
    memcpy(Graphic::get_vertex_buffer(),
      n.vi,
      sizeof(VertexInfo) * 4);
  }
  Graphic::RenderQuad();

  /* 4. draw mine */
}

}