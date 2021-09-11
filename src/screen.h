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


struct LOADITEM {
    int id;
    const char *name;
};

#pragma pack(push, 1)
struct RGB {
    char red, green, blue;
};
#pragma pack(pop)
extern char gStdColor[32];

extern bool DacInvalid;
extern RGB curDAC[256];
extern RGB baseDAC[256];
extern int gGammaLevels;
extern bool gFogMode;
extern int32_t gBrightness;
void scrCreateStdColors(void);
void scrResetPalette(void);
void gSetDacRange(int start, int end, RGB *pPal);
void scrLoadPLUs(void);
void scrLoadPalette(void);
void scrSetPalette(int palId);
void scrSetGamma(int nGamma);
void scrSetupFade(char red, char green, char blue);
void scrSetupUnfade(void);
void scrFadeAmount(int amount);
void scrSetDac(void);
void scrInit(void);
void scrUnInit(void);
void scrSetGameMode(int vidMode, int XRes, int YRes, int nBits);
void scrNextPage(void);