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

#include "callback.h"
#include "db.h"
#include "eventq.h"
#include "globals.h"
#include "loadsave.h"
#include "pqueue.h"
#include "triggers.h"
#include "view.h"
#ifdef NOONE_EXTENSIONS
#include "nnexts.h"
#endif

#ifndef EDUKE32
class PriorityQueue
{
public:
    queueItem<EVENT> queueItems[kPQueueSize + 1];
    uint32_t fNodeCount; // at2008
    uint32_t Size(void) { return fNodeCount; };
    void Clear(void)
    {
        fNodeCount = 0;
        memset(queueItems, 0, sizeof(queueItems));
    }
    void Upheap(void)
    {
        queueItem<EVENT> item = queueItems[fNodeCount];
        queueItems[0].at0 = 0;
        uint32_t x = fNodeCount;
        while (queueItems[x>>1] > item)
        {
            queueItems[x] = queueItems[x>>1];
            x >>= 1;
        }
        queueItems[x] = item;
    }
    void Downheap(uint32_t n)
    {
        queueItem<EVENT> item = queueItems[n];
        while (fNodeCount/2 >= n)
        {
            uint32_t t = n*2;
            if (t < fNodeCount && queueItems[t] > queueItems[t+1])
                t++;
            if (item <= queueItems[t])
                break;
            queueItems[n] = queueItems[t];
            n = t;
        }
        queueItems[n] = item;
    }
    void Delete(uint32_t k)
    {
        dassert(k <= fNodeCount);
        queueItems[k] = queueItems[fNodeCount--];
        Downheap(k);
    }
    void Insert(uint32_t a1, EVENT a2)
    {
        dassert(fNodeCount < kPQueueSize);
        fNodeCount++;
        queueItems[fNodeCount].at0 = a1;
        queueItems[fNodeCount].at4 = a2;
        Upheap();
    }
    EVENT Remove(void)
    {
        EVENT data = queueItems[1].at4;
        queueItems[1] = queueItems[fNodeCount--];
        Downheap(1);
        return data;
    }
    uint32_t LowestPriority(void)
    {
        dassert(fNodeCount > 0);
        return queueItems[1].at0;
    }
    void Kill(int a1, int a2)
    {
        for (unsigned int i = 1; i <= fNodeCount;)
        {
            EVENT &nItem = queueItems[i].at4;
            if (nItem.index == a1 && nItem.type == a2)
                Delete(i);
            else
                i++;
        }
    }
    void Kill(int idx, int type, int causer)
    {
        for (unsigned int i = 1; i <= fNodeCount;)
        {
            EVENT &nItem = queueItems[i].at4;
            if (nItem.index == idx && nItem.type == type && nItem.causer == causer)
                Delete(i);
            else
                i++;
        }
    }
    void Kill(int a1, int a2, CALLBACK_ID a3)
    {
        for (unsigned int i = 1; i <= fNodeCount;)
        {
            EVENT &nItem = queueItems[i].at4;
            if (nItem.index == a1 && nItem.type == a2 && nItem.cmd == kCmdCallback && nItem.funcID == (unsigned int)a3)
                Delete(i);
            else
                i++;
        }
    }
};
#endif

class EventQueue
{
public:
#ifndef EDUKE32
    PriorityQueue* PQueue;
#else
    PriorityQueue<EVENT>* PQueue;
#endif
    EventQueue()
    {
        PQueue = NULL;
    }
    bool IsNotEmpty(unsigned int nTime)
    {
        return PQueue->Size() > 0 && nTime >= PQueue->LowestPriority();
    }
    EVENT ERemove(void)
    {
        return PQueue->Remove();
    }
    void Kill(int, int);
    void Kill(int idx, int type, int causer);
    void Kill(int, int, CALLBACK_ID);
};

EventQueue eventQ;
void EventQueue::Kill(int a1, int a2)
{
#ifndef EDUKE32
    PQueue->Kill(a1, a2);
#else
    PQueue->Kill([=](EVENT nItem)->bool {return (nItem.index == a1 && nItem.type == a2); });
#endif
}

void EventQueue::Kill(int idx, int type, int causer)
{
#ifndef EDUKE32
    PQueue->Kill(idx, type, causer);
#else
    PQueue->Kill([=](EVENT nItem)->bool { return (nItem.index == idx && nItem.type == type && nItem.causer == causer); });
#endif
}

void EventQueue::Kill(int a1, int a2, CALLBACK_ID a3)
{
#ifndef EDUKE32
    PQueue->Kill(a1, a2, a3);
#else
    PQueue->Kill([=](EVENT nItem)->bool {return (nItem.index == a1 && nItem.type == a2 && nItem.cmd == kCmdCallback && nItem.funcID == (unsigned int)a3); });
#endif
}

RXBUCKET rxBucket[kChannelMax+1];

int GetBucketChannel(const RXBUCKET *pRX)
{
    switch (pRX->type) {
        case 6: {
            int nIndex = pRX->index;
            int nXIndex = sector[nIndex].extra;
            dassert(nXIndex > 0);
            return xsector[nXIndex].rxID;
        }
        case 0: {
            int nIndex = pRX->index;
            int nXIndex = wall[nIndex].extra;
            dassert(nXIndex > 0);
            return xwall[nXIndex].rxID;
        }
        case 3: {
            int nIndex = pRX->index;
            int nXIndex = sprite[nIndex].extra;
            dassert(nXIndex > 0);
            return xsprite[nXIndex].rxID;
        }
    }
    return 0;
}

#if 0
int CompareChannels(const void *a, const void *b)
{
    return GetBucketChannel((const RXBUCKET*)a)-GetBucketChannel((const RXBUCKET*)b);
}
#else
static int CompareChannels(RXBUCKET *a, RXBUCKET *b)
{
    return GetBucketChannel(a) - GetBucketChannel(b);
}
#endif

static RXBUCKET *SortGetMiddle(RXBUCKET *a1, RXBUCKET *a2, RXBUCKET *a3)
{
    if (CompareChannels(a1, a2) > 0)
    {
        if (CompareChannels(a1, a3) > 0)
        {
            if (CompareChannels(a2, a3) > 0)
                return a2;
            return a3;
        }
        return a1;
    }
    else
    {
        if (CompareChannels(a1, a3) < 0)
        {
            if (CompareChannels(a2, a3) > 0)
                return a3;
            return a2;
        }
        return a1;
    }
}

static void SortSwap(RXBUCKET *a, RXBUCKET *b)
{
    RXBUCKET t = *a;
    *a = *b;
    *b = t;
}

static void SortRXBucket(int nCount)
{
    RXBUCKET *v144[32];
    int vc4[32];
    int v14 = 0;
    RXBUCKET *pArray = rxBucket;
    while (true)
    {
        while (nCount > 1)
        {
            if (nCount < 16)
            {
                for (int nDist = 3; nDist > 0; nDist -= 2)
                {
                    for (RXBUCKET *pI = pArray+nDist; pI < pArray+nCount; pI += nDist)
                    {
                        for (RXBUCKET *pJ = pI; pJ > pArray && CompareChannels(pJ-nDist, pJ) > 0; pJ -= nDist)
                        {
                            SortSwap(pJ, pJ-nDist);
                        }
                    }
                }
                break;
            }
            RXBUCKET *v30, *vdi, *vsi;
            vdi = pArray + nCount / 2;
            if (nCount > 29)
            {
                v30 = pArray;
                vsi = pArray + nCount-1;
                if (nCount > 42)
                {
                    int v20 = nCount / 8;
                    v30 = SortGetMiddle(v30, v30+v20, v30+v20*2);
                    vdi = SortGetMiddle(vdi-v20, vdi, vdi+v20);
                    vsi = SortGetMiddle(vsi-v20*2, vsi-v20, vsi);
                }
                vdi = SortGetMiddle(v30, vdi, vsi);
            }
            RXBUCKET v44 = *vdi;
            RXBUCKET *vc = pArray;
            RXBUCKET *v8 = pArray+nCount-1;
            RXBUCKET *vbx = vc;
            RXBUCKET *v4 = v8;
            while (true)
            {
                while (vbx <= v4)
                {
                    int nCmp = CompareChannels(vbx, &v44);
                    if (nCmp > 0)
                        break;
                    if (nCmp == 0)
                    {
                        SortSwap(vbx, vc);
                        vc++;
                    }
                    vbx++;
                }
                while (vbx <= v4)
                {
                    int nCmp = CompareChannels(v4, &v44);
                    if (nCmp < 0)
                        break;
                    if (nCmp == 0)
                    {
                        SortSwap(v4, v8);
                        v8--;
                    }
                    v4--;
                }
                if (vbx > v4)
                    break;
                SortSwap(vbx, v4);
                v4--;
                vbx++;
            }
            RXBUCKET *v2c = pArray+nCount;
            int vt = ClipHigh(vbx-vc, vc-pArray);
            for (int i = 0; i < vt; i++)
            {
                SortSwap(&vbx[i-vt], &pArray[i]);
            }
            vt = ClipHigh(v8-v4, v2c-v8-1);
            for (int i = 0; i < vt; i++)
            {
                SortSwap(&v2c[i-vt], &vbx[i]);
            }
            int vvsi = v8-v4;
            int vvdi = vbx-vc;
            if (vvsi >= vvdi)
            {
                vc4[v14] = vvsi;
                v144[v14] = v2c-vvsi;
                nCount = vvdi;
                v14++;
            }
            else
            {
                vc4[v14] = vvdi;
                v144[v14] = pArray;
                nCount = vvsi;
                pArray = v2c - vvsi;
                v14++;
            }
        }
        if (v14 == 0)
            return;
        v14--;
        pArray = v144[v14];
        nCount = vc4[v14];
    }
}

unsigned short bucketHead[1024+1];

void evInit(void)
{
    if (eventQ.PQueue)
        delete eventQ.PQueue;
#ifndef EDUKE32
    eventQ.PQueue = new PriorityQueue();
#else
    if (VanillaMode())
        eventQ.PQueue = new VanillaPriorityQueue<EVENT>();
    else
        eventQ.PQueue = new StdPriorityQueue<EVENT>();
#endif
    eventQ.PQueue->Clear();
    int nCount = 0;
    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector >= kMaxXSectors)
            ThrowError("Invalid xsector reference in sector %d", i);
        if (nXSector > 0 && xsector[nXSector].rxID > 0)
        {
            dassert(nCount < kChannelMax);
            rxBucket[nCount].type = 6;
            rxBucket[nCount].index = i;
            nCount++;
        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        int nXWall = wall[i].extra;
        if (nXWall >= kMaxXWalls)
            ThrowError("Invalid xwall reference in wall %d", i);
        if (nXWall > 0 && xwall[nXWall].rxID > 0)
        {
            dassert(nCount < kChannelMax);
            rxBucket[nCount].type = 0;
            rxBucket[nCount].index = i;
            nCount++;
        }
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            int nXSprite = sprite[i].extra;
            if (nXSprite >= kMaxXSprites)
                ThrowError("Invalid xsprite reference in sprite %d", i);
            if (nXSprite > 0 && xsprite[nXSprite].rxID > 0)
            {
                dassert(nCount < kChannelMax);
                rxBucket[nCount].type = 3;
                rxBucket[nCount].index = i;
                nCount++;
            }
        }
    }
    SortRXBucket(nCount);
    int i, j = 0;
    for (i = 0; i < 1024; i++)
    {
        bucketHead[i] = j;
        while(j < nCount && GetBucketChannel(&rxBucket[j]) == i)
            j++;
    }
    bucketHead[i] = j;
}

char evGetSourceState(int nType, int nIndex)
{
    switch (nType)
    {
    case 6:
    {
        int nXIndex = sector[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXSectors);
        return xsector[nXIndex].state;
    }
    case 0:
    {
        int nXIndex = wall[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXWalls);
        return xwall[nXIndex].state;
    }
    case 3:
    {
        int nXIndex = sprite[nIndex].extra;
        dassert(nXIndex > 0 && nXIndex < kMaxXSprites);
        return xsprite[nXIndex].state;
    }
    }
    return 0;
}

void evSend(int nIndex, int nType, int rxId, COMMAND_ID command, int causerID)
{
    switch (command) {
        case kCmdState:
            command = evGetSourceState(nType, nIndex) ? kCmdOn : kCmdOff;
            break;
        case kCmdNotState:
            command = evGetSourceState(nType, nIndex) ? kCmdOff : kCmdOn;
            break;
        default:
            break;
    }
    
    EVENT event;
    event.index = nIndex;
    event.type = nType;
    event.cmd = command;
    #ifdef NOONE_EXTENSIONS
        event.causer = (gModernMap) ? causerID : kCauserGame;
    #else
        event.causer = kCauserGame;
    #endif

    switch (rxId) {
    case kChannelTextOver:
        if (command >= kCmdNumberic) trTextOver(command - kCmdNumberic);
        else viewSetSystemMessage("Invalid TextOver command by xobject #%d (object type %d)", nIndex, nType);
        return;
    case kChannelLevelExitNormal:
        levelEndLevel(kLevelExitNormal);
        return;
    case kChannelLevelExitSecret:
        levelEndLevel(kLevelExitSecret);
        return;
    #ifdef NOONE_EXTENSIONS
    // finished level and load custom level � via numbered command.
    case kChannelModernEndLevelCustom:
        if (command >= kCmdNumberic) levelEndLevelCustom(command - kCmdNumberic);
        else viewSetSystemMessage("Invalid Level-Exit# command by xobject #%d (object type %d)", nIndex, nType);
        return;
    #endif
    case kChannelSetTotalSecrets:
        if (command >= kCmdNumberic) levelSetupSecret(command - kCmdNumberic);
        else viewSetSystemMessage("Invalid Total-Secrets command by xobject #%d (object type %d)", nIndex, nType);
        break;
    case kChannelSecretFound:
        if (command >= kCmdNumberic) levelTriggerSecret(command - kCmdNumberic);
        else viewSetSystemMessage("Invalid Trigger-Secret command by xobject #%d (object type %d)", nIndex, nType);
        break;
    case kChannelRemoteBomb0:
    case kChannelRemoteBomb1:
    case kChannelRemoteBomb2:
    case kChannelRemoteBomb3:
    case kChannelRemoteBomb4:
    case kChannelRemoteBomb5:
    case kChannelRemoteBomb6:
    case kChannelRemoteBomb7:
        for (int nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite])
        {
            spritetype* pSprite = &sprite[nSprite];
            if (pSprite->flags & 32)
                continue;
            int nXSprite = pSprite->extra;
            if (nXSprite > 0)
            {
                XSPRITE* pXSprite = &xsprite[nXSprite];
                if (pXSprite->rxID == rxId)
                    trMessageSprite(nSprite, event);
            }
        }
        return;
    case kChannelTeamAFlagCaptured:
    case kChannelTeamBFlagCaptured:
        for (int nSprite = headspritestat[kStatItem]; nSprite >= 0; nSprite = nextspritestat[nSprite])
        {
            spritetype* pSprite = &sprite[nSprite];
            if (pSprite->flags & 32)
                continue;
            int nXSprite = pSprite->extra;
            if (nXSprite > 0)
            {
                XSPRITE* pXSprite = &xsprite[nXSprite];
                if (pXSprite->rxID == rxId)
                    trMessageSprite(nSprite, event);
            }
        }
        return;
    default:
        break;
    }

    #ifdef NOONE_EXTENSIONS
    if (gModernMap)
    {       
        // allow to send commands on player sprites
        PLAYER* pPlayer = NULL;
        if (playerRXRngIsFine(rxId))
        {
            if ((pPlayer = getPlayerById((rxId - kChannelPlayer7) + kMaxPlayers)) != NULL)
            {
                if (command == kCmdEventKillFull)
                    evKill(pPlayer->nSprite, OBJ_SPRITE);
                else
                    trMessageSprite(pPlayer->nSprite, event);
            }
           
            return;

        }
        else if (rxId == kChannelAllPlayers)
        {
            for (int i = 0; i < kMaxPlayers; i++)
            {
                if ((pPlayer = getPlayerById(i)) != NULL)
                {
                    if (command == kCmdEventKillFull)
                        evKill(pPlayer->nSprite, OBJ_SPRITE);
                    else
                        trMessageSprite(pPlayer->nSprite, event);
                }
            }
            
            return;
        }
        // send command on sprite which created the event sequence
        else if (rxId == kChannelEventCauser && event.causer < kCauserGame)
        {
            spritetype* pSpr = &sprite[event.causer];
            if (!(pSpr->flags & kHitagFree) && !(pSpr->flags & kHitagRespawn))
            {
                if (command == kCmdEventKillFull)
                    evKill(causerID, OBJ_SPRITE);
                else
                    trMessageSprite(event.causer, event);
            }
            
            return;
        }
        else if (command == kCmdEventKillFull)
        {
            killEvents(rxId, command);
            return;
        }
    }
    #endif

    for (int i = bucketHead[rxId]; i < bucketHead[rxId+1]; i++) {
        if (event.type != rxBucket[i].type || event.index != rxBucket[i].index) {
            switch (rxBucket[i].type) {
                case 6:
                    trMessageSector(rxBucket[i].index, event);
                    break;
                case 0:
                    trMessageWall(rxBucket[i].index, event);
                    break;
                case 3:
                {
                    int nSprite = rxBucket[i].index;
                    spritetype *pSprite = &sprite[nSprite];
                    if (pSprite->flags&32)
                        continue;
                    int nXSprite = pSprite->extra;
                    if (nXSprite > 0)
                    {
                        XSPRITE *pXSprite = &xsprite[nXSprite];
                        if (pXSprite->rxID > 0)
                            trMessageSprite(nSprite, event);
                    }
                    break;
                }
            }
        }
    }
}

void evPost(int nIndex, int nType, unsigned int nDelta, COMMAND_ID command, int causerID) {
    dassert(command != kCmdCallback);
    if (command == kCmdState) command = evGetSourceState(nType, nIndex) ? kCmdOn : kCmdOff;
    else if (command == kCmdNotState) command = evGetSourceState(nType, nIndex) ? kCmdOff : kCmdOn;
    EVENT evn = {};
    evn.index = nIndex;
    evn.type = nType;
    evn.cmd = command;
    #ifdef NOONE_EXTENSIONS
        evn.causer = (gModernMap) ? causerID : kCauserGame;
    #else
        evn.causer = kCauserGame;
    #endif

    eventQ.PQueue->Insert((int)gFrameClock+nDelta, evn);
}

void evPost(int nIndex, int nType, unsigned int nDelta, CALLBACK_ID callback) {
    EVENT evn = {};
    evn.index = nIndex;
    evn.type = nType;
    evn.cmd = kCmdCallback;
    evn.funcID = callback;
    evn.causer = kCauserGame;
    eventQ.PQueue->Insert((int)gFrameClock+nDelta, evn);
}

void evProcess(unsigned int nTime)
{
#if 0
    while (1)
    {
        // Inlined?
        char bDone;
        if (eventQ.fNodeCount > 0 && nTime >= eventQ.queueItems[1])
            bDone = 1;
        else
            bDone = 0;
        if (!bDone)
            break;
#endif
    while(eventQ.IsNotEmpty(nTime))
    {
        EVENT event = eventQ.ERemove();
        if (event.cmd == kCmdCallback)
        {
            dassert(event.funcID < kCallbackMax);
            dassert(gCallback[event.funcID] != NULL);
            gCallback[event.funcID](event.index);
        }
        else
        {
            switch (event.type)
            {
            case 6:
                trMessageSector(event.index, event);
                break;
            case 0:
                trMessageWall(event.index, event);
                break;
            case 3:
                trMessageSprite(event.index, event);
                break;
            }
        }
    }
}

void evKill(int a1, int a2)
{
    eventQ.Kill(a1, a2);
}

void evKill(int idx, int type, int causer)
{
    #ifdef NOONE_EXTENSIONS
    if (gModernMap)
        eventQ.Kill(idx, type, causer);
    else
    #endif
        eventQ.Kill(idx, type);
}

void evKill(int a1, int a2, CALLBACK_ID a3)
{
    eventQ.Kill(a1, a2, a3);
}

class EventQLoadSave : public LoadSave
{
public:
    virtual void Load();
    virtual void Save();
};

void EventQLoadSave::Load()
{
    if (eventQ.PQueue)
        delete eventQ.PQueue;
    Read(&eventQ, sizeof(eventQ));
#ifndef EDUKE32
    eventQ.PQueue = new PriorityQueue();
#else
    if (VanillaMode())
        eventQ.PQueue = new VanillaPriorityQueue<EVENT>();
    else
        eventQ.PQueue = new StdPriorityQueue<EVENT>();
#endif
    int nEvents;
    Read(&nEvents, sizeof(nEvents));
    for (int i = 0; i < nEvents; i++)
    {
        EVENT event = {};
        unsigned int eventtime;
        Read(&eventtime, sizeof(eventtime));
        Read(&event, sizeof(event));
        eventQ.PQueue->Insert(eventtime, event);
    }
    Read(rxBucket, sizeof(rxBucket));
    Read(bucketHead, sizeof(bucketHead));
}

void EventQLoadSave::Save()
{
#ifdef __AMIGA__
    EVENT *events = new EVENT[1024];
    unsigned int *eventstime = new unsigned int[1024];
#else
    EVENT events[1024];
    unsigned int eventstime[1024];
#endif
    Write(&eventQ, sizeof(eventQ));
    int nEvents = eventQ.PQueue->Size();
    Write(&nEvents, sizeof(nEvents));
    for (int i = 0; i < nEvents; i++)
    {
        eventstime[i] = eventQ.PQueue->LowestPriority();
        events[i] = eventQ.ERemove();
        Write(&eventstime[i], sizeof(eventstime[i]));
        Write(&events[i], sizeof(events[i]));
    }
    dassert(eventQ.PQueue->Size() == 0);
    for (int i = 0; i < nEvents; i++)
    {
        eventQ.PQueue->Insert(eventstime[i], events[i]);
    }
    Write(rxBucket, sizeof(rxBucket));
    Write(bucketHead, sizeof(bucketHead));
#ifdef __AMIGA__
    delete[] events;
    delete[] eventstime;
#endif
}

static EventQLoadSave *myLoadSave;

void EventQLoadSaveConstruct(void)
{
    myLoadSave = new EventQLoadSave();
}
