#include "NoteField.h"
#include "Player.h"
#include "Util.h"

namespace rhythmus
{

/* ---------------------------- class NoteField */

NoteField::NoteField() : player_idx_(0), player_(nullptr) {}

NoteField::~NoteField() {}

void NoteField::set_player(int player_idx)
{
  player_idx_ = player_idx;
}

void NoteField::Load(const Metric &metric)
{
  if (metric.exist("player"))
    player_idx_ = metric.get<int>("player");

  player_ = Player::getPlayer(player_idx_);
  if (!player_) return;
  auto *pctx = player_->GetPlayContext();
  if (!pctx) return;

  for (size_t i = 0; i < pctx->GetTrackCount(); ++i)
  {
    NoteLane *lane;
    lane = new NoteLane(player_idx_, (int)i);
    lane->Load(metric);
    lane->LoadCommandWithNamePrefix(metric);
    lanes_.push_back(lane);
    AddChild(lane);
  }

  BaseObject::Load(metric);
}

void NoteField::doUpdate(float delta)
{
  BaseObject::doUpdate(delta);

  /* refresh note rendering context */
  nctxs_.clear();
  if (player_idx_ >= 0)
  {
    FillNoteRenderContext(player_idx_);
  }
  else {
    Player *player = nullptr;
    FOR_EACH_PLAYER(player, i)
    {
      FillNoteRenderContext(i);
    }
    END_EACH_PLAYER()
  }

  /* post nctx to each lane */
  for (auto *lane : lanes_)
    lane->UpdateNoteRenderContext(nctxs_);
}

void NoteField::FillNoteRenderContext(int player_idx)
{
  Player *player = nullptr;
  PlayContext *pctx = nullptr;
  size_t track_cnt = 0;
  player = Player::getPlayer(player_idx);
  if (!player) return;
  pctx = player->GetPlayContext();
  if (!pctx) return;

  NoteRenderContext nctx;

  track_cnt = pctx->GetTrackCount();
  for (size_t track = 0; track < track_cnt; ++track)
  {
    auto iter = pctx->GetTrackIterator(track);
    while (!iter.is_end())
    {
      auto &n = *iter;
      double ny = (1.0 - (pctx->get_measure() - n.measure));
      //if (ny < 0.0) continue; /* TODO: improve algorithm */
      if (ny > 1.0) break;

      nctx.lane = track;
      nctx.player = player_idx;
      nctx.pos_start = ny;
      nctx.pos_end = 0; /* XXX: longnote */
      nctx.status = 0; /* XXX: not processed, processing, failed note. */

      nctxs_.push_back(nctx);

      iter.next();
    }

    /* XXX: add demo note? */
    nctx.pos_start = 0.4;
    nctxs_.push_back(nctx);
  }
}

/* ----------------------------- class NoteLane */

NoteLane::NoteLane(int player_idx, int track_idx)
  : player_idx_(player_idx), track_idx_(track_idx)
{
  set_name(format_string("Lane%d", track_idx_));
}

NoteLane::~NoteLane() {}

void NoteLane::UpdateNoteRenderContext(std::vector<NoteRenderContext> &nctxs)
{
  nctxs_.clear();
  for (auto &n : nctxs)
  {
    if (n.player == player_idx_ && n.lane == track_idx_)
      nctxs_.push_back(&n);
  }
}

void NoteLane::Load(const Metric &metric)
{
  BaseObject::Load(metric);

  int track_idx = track_idx_;
  /* XXX:
   * in LR2, there is no player indication but player is set by (laneidx / 10).
   * -1 to enable this player auto-detection. */
  //bool is_lr2type_lane = false;
  if (metric.exist("LR2TypeLane") && metric.get<bool>("LR2TypeLane"))
  {
    //is_lr2type_lane = true;
    track_idx = player_idx_ * 10 + track_idx_;
    set_name(format_string("Lane%d", track_idx));
  }

  note_spr_normal_.tap.set_name(format_string("Note%d", track_idx));
  note_spr_normal_.ln_head.set_name(format_string("NoteLnHead%d", track_idx));
  note_spr_normal_.ln_body.set_name(format_string("NoteLnBody%d", track_idx));
  note_spr_normal_.ln_tail.set_name(format_string("NoteLnTail%d", track_idx));
  note_spr_normal_.mine.set_name(format_string("NoteMine%d", track_idx));
  note_spr_auto_.tap.set_name(format_string("ANote%d", track_idx));
  note_spr_auto_.ln_head.set_name(format_string("ANoteLnHead%d", track_idx));
  note_spr_auto_.ln_body.set_name(format_string("ANoteLnBody%d", track_idx));
  note_spr_auto_.ln_tail.set_name(format_string("ANoteLnTail%d", track_idx));

  note_spr_normal_.tap.LoadByName();
  note_spr_normal_.ln_head.LoadByName();
  note_spr_normal_.ln_body.LoadByName();
  note_spr_normal_.ln_tail.LoadByName();
  note_spr_normal_.mine.LoadByName();
  note_spr_auto_.tap.LoadByName();
  note_spr_auto_.ln_head.LoadByName();
  note_spr_auto_.ln_body.LoadByName();
  note_spr_auto_.ln_tail.LoadByName();

  /* for sprite update (animated sprite) */
  AddChild(&note_spr_normal_.tap);
  AddChild(&note_spr_normal_.ln_head);
  AddChild(&note_spr_normal_.ln_body);
  AddChild(&note_spr_normal_.ln_tail);
  AddChild(&note_spr_normal_.mine);
  AddChild(&note_spr_auto_.tap);
  AddChild(&note_spr_auto_.ln_head);
  AddChild(&note_spr_auto_.ln_body);
  AddChild(&note_spr_auto_.ln_tail);
}

void NoteLane::doRender()
{
  /* 1. draw longnote body */

  /* 2. draw longnote head / foot */

  /* 3. draw normal note */
  for (auto *n : nctxs_)
  {
    note_spr_normal_.tap.SetPos(0, (-n->pos_start) * current_prop_.pi.y);
    note_spr_normal_.tap.Render();
  }

  /* 4. draw mine */
}

}