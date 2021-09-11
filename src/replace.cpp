//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include "compat.h"
#include "build.h"
#include "editor.h"
#include "common_game.h"
#include "crc32.h"

#include "globals.h"
#include "tile.h"
#include "screen.h"

#ifndef EDUKE32
extern "C"
#endif
int qanimateoffs(int a1, int a2)
{
    int offset = 0;
    if (a1 >= 0 && a1 < kMaxTiles)
    {
#ifndef EDUKE32
        int frames = picanm[a1]&63;
#else
        int frames = picanm[a1].num;
#endif
        if (frames > 0)
        {
            int const frameClock = (int)(editstatus ? totalclocklock : gFrameClock);
            int vd;
            if ((a2&0xc000) == 0x8000)
#ifndef EDUKE32
                vd = (crc32once((unsigned char *)&a2, 2)+frameClock)>>((picanm[a1]>>24)&15);
#else
                vd = (Bcrc32(&a2, 2, 0)+frameClock)>>(picanm[a1].sf&PICANM_ANIMSPEED_MASK);
#endif
            else
#ifndef EDUKE32
                vd = frameClock>>((picanm[a1]>>24)&15);
#else
                vd = frameClock>>(picanm[a1].sf&PICANM_ANIMSPEED_MASK);
#endif
#ifndef EDUKE32
            switch (picanm[a1]&192)
#else
            switch (picanm[a1].sf&PICANM_ANIMTYPE_MASK)
#endif
            {
            case PICANM_ANIMTYPE_OSC:
                offset = vd % (2*frames);
                if (offset >= frames)
                    offset = 2*frames-offset;
                break;
            case PICANM_ANIMTYPE_FWD:
                offset = vd % (frames+1);
                break;
            case PICANM_ANIMTYPE_BACK:
                offset = -(vd % (frames+1));
                break;
            }
        }
    }
    return offset;
}

#ifndef EDUKE32
extern "C"
#endif
void qloadpalette()
{
    scrLoadPalette();
}

#ifndef EDUKE32
extern "C"
#endif
int32_t qgetpalookup(int32_t a1, int32_t a2)
{
    if (gFogMode)
        return ClipHigh(a1 >> 8, 15) * 16 + ClipRange(a2, 0, 15);
    else
        return ClipRange((a1 >> 8) + a2, 0, 63);
}

void HookReplaceFunctions(void)
{
#ifdef EDUKE32
    void qinitspritelists();
    int32_t qinsertsprite(int16_t nSector, int16_t nStat);
    int32_t qdeletesprite(int16_t nSprite);
    int32_t qchangespritesect(int16_t nSprite, int16_t nSector);
    int32_t qchangespritestat(int16_t nSprite, int16_t nStatus);
    int32_t qloadboard(const char* filename, char flags, vec3_t* dapos, int16_t* daang, int16_t* dacursectnum);
    int32_t qsaveboard(const char* filename, const vec3_t* dapos, int16_t daang, int16_t dacursectnum);
    animateoffs_replace = qanimateoffs;//
    paletteLoadFromDisk_replace = qloadpalette;//
    getpalookup_replace = qgetpalookup;//
    initspritelists_replace = qinitspritelists;//
    insertsprite_replace = qinsertsprite;//
    deletesprite_replace = qdeletesprite;//
    changespritesect_replace = qchangespritesect;//
    changespritestat_replace = qchangespritestat;//
    loadvoxel_replace = qloadvoxel;//
    loadboard_replace = qloadboard;//
    saveboard_replace = qsaveboard; // unused
    bloodhack = true;
#endif
}
