#pragma once

#include "gltexture.h"
#include "egobootypedef.h"

#define MAXTEXTURE                      0x0200 //0x80         // Max number of textures

enum TX_TYPE
{
  TX_PRT = 0,
  TX_TILE0,
  TX_TILE1,
  TX_TILE2,
  TX_TILE3,
  TX_WATERTOP,
  TX_WATERLOW,
  TX_PHONG
};

struct Icon : public GLTexture, public TAllocClientStrict<Icon, MAXTEXTURE+1>
{
};

struct Icon_List : public TAllocListStrict<Icon, MAXTEXTURE+1>
{
  index_t load_one_icon(char *szLoadName, Uint32 force = INVALID);
  void    prime_icons();
  void    release_all_icons();

  rect_t  rect;                   // The 32x32 icon rectangle

  index_t book_icon[4];            // The book icons
  int     book_count;

  index_t null;
  index_t keyb;
  index_t mous;
  index_t joya;
  index_t joyb;
};

typedef Icon_List::index_t ICON_REF;

extern Icon_List IconList;


struct Tx : public GLTexture, public TAllocClientStrict<Tx, MAXTEXTURE>
{
};

struct Tx_List : public TAllocListStrict<Tx, MAXTEXTURE>
{
  index_t load_one(char *szLoadName, Uint32 key, Uint32 force = INVALID);
  void    release_all_textures();
};

typedef Tx_List::index_t TEX_REF;

extern Tx_List TxList;

/* Textures - ported */

extern GLTexture TxTrimX;                                        /* trim */
extern GLTexture TxTrimY;                                        /* trim */
extern GLTexture TxTrim;
extern GLTexture TxBars;          //OpenGL status bar surface
extern GLTexture TxBlip;          //OpenGL you are here surface
extern GLTexture TxMap;          //OpenGL map surface