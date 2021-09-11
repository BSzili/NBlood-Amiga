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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "build.h"
#include "common_game.h"
#include "blood.h"
#include "fire.h"
#include "globals.h"
#include "misc.h"
#include "tile.h"

int fireSize = 128;
int gDamping = 6;

/*extern "C" */char CoolTable[1024];

void CellularFrame(char *pFrame, int sizeX, int sizeY);

char FrameBuffer[17280];
char SeedBuffer[16][128];
char *gCLU;

void InitSeedBuffers(void)
{
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < fireSize; j += 2)
            SeedBuffer[i][j] = SeedBuffer[i][j+1] = wrand();
}

void BuildCoolTable(void)
{
    for (int i = 0; i < 1024; i++)
        CoolTable[i] = ClipLow((i-gDamping) / 4, 0);
}

void DoFireFrame(void)
{
    int nRand = qrand()&15;
    for (int i = 0; i < 3; i++)
    {
        memcpy(FrameBuffer+16896+i*128, SeedBuffer[nRand], 128);
    }
    CellularFrame(FrameBuffer, 128, 132);
    char *pData = tileLoadTile(2342);
    char *pSource = FrameBuffer;
    int x = fireSize;
    do
    {
        int y = fireSize;
        char *pDataBak = pData;
        do
        {
            *pData = gCLU[*pSource];
            pSource++;
            pData += fireSize;
        } while (--y);
        pData = pDataBak + 1;
    } while (--x);
}

void FireInit(void)
{
    memset(FrameBuffer, 0, sizeof(FrameBuffer));
    BuildCoolTable();
    InitSeedBuffers();
    DICTNODE *pNode = gSysRes.Lookup("RFIRE", "CLU");
    if (!pNode)
        ThrowError("RFIRE.CLU not found");
    gCLU = (char*)gSysRes.Lock(pNode);
    for (int i = 0; i < 100; i++)
        DoFireFrame();
}

void FireProcess(void)
{
    static ClockTicks lastUpdate;
    if (totalclock < lastUpdate || lastUpdate + 2 < totalclock)
    {
        DoFireFrame();
        lastUpdate = totalclock;
        tileInvalidate(2342, -1, -1);
    }
}

void CellularFrame(char *pFrame, int sizeX, int sizeY)
{
    int nSquare = sizeX * sizeY;
    unsigned char *pPtr1 = (unsigned char*)pFrame;
    while (nSquare--)
    {
        unsigned char *pPtr2 = pPtr1+sizeX;
        int sum = *(pPtr2-1) + *pPtr2 + *(pPtr2+1) + *(pPtr2+sizeX);
        if (*(pPtr2+sizeX) > 96)
        {
            pPtr2 += sizeX;
            sum += *(pPtr2-1) + *pPtr2 + *(pPtr2+1) + *(pPtr2+sizeX);
            sum >>= 1;
        }
        *pPtr1 = CoolTable[sum];
        pPtr1++;
    }
}
