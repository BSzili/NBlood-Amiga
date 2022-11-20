//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

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


///////////////////////////////////////////////////////////////////
// This file provides modern features for mappers.
// For full documentation please visit http://cruo.bloodgame.ru/xxsystem
///////////////////////////////////////////////////////////////////

#ifndef __NNEXTSIF_H
#define __NNEXTSIF_H
#include "db.h"
#include "eventq.h"

#define CONDITIONS_USE_BUBBLE_ACTION        1


void conditionsInit(bool bSaveLoad);
void conditionsError(XSPRITE* pXSprite, const char* pFormat, ...);
void conditionsTrackingAlloc(bool bSaveLoad);
void conditionsTrackingClear();
void conditionsTrackingProcess();
void conditionsLinkPlayer(XSPRITE* pXCtrl, PLAYER* pPlay);
void conditionsUpdateIndex(int oType, int oldIndex, int newIndex);
void useCondition(spritetype* pSource, XSPRITE* pXSource, EVENT event);

#ifdef CONDITIONS_USE_BUBBLE_ACTION
    void conditionsSetIsLocked(XSPRITE* pXSprite, int nValue);
    void conditionsSetIsTriggered(XSPRITE* pXSprite, int nValue);
    void conditionsBubble(XSPRITE* pXStart, void(*pActionFunc)(XSPRITE*, int), int nValue);
#endif

#endif