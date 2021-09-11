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
#include "compat.h"
#include "build.h"
#include "palette.h"
#include "common_game.h"
#include "controls.h"
#include "messages.h"
#include "player.h"

enum VIEW_EFFECT {
    kViewEffectShadow = 0,
    kViewEffectFlareHalo,
    kViewEffectCeilGlow,
    kViewEffectFloorGlow,
    kViewEffectTorchHigh,
    kViewEffectTorchLow,
    kViewEffectSmokeHigh,
    kViewEffectSmokeLow,
    kViewEffectFlame,
    kViewEffectSpear,
    kViewEffectTrail,
    kViewEffectPhase,
    kViewEffectShowWeapon,
    kViewEffectReflectiveBall,
    kViewEffectShoot,
    kViewEffectTesla,
    kViewEffectFlag,
    kViewEffectBigFlag,
    kViewEffectAtom,
#ifdef NOONE_EXTENSIONS
    kViewEffectSpotProgress,
#endif
};

enum VIEWPOS {
    VIEWPOS_0 = 0,
    VIEWPOS_1
};

enum INTERPOLATE_TYPE {
    INTERPOLATE_TYPE_INT = 0,
    INTERPOLATE_TYPE_SHORT,
};

#define CROSSHAIR_PAL (MAXPALOOKUPS-RESERVEDPALS-1)
#define kCrosshairTile 2319
#define kLoadScreen 2049
#define kLoadScreenCRC -2051908571
#define kLoadScreenWideBackWidth 256
#define kLoadScreenWideSideWidth 128
#define kLoadScreenWideBack 9216
#define kLoadScreenWideLeft 9217
#define kLoadScreenWideRight 9218
#define kLoadScreenWideMiddle 9219

#define kFontNum 5

struct FONT {
    int tile, xSize, ySize, space, yoff;
};

extern int gZoom;
extern FONT gFont[kFontNum];
extern int gViewMode;
extern VIEWPOS gViewPos;
extern int gViewIndex;
extern int gScreenTilt;
extern int deliriumTilt, deliriumTurn, deliriumPitch;
extern int gScreenTiltO, deliriumTurnO, deliriumPitchO;
extern int gShowFrameRate;
extern char gInterpolateSprite[];
extern char gInterpolateWall[];
extern char gInterpolateSector[];
extern LOCATION gPrevSpriteLoc[kMaxSprites];
extern int gViewSize;
extern CGameMessageMgr gGameMessageMgr;
extern int gViewXCenter, gViewYCenter;
extern int gViewX0, gViewY0, gViewX1, gViewY1;
extern int gViewX0S, gViewY0S, gViewX1S, gViewY1S;
extern palette_t CrosshairColors;
extern bool g_isAlterDefaultCrosshair;
extern float r_ambientlight, r_ambientlightrecip;
extern int gLastPal;
extern int32_t gShowFps, gFramePeriod;


static inline double calcFrameDelay(unsigned int const maxFPS) { return maxFPS ? timerGetPerformanceFrequency() / (double)maxFPS : 0.0; }

void viewGetFontInfo(int id, const char *unk1, int *pXSize, int *pYSize);
void viewUpdatePages(void);
void viewToggle(int viewMode);
void viewInitializePrediction(void);
void viewUpdatePrediction(GINPUT *pInput);
void sub_158B4(PLAYER *pPlayer);
void fakeProcessInput(PLAYER *pPlayer, GINPUT *pInput);
void fakePlayerProcess(PLAYER *pPlayer, GINPUT *pInput);
void fakeMoveDude(spritetype *pSprite);
void fakeActAirDrag(spritetype *pSprite, int num);
void fakeActProcessSprites(void);
void viewCorrectPrediction(void);
void viewBackupView(int nPlayer);
void viewCorrectViewOffsets(int nPlayer, vec3_t const *oldpos);
void viewClearInterpolations(void);
void viewAddInterpolation(void *data, INTERPOLATE_TYPE type);
void CalcInterpolations(void);
void RestoreInterpolations(void);
void viewDrawText(int nFont, const char *pString, int x, int y, int nShade, int nPalette, int position, char shadow, unsigned int nStat = 0, uint8_t alpha = 0);
void viewTileSprite(int nTile, int nShade, int nPalette, int x1, int y1, int x2, int y2);
void InitStatusBar(void);
void DrawStatSprite(int nTile, int x, int y, int nShade = 0, int nPalette = 0, unsigned int nStat = 0, int nScale = 65536);
void DrawStatMaskedSprite(int nTile, int x, int y, int nShade = 0, int nPalette = 0, unsigned int nStat = 0, int nScale = 65536);
void DrawStatNumber(const char *pFormat, int nNumber, int nTile, int x, int y, int nShade, int nPalette, unsigned int nStat = 0, int nScale = 65536);
void TileHGauge(int nTile, int x, int y, int nMult, int nDiv, int nStat = 0, int nScale = 65536);
void viewDrawPack(PLAYER *pPlayer, int x, int y);
void DrawPackItemInStatusBar(PLAYER *pPlayer, int x, int y, int x2, int y2, int nStat = 0);
void UpdateStatusBar(ClockTicks arg);
void viewInit(void);
void viewResizeView(int size);
void UpdateFrame(void);
void viewDrawInterface(ClockTicks arg);
tspritetype *viewAddEffect(int nTSprite, VIEW_EFFECT nViewEffect);
void viewProcessSprites(int32_t cX, int32_t cY, int32_t cZ, int32_t cA, int32_t smooth);
void CalcOtherPosition(spritetype *pSprite, int *pX, int *pY, int *pZ, int *vsectnum, int nAng, fix16_t zm);
void CalcPosition(spritetype *pSprite, int *pX, int *pY, int *pZ, int *vsectnum, int nAng, int zm);
void viewSetMessage(const char *pMessage, const int pal = 0, const MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL);
void viewDisplayMessage(void);
void viewSetErrorMessage(const char *pMessage);
void DoLensEffect(void);
void UpdateDacs(int nPalette, bool bNoTint = false);
void viewDrawScreen(void);
void viewLoadingScreenWide(void);
void viewLoadingScreenUpdate(const char *pzText4 = NULL, int nPercent = -1);
void viewLoadingScreen(int nTile, const char *pText, const char *pText2, const char *pText3);
void viewUpdateDelirium(void);
void viewUpdateShake(void);
void viewSetCrosshairColor(int32_t r, int32_t g, int32_t b);
void viewResetCrosshairToDefault(void);
void viewPrintFPS(void);
void viewSetSystemMessage(const char* pMessage, ...);
void viewPrecacheTiles(void);

inline void viewInterpolateSector(int nSector, sectortype *pSector)
{
#ifndef EDUKE32
    if (!gViewInterpolate) return;
#endif
    if (!TestBitString(gInterpolateSector, nSector))
    {
        viewAddInterpolation(&pSector->floorz, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pSector->ceilingz, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pSector->floorheinum, INTERPOLATE_TYPE_SHORT);
        SetBitString(gInterpolateSector, nSector);
    }
}

inline void viewInterpolateWall(int nWall, walltype *pWall)
{
#ifndef EDUKE32
    if (!gViewInterpolate) return;
#endif
    if (!TestBitString(gInterpolateWall, nWall))
    {
        viewAddInterpolation(&pWall->x, INTERPOLATE_TYPE_INT);
        viewAddInterpolation(&pWall->y, INTERPOLATE_TYPE_INT);
        SetBitString(gInterpolateWall, nWall);
    }
}

inline void viewBackupSpriteLoc(int nSprite, spritetype *pSprite)
{
#ifndef EDUKE32
    if (!gViewInterpolate) return;
#endif
    if (!TestBitString(gInterpolateSprite, nSprite))
    {
        LOCATION *pPrevLoc = &gPrevSpriteLoc[nSprite];
        pPrevLoc->x = pSprite->x;
        pPrevLoc->y = pSprite->y;
        pPrevLoc->z = pSprite->z;
        pPrevLoc->ang = pSprite->ang;
        SetBitString(gInterpolateSprite, nSprite);
    }
}
