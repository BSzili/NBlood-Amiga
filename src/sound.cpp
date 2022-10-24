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
#include "build.h"
#include "compat.h"
#include "music.h"
#include "fx_man.h"
#ifndef EDUKE32
#include "cd.h"
#else
#include "al_midi.h"
#endif
#include "common_game.h"
#include "config.h"
#include "levels.h"
#include "resource.h"
#include "sound.h"
#include "renderlayer.h"


int32_t SoundToggle;
int32_t MusicToggle;
int32_t CDAudioToggle;
int32_t FXVolume;
int32_t MusicVolume;
int32_t CDVolume;
int32_t NumVoices;
int32_t NumChannels;
int32_t NumBits;
int32_t MixRate;
#ifdef ASS_REVERSESTEREO
int32_t ReverseStereo;
#endif
int32_t MusicDevice;
#ifndef EDUKE32
int32_t FXDevice;
#endif

Resource gSoundRes;

bool sndActive = false;

int soundRates[13] = {
    11025,
    11025,
    11025,
    11025,
    11025,
    22050,
    22050,
    22050,
    22050,
    44100,
    44100,
    44100,
    44100,
};
#define kChannelMax 32

int sndGetRate(int format)
{
    if (format < 13)
        return soundRates[format];
    return 11025;
}

SAMPLE2D Channel[kChannelMax];

SAMPLE2D * FindChannel(void)
{
    for (int i = kChannelMax - 1; i >= 0; i--)
        if (Channel[i].at5 == 0) return &Channel[i];
    consoleSysMsg("No free channel available for sample");
    //ThrowError("No free channel available for sample");
    return NULL;
}

DICTNODE *hSong;
char *pSongPtr;
int nSongSize;
bool bWaveMusic;
int nWaveMusicHandle;

#ifndef EDUKE32
// MIDI -> MP3, WAV replacement function
#undef S_OpenAudio
static int32_t S_OpenAudio(const char *fn, char searchfirst, uint8_t const ismusic)
{
    char testfn[16];
    strcpy(testfn, fn);
    strcat(testfn, ".mp3");
    int32_t fp = kopen4loadfrommod(testfn, searchfirst);
    if (fp < 0)
    {
        strcpy(testfn, fn);
        strcat(testfn, ".wav");
        fp = kopen4loadfrommod(testfn, searchfirst);
    }
    return fp;
}
#endif

int sndPlaySong(const char *songName, bool bLoop)
{
    if (!MusicToggle)
        return 0;
    if (!songName || strlen(songName) == 0)
        return 1;

    int32_t fp = S_OpenAudio(songName, 0, 1);
#ifndef EDUKE32
    if (fp < 0 && CDAudioToggle)
    {
        int songId = -1;
        int numMatches = sscanf(songName, "blood%02d", &songId);
        if (numMatches > 0 && songId > 0)
        {
            sndStopSong();
            int32_t retval = CD_Play(songId, bLoop);
            if (retval == CD_Ok)
            {
                return 0;
            }
            OSD_Printf(OSD_ERROR "sndPlaySong(): error: %s\n", CD_ErrorString(retval));
        }
    }
#endif
    if (EDUKE32_PREDICT_FALSE(fp < 0))
    {
        hSong = gSoundRes.Lookup(songName, "MID");
        if (!hSong)
        {
            OSD_Printf(OSD_ERROR "sndPlaySong(): error: can't open \"%s\" for playback!\n", songName);
            return 2;
        }
        int nNewSongSize = hSong->size;
        char *pNewSongPtr = (char *)Xaligned_alloc(16, nNewSongSize);
#ifdef __AMIGA__
        if (!pNewSongPtr)
        {
            OSD_Printf(OSD_ERROR "sndPlaySong(): error: can't allocate %d bytes\n", nNewSongSize);
            return 1;
        }
#endif
        gSoundRes.Load(hSong, pNewSongPtr);
        MUSIC_SetVolume(MusicVolume);
        int32_t retval = MUSIC_PlaySong(pNewSongPtr, nNewSongSize, bLoop);

        if (retval != MUSIC_Ok)
        {
#ifndef EDUKE32
            OSD_Printf(OSD_ERROR "sndPlaySong(): error: can't play music: %s\n", MUSIC_ErrorString(MUSIC_ErrorCode)); 
#endif
            ALIGNED_FREE_AND_NULL(pNewSongPtr);
            return 5;
        }

        if (bWaveMusic && nWaveMusicHandle >= 0)
        {
            FX_StopSound(nWaveMusicHandle);
            nWaveMusicHandle = -1;
        }
#ifndef EDUKE32
        CD_Stop();
#endif

        bWaveMusic = false;
        ALIGNED_FREE_AND_NULL(pSongPtr);
        pSongPtr = pNewSongPtr;
        nSongSize = nNewSongSize;
        return 0;
    }

    int32_t nSongLen = kfilelength(fp);

    if (EDUKE32_PREDICT_FALSE(nSongLen < 4))
    {
        OSD_Printf(OSD_ERROR "sndPlaySong(): error: empty music file \"%s\"\n", songName);
        kclose(fp);
        return 3;
    }

#ifdef __AMIGA__
    if (pSongPtr) sndStopSong();
#endif
    char * pNewSongPtr = (char *)Xaligned_alloc(16, nSongLen);
#ifdef __AMIGA__
    if (!pNewSongPtr)
    {
        OSD_Printf(OSD_ERROR "sndPlaySong(): error: can't allocate %d bytes\n", nSongLen);
        return 1;
    }
#endif
    int nNewSongSize = kread(fp, pNewSongPtr, nSongLen);

    if (EDUKE32_PREDICT_FALSE(nNewSongSize != nSongLen))
    {
        OSD_Printf(OSD_ERROR "sndPlaySong(): error: read %d bytes from \"%s\", expected %d\n",
            nNewSongSize, songName, nSongLen);
        kclose(fp);
        ALIGNED_FREE_AND_NULL(pNewSongPtr);
        return 4;
    }

    kclose(fp);

    if (!Bmemcmp(pNewSongPtr, "MThd", 4))
    {
        int32_t retval = MUSIC_PlaySong(pNewSongPtr, nNewSongSize, bLoop);

        if (retval != MUSIC_Ok)
        {
            ALIGNED_FREE_AND_NULL(pNewSongPtr);
            return 5;
        }

        if (bWaveMusic && nWaveMusicHandle >= 0)
        {
            FX_StopSound(nWaveMusicHandle);
            nWaveMusicHandle = -1;
        }
#ifndef EDUKE32
        CD_Stop();
#endif

        bWaveMusic = false;
        ALIGNED_FREE_AND_NULL(pSongPtr);
        pSongPtr = pNewSongPtr;
        nSongSize = nNewSongSize;
    }
    else
    {
        int nNewWaveMusicHandle = FX_Play(pNewSongPtr, bLoop ? nNewSongSize : -1, 0, 0, 0, MusicVolume, MusicVolume, MusicVolume,
                                   FX_MUSIC_PRIORITY, fix16_one, (intptr_t)&nWaveMusicHandle);

        if (nNewWaveMusicHandle <= FX_Ok)
        {
            ALIGNED_FREE_AND_NULL(pNewSongPtr);
            return 5;
        }

        if (bWaveMusic && nWaveMusicHandle >= 0)
            FX_StopSound(nWaveMusicHandle);

        MUSIC_StopSong();
#ifndef EDUKE32
        CD_Stop();
#endif

        nWaveMusicHandle = nNewWaveMusicHandle;
        bWaveMusic = true;
        ALIGNED_FREE_AND_NULL(pSongPtr);
        pSongPtr = pNewSongPtr;
        nSongSize = nNewSongSize;
    }

    return 0;
}

bool sndIsSongPlaying(void)
{
    //return MUSIC_SongPlaying();
    return false;
}

void sndFadeSong(int nTime)
{
    UNREFERENCED_PARAMETER(nTime);
    // NUKE-TODO:
    //if (MusicDevice == -1)
    //    return;
    //if (gEightyTwoFifty && sndMultiPlayer)
    //    return;
    //MUSIC_FadeVolume(0, nTime);
    if (bWaveMusic && nWaveMusicHandle >= 0)
    {
        FX_StopSound(nWaveMusicHandle);
        nWaveMusicHandle = -1;
        bWaveMusic = false;
    }
    // MUSIC_SetVolume(0);
    MUSIC_StopSong();
#ifndef EDUKE32
    CD_Stop();
#endif
}

void sndSetMusicVolume(int nVolume)
{
    MusicVolume = nVolume;
    if (bWaveMusic && nWaveMusicHandle >= 0)
        FX_SetPan(nWaveMusicHandle, nVolume, nVolume, nVolume);
    MUSIC_SetVolume(nVolume);
#ifndef EDUKE32
    CD_SetVolume(nVolume);
#endif
}

void sndSetFXVolume(int nVolume)
{
    FXVolume = nVolume;
    FX_SetVolume(nVolume);
}

void sndStopSong(void)
{
    if (bWaveMusic && nWaveMusicHandle >= 0)
    {
        FX_StopSound(nWaveMusicHandle);
        nWaveMusicHandle = -1;
        bWaveMusic = false;
    }

    MUSIC_StopSong();
#ifndef EDUKE32
    CD_Stop();
#endif

    ALIGNED_FREE_AND_NULL(pSongPtr);
    nSongSize = 0;
}

#ifndef EDUKE32
void SoundCallback(unsigned int val)
#else
void SoundCallback(intptr_t val)
#endif
{
    int *phVoice = (int*)val;
    *phVoice = 0;
}

void sndKillSound(SAMPLE2D *pChannel);

void sndStartSample(const char *pzSound, int nVolume, int nChannel)
{
    if (!SoundToggle)
        return;
    if (!strlen(pzSound))
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(pzSound, "RAW");
    if (!pChannel->at5)
        return;
    int nSize = pChannel->at5->size;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->hVoice = FX_PlayRaw(pData, nSize, sndGetRate(1), 0, nVolume, nVolume, nVolume, nVolume, fix16_one, (intptr_t)&pChannel->hVoice);
}

#ifdef __AMIGA__
#define MAXSFXCACHEID (9017+1)
static DICTNODE *rawResCache[MAXSFXCACHEID] = {0};
DICTNODE *sndLookupRawCached(int soundId, const char *rawName)
{
    DICTNODE *hRes =  NULL;
    if (soundId < MAXSFXCACHEID /*&& rawResCache[soundId]*/)
        hRes = rawResCache[soundId];
    if (!hRes)
    {
        hRes = gSoundRes.Lookup(rawName, "RAW");
        if (soundId < MAXSFXCACHEID)
            rawResCache[soundId] = hRes;
    }
    return hRes;
}
#endif

void sndStartSample(unsigned int nSound, int nVolume, int nChannel, bool bLoop)
{
    if (!SoundToggle)
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    DICTNODE *hSfx = gSoundRes.Lookup(nSound, "SFX");
    if (!hSfx)
        return;
    SFX *pEffect = (SFX*)gSoundRes.Lock(hSfx);
    dassert(pEffect != NULL);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
#ifdef __AMIGA__
    pChannel->at5 = sndLookupRawCached(nSound, pEffect->rawName);
#else
    pChannel->at5 = gSoundRes.Lookup(pEffect->rawName, "RAW");
#endif
    if (!pChannel->at5)
        return;
    if (nVolume < 0)
        nVolume = pEffect->relVol;
    nVolume *= 80;
    nVolume = clamp(nVolume, 0, 255); // clamp to range that audiolib accepts
    int nSize = pChannel->at5->size;
    int nLoopEnd = nSize - 1;
    if (nLoopEnd < 0)
        nLoopEnd = 0;
    if (nSize <= 0)
        return;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    if (nChannel < 0)
        bLoop = false;
    if (bLoop)
    {
        pChannel->hVoice = FX_PlayLoopedRaw(pData, nSize, pData + pEffect->loopStart, pData + nLoopEnd, sndGetRate(pEffect->format),
            0, nVolume, nVolume, nVolume, nVolume, fix16_one, (intptr_t)&pChannel->hVoice);
        pChannel->at4 |= 1;
    }
    else
    {
        pChannel->hVoice = FX_PlayRaw(pData, nSize, sndGetRate(pEffect->format), 0, nVolume, nVolume, nVolume, nVolume, fix16_one, (intptr_t)&pChannel->hVoice);
        pChannel->at4 &= ~1;
    }
}

void sndStartWavID(unsigned int nSound, int nVolume, int nChannel)
{
    if (!SoundToggle)
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(nSound, "WAV");
    if (!pChannel->at5)
        return;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->hVoice = FX_Play(pData, pChannel->at5->size, 0, -1, 0, nVolume, nVolume, nVolume, nVolume, fix16_one, (intptr_t)&pChannel->hVoice);
}

void sndKillSound(SAMPLE2D *pChannel)
{
    if (pChannel->at4 & 1)
    {
        FX_EndLooping(pChannel->hVoice);
        pChannel->at4 &= ~1;
    }
    FX_StopSound(pChannel->hVoice);
}

void sndStartWavDisk(const char *pzFile, int nVolume, int nChannel)
{
    if (!SoundToggle)
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    int hFile = kopen4loadfrommod(pzFile, 0);
    if (hFile == -1)
        return;
    int nLength = kfilelength(hFile);
    char *pData = (char*)gSoundRes.Alloc(nLength);
    if (!pData)
    {
        kclose(hFile);
        return;
    }
    kread(hFile, pData, kfilelength(hFile));
    kclose(hFile);
    pChannel->at5 = (DICTNODE*)pData;
    pChannel->at4 |= 2;
    pChannel->hVoice = FX_Play(pData, nLength, 0, -1, 0, nVolume, nVolume, nVolume, nVolume, fix16_one, (intptr_t)&pChannel->hVoice);
}

void sndKillAllSounds(void)
{
    for (int i = 0; i < kChannelMax; i++)
    {
        SAMPLE2D *pChannel = &Channel[i];
        if (pChannel->hVoice > 0)
            sndKillSound(pChannel);
        if (pChannel->at5)
        {
            if (pChannel->at4 & 2)
            {
#if 0
                free(pChannel->at5);
#else
                gSoundRes.Free(pChannel->at5);
#endif
                pChannel->at4 &= ~2;
            }
            else
            {
                gSoundRes.Unlock(pChannel->at5);
            }
            pChannel->at5 = 0;
        }
    }
}

void sndProcess(void)
{
    for (int i = 0; i < kChannelMax; i++)
    {
        if (Channel[i].hVoice <= 0 && Channel[i].at5)
        {
            if (Channel[i].at4 & 2)
            {
                gSoundRes.Free(Channel[i].at5);
                Channel[i].at4 &= ~2;
            }
            else
            {
                gSoundRes.Unlock(Channel[i].at5);
            }
            Channel[i].at5 = 0;
        }
    }
}

void InitSoundDevice(void)
{
#ifdef MIXERTYPEWIN
    void *initdata = (void *)win_gethwnd(); // used for DirectSound
#else
    void *initdata = NULL;
#endif
    int nStatus;
#ifndef EDUKE32
    int fxdevicetype;
    if (FXDevice < 0) {
        return;
    } else if (FXDevice == 0) {
        fxdevicetype = ASS_AutoDetect;
    } else {
        fxdevicetype = FXDevice - 1;
    }
    nStatus = FX_Init(fxdevicetype, NumVoices, &NumChannels, &NumBits, &MixRate, initdata);
#else
    nStatus = FX_Init(NumVoices, NumChannels, MixRate, initdata);
#endif
    if (nStatus != 0)
    {
        initprintf("InitSoundDevice: %s\n", FX_ErrorString(nStatus));
        return;
    }
#ifndef EDUKE32
    buildprintf("FX driver is %s\n", FX_GetCurrentDriverName());
    buildprintf("Format is %dHz %d-bit %d-channel\n", MixRate, NumBits, NumChannels);
#endif
#ifdef ASS_REVERSESTEREO
    if (ReverseStereo == 1)
        FX_SetReverseStereo(!FX_GetReverseStereo());
#endif
    FX_SetVolume(FXVolume);
    FX_SetCallBack(SoundCallback);
}

void DeinitSoundDevice(void)
{
    int nStatus = FX_Shutdown();
    if (nStatus != 0)
        ThrowError(FX_ErrorString(nStatus));
}

void sndLoadGMTimbre(void)
{
#ifdef EDUKE32
    if (!sndActive)
        return;
    DICTNODE *hTmb = gSoundRes.Lookup("GMTIMBRE", "TMB");
    if (hTmb)
    {
        unsigned char *timbre = (unsigned char*)Xmalloc(hTmb->size);
        gSoundRes.Load(hTmb, timbre);
        if (gFMPianoFix)
        {
            static uint8_t pianobroken[13] = {
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };
            static uint8_t pianofixed[13] = {
                0x33, 0x31, 0x5a, 0x00, 0xb2, 0xb1, 0x50, 0xf5, 0x00, 0x01, 0x00, 0x00, 0x00
            };
            if (!memcmp(&timbre[0], pianobroken, 13))
            {
                memcpy(&timbre[0], pianofixed, 13);
            }
        }
        AL_RegisterTimbreBank(timbre);
        Xfree(timbre);
    }
#endif
}

void InitMusicDevice(void)
{
    int nStatus;
#ifndef EDUKE32
    // TODO store the CDDevice in the config
    if ((nStatus = CD_Init(ASS_AutoDetect)) != CD_Ok) {
        buildprintf("CD error: %s\n", CD_ErrorString(nStatus));
    } else {
        buildprintf("CD driver is %s\n", CD_GetCurrentDriverName());
        CD_SetVolume(MusicVolume);
    }

    int musicdevicetype;
    if (MusicDevice < 0) {
        return;
    } else if (MusicDevice == 0) {
        musicdevicetype = ASS_AutoDetect;
    } else {
        musicdevicetype = MusicDevice - 1;
    }
    if ((nStatus = MUSIC_Init(musicdevicetype, "")) != MUSIC_Ok)
#else
    if ((nStatus = MUSIC_Init(MusicDevice)) == MUSIC_Ok)
    {
        if (MusicDevice == ASS_AutoDetect)
            MusicDevice = MIDI_GetDevice();
    }
    else if ((nStatus = MUSIC_Init(ASS_AutoDetect)) == MUSIC_Ok)
#endif
    {
        initprintf("InitMusicDevice: %s\n", MUSIC_ErrorString(nStatus));
        return;
    }
#ifndef EDUKE32
    buildprintf("Music driver is %s\n", MUSIC_GetCurrentDriverName());
#endif
    MUSIC_SetVolume(MusicVolume);

}

void DeinitMusicDevice(void)
{
    FX_StopAllSounds();
#ifndef EDUKE32
    CD_Shutdown();
#endif
    int nStatus = MUSIC_Shutdown();
    if (nStatus != 0)
        ThrowError(MUSIC_ErrorString(nStatus));
}

void sndTerm(void)
{
    if (!sndActive)
        return;
    sndActive = false;
    sndStopSong();
    DeinitSoundDevice();
    DeinitMusicDevice();
}
extern char *pUserSoundRFF;
void sndInit(void)
{
    memset(Channel, 0, sizeof(Channel));
    pSongPtr = NULL;
    nSongSize = 0;
    bWaveMusic = false;
    nWaveMusicHandle = -1;
    InitSoundDevice();
    InitMusicDevice();
    sndActive = true;
    sndLoadGMTimbre();
}
