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
#include <string.h>
#ifndef EDUKE32
#include "compat.h"
#else
#include "a.h"
#endif
#include "build.h"
#ifndef EDUKE32

#else
#include "colmatch.h"
#endif
#include "common_game.h"

#include "globals.h"
#include "config.h"
#include "gfx.h"
#include "resource.h"
#include "screen.h"

RGB StdPal[32] = {
    { 0, 0, 0 },
    { 0, 0, 170 },
    { 0, 170, 170 },
    { 0, 170, 170 },
    { 170, 0, 0 },
    { 170, 0, 170 },
    { 170, 85, 0 },
    { 170, 170, 170 },
    { 85, 85, 85 },
    { 85, 85, 255 },
    { 85, 255, 85 },
    { 85, 255, 255 },
    { 255, 85, 85 },
    { 255, 85, 255 },
    { 255, 255, 85 },
    { 255, 255, 255 },
    { 241, 241, 241 },
    { 226, 226, 226 },
    { 211, 211, 211 },
    { 196, 196, 196 },
    { 181, 181, 181 },
    { 166, 166, 166 },
    { 151, 151, 151 },
    { 136, 136, 136 },
    { 120, 120, 120 },
    { 105, 105, 105 },
    { 90, 90, 90 },
    { 75, 75, 75 },
    { 60, 60, 60 },
    { 45, 45, 45 },
    { 30, 30, 30 },
    { 15, 15, 15 }
};

LOADITEM PLU[15] = {
    { 0, "NORMAL" },
    { 1, "SATURATE" },
    { 2, "BEAST" },
    { 3, "TOMMY" },
    { 4, "SPIDER3" },
    { 5, "GRAY" },
    { 6, "GRAYISH" },
    { 7, "SPIDER1" },
    { 8, "SPIDER2" },
    { 9, "FLAME" },
    { 10, "COLD" },
    { 11, "P1" },
    { 12, "P2" },
    { 13, "P3" },
    { 14, "P4" }
};

LOADITEM PAL[5] = {
    { 0, "BLOOD" },
    { 1, "WATER" },
    { 2, "BEAST" },
    { 3, "SEWER" },
    { 4, "INVULN1" }
};


bool DacInvalid = true;
static char(*gammaTable)[256];
RGB curDAC[256];
RGB baseDAC[256];
static RGB fromDAC[256];
static RGB toRGB;
static RGB *palTable[5];
static int curPalette;
static int curGamma;
int gGammaLevels;
bool gFogMode = false;
char gStdColor[32];
int32_t gBrightness;

char scrFindClosestColor(int red, int green, int blue)
{
    int dist = 0x7fffffff;
    int best;
    for (int i = 0; i < 256; i++)
    {
        int sum = (palette[i*3+1]-green)*(palette[i*3+1]-green);
        if (sum >= dist) continue;
        sum += (palette[i*3+0]-red)*(palette[i*3+0]-red);
        if (sum >= dist) continue;
        sum += (palette[i*3+2]-blue)*(palette[i*3+2]-blue);
        if (sum >= dist) continue;
        best = i;
        dist = sum;
        if (sum == 0)
            break;
    }
    return best;
}

void scrCreateStdColors(void)
{
    for (int i = 0; i < 32; i++)
        gStdColor[i] = scrFindClosestColor(StdPal[i].red, StdPal[i].green, StdPal[i].blue);
}

void scrResetPalette(void)
{
    if (palTable[0] == nullptr)
        return;

    paletteSetColorTable(0, (uint8_t*)palTable[0]);
}

void gSetDacRange(int start, int end, RGB *pPal)
{
    UNREFERENCED_PARAMETER(start);
    UNREFERENCED_PARAMETER(end);
    if (videoGetRenderMode() == REND_CLASSIC)
    {
#ifndef EDUKE32
        paletteSetColorTable(0, (const uint8_t *)pPal);
#else
        memcpy(palette, pPal, sizeof(palette));
#endif
        videoSetPalette(gBrightness>>2, 0, 0);
    }
}

void scrLoadPLUs(void)
{
    if (gFogMode)
    {
        DICTNODE *pFog = gSysRes.Lookup("FOG", "FLU");
        if (!pFog)
            ThrowError("FOG.FLU not found");
#ifndef EDUKE32
        palookup[0] = (unsigned char*)gSysRes.Lock(pFog);
#else
        palookup[0] = (char*)gSysRes.Lock(pFog);
#endif
        for (int i = 0; i < 15; i++)
            palookup[PLU[i].id] = palookup[0];
        parallaxvisibility = 3072;
        return;
    }
    
    // load default palookups
    for (int i = 0; i < 15; i++) {
        DICTNODE *pPlu = gSysRes.Lookup(PLU[i].name, "PLU");
        if (!pPlu)
            ThrowError("%s.PLU not found", PLU[i].name);
        if (pPlu->size / 256 != 64)
            ThrowError("Incorrect PLU size");
#ifndef EDUKE32
        palookup[PLU[i].id] = (unsigned char*)gSysRes.Lock(pPlu);
#else
        palookup[PLU[i].id] = (char*)gSysRes.Lock(pPlu);
#endif
    }

    // by NoOne: load user palookups
    for (int i = kUserPLUStart; i < MAXPALOOKUPS; i++) {
        DICTNODE* pPlu = gSysRes.Lookup(i, "PLU");
        if (!pPlu) continue;
        else if (pPlu->size / 256 != 64) { consoleSysMsg("Incorrect filesize of PLU#%d", i); }
#ifndef EDUKE32
        else palookup[i] = (unsigned char*)gSysRes.Lock(pPlu);
#else
        else palookup[i] = (char*)gSysRes.Lock(pPlu);
#endif
    }

#ifdef USE_OPENGL
    palookupfog[1].r = 255;
    palookupfog[1].g = 255;
    palookupfog[1].b = 255;
#endif
}

#ifdef USE_OPENGL
glblend_t const bloodglblend =
{
    {
        { 1.f/3.f, BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, 0 },
        { 2.f/3.f, BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, 0 },
    },
};
#endif

void scrLoadPalette(void)
{
    paletteInitClosestColorScale(30, 59, 11);
    paletteInitClosestColorGrid();
    paletteloaded = 0;
    initprintf("Loading palettes\n");
    for (int i = 0; i < 5; i++)
    {
        DICTNODE *pPal = gSysRes.Lookup(PAL[i].name, "PAL");
        if (!pPal)
            ThrowError("%s.PAL not found (RFF files may be wrong version)", PAL[i].name);
        palTable[PAL[i].id] = (RGB*)gSysRes.Lock(pPal);
        paletteSetColorTable(PAL[i].id, (uint8_t*)palTable[PAL[i].id]);
    }
#ifndef EDUKE32
    memcpy(palette, basepaltable[PAL[0].id], sizeof(palette));
#else
    memcpy(palette, palTable[0], sizeof(palette));
#endif
    numshades = 64;
    paletteloaded |= PALETTE_MAIN;
    scrLoadPLUs();
    paletteloaded |= PALETTE_SHADE;
    initprintf("Loading translucency table\n");
    DICTNODE *pTrans = gSysRes.Lookup("TRANS", "TLU");
    if (!pTrans)
        ThrowError("TRANS.TLU not found");
#ifndef EDUKE32
    transluc = (unsigned char*)gSysRes.Lock(pTrans);
#else
    blendtable[0] = (char*)gSysRes.Lock(pTrans);
#endif
    paletteloaded |= PALETTE_TRANSLUC;

#ifdef USE_OPENGL
    for (auto & x : glblend)
        x = bloodglblend;

    for (int i = 0; i < MAXPALOOKUPS; i++)
        palookupfogfactor[i] = 1.f;
#endif

    paletteInitClosestColorMap((uint8_t*)palTable[0]);
    palettePostLoadTables();
    // Make color index 255 of palette black.
    for (int i = 0; i < 5; i++)
    {
        if (basepaltable[i] != NULL)
            Bmemset(&basepaltable[i][255 * 3], 0, 3);
    }
    palettePostLoadLookups();
}

void scrSetPalette(int palId)
{
    curPalette = palId;
#ifndef EDUKE32
    scrSetGamma(curGamma);
#else
    scrSetGamma(0/*curGamma*/);
#endif
}

void scrSetGamma(int nGamma)
{
    dassert(nGamma < gGammaLevels);
    curGamma = nGamma;
    for (int i = 0; i < 256; i++)
    {
        baseDAC[i].red = gammaTable[curGamma][palTable[curPalette][i].red];
        baseDAC[i].green = gammaTable[curGamma][palTable[curPalette][i].green];
        baseDAC[i].blue = gammaTable[curGamma][palTable[curPalette][i].blue];
    }
    DacInvalid = 1;
}

void scrSetupFade(char red, char green, char blue)
{
    memcpy(fromDAC, curDAC, sizeof(fromDAC));
    toRGB.red = red;
    toRGB.green = green;
    toRGB.blue = blue;
}

void scrSetupUnfade(void)
{
    memcpy(fromDAC, baseDAC, sizeof(fromDAC));
}

void scrFadeAmount(int amount)
{
	for (int i = 0; i < 256; i++)
	{
		curDAC[i].red = interpolate(fromDAC[i].red, toRGB.red, amount);
        curDAC[i].green = interpolate(fromDAC[i].green, toRGB.green, amount);
        curDAC[i].blue = interpolate(fromDAC[i].blue, toRGB.blue, amount);
	}
	gSetDacRange(0, 256, curDAC);
}

void scrSetDac(void)
{
	if (DacInvalid)
		gSetDacRange(0, 256, baseDAC);
	DacInvalid = 0;
}

void scrInit(void)
{
    initprintf("Initializing engine\n");
#ifdef USE_OPENGL
    glrendmode = REND_POLYMOST;
#endif
#ifdef __AMIGA__
    if (engineInit())
    {
#ifdef WM_MSGBOX_WINDOW
        wm_msgbox("Build Engine Initialization Error",
                  "There was a problem initializing the Build engine: %s", engineerrstr);
#endif
        ERRprintf("app_main: There was a problem initializing the Build engine: %s\n", engineerrstr);
        Bexit(2);
    }
#else
    engineInit();
#endif
    curPalette = 0;
    curGamma = 0;
    initprintf("Loading gamma correction table\n");
    DICTNODE *pGamma = gSysRes.Lookup("gamma", "DAT");
    if (!pGamma)
        ThrowError("Gamma table not found");
    gGammaLevels = pGamma->size / 256;
    gammaTable = (char(*)[256])gSysRes.Lock(pGamma);
}

void scrUnInit(void)
{
    memset(palookup, 0, sizeof(palookup));
#ifndef EDUKE32
    transluc = NULL;
#else
    memset(blendtable, 0, sizeof(blendtable));
#endif
    engineUnInit();
}


void scrSetGameMode(int vidMode, int XRes, int YRes, int nBits)
{
#ifdef EDUKE32
    videoResetMode();
#endif
    //videoSetGameMode(vidMode, XRes, YRes, nBits, 0);
    if (videoSetGameMode(vidMode, XRes, YRes, nBits, 0) < 0)
    {
        initprintf("Failure setting video mode %dx%dx%d %s! Trying next mode...\n", XRes, YRes,
                    nBits, vidMode ? "fullscreen" : "windowed");

#ifdef __AMIGA__
		// TODO check the window/fullscreen counterpart with the same resolution first
		gSetup.fullscreen = 1;
		gSetup.xdim = 320;
		gSetup.ydim = 200;
		gSetup.bpp = 8;
		if (videoSetGameMode(gSetup.fullscreen, gSetup.xdim, gSetup.ydim, gSetup.bpp, 0) < 0)
			ThrowError("Fatal error: unable to set any video mode!");
#else
        int resIdx = 0;

        for (int i=0; i < validmodecnt; i++)
        {
            if (validmode[i].xdim == XRes && validmode[i].ydim == YRes)
            {
                resIdx = i;
                break;
            }
        }

        int const savedIdx = resIdx;
        int bpp = nBits;

        while (videoSetGameMode(0, validmode[resIdx].xdim, validmode[resIdx].ydim, bpp, 0) < 0)
        {
            initprintf("Failure setting video mode %dx%dx%d windowed! Trying next mode...\n",
                        validmode[resIdx].xdim, validmode[resIdx].ydim, bpp);

            if (++resIdx == validmodecnt)
            {
                if (bpp == 8)
                    ThrowError("Fatal error: unable to set any video mode!");

                resIdx = savedIdx;
                bpp = 8;
            }
        }

        gSetup.xdim = validmode[resIdx].xdim;
        gSetup.ydim = validmode[resIdx].ydim;
        gSetup.bpp  = bpp;
#endif
    }
    videoClearViewableArea(0);
    scrNextPage();
    scrSetPalette(curPalette);
    gfxSetClip(0, 0, xdim, ydim);
}

void scrNextPage(void)
{
    videoNextPage();
}
