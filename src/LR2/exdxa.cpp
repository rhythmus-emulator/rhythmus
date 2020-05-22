// exdxa.cpp, v1.01 2012/02/22
// coded by asmodean
// modified / modulized by @lazykuna

// contact: 
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// This tool extracts *.dxa archives.

#include "exdxa.h"
#include "common.h"
#include <stdio.h>
#include <string>
using namespace std;

#define DXA_VERSION 1
//#define DXA_VERSION 2

// ULONG MUST be 4 bytes
#ifdef WIN32
#define ULONG unsigned long
#else
#define ULONG unsigned int
#endif

struct DXAHDR {
  unsigned char signature[4]; // "DX\x03\x00"
  ULONG toc_length;
  ULONG unknown1;
  ULONG toc_offset;
  ULONG entries_offset;
  ULONG directories_offset;
#if DXA_VERSION >= 2
  ULONG unknown2;
#endif
};

struct DXADIRENTRY {
  ULONG unknown1;
  ULONG unknown2;
  ULONG entry_count;
  ULONG entries_offset;
};

struct DXAENTRY {
  ULONG filename_offset;
  ULONG unknown1;
  ULONG unknown2;
  ULONG unknown3;
  ULONG unknown4;
  ULONG unknown5;
  ULONG unknown6;
  ULONG unknown7;
  ULONG offset;
  ULONG original_length;
  ULONG length;
};

#pragma pack(1)
  struct DXACOMPHDR {
    ULONG original_length;
    ULONG length;
    unsigned char escape;
  };
#pragma pack()  

void unobfuscate(ULONG  offset, 
                 unsigned char* buff, 
                 ULONG  len,
                 char*          seed)
{
  static const ULONG KEY_LEN = 12;

  unsigned char key[KEY_LEN] = { 0 };

  if (seed) {
    unsigned long seed_len = strlen(seed);

    for (ULONG i = 0; i < KEY_LEN; i++) {
      key[i] = seed[i % seed_len];
    }
  } else {
    memset(key, 0xAA, KEY_LEN);
  }

  key[0]  ^= 0xFF;
  key[1]   = (key[1] << 4) | (key[1] >> 4);
  key[2]  ^= 0x8A;
  key[3]   = ~((key[3] << 4) | (key[3] >> 4));
  key[4]  ^= 0xFF;
  key[5]  ^= 0xAC;
  key[6]  ^= 0xFF;
  key[7]   = ~((key[7] << 5) | (key[7] >> 3));
  key[8]   = (key[8] << 3) | (key[8] >> 5);
  key[9]  ^= 0x7F;
  key[10]  = ((key[10] << 4) | (key[10] >> 4)) ^ 0xD6;
  key[11] ^= 0xCC;

  for (ULONG i = 0; i < len; i++) {
    buff[i] ^= key[(offset + i) % KEY_LEN];
  }
}

void read_unobfuscate(FILE*         fp, 
                      ULONG offset, 
                      void*         buff, 
                      ULONG len,
                      char*         seed)
{
  fseek(fp, offset, SEEK_SET);
  fread(buff, 1, len, fp);

  unobfuscate(offset, (unsigned char*) buff, len, seed);
}

void uncompress(unsigned char*  buff,
                ULONG   len,
                unsigned char*& out_buff,
                ULONG&  out_len)
{
  DXACOMPHDR* hdr = (DXACOMPHDR*) buff;

  unsigned char* end = buff + hdr->length;

  buff += sizeof(*hdr);

  out_len  = hdr->original_length;
  out_buff = new unsigned char[out_len];

  unsigned char* out_p   = out_buff;
  unsigned char* out_end = out_buff + out_len;

  while (buff < end) {
    while (buff < end) {
      unsigned char c = *buff++;

      if (c == hdr->escape) {
        c = *buff++;

        if (c == hdr->escape) {
          break;
        }

        if (c > hdr->escape) {
          c--;
        }

        ULONG n = c >> 3;

        if (c & 4) {
          n |= *buff++ << 5;
        }

        n += 4;

        ULONG p = 0;

        switch (c & 3) {
        case 0:
          p = *buff++;
          break;

        case 1:
          p = *(unsigned short*)buff;
          buff += 2;
          break;

        case 2:
          p = *(unsigned short*)buff;
          buff += 2;
          p |= *buff++ << 16;
          break;

        case 3:
          p = 0; // unused case?
        }

        p++;

        unsigned char* src = out_p - p;

        while (n--) {
          *out_p++ = *src++;
        }

      } else {
        *out_p++ = c;
      }
    }

    if (out_p < out_end) {
      *out_p++ = hdr->escape;
    }
  }
}

#if 0
void process_dir(FILE*          fp,
                 DXAHDR&        hdr,
                 unsigned char* toc_buff, 
                 ULONG  dir_offset,
                 const string&  prefix,
                 char*          seed)
{
  char*        filenames = (char*) toc_buff;
  DXADIRENTRY* dir       = (DXADIRENTRY*) (toc_buff + hdr.directories_offset + dir_offset);
  DXAENTRY*    entries   = (DXAENTRY*) (toc_buff + hdr.entries_offset + dir->entries_offset);

  ULONG base_offset = sizeof(hdr);

  for (ULONG i = 0; i < dir->entry_count; i++) {
    // Not really sure what all the crap and duplicate strings are in the filenames
    char* filename = filenames + entries[i].filename_offset + 4;

    string out_filename = prefix + "/" + filename;

    if (entries[i].original_length) {
      if (entries[i].length != -1) {
        ULONG  len  = entries[i].length;
        unsigned char* buff = new unsigned char[len];    
        read_unobfuscate(fp, base_offset + entries[i].offset, buff, len, seed);

        ULONG  out_len  = 0;
        unsigned char* out_buff = NULL;
        uncompress(buff, len, out_buff, out_len);

        as::make_path(out_filename);
        as::write_file(out_filename, out_buff, out_len);

        delete [] out_buff;
        delete [] buff;
      } else {
        ULONG  len  = entries[i].original_length;
        unsigned char* buff = new unsigned char[len];    
        read_unobfuscate(fp, base_offset + entries[i].offset, buff, len, seed);

        as::make_path(out_filename);
        as::write_file(out_filename, buff, len);

        delete [] buff;
      }
    } else {
      if (entries[i].offset) {
        process_dir(fp, hdr, toc_buff, entries[i].offset, out_filename, seed);
      }
    }
  }

}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "exdxa v1.01 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.dxa|input.bin> [key]\n", argv[0]);
    return -1;
  }

  string in_filename(argv[1]);

  char* seed = NULL;
  if (argc > 2) seed = argv[2];

  FILE *fp = fopen(in_filename.c_str(), "r");
  if (!fp) return -1;

  DXAHDR hdr;
  read_unobfuscate(fp, 0, &hdr, sizeof(hdr), seed);

  if (memcmp(hdr.signature, "DX", 2)) {
    fprintf(stderr, "%s: unrecognized archive (incorrect key?)\n", in_filename.c_str());
    return -1;
  }

  ULONG  toc_len  = hdr.toc_length;
  unsigned char* toc_buff = new unsigned char[toc_len];
  read_unobfuscate(fp, hdr.toc_offset, toc_buff, toc_len, seed);

  process_dir(fp, hdr, toc_buff, 0, ".", seed);

  delete [] toc_buff;

  return 0;
}
#endif

void process_dir_vec
                (FILE*          fp,
                 DXAHDR&        hdr,
                 unsigned char* toc_buff,
                 ULONG  dir_offset,
                 const string&  prefix,
                 char*          seed,
                 std::vector<DXAExtractor::DXAFile> &files)
{
  char*        filenames = (char*)toc_buff;
  DXADIRENTRY* dir = (DXADIRENTRY*)(toc_buff + hdr.directories_offset + dir_offset);
  DXAENTRY*    entries = (DXAENTRY*)(toc_buff + hdr.entries_offset + dir->entries_offset);

  ULONG base_offset = sizeof(hdr);

  for (ULONG i = 0; i < dir->entry_count; i++) {
    // Not really sure what all the crap and duplicate strings are in the filenames
    char* filename = filenames + entries[i].filename_offset + 4;

    string out_filename = prefix + "/" + filename;
    DXAExtractor::DXAFile dxafile;

    if (entries[i].original_length) {
      if (entries[i].length != -1) {
        ULONG  len = entries[i].length;
        unsigned char* buff = new unsigned char[len];
        read_unobfuscate(fp, base_offset + entries[i].offset, buff, len, seed);

        ULONG  out_len = 0;
        unsigned char* out_buff = NULL;
        uncompress(buff, len, out_buff, out_len);

        dxafile.p = (char*)malloc(out_len);
        memcpy(dxafile.p, out_buff, out_len);
        dxafile.filename = out_filename;
        dxafile.len = out_len;
        files.push_back(dxafile);

        delete[] out_buff;
        delete[] buff;
      }
      else {
        ULONG  len = entries[i].original_length;
        unsigned char* buff = new unsigned char[len];
        read_unobfuscate(fp, base_offset + entries[i].offset, buff, len, seed);

        dxafile.p = (char*)malloc(len);
        memcpy(dxafile.p, buff, len);
        dxafile.filename = out_filename;
        dxafile.len = len;
        files.push_back(dxafile);

        delete[] buff;
      }
    }
    else {
      if (entries[i].offset) {
        process_dir_vec(fp, hdr, toc_buff, entries[i].offset, out_filename, seed, files);
      }
    }
  }

}


// ------------------------- class DXAExtractor

#include "rparser.h"  // to use utf-8 filename (rutil)

DXAExtractor::DXAExtractor()
{
}

DXAExtractor::~DXAExtractor()
{
  Close();
}

bool DXAExtractor::Open(const char* path)
{
  Close();

  FILE *fp = rutil::fopen_utf8(path, "rb");
  if (!fp) return false;

  char* seed = NULL;
  DXAHDR hdr;
  read_unobfuscate(fp, 0, &hdr, sizeof(hdr), seed);

  if (memcmp(hdr.signature, "DX", 2)) {
    fprintf(stderr, "EXDXA %s: unrecognized archive (incorrect key?)\n", path);
    return false;
  }

  ULONG  toc_len = hdr.toc_length;
  unsigned char* toc_buff = new unsigned char[toc_len];
  read_unobfuscate(fp, hdr.toc_offset, toc_buff, toc_len, seed);

  process_dir_vec(fp, hdr, toc_buff, 0, ".", seed, files_);

  delete[] toc_buff;

  return true;
}

void DXAExtractor::Close()
{
  for (auto &dxa : files_)
  {
    free(dxa.p);
  }
  files_.clear();
}
