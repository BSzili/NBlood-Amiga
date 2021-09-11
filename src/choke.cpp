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
#include "build.h"
#include "compat.h"
#include "common_game.h"
#include "blood.h"
#include "choke.h"
#include "globals.h"
#include "levels.h"
#include "player.h"
#include "qav.h"
#include "resource.h"


void CChoke::sub_83F54(char *a1, int _x, int _y, void (*a2)(PLAYER*))
{
    at14 = _x;
    at18 = _y;
    at0 = a1;
    at1c = a2;
    if (!at4 && at0)
    {
        at4 = gSysRes.Lookup(at0, "QAV");
        if (!at4)
            ThrowError("Could not load QAV %s\n", at0);
        at8 = (QAV*)gSysRes.Lock(at4);
        at8->nSprite = -1;
        at8->x = at14;
        at8->y = at18;
        at8->Preload();
        sub_84218();
    }
}

void CChoke::sub_83ff0(int a1, void(*a2)(PLAYER*))
{
    at0 = NULL;
    at1c = a2;
    if (!at4 && a1 != -1)
    {
        at4 = gSysRes.Lookup(a1, "QAV");
        if (!at4)
            ThrowError("Could not load QAV %d\n", a1);
        at8 = (QAV*)gSysRes.Lock(at4);
        at8->nSprite = -1;
        at8->x = at14;
        at8->y = at18;
        at8->Preload();
        sub_84218();
    }
}

void CChoke::sub_84080(char *a1, void(*a2)(PLAYER*))
{
    at0 = a1;
    at1c = a2;
    if (!at4 && at0)
    {
        at4 = gSysRes.Lookup(at0, "QAV");
        if (!at4)
            ThrowError("Could not load QAV %s\n", at0);
        at8 = (QAV*)gSysRes.Lock(at4);
        at8->nSprite = -1;
        at8->x = at14;
        at8->y = at18;
        at8->Preload();
        sub_84218();
    }
}

void CChoke::sub_84110(int x, int y)
{
    if (!at4)
        return;
    ClockTicks v4 = gFrameClock;
    gFrameClock = totalclock;
    at8->x = x;
    at8->y = y;
    int vd = (int)totalclock-at10;
    at10 = (int)totalclock;
    atc -= vd;
    if (atc <= 0 || atc > at8->at10)
        atc = at8->at10;
    int vdi = at8->at10-atc;
    at8->Play(vdi-vd, vdi, -1, NULL);
#ifndef EDUKE32
    int vb = windowx1;
    int v10 = windowy1;
    int vc = windowx2;
    int v8 = windowy2;
    windowx1 = windowy1 = 0;
    windowx2 = xdim-1;
    windowy2 = ydim-1;
#else
    int vb = windowxy1.x;
    int v10 = windowxy1.y;
    int vc = windowxy2.x;
    int v8 = windowxy2.y;
    windowxy1.x = windowxy1.y = 0;
    windowxy2.x = xdim-1;
    windowxy2.y = ydim-1;
#endif
    at8->Draw(vdi, 10, 0, 0);
#ifndef EDUKE32
    windowx1 = vb;
    windowy1 = v10;
    windowx2 = vc;
    windowy2 = v8;
#else
    windowxy1.x = vb;
    windowxy1.y = v10;
    windowxy2.x = vc;
    windowxy2.y = v8;
#endif
    gFrameClock = v4;
}

void CChoke::sub_84218()
{
    atc = at8->at10;
    at10 = (int)totalclock;
}

void sub_84230(PLAYER *pPlayer)
{
    int t = gGameOptions.nDifficulty+2;
    if (pPlayer->handTime < 64)
        pPlayer->handTime = ClipHigh(pPlayer->handTime+t, 64);
    if (pPlayer->handTime > (7-gGameOptions.nDifficulty)*5)
        pPlayer->blindEffect = ClipHigh(pPlayer->blindEffect+t*4, 128);
}

CChoke gChoke;