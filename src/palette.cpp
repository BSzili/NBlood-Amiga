// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#include "palette.h"
#include "baselayer.h"


uint8_t basepaltable[BASEPALCOUNT][768];
int g_lastpalettesum; // TODO lastpalettesum

void paletteSetColorTable(int32_t id, const uint8_t *table)
{
	//Bmemcpy(basepaltable[id], table, 768);
	uint8_t *dpal = basepaltable[id];
	for(int i = 0; i < 768; i++)
	{
		*dpal = *table >> 2;
		dpal++;
		table++;
	}
}

void videoSetPalette(char dabrightness, uint8_t dapalid, uint8_t flags)
{
	setbrightness(dabrightness, basepaltable[dapalid], flags);
}

int32_t videoUpdatePalette(int32_t start, int32_t num)
{
	setpalette(start, num, NULL);
	return 0;
}

int32_t paletteSetLookupTable(int32_t palnum, const uint8_t *shtab)
{
	makepalookup(palnum, NULL, 0, 0, 0, 0); // just allocate it
	if (shtab != NULL)
		Bmemcpy(palookup[palnum], shtab, numpalookups<<8);

	return 0;
}
