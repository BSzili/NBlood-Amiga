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
#include "pragmas.h"
#include "mmulti.h"
#include "common_game.h"

#include "actor.h"
#include "ai.h"
#include "aispid.h"
#include "blood.h"
#include "db.h"
#include "dude.h"
#include "endgame.h"
#include "eventq.h"
#include "levels.h"
#include "player.h"
#include "seq.h"
#include "sfx.h"
#include "trig.h"

static void SpidBiteSeqCallback(int, int);
static void SpidJumpSeqCallback(int, int);
static void SpidBirthSeqCallback(int, int);
static void thinkSearch(spritetype *, XSPRITE *);
static void thinkGoto(spritetype *, XSPRITE *);
static void thinkChase(spritetype *, XSPRITE *);

static int nBiteClient = seqRegisterClient(SpidBiteSeqCallback);
static int nJumpClient = seqRegisterClient(SpidJumpSeqCallback);
static int nBirthClient = seqRegisterClient(SpidBirthSeqCallback);

AISTATE spidIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, aiThinkTarget, NULL };
AISTATE spidChase = { kAiStateChase, 7, -1, 0, NULL, aiMoveForward, thinkChase, NULL };
AISTATE spidDodge = { kAiStateMove, 7, -1, 90, NULL, aiMoveDodge, NULL, &spidChase };
AISTATE spidGoto = { kAiStateMove, 7, -1, 600, NULL, aiMoveForward, thinkGoto, &spidIdle };
AISTATE spidSearch = { kAiStateSearch, 7, -1, 1800, NULL, aiMoveForward, thinkSearch, &spidIdle };
AISTATE spidBite = { kAiStateChase, 6, nBiteClient, 60, NULL, NULL, NULL, &spidChase };
AISTATE spidJump = { kAiStateChase, 8, nJumpClient, 60, NULL, aiMoveForward, NULL, &spidChase };
AISTATE spidBirth = { kAiStateOther, 0, nBirthClient, 60, NULL, NULL, NULL, &spidIdle };

static char SpidPoisonPlayer(XSPRITE *pXDude, int nBlind, int max)
{
    dassert(pXDude != NULL);
    int nDude = pXDude->reference;
    spritetype *pDude = &sprite[nDude];
    if (IsPlayerSprite(pDude))
    {
        nBlind <<= 4;
        max <<= 4;
        PLAYER *pPlayer = &gPlayer[pDude->type-kDudePlayer1];
        if (pPlayer->blindEffect < max)
        {
            pPlayer->blindEffect = ClipHigh(pPlayer->blindEffect+nBlind, max);
            return 1;
        }
    }
    return 0;
}

static void SpidBiteSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    dx += Random2(2000);
    dy += Random2(2000);
    int dz = Random2(2000);
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    if (IsPlayerSprite(pTarget)) {
        
        int hit = HitScan(pSprite, pSprite->z, dx, dy, 0, CLIPMASK1, 0);
        if (hit == 3 && IsPlayerSprite(&sprite[gHitInfo.hitsprite])) {
            dz += pTarget->z - pSprite->z;
            PLAYER *pPlayer = &gPlayer[pTarget->type - kDudePlayer1];
            switch (pSprite->type) {
                case kDudeSpiderBrown:
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                    if (IsPlayerSprite(pTarget) && !pPlayer->godMode && powerupCheck(pPlayer, kPwUpDeathMask) <= 0 && Chance(0x4000))
                        powerupActivate(pPlayer, kPwUpDeliriumShroom);
                    break;
                case kDudeSpiderRed:
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                    if (Chance(0x5000)) SpidPoisonPlayer(pXTarget, 4, 16);
                    break;
                case kDudeSpiderBlack:
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                    SpidPoisonPlayer(pXTarget, 8, 16);
                    break;
                case kDudeSpiderMother: {
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);

                    dx += Random2(2000);
                    dy += Random2(2000);
                    dz += Random2(2000);
                    actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorSpiderBite);
                    SpidPoisonPlayer(pXTarget, 8, 16);
                    break;
                }
            }
        }

    }
}

static void SpidJumpSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    int dx = Cos(pSprite->ang)>>16;
    int dy = Sin(pSprite->ang)>>16;
    dx += Random2(200);
    dy += Random2(200);
    int dz = Random2(200);
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    if (IsPlayerSprite(pTarget)) {
        dz += pTarget->z-pSprite->z;
        switch (pSprite->type) {
            case kDudeSpiderBrown:
            case kDudeSpiderRed:
            case kDudeSpiderBlack:
                xvel[nSprite] = dx << 16;
                yvel[nSprite] = dy << 16;
                zvel[nSprite] = dz << 16;
                break;
        }
    }
}

static void SpidBirthSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    DUDEEXTRA_STATS *pDudeExtraE = &gDudeExtra[pSprite->extra].stats;
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    
    spritetype *pSpawn = NULL;
    if (IsPlayerSprite(pTarget) && pDudeExtraE->birthCounter < 10) {
        if (klabs(pSprite->ang-nAngle) < pDudeInfo->periphery) {
            if (nDist < 0x1a00 && nDist > 0x1400)
                pSpawn = actSpawnDude(pSprite, kDudeSpiderRed, pSprite->clipdist, 0);
            else if ((nDist < 0xc00) || (nDist < 0x1400 && nDist > 0xc00))
                pSpawn = actSpawnDude(pSprite, kDudeSpiderBrown, pSprite->clipdist, 0);
        }
        
        if (pSpawn) {
            pDudeExtraE->birthCounter++;
            pSpawn->owner = nSprite;
            gKillMgr.AddCount(pSpawn);
        }
    }

}

static void thinkSearch(spritetype *pSprite, XSPRITE *pXSprite)
{
    aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkGoto(spritetype *pSprite, XSPRITE *pXSprite)
{
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    int dx = pXSprite->targetX-pSprite->x;
    int dy = pXSprite->targetY-pSprite->y;
    int nAngle = getangle(dx, dy);
    int nDist = approxDist(dx, dy);
    aiChooseDirection(pSprite, pXSprite, nAngle);
    if (nDist < 512 && klabs(pSprite->ang - nAngle) < pDudeInfo->periphery)
        aiNewState(pSprite, pXSprite, &spidSearch);
    aiThinkTarget(pSprite, pXSprite);
}

static void thinkChase(spritetype *pSprite, XSPRITE *pXSprite)
{
    if (pXSprite->target == -1)
    {
        aiNewState(pSprite, pXSprite, &spidGoto);
        return;
    }
    dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
    DUDEINFO *pDudeInfo = getDudeInfo(pSprite->type);
    dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
    spritetype *pTarget = &sprite[pXSprite->target];
    XSPRITE *pXTarget = &xsprite[pTarget->extra];
    int dx = pTarget->x-pSprite->x;
    int dy = pTarget->y-pSprite->y;
    aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));
    if (pXTarget->health == 0)
    {
        aiNewState(pSprite, pXSprite, &spidSearch);
        return;
    }
    if (IsPlayerSprite(pTarget) && powerupCheck(&gPlayer[pTarget->type-kDudePlayer1], kPwUpShadowCloak) > 0)
    {
        aiNewState(pSprite, pXSprite, &spidSearch);
        return;
    }
    int nDist = approxDist(dx, dy);
    if (nDist <= pDudeInfo->seeDist) {
        int nDeltaAngle = ((getangle(dx,dy)+1024-pSprite->ang)&2047)-1024;
        int height = (pDudeInfo->eyeHeight*pSprite->yrepeat)<<2;
        if (cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum, pSprite->x, pSprite->y, pSprite->z - height, pSprite->sectnum)) {
            if (nDist < pDudeInfo->seeDist && klabs(nDeltaAngle) <= pDudeInfo->periphery) {
                aiSetTarget(pXSprite, pXSprite->target);
                
                switch (pSprite->type) {
                    case kDudeSpiderRed:
                        if (nDist < 0x399 && klabs(nDeltaAngle) < 85)
                            aiNewState(pSprite, pXSprite, &spidBite);
                        break;
                    case kDudeSpiderBrown:
                    case kDudeSpiderBlack:
                        if (nDist < 0x733 && nDist > 0x399 && klabs(nDeltaAngle) < 85)
                            aiNewState(pSprite, pXSprite, &spidJump);
                        else if (nDist < 0x399 && klabs(nDeltaAngle) < 85)
                            aiNewState(pSprite, pXSprite, &spidBite);
                        break;
                    case kDudeSpiderMother:
                        if (nDist < 0x733 && nDist > 0x399 && klabs(nDeltaAngle) < 85)
                            aiNewState(pSprite, pXSprite, &spidJump);
                        else if (Chance(0x8000))
                            aiNewState(pSprite, pXSprite, &spidBirth);
                        break;
                }

                return;
            }
        }
    }

    aiNewState(pSprite, pXSprite, &spidGoto);
    pXSprite->target = -1;
}
