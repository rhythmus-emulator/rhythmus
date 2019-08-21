#include "LR2Font.h"
#include "exdxa.h"
#include <iostream>
#include <string>
#include <map>
#include <FreeImage.h>
#include "LR2JIS.h" // customized SHIFTJIS table for ease of use.
#include "rutil.h"    // string modification function

namespace rhythmus
{

LR2Font::LR2Font()
{
  is_ttf_font_ = false; /* overwrite */
}

LR2Font::~LR2Font() {}

void LR2Font::ReadLR2Font(const char* path)
{
  ClearGlyph();
  ReleaseFont();

  DXAExtractor dxa;
  if (!dxa.Open(path))
  {
    std::cerr << "LR2Font : Cannot open file " << path << std::endl;
    return;
  }

  std::map<std::string, const DXAExtractor::DXAFile*> dxa_file_mapper;
  const DXAExtractor::DXAFile* dxa_file_lr2font;
  for (const auto& f : dxa)
  {
    // make file mapper & find font file (LR2FONT)
    std::string fn = f.filename;
    if (fn.size() > 2 && fn[0] == '.' && fn[1] == '/')
      fn = fn.substr(2);
    dxa_file_mapper[rutil::upper(fn)] = &f;
    if (rutil::upper(rutil::GetExtension(fn)) == "LR2FONT")
      dxa_file_lr2font = &f;
  }

  if (!dxa_file_lr2font)
  {
    std::cerr << "LR2Font : Invalid dxa file (no font info found) " << path << std::endl;
    return;
  }

  // -- parse LR2FONT file --

  const char *p, *p_old, *p_end;
  p = p_old = dxa_file_lr2font->p;
  p_end = p + dxa_file_lr2font->len;
  std::vector<std::string> col;
  while (p < p_end)
  {
    while (p < p_end && *p != '\n' && *p != '\r')
      ++p;
    std::string cmd(p_old, p - p_old);
    while (*p == '\n' || *p == '\r') ++p;
    p_old = p;

    if (cmd.size() <= 2) continue;
    if (cmd[0] == '/' && cmd[1] == '/') continue;
    rutil::split(cmd, ',', col);
    if (col[0] == "#S")
    {
      fontattr_.height = atoi(col[1].c_str());
      fontattr_.baseline_offset = fontattr_.height;
    }
    else if (col[0] == "#M")
    {
      // XXX: I don't know what is this ...
    }
    else if (col[0] == "#T")
    {
      const auto* f = dxa_file_mapper[rutil::upper(col[2])];
      if (!f)
      {
        std::cerr << "LR2Font : Cannot find texture " << col[1] <<
          " while opening " << path << std::endl;
        return;
      }
      UploadTextureFile(f->p, f->len);
    }
    else if (col[0] == "#R")
    {
      /* glyphnum, texid, x, y, width, height */
      FontGlyph g;
      g.codepoint = atoi(col[1].c_str());
      g.codepoint = ConvertLR2JIStoUTF16(g.codepoint);
      g.texidx = atoi(col[2].c_str());
      g.srcx = atoi(col[3].c_str());
      g.srcy = atoi(col[4].c_str());
      g.width = atoi(col[5].c_str());
      g.height = atoi(col[6].c_str());
      g.adv_x = g.width;
      g.pos_x = 0;
      g.pos_y = g.height;

      /* in case of unexpected texture */
      if (g.texidx >= fontbitmap_.size())
        continue;

      /* need to calculate src pos by texture, so need to get texture. */
      float tex_width = fontbitmap_[g.texidx]->width();
      float tex_height = fontbitmap_[g.texidx]->height();
      int texid_real = fontbitmap_[g.texidx]->get_texid();

      /* if width / height is zero, indicates invalid glyph (due to image loading failed) */
      if (tex_width == 0 || tex_height == 0)
        continue;

      g.sx1 = (float)g.srcx / tex_width;
      g.sx2 = (float)(g.srcx + g.width) / tex_width;
      g.sy1 = (float)g.srcy / tex_height;
      g.sy2 = (float)(g.srcy + g.height) / tex_height;
      g.texidx = texid_real; /* change to real one */
      
      glyph_.push_back(g);
    }
  }

  path_ = path;
}

void LR2Font::UploadTextureFile(const char* p, size_t len)
{
  FIMEMORY *memstream = FreeImage_OpenMemory((BYTE*)p, len);
  FREE_IMAGE_FORMAT fmt = FreeImage_GetFileTypeFromMemory(memstream);
  if (fmt == FREE_IMAGE_FORMAT::FIF_UNKNOWN)
  {
    std::cerr << "LR2Font open failed: invalid image internally." << std::endl;
    FreeImage_CloseMemory(memstream);
    return;
  }

  // from here, font bitmap texture must uploaded to memory.
  FIBITMAP *bitmap, *temp;
  temp = FreeImage_LoadFromMemory(fmt, memstream);
  // I don't know why, but image need to be flipped
  FreeImage_FlipVertical(temp);
  bitmap = FreeImage_ConvertTo32Bits(temp);
  FreeImage_Unload(temp);

  int width = FreeImage_GetWidth(bitmap);
  int height = FreeImage_GetHeight(bitmap);
  const uint8_t* data_ptr = FreeImage_GetBits(bitmap);

  FontBitmap *fbitmap = new FontBitmap((const uint32_t*)data_ptr, width, height);
  fontbitmap_.push_back(fbitmap);

  // done, release all FreeImage resource
  FreeImage_Unload(bitmap);
}

// ------------------------------ class LR2Text

LR2Text::LR2Text()
{
  sprite_name_ = "LR2Text";
}

LR2Text::~LR2Text()
{

}

LR2SprInfo& LR2Text::get_sprinfo()
{
  return spr_info_;
}

LR2FontSRC& LR2Text::get_fontsrc()
{
  return (LR2FontSRC&)spr_info_.get_src();
}

void LR2Text::SetSpriteFromLR2Data()
{
  // XXX: (0, 0) SRC size prevent tween not be updated
  // (considered as invalid sprite)
  // So just use dummy value.
  spr_info_.set_src_size(1, 1);
  spr_info_.SetSpriteFromLR2Data(ani_);
}

void LR2Text::Update()
{
  Text::Update();

  // TEMP CODE
  //SetPos(200, 200);

  // hide sprite if currently showing & op(cond) is false
  if (ani_.IsDisplay() && !spr_info_.IsSpriteShow())
    ani_.Hide();
}


void LR2Text::Render()
{
  Text::Render();
}

}
