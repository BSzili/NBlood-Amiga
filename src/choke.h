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
#pragma once

#include "common_game.h"
#include "player.h"
#include "qav.h"
#include "resource.h"

class CChoke
{
public:
    CChoke()
    {
        at0 = NULL;
        at4 = NULL;
        at8 = NULL;
        atc = 0;
        at10 = 0;
        at1c = NULL;
        at14 = 0;
        at18 = 0;
    };
    void sub_83F54(char *a1, int _x, int _y, void(*a2)(PLAYER*));
    void sub_83ff0(int a1, void(*a2)(PLAYER*));
    void sub_84080(char *a1, void(*a2)(PLAYER*));
    void sub_84110(int x, int y);
    void sub_84218();
    char *at0;
    DICTNODE *at4;
    QAV *at8;
    int atc;
    int at10;
    int at14;
    int at18;
    void(*at1c)(PLAYER *);
};

void sub_84230(PLAYER*);

extern CChoke gChoke;
