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
#include <random>
#ifdef EDUKE32
#include <iostream>
#endif

#include "build.h"
#include "compat.h"
#include "mmulti.h"
#include "common_game.h"

#include "ai.h"
#include "actor.h"
#include "blood.h"
#include "db.h"
#include "endgame.h"
#include "eventq.h"
#ifdef NOONE_EXTENSIONS
#include "aiunicult.h"
#endif
#include "fx.h"
#include "gameutil.h"
#include "gib.h"
#include "globals.h"
#include "levels.h"
#include "loadsave.h"
#include "player.h"
#include "seq.h"
#include "qav.h"
#include "sfx.h"
#include "sound.h"
#include "triggers.h"
#include "trig.h"
#include "view.h"
#include "messages.h"
#include "weapon.h"
#ifdef NOONE_EXTENSIONS
#include "nnexts.h"
#endif

int basePath[kMaxSectors];

void FireballTrapSeqCallback(int, int);
void MGunFireSeqCallback(int, int);
void MGunOpenSeqCallback(int, int);

int nFireballTrapClient = seqRegisterClient(FireballTrapSeqCallback);
int nMGunFireClient = seqRegisterClient(MGunFireSeqCallback);
int nMGunOpenClient = seqRegisterClient(MGunOpenSeqCallback);



unsigned int GetWaveValue(unsigned int nPhase, int nType)
{
    switch (nType)
    {
    case 0:
        return 0x8000-(Cos((nPhase<<10)>>16)>>15);
    case 1:
        return nPhase;
    case 2:
        return 0x10000-(Cos((nPhase<<9)>>16)>>14);
    case 3:
        return Sin((nPhase<<9)>>16)>>14;
    }
    return nPhase;
}

char SetSpriteState(int nSprite, XSPRITE* pXSprite, int nState, int causerID)
{
    if ((pXSprite->busy & 0xffff) == 0 && pXSprite->state == nState)
        return 0;
    pXSprite->busy = nState << 16;
    pXSprite->state = nState;
    evKill(nSprite, 3, causerID);
    if ((sprite[nSprite].flags & kHitagRespawn) != 0 && sprite[nSprite].inittype >= kDudeBase && sprite[nSprite].inittype < kDudeMax)
    {
        pXSprite->respawnPending = 3;
        evPost(nSprite, 3, gGameOptions.nMonsterRespawnTime, kCallbackRespawn);
        return 1;
    }
    if (pXSprite->restState != nState && pXSprite->waitTime > 0)
        evPost(nSprite, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff, causerID);
    if (pXSprite->txID)
    {
        if (pXSprite->command != kCmdLink && pXSprite->triggerOn && pXSprite->state)
            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command, causerID);
        if (pXSprite->command != kCmdLink && pXSprite->triggerOff && !pXSprite->state)
            evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command, causerID);
    }
    return 1;
}



char SetWallState(int nWall, XWALL *pXWall, int nState, int causerID)
{
    if ((pXWall->busy&0xffff) == 0 && pXWall->state == nState)
        return 0;
    pXWall->busy = nState<<16;
    pXWall->state = nState;
    evKill(nWall, 0, causerID);
    if (pXWall->restState != nState && pXWall->waitTime > 0)
        evPost(nWall, 0, (pXWall->waitTime*120) / 10, pXWall->restState ? kCmdOn : kCmdOff, causerID);
    if (pXWall->txID)
    {
        if (pXWall->command != kCmdLink && pXWall->triggerOn && pXWall->state)
            evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command, causerID);
        if (pXWall->command != kCmdLink && pXWall->triggerOff && !pXWall->state)
            evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command, causerID);
    }
    return 1;
}

char SetSectorState(int nSector, XSECTOR *pXSector, int nState, int causerID)
{
    if ((pXSector->busy&0xffff) == 0 && pXSector->state == nState)
        return 0;
    pXSector->busy = nState<<16;
    pXSector->state = nState;
    evKill(nSector, 6, causerID);
    if (nState == 1)
    {
        if (pXSector->command != kCmdLink && pXSector->triggerOn && pXSector->txID)
            evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command, causerID);
        if (pXSector->stopOn)
        {
            pXSector->stopOn = 0;
            pXSector->stopOff = 0;
        }
        else if (pXSector->reTriggerA)
            evPost(nSector, 6, (pXSector->waitTimeA * 120) / 10, kCmdOff, causerID);
    }
    else
    {
        if (pXSector->command != kCmdLink && pXSector->triggerOff && pXSector->txID)
            evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command, causerID);
        if (pXSector->stopOff)
        {
            pXSector->stopOn = 0;
            pXSector->stopOff = 0;
        }
        else if (pXSector->reTriggerB)
            evPost(nSector, 6, (pXSector->waitTimeB * 120) / 10, kCmdOn, causerID);
    }
    return 1;
}

int gBusyCount = 0;
BUSY gBusy[kMaxBusyCount];

void AddBusy(int a1, BUSYID a2, int nDelta)
{
    dassert(nDelta != 0);
    
    int i;
    for (i = 0; i < gBusyCount; i++)
    {
        if (gBusy[i].at0 == a1 && gBusy[i].atc == a2)
            break;
    }

    if (i == gBusyCount)
    {
    #ifdef NOONE_EXTENSIONS
        if ((!gModernMap && gBusyCount >= kMaxBusyCountVanilla) || gBusyCount >= kMaxBusyCount)
    #else
        if (gBusyCount >= kMaxBusyCountVanilla)
    #endif
        {
            consoleSysMsg("Failed to AddBusy for #%d! Max busy reached (%d)", a1, gBusyCount);
            return;
        }

        gBusy[i].at0 = a1;
        gBusy[i].atc = a2;
        gBusy[i].at8 = nDelta > 0 ? 0 : 65536;
        gBusyCount++;
    }

    gBusy[i].at4 = nDelta;
}

void ReverseBusy(int a1, BUSYID a2)
{
    int i;
    for (i = 0; i < gBusyCount; i++)
    {
        if (gBusy[i].at0 == a1 && gBusy[i].atc == a2)
        {
            gBusy[i].at4 = -gBusy[i].at4;
            break;
        }
    }
}

unsigned int GetSourceBusy(const EVENT &a1)
{
    int nIndex = a1.index;
    switch (a1.type)
    {
    case 6:
    {
        int nXIndex = sector[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXSectors);
        return xsector[nXIndex].busy;
    }
    case 0:
    {
        int nXIndex = wall[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXWalls);
        return xwall[nXIndex].busy;
    }
    case 3:
    {
        int nXIndex = sprite[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXSprites);
        return xsprite[nXIndex].busy;
    }
    }
    return 0;
}

void LifeLeechOperate(spritetype *pSprite, XSPRITE *pXSprite, const EVENT &event)
{
    switch (event.cmd) {
    case kCmdSpritePush:
    {
        int nPlayer = pXSprite->data4;
        if (nPlayer >= 0 && nPlayer < gNetPlayers)
        {
            PLAYER *pPlayer = &gPlayer[nPlayer];
            if (pPlayer->pXSprite->health > 0)
            {
                evKill(pSprite->index, 3);
                pPlayer->ammoCount[8] = ClipHigh(pPlayer->ammoCount[8]+pXSprite->data3, gAmmoInfo[8].max);
                pPlayer->hasWeapon[kWeaponLifeLeech] = 1;
                if (pPlayer->curWeapon != kWeaponLifeLeech)
                {
                    if (!VanillaMode() && checkLitSprayOrTNT(pPlayer)) // if tnt/spray is actively used, do not switch weapon
                        break;
                    pPlayer->weaponState = 0;
                    pPlayer->nextWeapon = kWeaponLifeLeech;
                }
            }
        }
        break;
    }
    case kCmdSpriteProximity:
    {
        int nTarget = pXSprite->target;
        if (nTarget >= 0 && nTarget < kMaxSprites)
        {
            if (!pXSprite->stateTimer)
            {
                spritetype *pTarget = &sprite[nTarget];
                if (pTarget->statnum == kStatDude && !(pTarget->flags&32) && pTarget->extra > 0 && pTarget->extra < kMaxXSprites)
                {
                    int top, bottom;
                    GetSpriteExtents(pSprite, &top, &bottom);
                    int nType = pTarget->type-kDudeBase;
                    DUDEINFO *pDudeInfo = getDudeInfo(nType+kDudeBase);
                    int z1 = (top-pSprite->z)-256;
                    int x = pTarget->x;
                    int y = pTarget->y;
                    int z = pTarget->z;
                    int nDist = approxDist(x - pSprite->x, y - pSprite->y);
                    if (nDist != 0 && cansee(pSprite->x, pSprite->y, top, pSprite->sectnum, x, y, z, pTarget->sectnum))
                    {
                        int t = divscale12(nDist, 0x1aaaaa);
                        x += (xvel[nTarget]*t)>>12;
                        y += (yvel[nTarget]*t)>>12;
                        int angBak = pSprite->ang;
                        pSprite->ang = getangle(x-pSprite->x, y-pSprite->y);
                        int dx = Cos(pSprite->ang)>>16;
                        int dy = Sin(pSprite->ang)>>16;
                        int tz = pTarget->z - (pTarget->yrepeat * pDudeInfo->aimHeight) * 4;
                        int dz = divscale10(tz - top - 256, nDist);
                        int nMissileType = kMissileLifeLeechAltNormal + (pXSprite->data3 ? 1 : 0);
                        int t2;
                        if (!pXSprite->data3)
                            t2 = 120 / 10.0;
                        else
                            t2 = (3*120) / 10.0;
                        spritetype *pMissile = actFireMissile(pSprite, 0, z1, dx, dy, dz, nMissileType);
                        if (pMissile)
                        {
                            pMissile->owner = pSprite->owner;
                            pXSprite->stateTimer = 1;
                            evPost(pSprite->index, 3, t2, kCallbackLeechStateTimer);
                            pXSprite->data3 = ClipLow(pXSprite->data3-1, 0);
                        }
                        pSprite->ang = angBak;
                    }
                }
            }
        }
        return;
    }
    }
    actPostSprite(pSprite->index, kStatFree);
}

void ActivateGenerator(int);

void OperateSprite(int nSprite, XSPRITE *pXSprite, const EVENT &event)
{
    int causerID = event.causer;
    spritetype *pSprite = &sprite[nSprite];
    
    #ifdef NOONE_EXTENSIONS
        if (gModernMap && modernTypeOperateSprite(nSprite, pSprite, pXSprite, event))
            return;
    #endif
    
    switch (event.cmd) {
        case kCmdLock:
            pXSprite->locked = 1;
            return;
        case kCmdUnlock:
            pXSprite->locked = 0;
            return;
        case kCmdToggleLock:
            pXSprite->locked = pXSprite->locked ^ 1;
            return;
    }

    if (pSprite->statnum == kStatDude && pSprite->type >= kDudeBase && pSprite->type < kDudeMax) {
        
        switch (event.cmd) {
            case kCmdOff:
                SetSpriteState(nSprite, pXSprite, 0, causerID);
                break;
            case kCmdSpriteProximity:
                if (pXSprite->state) break;
                fallthrough__;
            case kCmdOn:
            case kCmdSpritePush:
            case kCmdSpriteTouch:
                if (!pXSprite->state) SetSpriteState(nSprite, pXSprite, 1, causerID);
                aiActivateDude(pSprite, pXSprite);
                break;
        }

        return;
    }


    switch (pSprite->type) {
    case kTrapMachinegun:
        if (pXSprite->health <= 0) break; 
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0, causerID)) break;
                seqSpawn(40, 3, pSprite->extra, -1);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1, causerID)) break;
                seqSpawn(38, 3, pSprite->extra, nMGunOpenClient);
                if (pXSprite->data1 > 0)
                    pXSprite->data2 = pXSprite->data1;
                break;
        }
        break;
    case kThingFallingRock:
        if (SetSpriteState(nSprite, pXSprite, 1, causerID))
            pSprite->flags |= 7;
        break;
    case kThingWallCrack:
        if (SetSpriteState(nSprite, pXSprite, 0, causerID))
            actPostSprite(nSprite, kStatFree);
        break;
    case kThingCrateFace:
        if (SetSpriteState(nSprite, pXSprite, 0, causerID))
            actPostSprite(nSprite, kStatFree);
        break;
    case kTrapZapSwitchable:
        switch (event.cmd) {
            case kCmdOff:
                pXSprite->state = 0;
                pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
                pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
                break;
            case kCmdOn:
                pXSprite->state = 1;
                pSprite->cstat &= (unsigned short)~CSTAT_SPRITE_INVISIBLE;
                pSprite->cstat |= CSTAT_SPRITE_BLOCK;
                break;
            case kCmdToggle:
                pXSprite->state ^= 1;
                pSprite->cstat ^= CSTAT_SPRITE_INVISIBLE;
                pSprite->cstat ^= CSTAT_SPRITE_BLOCK;
                break;
        }
        break;
    case kTrapFlame:
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0, causerID)) break;
                seqSpawn(40, 3, pSprite->extra, -1);
                sfxKill3DSound(pSprite, 0, -1);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1, causerID)) break;
                seqSpawn(38, 3, pSprite->extra, -1);
                sfxPlay3DSound(pSprite, 441, 0, 0);
                break;
        }
        break;
    case kSwitchPadlock:
        switch (event.cmd) {
            case kCmdOff:
                SetSpriteState(nSprite, pXSprite, 0, causerID);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1, causerID)) break;
                seqSpawn(37, 3, pSprite->extra, -1);
                break;
            default:
                SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1, causerID);
                if (pXSprite->state) seqSpawn(37, 3, pSprite->extra, -1);
                break;
        }
        break;
    case kSwitchToggle:
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0, causerID)) break;
                sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1, causerID)) break;
                sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                break;
            default:
                if (!SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1, causerID)) break;
                if (pXSprite->state) sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                else sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
                break;
        }
        break;
    case kSwitchOneWay:
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0, causerID)) break;
                sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1, causerID)) break;
                sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                break;
            default:
                if (!SetSpriteState(nSprite, pXSprite, pXSprite->restState ^ 1, causerID)) break;
                if (pXSprite->state) sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
                else sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
                break;
        }
        break;
    case kSwitchCombo:
        switch (event.cmd) {
            case kCmdOff:
                pXSprite->data1--;
                if (pXSprite->data1 < 0)
                    pXSprite->data1 += pXSprite->data3;
                break;
            default:
                pXSprite->data1++;
                if (pXSprite->data1 >= pXSprite->data3)
                    pXSprite->data1 -= pXSprite->data3;
                break;
        }
        
        sfxPlay3DSound(pSprite, pXSprite->data4, -1, 0);
        
        if (pXSprite->command == kCmdLink && pXSprite->txID > 0)
            evSend(nSprite, 3, pXSprite->txID, kCmdLink, causerID);

        if (pXSprite->data1 == pXSprite->data2) 
            SetSpriteState(nSprite, pXSprite, 1, causerID);
        else 
            SetSpriteState(nSprite, pXSprite, 0, causerID);

        break;
    case kMarkerDudeSpawn:
        if (gGameOptions.nMonsterSettings && pXSprite->data1 >= kDudeBase && pXSprite->data1 < kDudeMax) {
            spritetype* pSpawn = actSpawnDude(pSprite, pXSprite->data1, -1, 0);
            if (pSpawn) {
                XSPRITE *pXSpawn = &xsprite[pSpawn->extra];
                gKillMgr.AddCount(pSpawn);
                switch (pXSprite->data1) {
                    case kDudeBurningInnocent:
                    case kDudeBurningCultist:
                    case kDudeBurningZombieAxe:
                    case kDudeBurningZombieButcher:
                    case kDudeBurningTinyCaleb:
                    case kDudeBurningBeast: {
                        pXSpawn->health = getDudeInfo(pXSprite->data1)->startHealth << 4;
                        pXSpawn->burnTime = 10;
                        pXSpawn->target = -1;
                        aiActivateDude(pSpawn, pXSpawn);
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        break;
    case kMarkerEarthQuake:
        pXSprite->triggerOn = 0;
        pXSprite->isTriggered = 1;
        SetSpriteState(nSprite, pXSprite, 1, causerID);
        for (int p = connecthead; p >= 0; p = connectpoint2[p]) {
            spritetype *pPlayerSprite = gPlayer[p].pSprite;
            int dx = (pSprite->x - pPlayerSprite->x)>>4;
            int dy = (pSprite->y - pPlayerSprite->y)>>4;
            int dz = (pSprite->z - pPlayerSprite->z)>>8;
            int nDist = dx*dx+dy*dy+dz*dz+0x40000;
            gPlayer[p].quakeEffect = divscale16(pXSprite->data1, nDist);
        }
        break;
    case kThingTNTBarrel:
        if (pSprite->flags & kHitagRespawn) return;
        fallthrough__;
    case kThingArmedTNTStick:
    case kThingArmedTNTBundle:
    case kThingArmedSpray:
        actExplodeSprite(pSprite);
        break;
    case kTrapExploder:
        switch (event.cmd) {
            case kCmdOn:
                SetSpriteState(nSprite, pXSprite, 1, causerID);
                break;
            default:
                pSprite->cstat &= (unsigned short)~CSTAT_SPRITE_INVISIBLE;
                actExplodeSprite(pSprite);
                break;
        }
        break;
    case kThingArmedRemoteBomb:
        if (pSprite->statnum != kStatRespawn) {
            if (event.cmd != kCmdOn) actExplodeSprite(pSprite);
            else {
                sfxPlay3DSound(pSprite, 454, 0, 0);
                evPost(nSprite, 3, 18, kCmdOff, causerID);
            }
        }
        break;    
    case kThingArmedProxBomb:
        if (pSprite->statnum != kStatRespawn) {
            switch (event.cmd) {
                case kCmdSpriteProximity:
                    if (pXSprite->state) break;
                    sfxPlay3DSound(pSprite, 452, 0, 0);
                    evPost(nSprite, 3, 30, kCmdOff, causerID);
                    pXSprite->state = 1;
                    fallthrough__;
                case kCmdOn:
                    sfxPlay3DSound(pSprite, 451, 0, 0);
                    pXSprite->Proximity = 1;
                    break;
                default:
                    actExplodeSprite(pSprite);
                    break;
            }
        }
        break;
    case kThingDroppedLifeLeech:
        LifeLeechOperate(pSprite, pXSprite, event);
        break;
    case kGenTrigger:
    case kGenDripWater:
    case kGenDripBlood:
    case kGenMissileFireball:
    case kGenMissileEctoSkull:
    case kGenDart:
    case kGenBubble:
    case kGenBubbleMulti:
    case kGenSound:
        switch (event.cmd) {
            case kCmdOff:
                SetSpriteState(nSprite, pXSprite, 0, causerID);
                break;
            case kCmdRepeat:
                if (pSprite->type != kGenTrigger) ActivateGenerator(nSprite);
                if (pXSprite->txID) evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command, causerID);
                if (pXSprite->busyTime > 0) {
                    int nRand = Random2(pXSprite->data1);
                    evPost(nSprite, 3, 120*(nRand+pXSprite->busyTime) / 10, kCmdRepeat, causerID);
                }
                break;
            default:
                if (!pXSprite->state) {
                    SetSpriteState(nSprite, pXSprite, 1, causerID);
                    evPost(nSprite, 3, 0, kCmdRepeat, causerID);
                }
                break;
        }
        break;
    case kSoundPlayer:
        if (gGameOptions.nGameType == kGameTypeSinglePlayer)
        {
            if (gMe->pXSprite->health <= 0)
                break;
            gMe->restTime = 0;
        }
        sndStartSample(pXSprite->data1, -1, 1, 0);
        break;
    case kThingObjectGib:
    case kThingObjectExplode:
    case kThingBloodBits:
    case kThingBloodChunks:
    case kThingZombieHead:
        switch (event.cmd) {
            case kCmdOff:
                if (!SetSpriteState(nSprite, pXSprite, 0, causerID)) break;
                actActivateGibObject(pSprite, pXSprite);
                break;
            case kCmdOn:
                if (!SetSpriteState(nSprite, pXSprite, 1, causerID)) break;
                actActivateGibObject(pSprite, pXSprite);
                break;
            default:
                if (!SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1, causerID)) break;
                actActivateGibObject(pSprite, pXSprite);
                break;
        }
        break;
    default:
        switch (event.cmd) {
            case kCmdOff:
                SetSpriteState(nSprite, pXSprite, 0, causerID);
                break;
            case kCmdOn:
                SetSpriteState(nSprite, pXSprite, 1, causerID);
                break;
            default:
                SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1, causerID);
                break;
        }
        break;
    }
}

void SetupGibWallState(walltype *pWall, XWALL *pXWall)
{
    walltype *pWall2 = NULL;
    if (pWall->nextwall >= 0)
        pWall2 = &wall[pWall->nextwall];
    if (pXWall->state)
    {
        pWall->cstat &= ~65;
        if (pWall2)
        {
            pWall2->cstat &= ~65;
            pWall->cstat &= ~16;
            pWall2->cstat &= ~16;
        }
        return;
    }
    char bVector = pXWall->triggerVector != 0;
    pWall->cstat |= 1;
    if (bVector)
        pWall->cstat |= 64;
    if (pWall2)
    {
        pWall2->cstat &= ~1;
        if (bVector)
            pWall2->cstat |= 64;
        pWall->cstat |= 16;
        pWall2->cstat |= 16;
    }
}

void OperateWall(int nWall, XWALL *pXWall, const EVENT &event) {
    
    int causerID = event.causer;
    walltype *pWall = &wall[nWall];
    
    switch (event.cmd) {
        case kCmdLock:
            pXWall->locked = 1;
            return;
        case kCmdUnlock:
            pXWall->locked = 0;
            return;
        case kCmdToggleLock:
            pXWall->locked ^= 1;
            return;
    }
    
    #ifdef NOONE_EXTENSIONS
    if (gModernMap && modernTypeOperateWall(nWall, pWall, pXWall, event))
        return;
    #endif

    switch (pWall->type) {
        case kWallGib:
            if (GetWallType(nWall) != pWall->type) break;
            char bStatus;
            switch (event.cmd) {
                case kCmdOn:
                case kCmdWallImpact:
                    bStatus = SetWallState(nWall, pXWall, 1, causerID);
                    break;
                case kCmdOff:
                    bStatus = SetWallState(nWall, pXWall, 0, causerID);
                    break;
                default:
                    bStatus = SetWallState(nWall, pXWall, pXWall->state ^ 1, causerID);
                    break;
            }

            if (bStatus) {
                SetupGibWallState(pWall, pXWall);
                if (pXWall->state) {
                    CGibVelocity vel(100, 100, 250);
                    int nType = ClipRange(pXWall->data, 0, 31);
                    if (nType > 0)
                        GibWall(nWall, (GIBTYPE)nType, &vel);
                }
            }
            return;
        default:
            switch (event.cmd) {
                case kCmdOff:
                    SetWallState(nWall, pXWall, 0, causerID);
                    break;
                case kCmdOn:
                    SetWallState(nWall, pXWall, 1, causerID);
                    break;
                default:
                    SetWallState(nWall, pXWall, pXWall->state ^ 1, causerID);
                    break;
            }
            return;
    }


}

void SectorStartSound(int nSector, int nState)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
        {
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (nState)
            {
                if (pXSprite->data3)
                    sfxPlay3DSound(pSprite, pXSprite->data3, 0, 0);
            }
            else
            {
                if (pXSprite->data1)
                    sfxPlay3DSound(pSprite, pXSprite->data1, 0, 0);
            }
        }
    }
}

void SectorEndSound(int nSector, int nState)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
        {
            int nXSprite = pSprite->extra;
            dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (nState)
            {
                if (pXSprite->data2)
                    sfxPlay3DSound(pSprite, pXSprite->data2, 0, 0);
            }
            else
            {
                if (pXSprite->data4)
                    sfxPlay3DSound(pSprite, pXSprite->data4, 0, 0);
            }
        }
    }
}

void PathSound(int nSector, int nSound)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatDecoration && pSprite->type == kSoundSector)
            sfxPlay3DSound(pSprite, nSound, 0, 0);
    }
}

void DragPoint(int nWall, int x, int y)
{
    viewInterpolateWall(nWall, &wall[nWall]);
    wall[nWall].x = x;
    wall[nWall].y = y;

    int vsi = numwalls;
    int vb = nWall;
    do
    {
        if (wall[vb].nextwall >= 0)
        {
            vb = wall[wall[vb].nextwall].point2;
            viewInterpolateWall(vb, &wall[vb]);
            wall[vb].x = x;
            wall[vb].y = y;
        }
        else
        {
            vb = nWall;
            do
            {
                if (wall[lastwall(vb)].nextwall >= 0)
                {
                    vb = wall[lastwall(vb)].nextwall;
                    viewInterpolateWall(vb, &wall[vb]);
                    wall[vb].x = x;
                    wall[vb].y = y;
                }
                else
                    break;
                vsi--;
            } while (vb != nWall && vsi > 0);
            break;
        }
        vsi--;
    } while (vb != nWall && vsi > 0);
}

void TranslateSector(int nSector, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, char bAllWalls)
{
    int x, y;
    int nXSector = sector[nSector].extra;
    XSECTOR *pXSector = &xsector[nXSector];
    int v20 = interpolate(a6, a9, a2);
    int vc = interpolate(a6, a9, a3);
    int v28 = vc - v20;
    int v24 = interpolate(a7, a10, a2);
    int v8 = interpolate(a7, a10, a3);
    int v2c = v8 - v24;
    int v44 = interpolate(a8, a11, a2);
    int vbp = interpolate(a8, a11, a3);
    int v14 = vbp - v44;
    int nWall = sector[nSector].wallptr;
    
    #ifdef NOONE_EXTENSIONS
    // fix Y arg in RotatePoint for reverse (green) moving sprites?
    int sprDy = (gModernMap) ? a5 : a4;
    #else
    int sprDy = a4;
    #endif

    if (bAllWalls)
    {
        for (int i = 0; i < sector[nSector].wallnum; nWall++, i++)
        {
            x = baseWall[nWall].x;
            y = baseWall[nWall].y;
            if (vbp)
                RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
            DragPoint(nWall, x+vc-a4, y+v8-a5);
        }
    }
    else
    {
        for (int i = 0; i < sector[nSector].wallnum; nWall++, i++)
        {
            int v10 = wall[nWall].point2;
            x = baseWall[nWall].x;
            y = baseWall[nWall].y;
            if (wall[nWall].cstat&16384)
            {
                if (vbp)
                    RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
                DragPoint(nWall, x+vc-a4, y+v8-a5);
                if ((wall[v10].cstat&49152) == 0)
                {
                    x = baseWall[v10].x;
                    y = baseWall[v10].y;
                    if (vbp)
                        RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
                    DragPoint(v10, x+vc-a4, y+v8-a5);
                }
                continue;
            }
            if (wall[nWall].cstat&32768)
            {
                if (vbp)
                    RotatePoint((int*)&x, (int*)&y, -vbp, a4, a5);
                DragPoint(nWall, x-(vc-a4), y-(v8-a5));
                if ((wall[v10].cstat&49152) == 0)
                {
                    x = baseWall[v10].x;
                    y = baseWall[v10].y;
                    if (vbp)
                        RotatePoint((int*)&x, (int*)&y, -vbp, a4, a5);
                    DragPoint(v10, x-(vc-a4), y-(v8-a5));
                }
                continue;
            }
        }
    }
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        // allow to move markers by sector movements in game if flags 1 is added in editor.
        switch (pSprite->statnum) {
            case kStatMarker:
            case kStatPathMarker:
                #ifdef NOONE_EXTENSIONS
                    if (!gModernMap || !(pSprite->flags & 0x1)) continue;
                #else
                    continue;
                #endif
                break;
        }

        x = baseSprite[nSprite].x;
        y = baseSprite[nSprite].y;
        if (sprite[nSprite].cstat&8192)
        {
            if (vbp)
                RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
            viewBackupSpriteLoc(nSprite, pSprite);
            pSprite->ang = (pSprite->ang+v14)&2047;
            pSprite->x = x+vc-a4;
            pSprite->y = y+v8-a5;
        }
        else if (sprite[nSprite].cstat&16384)
        {
            if (vbp)
                RotatePoint((int*)& x, (int*)& y, -vbp, a4, sprDy);
            viewBackupSpriteLoc(nSprite, pSprite);
            pSprite->ang = (pSprite->ang-v14)&2047;
            pSprite->x = x-(vc-a4);
            pSprite->y = y-(v8-a5);
        }
        else if (pXSector->Drag)
        {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            int floorZ = getflorzofslope(nSector, pSprite->x, pSprite->y);
            if (!(pSprite->cstat&48) && floorZ <= bottom)
            {
                if (!VanillaMode()) viewBackupSpriteLoc(nSprite, pSprite);
                if (v14)
                    RotatePoint((int*)&pSprite->x, (int*)&pSprite->y, v14, v20, v24);
                if (VanillaMode()) viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->ang = (pSprite->ang+v14)&2047;
                pSprite->x += v28;
                pSprite->y += v2c;
            }
        }
    }

    #ifdef NOONE_EXTENSIONS
    // translate sprites near outside walls
    ////////////////////////////////////////////////////////////
    
    int nSprite, *ptr;
    if (gModernMap && (ptr = gSprNSect.GetSprPtr(nSector)) != NULL)
    {
        while (*ptr >= 0)
        {
            spritetype* pSprite = &sprite[*ptr++];
            if (pSprite->statnum >= kMaxStatus)
                continue;

            nSprite = pSprite->index;
            x = baseSprite[nSprite].x;
            y = baseSprite[nSprite].y;
            if (pSprite->cstat & 8192)
            {
                if (vbp)
                    RotatePoint((int*)&x, (int*)&y, vbp, a4, a5);
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->ang = (pSprite->ang + v14) & 2047;
                pSprite->x = x + vc - a4;
                pSprite->y = y + v8 - a5;
            }
            else if (pSprite->cstat & 16384)
            {
                if (vbp)
                    RotatePoint((int*)&x, (int*)&y, -vbp, a4, sprDy);
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->ang = (pSprite->ang - v14) & 2047;
                pSprite->x = x - (vc - a4);
                pSprite->y = y - (v8 - a5);
            }
        }
    }
    /////////////////////
    #endif

}

void ZTranslateSector(int nSector, XSECTOR *pXSector, int a3, int a4)
{
    sectortype *pSector = &sector[nSector];
    viewInterpolateSector(nSector, pSector);

    int *ptr1 = NULL, *ptr2;
    int dfz = pXSector->onFloorZ - pXSector->offFloorZ;
    int dcz = pXSector->onCeilZ - pXSector->offCeilZ;
    
    #ifdef NOONE_EXTENSIONS
    // get pointer to sprites near outside walls before translation
    ///////////////////////////////////////////////////////////////
    if (gModernMap && (dfz || dcz))
        ptr1 = gSprNSect.GetSprPtr(nSector);
    #endif

    if (dfz != 0)
    {
        int oldZ = pSector->floorz;
        baseFloor[nSector] = pSector->floorz = pXSector->offFloorZ + mulscale16(dfz, GetWaveValue(a3, a4));
        velFloor[nSector] += (pSector->floorz-oldZ)<<8;
        for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->statnum == kStatMarker || pSprite->statnum == kStatPathMarker)
                continue;
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (pSprite->cstat&8192)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z += pSector->floorz-oldZ;
            }
            else if (pSprite->flags&2)
                pSprite->flags |= 4;
            else if (oldZ <= bottom && !(pSprite->cstat&48))
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z += pSector->floorz-oldZ;
            }
        }
        
        #ifdef NOONE_EXTENSIONS
        // translate sprites near outside walls (floor)
        ////////////////////////////////////////////////////////////
        if (ptr1)
        {
            ptr2 = ptr1;
            while (*ptr2 >= 0)
            {
                spritetype* pSprite = &sprite[*ptr2++];
                if (pSprite->statnum < kMaxStatus && (pSprite->cstat & 8192))
                {
                    viewBackupSpriteLoc(pSprite->index, pSprite);
                    pSprite->z += pSector->floorz - oldZ;
                }
            }
        }
        /////////////////////
        #endif
    }

    if (dcz != 0)
    {
        int oldZ = pSector->ceilingz;
        baseCeil[nSector] = pSector->ceilingz = pXSector->offCeilZ + mulscale16(dcz, GetWaveValue(a3, a4));
        velCeil[nSector] += pSector->ceilingz-oldZ;
        for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->statnum == kStatMarker || pSprite->statnum == kStatPathMarker)
                continue;
            if (pSprite->cstat&16384)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z += pSector->ceilingz-oldZ;
            }
        }
        
        #ifdef NOONE_EXTENSIONS
        // translate sprites near outside walls (ceil)
        ////////////////////////////////////////////////////////////
        if (ptr1)
        {
            ptr2 = ptr1;
            while (*ptr2 >= 0)
            {
                spritetype* pSprite = &sprite[*ptr2++];
                if (pSprite->statnum < kMaxStatus && (pSprite->cstat & 16384))
                {
                    viewBackupSpriteLoc(pSprite->index, pSprite);
                    pSprite->z += pSector->ceilingz - oldZ;
                }
            }
        }
        /////////////////////
        #endif
    }
}

int GetHighestSprite(int nSector, int nStatus, int *a3)
{
    *a3 = sector[nSector].floorz;
    int v8 = -1;
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        if (sprite[nSprite].statnum == nStatus || nStatus == kStatFree)
        {
            spritetype *pSprite = &sprite[nSprite];
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (pSprite->z-top < *a3)
            {
                *a3 = pSprite->z-top;
                v8 = nSprite;
            }
        }
    }
    return v8;
}

int GetCrushedSpriteExtents(unsigned int nSector, int *pzTop, int *pzBot)
{
    dassert(pzTop != NULL && pzBot != NULL);
    dassert(nSector < (unsigned int)numsectors);
    int vc = -1;
    sectortype *pSector = &sector[nSector];
    int vbp = pSector->ceilingz;
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatDude || pSprite->statnum == kStatThing)
        {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            if (vbp > top)
            {
                vbp = top;
                *pzTop = top;
                *pzBot = bottom;
                vc = nSprite;
            }
        }
    }
    return vc;
}

int VCrushBusy(unsigned int nSector, unsigned int a2, int causerID)
{
    dassert(nSector < (unsigned int)numsectors);
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->busyWaveA;
    else
        nWave = pXSector->busyWaveB;
    int dz1 = pXSector->onCeilZ - pXSector->offCeilZ;
    int vc = pXSector->offCeilZ;
    if (dz1 != 0)
        vc += mulscale16(dz1, GetWaveValue(a2, nWave));
    int dz2 = pXSector->onFloorZ - pXSector->offFloorZ;
    int v10 = pXSector->offFloorZ;
    if (dz2 != 0)
        v10 += mulscale16(dz2, GetWaveValue(a2, nWave));
    int v18;
    if (GetHighestSprite(nSector, 6, &v18) >= 0 && vc >= v18)
        return 1;
    viewInterpolateSector(nSector, &sector[nSector]);
    if (dz1 != 0)
        sector[nSector].ceilingz = vc;
    if (dz2 != 0)
        sector[nSector].floorz = v10;
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink, causerID);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16, causerID);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int VSpriteBusy(unsigned int nSector, unsigned int a2, int causerID)
{
    dassert(nSector < (unsigned int)numsectors);
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->busyWaveA;
    else
        nWave = pXSector->busyWaveB;
    int dz1 = pXSector->onFloorZ - pXSector->offFloorZ;
    if (dz1 != 0)
    {
        for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->cstat&8192)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z = baseSprite[nSprite].z+mulscale16(dz1, GetWaveValue(a2, nWave));
            }
        }
    }
    int dz2 = pXSector->onCeilZ - pXSector->offCeilZ;
    if (dz2 != 0)
    {
        for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
        {
            spritetype *pSprite = &sprite[nSprite];
            if (pSprite->cstat&16384)
            {
                viewBackupSpriteLoc(nSprite, pSprite);
                pSprite->z = baseSprite[nSprite].z+mulscale16(dz2, GetWaveValue(a2, nWave));
            }
        }
    }
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink, causerID);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16, causerID);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int VDoorBusy(unsigned int nSector, unsigned int a2, int causerID)
{
    dassert(nSector < (unsigned int)numsectors);
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int vbp;
    if (pXSector->state)
        vbp = 65536/ClipLow((120*pXSector->busyTimeA)/10, 1);
    else
        vbp = -65536/ClipLow((120*pXSector->busyTimeB)/10, 1);
    int top, bottom;
    int nSprite = GetCrushedSpriteExtents(nSector,&top,&bottom);
    if (nSprite >= 0 && a2 > pXSector->busy)
    {
        spritetype *pSprite = &sprite[nSprite];
        dassert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
        XSPRITE *pXSprite = &xsprite[pSprite->extra];
        if (pXSector->onCeilZ > pXSector->offCeilZ || pXSector->onFloorZ < pXSector->offFloorZ)
        {
            if (pXSector->interruptable)
            {
                if (pXSector->Crush)
                {
                    if (pXSprite->health <= 0)
                        return 2;
                    int nDamage;
                    if (pXSector->data == 0)
                        nDamage = 500;
                    else
                        nDamage = pXSector->data;
                    actDamageSprite(nSprite, &sprite[nSprite], kDamageFall, nDamage<<4);
                }
                a2 = ClipRange(a2-(vbp/2)*4, 0, 65536);
            }
            else if (pXSector->Crush && pXSprite->health > 0)
            {
                int nDamage;
                if (pXSector->data == 0)
                    nDamage = 500;
                else
                    nDamage = pXSector->data;
                actDamageSprite(nSprite, &sprite[nSprite], kDamageFall, nDamage<<4);
                a2 = ClipRange(a2-(vbp/2)*4, 0, 65536);
            }
        }
    }
    else if (nSprite >= 0 && a2 < pXSector->busy)
    {
        spritetype *pSprite = &sprite[nSprite];
        dassert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
        XSPRITE *pXSprite = &xsprite[pSprite->extra];
        if (pXSector->offCeilZ > pXSector->onCeilZ || pXSector->offFloorZ < pXSector->onFloorZ)
        {
            if (pXSector->interruptable)
            {
                if (pXSector->Crush)
                {
                    if (pXSprite->health <= 0)
                        return 2;
                    int nDamage;
                    if (pXSector->data == 0)
                        nDamage = 500;
                    else
                        nDamage = pXSector->data;
                    actDamageSprite(nSprite, &sprite[nSprite], kDamageFall, nDamage<<4);
                }
                a2 = ClipRange(a2+(vbp/2)*4, 0, 65536);
            }
            else if (pXSector->Crush && pXSprite->health > 0)
            {
                int nDamage;
                if (pXSector->data == 0)
                    nDamage = 500;
                else
                    nDamage = pXSector->data;
                actDamageSprite(nSprite, &sprite[nSprite], kDamageFall, nDamage<<4);
                a2 = ClipRange(a2+(vbp/2)*4, 0, 65536);
            }
        }
    }
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->busyWaveA;
    else
        nWave = pXSector->busyWaveB;
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink, causerID);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16, causerID);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int HDoorBusy(unsigned int nSector, unsigned int a2, int causerID)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->busyWaveA;
    else
        nWave = pXSector->busyWaveB;
    spritetype *pSprite1 = &sprite[pXSector->marker0];
    spritetype *pSprite2 = &sprite[pXSector->marker1];
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite1->x, pSprite1->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, pSector->type == kSectorSlide);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink, causerID);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16, causerID);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int RDoorBusy(unsigned int nSector, unsigned int a2, int causerID)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    int nWave;
    if (pXSector->busy < a2)
        nWave = pXSector->busyWaveA;
    else
        nWave = pXSector->busyWaveB;
    spritetype *pSprite = &sprite[pXSector->marker0];
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, 0, pSprite->x, pSprite->y, pSprite->ang, pSector->type == kSectorRotate);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink, causerID);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16, causerID);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int StepRotateBusy(unsigned int nSector, unsigned int a2, int causerID)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    spritetype *pSprite = &sprite[pXSector->marker0];
    int vbp;
    if (pXSector->busy < a2)
    {
        vbp = pXSector->data+pSprite->ang;
        int nWave = pXSector->busyWaveA;
        TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, pXSector->data, pSprite->x, pSprite->y, vbp, 1);
    }
    else
    {
        vbp = pXSector->data-pSprite->ang;
        int nWave = pXSector->busyWaveB;
        TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite->x, pSprite->y, vbp, pSprite->x, pSprite->y, pXSector->data, 1);
    }
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink, causerID);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16, causerID);
        SectorEndSound(nSector, a2>>16);
        pXSector->data = vbp&2047;
        return 3;
    }
    return 0;
}

int GenSectorBusy(unsigned int nSector, unsigned int a2, int causerID)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    pXSector->busy = a2;
    if (pXSector->command == kCmdLink && pXSector->txID)
        evSend(nSector, 6, pXSector->txID, kCmdLink, causerID);
    if ((a2&0xffff) == 0)
    {
        SetSectorState(nSector, pXSector, a2>>16, causerID);
        SectorEndSound(nSector, a2>>16);
        return 3;
    }
    return 0;
}

int PathBusy(unsigned int nSector, unsigned int a2, int causerID)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    int nXSector = pSector->extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR *pXSector = &xsector[nXSector];
    spritetype *pSprite = &sprite[basePath[nSector]];
    spritetype *pSprite1 = &sprite[pXSector->marker0];
    XSPRITE *pXSprite1 = &xsprite[pSprite1->extra];
    spritetype *pSprite2 = &sprite[pXSector->marker1];
    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
    int nWave = pXSprite1->wave;
    TranslateSector(nSector, GetWaveValue(pXSector->busy, nWave), GetWaveValue(a2, nWave), pSprite->x, pSprite->y, pSprite1->x, pSprite1->y, pSprite1->ang, pSprite2->x, pSprite2->y, pSprite2->ang, 1);
    ZTranslateSector(nSector, pXSector, a2, nWave);
    pXSector->busy = a2;
    if ((a2&0xffff) == 0)
    {
        evPost(nSector, 6, (120*pXSprite2->waitTime)/10, kCmdOn, causerID);
        pXSector->state = 0;
        pXSector->busy = 0;
        if (pXSprite1->data4)
            PathSound(nSector, pXSprite1->data4);
        pXSector->marker0 = pXSector->marker1;
        pXSector->data = pXSprite2->data1;
        return 3;
    }
    return 0;
}

void OperateDoor(unsigned int nSector, XSECTOR *pXSector, const EVENT &event, BUSYID busyWave) 
{
    switch (event.cmd) {
        case kCmdOff:
            if (!pXSector->busy) break;
            AddBusy(nSector, busyWave, -65536/ClipLow((pXSector->busyTimeB*120)/10, 1));
            SectorStartSound(nSector, 1);
            break;
        case kCmdOn:
            if (pXSector->busy == 0x10000) break;
            AddBusy(nSector, busyWave, 65536/ClipLow((pXSector->busyTimeA*120)/10, 1));
            SectorStartSound(nSector, 0);
            break;
        default:
            if (pXSector->busy & 0xffff)  {
                if (pXSector->interruptable) {
                    ReverseBusy(nSector, busyWave);
                    pXSector->state = !pXSector->state;
                }
            } else {
                char t = !pXSector->state; int nDelta;
            
                if (t) nDelta = 65536/ClipLow((pXSector->busyTimeA*120)/10, 1);
                else nDelta = -65536/ClipLow((pXSector->busyTimeB*120)/10, 1);
            
                AddBusy(nSector, busyWave, nDelta);
                SectorStartSound(nSector, pXSector->state);
            }
            break;
    }
}

char SectorContainsDudes(int nSector)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        if (sprite[nSprite].statnum == kStatDude)
            return 1;
    }
    return 0;
}

void TeleFrag(int nKiller, int nSector)
{
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatDude)
            actDamageSprite(nKiller, pSprite, kDamageExplode, 4000);
        else if (pSprite->statnum == kStatThing)
            actDamageSprite(nKiller, pSprite, kDamageExplode, 4000);
    }
}

void OperateTeleport(unsigned int nSector, XSECTOR *pXSector)
{
    dassert(nSector < (unsigned int)numsectors);
    int nDest = pXSector->marker0;
    dassert(nDest < kMaxSprites);
    spritetype *pDest = &sprite[nDest];
    dassert(pDest->statnum == kStatMarker);
    dassert(pDest->type == kMarkerWarpDest);
    dassert(pDest->sectnum >= 0 && pDest->sectnum < kMaxSectors);
    for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatDude)
        {
            PLAYER *pPlayer;
            char bPlayer = IsPlayerSprite(pSprite);
            if (bPlayer)
                pPlayer = &gPlayer[pSprite->type-kDudePlayer1];
            else
                pPlayer = NULL;
            if (bPlayer || !SectorContainsDudes(pDest->sectnum))
            {
                if (!(gGameOptions.uNetGameFlags&2))
                    TeleFrag(pXSector->data, pDest->sectnum);
                pSprite->x = pDest->x;
                pSprite->y = pDest->y;
                pSprite->z += sector[pDest->sectnum].floorz-sector[nSector].floorz;
                pSprite->ang = pDest->ang;
                ChangeSpriteSect(nSprite, pDest->sectnum);
                sfxPlay3DSound(pDest, 201, -1, 0);
                xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;
                ClearBitString(gInterpolateSprite, nSprite);
                viewBackupSpriteLoc(nSprite, pSprite);
                if (pPlayer)
                {
                    playerResetInertia(pPlayer);
                    pPlayer->zViewVel = pPlayer->zWeaponVel = 0;
                    if (!VanillaMode()) // if player teleported, clear old angles
                    {
                        pPlayer->angold = pSprite->ang;
                        pPlayer->q16ang = fix16_from_int(pSprite->ang);
                    }
                }
            }
        }
    }
}

void OperatePath(unsigned int nSector, XSECTOR *pXSector, const EVENT &event)
{
    int nSprite;
    spritetype *pSprite = NULL;
    XSPRITE *pXSprite;
    dassert(nSector < (unsigned int)numsectors);
    spritetype *pSprite2 = &sprite[pXSector->marker0];
    XSPRITE *pXSprite2 = &xsprite[pSprite2->extra];
    int nId = pXSprite2->data2;
    for (nSprite = headspritestat[kStatPathMarker]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        pSprite = &sprite[nSprite];
        if (pSprite->type == kMarkerPath)
        {
            pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->data1 == nId)
                break;
        }
    }

    // trigger marker after it gets reached
    #ifdef NOONE_EXTENSIONS
        if (gModernMap && pXSprite2->state != 1)
            trTriggerSprite(pSprite2->index, pXSprite2, kCmdOn, event.causer);
    #endif

    if (nSprite < 0) {
        viewSetSystemMessage("Unable to find path marker with id #%d for path sector #%d", nId, nSector);
        pXSector->state = 0;
        pXSector->busy = 0;
        return;
    }
        
    pXSector->marker1 = nSprite;
    pXSector->offFloorZ = pSprite2->z;
    pXSector->onFloorZ = pSprite->z;
    switch (event.cmd) {
        case kCmdOn:
            pXSector->state = 0;
            pXSector->busy = 0;
            AddBusy(nSector, BUSYID_7, 65536/ClipLow((120*pXSprite2->busyTime)/10,1));
            if (pXSprite2->data3) PathSound(nSector, pXSprite2->data3);
            break;
    }
}

void OperateSector(unsigned int nSector, XSECTOR *pXSector, const EVENT &event)
{
    dassert(nSector < (unsigned int)numsectors);
    sectortype *pSector = &sector[nSector];
    
    #ifdef NOONE_EXTENSIONS
    if (gModernMap && modernTypeOperateSector(nSector, pSector, pXSector, event))
        return;
    #endif

    switch (event.cmd) {
        case kCmdLock:
            pXSector->locked = 1;
            break;
        case kCmdUnlock:
            pXSector->locked = 0;
            break;
        case kCmdToggleLock:
            pXSector->locked ^= 1;
            break;
        case kCmdStopOff:
            pXSector->stopOn = 0;
            pXSector->stopOff = 1;
            break;
        case kCmdStopOn:
            pXSector->stopOn = 1;
            pXSector->stopOff = 0;
            break;
        case kCmdStopNext:
            pXSector->stopOn = 1;
            pXSector->stopOff = 1;
            break;
        default:
        #ifdef NOONE_EXTENSIONS
        if (gModernMap && pXSector->unused1) break;
        #endif
            switch (pSector->type) {
                case kSectorZMotionSprite:
                    OperateDoor(nSector, pXSector, event, BUSYID_1);
                    break;
                case kSectorZMotion:
                    OperateDoor(nSector, pXSector, event, BUSYID_2);
                    break;
                case kSectorSlideMarked:
                case kSectorSlide:
                    OperateDoor(nSector, pXSector, event, BUSYID_3);
                    break;
                case kSectorRotateMarked:
                case kSectorRotate:
                    OperateDoor(nSector, pXSector, event, BUSYID_4);
                    break;
                case kSectorRotateStep:
                    switch (event.cmd) {
                        case kCmdOn:
                            pXSector->state = 0;
                            pXSector->busy = 0;
                            AddBusy(nSector, BUSYID_5, 65536/ClipLow((120*pXSector->busyTimeA)/10, 1));
                            SectorStartSound(nSector, 0);
                            break;
                        case kCmdOff:
                            pXSector->state = 1;
                            pXSector->busy = 65536;
                            AddBusy(nSector, BUSYID_5, -65536/ClipLow((120*pXSector->busyTimeB)/10, 1));
                            SectorStartSound(nSector, 1);
                            break;
                    }
                    break;
                case kSectorTeleport:
                    OperateTeleport(nSector, pXSector);
                    break;
                case kSectorPath:
                    OperatePath(nSector, pXSector, event);
                    break;
                default:
                    if (!pXSector->busyTimeA && !pXSector->busyTimeB) {
                        
                        switch (event.cmd) {
                            case kCmdOff:
                                SetSectorState(nSector, pXSector, 0, event.causer);
                                break;
                            case kCmdOn:
                                SetSectorState(nSector, pXSector, 1, event.causer);
                                break;
                            default:
                                SetSectorState(nSector, pXSector, pXSector->state ^ 1, event.causer);
                                break;
                        }

                    } else {
                        
                        OperateDoor(nSector, pXSector, event, BUSYID_6);

                    }

                    break;
            }
            break;
    }
}

void InitPath(unsigned int nSector, XSECTOR *pXSector)
{
    int nSprite;
    spritetype *pSprite;
    XSPRITE *pXSprite;
    dassert(nSector < (unsigned int)numsectors);
    int nId = pXSector->data;
    for (nSprite = headspritestat[kStatPathMarker]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        pSprite = &sprite[nSprite];
        if (pSprite->type == kMarkerPath)
        {
            pXSprite = &xsprite[pSprite->extra];
            if (pXSprite->data1 == nId)
                break;
        }
    }
    
    if (nSprite < 0) {
        //ThrowError("Unable to find path marker with id #%d", nId);
        viewSetSystemMessage("Unable to find path marker with id #%d for path sector #%d", nId, nSector);
        return;
        
    }

    pXSector->marker0 = nSprite;
    basePath[nSector] = nSprite;
    if (pXSector->state)
        evPost(nSector, 6, 0, kCmdOn, kCauserGame);
}

void LinkSector(int nSector, XSECTOR *pXSector, const EVENT &event)
{
    sectortype *pSector = &sector[nSector];
    int nBusy = GetSourceBusy(event);
    switch (pSector->type) {
        case kSectorZMotionSprite:
            VSpriteBusy(nSector, nBusy, event.causer);
            break;
        case kSectorZMotion:
            VDoorBusy(nSector, nBusy, event.causer);
            break;
        case kSectorSlideMarked:
        case kSectorSlide:
            HDoorBusy(nSector, nBusy, event.causer);
            break;
        case kSectorRotateMarked:
        case kSectorRotate:
            RDoorBusy(nSector, nBusy, event.causer);
            break;
        default:
            pXSector->busy = nBusy;
            if ((pXSector->busy&0xffff) == 0)
                SetSectorState(nSector, pXSector, nBusy>>16, event.causer);
            break;
    }
}

void LinkSprite(int nSprite, XSPRITE *pXSprite, const EVENT &event) {
    spritetype *pSprite = &sprite[nSprite];
    int nBusy = GetSourceBusy(event);

    switch (pSprite->type)  {
        case kSwitchCombo:
        {
            if (event.type == 3)
            {
                int nSprite2 = event.index;
                int nXSprite2 = sprite[nSprite2].extra;
                dassert(nXSprite2 > 0 && nXSprite2 < kMaxXSprites);
                pXSprite->data1 = xsprite[nXSprite2].data1;
                if (pXSprite->data1 == pXSprite->data2)
                    SetSpriteState(nSprite, pXSprite, 1, event.causer);
                else
                    SetSpriteState(nSprite, pXSprite, 0, event.causer);
            }
        }
        break;
        default:
        {
            pXSprite->busy = nBusy;
            if ((pXSprite->busy & 0xffff) == 0)
                SetSpriteState(nSprite, pXSprite, nBusy >> 16, event.causer);
        }
        break;
    }
}

void LinkWall(int nWall, XWALL *pXWall, const EVENT &event)
{
    int nBusy = GetSourceBusy(event);
    pXWall->busy = nBusy;
    if ((pXWall->busy & 0xffff) == 0)
        SetWallState(nWall, pXWall, nBusy>>16, event.causer);
}

void trTriggerSector(unsigned int nSector, XSECTOR *pXSector, int command, int causerID) {
    dassert(nSector < (unsigned int)numsectors);
    if (!pXSector->locked && !pXSector->isTriggered) {
        
        if (pXSector->triggerOnce) 
            pXSector->isTriggered = 1;
        
        if (pXSector->decoupled && pXSector->txID > 0)
            evSend(nSector, 6, pXSector->txID, (COMMAND_ID)pXSector->command, causerID);
        
        else {
            EVENT event;
            event.cmd = command;
            #ifdef NOONE_EXTENSIONS
                event.causer = (gModernMap) ? causerID : kCauserGame;
            #else
                event.causer = kCauserGame;
            #endif
            OperateSector(nSector, pXSector, event);
        }

    }
}

void trTriggerWall(unsigned int nWall, XWALL *pXWall, int command, int causerID) {
    dassert(nWall < (unsigned int)numwalls);
    if (!pXWall->locked && !pXWall->isTriggered) {
        
        if (pXWall->triggerOnce)
            pXWall->isTriggered = 1;
        
        if (pXWall->decoupled && pXWall->txID > 0)
            evSend(nWall, 0, pXWall->txID, (COMMAND_ID)pXWall->command, causerID);

        else {
            EVENT event;
            event.cmd = command;
            #ifdef NOONE_EXTENSIONS
                event.causer = (gModernMap) ? causerID : kCauserGame;
            #else
                event.causer = kCauserGame;
            #endif
            OperateWall(nWall, pXWall, event);
        }

    }
}

void trTriggerSprite(unsigned int nSprite, XSPRITE *pXSprite, int command, int causerID) {
    if (!pXSprite->locked && !pXSprite->isTriggered) {
        
        if (pXSprite->triggerOnce)
            pXSprite->isTriggered = 1;

        if (pXSprite->Decoupled && pXSprite->txID > 0)
           evSend(nSprite, 3, pXSprite->txID, (COMMAND_ID)pXSprite->command, causerID);
        
        else {
            EVENT event;
            event.cmd = command;
            #ifdef NOONE_EXTENSIONS
                event.causer = (gModernMap) ? causerID : kCauserGame;
            #else
                event.causer = kCauserGame;
            #endif
            OperateSprite(nSprite, pXSprite, event);
        }

    }
}


void trMessageSector(unsigned int nSector, const EVENT &event) {
    dassert(nSector < (unsigned int)numsectors);
    dassert(sector[nSector].extra > 0 && sector[nSector].extra < kMaxXSectors);
    XSECTOR *pXSector = &xsector[sector[nSector].extra];
    if (!pXSector->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
        switch (event.cmd) {
            case kCmdLink:
                LinkSector(nSector, pXSector, event);
                break;
            #ifdef NOONE_EXTENSIONS
            case kCmdModernUse:
                modernTypeTrigger(6, nSector, event);
                break;
            #endif
            default:
                OperateSector(nSector, pXSector, event);
                break;
        }
    }
}

void trMessageWall(unsigned int nWall, const EVENT &event) {
    dassert(nWall < (unsigned int)numwalls);
    dassert(wall[nWall].extra > 0 && wall[nWall].extra < kMaxXWalls);
    
    XWALL *pXWall = &xwall[wall[nWall].extra];
    if (!pXWall->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
        switch (event.cmd) {
            case kCmdLink:
                LinkWall(nWall, pXWall, event);
                break;
            #ifdef NOONE_EXTENSIONS
            case kCmdModernUse:
                modernTypeTrigger(0, nWall, event);
                break;
            #endif
            default:
                OperateWall(nWall, pXWall, event);
                break;
        }
    }
}

void trMessageSprite(unsigned int nSprite, const EVENT &event) {
    if (sprite[nSprite].statnum == kStatFree)
        return;
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->extra < 0 || pSprite->extra >= kMaxXSprites)
        return;
    XSPRITE* pXSprite = &xsprite[sprite[nSprite].extra];
    if (!pXSprite->locked || event.cmd == kCmdUnlock || event.cmd == kCmdToggleLock) {
        switch (event.cmd) {
            case kCmdLink:
                LinkSprite(nSprite, pXSprite, event);
                break;
            #ifdef NOONE_EXTENSIONS
            case kCmdModernUse:
                modernTypeTrigger(3, nSprite, event);
                break;
            #endif
            default:
                OperateSprite(nSprite, pXSprite, event);
                break;
        }
    }
}



void ProcessMotion(void)
{
    sectortype *pSector;
    int nSector;
    for (pSector = sector, nSector = 0; nSector < numsectors; nSector++, pSector++)
    {
        int nXSector = pSector->extra;
        if (nXSector <= 0)
            continue;
        XSECTOR *pXSector = &xsector[nXSector];
        if (pXSector->bobSpeed != 0)
        {
            if (pXSector->bobAlways)
                pXSector->bobTheta += pXSector->bobSpeed;
            else if (pXSector->busy == 0)
                continue;
            else
                pXSector->bobTheta += mulscale16(pXSector->bobSpeed, pXSector->busy);
            int vdi = mulscale30(Sin(pXSector->bobTheta), pXSector->bobZRange<<8);
            for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
            {
                spritetype *pSprite = &sprite[nSprite];
                if (pSprite->cstat&24576)
                {
                    viewBackupSpriteLoc(nSprite, pSprite);
                    pSprite->z += vdi;
                }
            }
            if (pXSector->bobFloor)
            {
                int floorZ = pSector->floorz;
                viewInterpolateSector(nSector, pSector);
                pSector->floorz = baseFloor[nSector]+vdi;
                for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
                {
                    spritetype *pSprite = &sprite[nSprite];
                    if (pSprite->flags&2)
                        pSprite->flags |= 4;
                    else
                    {
                        int top, bottom;
                        GetSpriteExtents(pSprite, &top, &bottom);
                        if (bottom >= floorZ && (pSprite->cstat&48) == 0)
                        {
                            viewBackupSpriteLoc(nSprite, pSprite);
                            pSprite->z += vdi;
                        }
                    }
                }
            }
            if (pXSector->bobCeiling)
            {
                int ceilZ = pSector->ceilingz;
                viewInterpolateSector(nSector, pSector);
                pSector->ceilingz = baseCeil[nSector]+vdi;
                for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
                {
                    spritetype *pSprite = &sprite[nSprite];
                    int top, bottom;
                    GetSpriteExtents(pSprite, &top, &bottom);
                    if (top <= ceilZ && (pSprite->cstat&48) == 0)
                    {
                        viewBackupSpriteLoc(nSprite, pSprite);
                        pSprite->z += vdi;
                    }
                }
            }
        }
    }
}

void AlignSlopes(void)
{
    sectortype *pSector;
    int nSector;
    for (pSector = sector, nSector = 0; nSector < numsectors; nSector++, pSector++)
    {
#ifndef EDUKE32
        if (pSector->filler)
#else
        if (qsector_filler[nSector])
#endif
        {
#ifndef EDUKE32
            walltype *pWall = &wall[pSector->wallptr+pSector->filler];
#else
            walltype *pWall = &wall[pSector->wallptr+qsector_filler[nSector]];
#endif
            walltype *pWall2 = &wall[pWall->point2];
            int nNextSector = pWall->nextsector;
            if (nNextSector >= 0)
            {
                int x = (pWall->x+pWall2->x)/2;
                int y = (pWall->y+pWall2->y)/2;
                viewInterpolateSector(nSector, pSector);
                alignflorslope(nSector, x, y, getflorzofslope(nNextSector, x, y));
                alignceilslope(nSector, x, y, getceilzofslope(nNextSector, x, y));
            }
        }
    }
}

int(*gBusyProc[])(unsigned int, unsigned int, int) =
{
    VCrushBusy,
    VSpriteBusy,
    VDoorBusy,
    HDoorBusy,
    RDoorBusy,
    StepRotateBusy,
    GenSectorBusy,
    PathBusy
};

void trProcessBusy(void)
{
    memset(velFloor, 0, sizeof(velFloor));
    memset(velCeil, 0, sizeof(velCeil));
    for (int i = gBusyCount-1; i >= 0; i--)
    {
        int nStatus;
        int oldBusy = gBusy[i].at8;
        gBusy[i].at8 = ClipRange(oldBusy+gBusy[i].at4*4, 0, 65536);
        #ifdef NOONE_EXTENSIONS
            if (!gModernMap || !xsector[sector[gBusy[i].at0].extra].unused1) nStatus = gBusyProc[gBusy[i].atc](gBusy[i].at0, gBusy[i].at8, -1);
            else nStatus = 3; // allow to pause/continue motion for sectors any time by sending special command
        #else
            nStatus = gBusyProc[gBusy[i].atc](gBusy[i].at0, gBusy[i].at8, -1);
        #endif
        switch (nStatus) {
            case 1:
                gBusy[i].at8 = oldBusy;
                break;
            case 2:
                gBusy[i].at8 = oldBusy;
                gBusy[i].at4 = -gBusy[i].at4;
                break;
            case 3:
                gBusy[i] = gBusy[--gBusyCount];
                break;
        }
    }
    ProcessMotion();
    AlignSlopes();
}

void InitGenerator(int);

void setBasePoint(int objType, int objIdx)
{
    switch (objType) {
        case 1:
            baseSprite[objIdx].x = sprite[objIdx].x;
            baseSprite[objIdx].y = sprite[objIdx].y;
            baseSprite[objIdx].z = sprite[objIdx].z;
            break;
        case 2:
            baseWall[objIdx].x = wall[objIdx].x;
            baseWall[objIdx].y = wall[objIdx].y;
            break;
        case 3:
            baseFloor[objIdx] = sector[objIdx].floorz;
            baseCeil[objIdx] = sector[objIdx].ceilingz;
            break;
    }
}

void setBaseWallSect(int nSect)
{
    int swal, ewal;
    swal = sector[nSect].wallptr;
    ewal = swal + sector[nSect].wallnum - 1;
    while(swal <= ewal)
        setBasePoint(2, swal++);
}

void setBaseSpriteSect(int nSect)
{
    int i;
    i = headspritesect[nSect];
    while (i >= 0)
    {
        setBasePoint(1, i);
        i = nextspritesect[i];
    }
}

void trInit(void)
{
    gBusyCount = 0;
    spritetype* pMark1, * pMark2;
    int i, * ptr;

    dassert((numsectors >= 0) && (numsectors < kMaxSectors));

#ifdef NOONE_EXTENSIONS
    if (gModernMap)
        gSprNSect.Init(); // collect sprites near outside walls
#endif
    
    for (i = 0; i < numwalls; i++)
    {
        setBasePoint(2, i);
        if (wall[i].extra <= 0)
            continue;

        dassert(wall[i].extra < kMaxXWalls);
        XWALL* pXWall = &xwall[wall[i].extra];
        if (pXWall->state)
            pXWall->busy = 65536;
    }

    for (i = 0; i < kMaxSprites; i++)
    {
        sprite[i].inittype = -1;
        if (sprite[i].statnum >= kStatFree)
            continue;

        setBasePoint(1, i);
        sprite[i].inittype = sprite[i].type;
    }


    for (i = 0; i < numsectors; i++)
    {
        pMark2 = NULL;
        sectortype* pSector = &sector[i];
        setBasePoint(3, i);
        int nXSector = pSector->extra;
        if (nXSector > 0)
        {
            dassert(nXSector < kMaxXSectors);
            XSECTOR* pXSector = &xsector[nXSector];
            if (pXSector->state)
                pXSector->busy = 65536;
            switch (pSector->type) {
            case kSectorCounter:
                #ifdef NOONE_EXTENSIONS
                    if (gModernMap) pXSector->triggerOff = false;
                    else pXSector->triggerOnce = 1;
                #else
                    pXSector->triggerOnce = 1;
                #endif
                evPost(i, 6, 0, kCallbackCounterCheck);
                break;
            case kSectorSlideMarked:
            case kSectorSlide:
                // grab second marker
                pMark2 = &sprite[pXSector->marker1];
                fallthrough__;
            case kSectorRotateMarked:
            case kSectorRotate:
                // grab first marker
                pMark1 = &sprite[pXSector->marker0];
                if (pMark2) TranslateSector(i, 0, -65536, pMark1->x, pMark1->y, pMark1->x, pMark1->y, pMark1->ang, pMark2->x, pMark2->y, pMark2->ang, pSector->type == kSectorSlide);
                else TranslateSector(i, 0, -65536, pMark1->x, pMark1->y, pMark1->x, pMark1->y, 0, pMark1->x, pMark1->y, pMark1->ang, pSector->type == kSectorRotate);
                
                #ifdef NOONE_EXTENSIONS
                // must set basepoint for outside sprites as well
                if (gModernMap && (ptr = gSprNSect.GetSprPtr(i)) != NULL)
                {
                    while (*ptr >= 0)
                        setBasePoint(1, *ptr++);
                }
                #endif

                setBaseWallSect(i); setBaseSpriteSect(i);

                if (pMark2) TranslateSector(i, 0, pXSector->busy, pMark1->x, pMark1->y, pMark1->x, pMark1->y, pMark1->ang, pMark2->x, pMark2->y, pMark2->ang, pSector->type == kSectorSlide);
                else TranslateSector(i, 0, pXSector->busy, pMark1->x, pMark1->y, pMark1->x, pMark1->y, 0, pMark1->x, pMark1->y, pMark1->ang, pSector->type == kSectorRotate);
                fallthrough__;
            case kSectorZMotionSprite:
            case kSectorZMotion:
                ZTranslateSector(i, pXSector, pXSector->busy, 1);
                break;
            case kSectorPath:
                InitPath(i, pXSector);
                break;
            default:
                break;
            }
        }
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        int nXSprite = sprite[i].extra;
        if (sprite[i].statnum < kStatFree && nXSprite > 0)
        {
            dassert(nXSprite < kMaxXSprites);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (pXSprite->state)
                pXSprite->busy = 65536;
            switch (sprite[i].type) {
            case kSwitchPadlock:
                pXSprite->triggerOnce = 1;
                break;
            #ifdef NOONE_EXTENSIONS
            case kModernRandom:
            case kModernRandom2:
                if (!gModernMap || pXSprite->state == pXSprite->restState) break;
                evPost(i, 3, (120 * pXSprite->busyTime) / 10, kCmdRepeat, i);
                if (pXSprite->waitTime > 0)
                    evPost(i, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff, i);
                break;
            case kModernSeqSpawner:
            case kModernObjDataAccumulator:
            case kModernDudeTargetChanger:
            case kModernEffectSpawner:
            case kModernWindGenerator:
                if (pXSprite->state == pXSprite->restState) break;
                evPost(i, 3, 0, kCmdRepeat, i);
                if (pXSprite->waitTime > 0)
                    evPost(i, 3, (pXSprite->waitTime * 120) / 10, pXSprite->restState ? kCmdOn : kCmdOff, i);
                break;
            #endif
            case kGenTrigger:
            case kGenDripWater:
            case kGenDripBlood:
            case kGenMissileFireball:
            case kGenDart:
            case kGenBubble:
            case kGenBubbleMulti:
            case kGenMissileEctoSkull:
            case kGenSound:
                InitGenerator(i);
                break;
            case kThingArmedProxBomb:
                pXSprite->Proximity = 1;
                break;
            case kThingFallingRock:
                if (pXSprite->state) sprite[i].flags |= 7;
                else sprite[i].flags &= ~7;
                break;
            }
            if (pXSprite->Vector) sprite[i].cstat |= CSTAT_SPRITE_BLOCK_HITSCAN;
            if (pXSprite->Push) sprite[i].cstat |= 4096;
        }
    }
    
    evSend(0, 0, kChannelLevelStart, kCmdOn, kCauserGame);
    #ifdef NOONE_EXTENSIONS
        if (gModernMap)
        {
            #ifdef TRIGGER_START_CHANNEL_NBLOOD
                evSend(0, 0, TRIGGER_START_CHANNEL_NBLOOD, kCmdOn, kCauserGame);
            #endif

            #ifdef TRIGGER_START_CHANNEL_RAZE
                 evSend(0, 0, TRIGGER_START_CHANNEL_RAZE, kCmdOn, kCauserGame);
            #endif
        }
    #endif

    switch (gGameOptions.nGameType) {
        case 1:
            evSend(0, 0, kChannelLevelStartCoop, kCmdOn, kCauserGame);
            break;
        case 2:
            evSend(0, 0, kChannelLevelStartMatch, kCmdOn, kCauserGame);
            break;
        case 3:
            evSend(0, 0, kChannelLevelStartMatch, kCmdOn, kCauserGame);
            evSend(0, 0, kChannelLevelStartTeamsOnly, kCmdOn, kCauserGame);
            break;
    }
}

void trTextOver(int nId)
{
    char *pzMessage = levelGetMessage(nId);
    if (pzMessage)
        viewSetMessage(pzMessage, VanillaMode() ? 0 : 8, MESSAGE_PRIORITY_INI); // 8: gold
}

void InitGenerator(int nSprite)
{
    dassert(nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    dassert(pSprite->statnum != kMaxStatus);
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0);
    XSPRITE *pXSprite = &xsprite[nXSprite];
    switch (sprite[nSprite].type) {
        case kGenTrigger:
            pSprite->cstat &= ~CSTAT_SPRITE_BLOCK;
            pSprite->cstat |= CSTAT_SPRITE_INVISIBLE;
            break;
    }
    if (pXSprite->state != pXSprite->restState && pXSprite->busyTime > 0)
        evPost(nSprite, 3, (120*(pXSprite->busyTime+Random2(pXSprite->data1)))/10, kCmdRepeat, nSprite);
}

void ActivateGenerator(int nSprite)
{
    dassert(nSprite < kMaxSprites);
    spritetype *pSprite = &sprite[nSprite];
    dassert(pSprite->statnum != kMaxStatus);
    int nXSprite = pSprite->extra;
    dassert(nXSprite > 0);
    XSPRITE *pXSprite = &xsprite[nXSprite];
    switch (pSprite->type) {
        case kGenDripWater:
        case kGenDripBlood: {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            actSpawnThing(pSprite->sectnum, pSprite->x, pSprite->y, bottom, (pSprite->type == kGenDripWater) ? kThingDripWater : kThingDripBlood);
            break;
        }
        case kGenSound:
            sfxPlay3DSound(pSprite, pXSprite->data2, -1, 0);
            break;
        case kGenMissileFireball:
            switch (pXSprite->data2) {
                case 0:
                    FireballTrapSeqCallback(3, nXSprite);
                    break;
                case 1:
                    seqSpawn(35, 3, nXSprite, nFireballTrapClient);
                    break;
                case 2:
                    seqSpawn(36, 3, nXSprite, nFireballTrapClient);
                    break;
            }
            break;
        case kGenMissileEctoSkull:
            break;
        case kGenBubble:
        case kGenBubbleMulti: {
            int top, bottom;
            GetSpriteExtents(pSprite, &top, &bottom);
            gFX.fxSpawn((pSprite->type == kGenBubble) ? FX_23 : FX_26, pSprite->sectnum, pSprite->x, pSprite->y, top);
            break;
        }
    }
}

void FireballTrapSeqCallback(int, int nXSprite)
{
    XSPRITE *pXSprite = &xsprite[nXSprite];
    int nSprite = pXSprite->reference;
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->cstat&32)
        actFireMissile(pSprite, 0, 0, 0, 0, (pSprite->cstat&8) ? 0x4000 : -0x4000, kMissileFireball);
    else
        actFireMissile(pSprite, 0, 0, Cos(pSprite->ang)>>16, Sin(pSprite->ang)>>16, 0, kMissileFireball);
}


void MGunFireSeqCallback(int, int nXSprite)
{
    int nSprite = xsprite[nXSprite].reference;
    spritetype *pSprite = &sprite[nSprite];
    XSPRITE *pXSprite = &xsprite[nXSprite];
    if (pXSprite->data2 > 0 || pXSprite->data1 == 0)
    {
        if (pXSprite->data2 > 0)
        {
            pXSprite->data2--;
            if (pXSprite->data2 == 0)
                evPost(nSprite, 3, 1, kCmdOff, nSprite);
        }
        int dx = (Cos(pSprite->ang)>>16)+Random2(1000);
        int dy = (Sin(pSprite->ang)>>16)+Random2(1000);
        int dz = Random2(1000);
        actFireVector(pSprite, 0, 0, dx, dy, dz, kVectorBullet);
        sfxPlay3DSound(pSprite, 359, -1, 0);
    }
}

void MGunOpenSeqCallback(int, int nXSprite)
{
    seqSpawn(39, 3, nXSprite, nMGunFireClient);
}

class TriggersLoadSave : public LoadSave
{
public:
    virtual void Load();
    virtual void Save();
};

void TriggersLoadSave::Load()
{
    Read(&gBusyCount, sizeof(gBusyCount));
    Read(gBusy, sizeof(gBusy));
    Read(basePath, sizeof(basePath));
}

void TriggersLoadSave::Save()
{
    Write(&gBusyCount, sizeof(gBusyCount));
    Write(gBusy, sizeof(gBusy));
    Write(basePath, sizeof(basePath));
}

static TriggersLoadSave *myLoadSave;

void TriggersLoadSaveConstruct(void)
{
    myLoadSave = new TriggersLoadSave();
}
