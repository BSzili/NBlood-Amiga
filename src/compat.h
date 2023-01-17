#ifndef EDUKE32_COMPAT_H
#define EDUKE32_COMPAT_H

//
// EDuke32 compatibility
//

/*#ifdef __cplusplus
#define min
#define max
#endif*/
#include_next "compat.h" // include the actual build/include/compat.h
/*#ifdef __cplusplus
#undef min
#undef max
#endif*/
//#include "baselayer.h"
#include "build.h"
#include "cache1d.h"
#include "pragmas.h"
#include "osd.h"
#include "vfs.h"
#include "types.h"
#include "keyboard.h"
#include "control.h"
#include "assert.h"
#include "fx_man.h"
#include "scriptfile.h"
#ifdef RENDERTYPEWIN
#include "winlayer.h"
#include "winbits.h"
#endif
//#include "palette.h"

#include <math.h>
#include <stdlib.h>

//#include "mutex.h"

#if !USE_OPENGL
#undef USE_OPENGL
#endif
#ifdef _WIN32
#define MIXERTYPEWIN
#endif

#define initprintf buildprintf
#define ERRprintf buildprintf

typedef struct
{
	int32_t x, y;
} vec2_t; // TODO mapwinxy1, mapwinxy2

typedef struct
{
	int32_t x, y, z;
} vec3_t; // TODO remove opos

#undef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(...)
#define UNREFERENCED_CONST_PARAMETER(...)
/*#ifdef __GNUC__
#define fallthrough__ __attribute__ ((fallthrough))
#else*/
#define fallthrough__
//#endif
#define EDUKE32_FUNCTION __FUNCTION__
#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))
#define ARRAY_SSIZE(arr) (int)ARRAY_SIZE(arr)
#define EDUKE32_STATIC_ASSERT(...)
#define EDUKE32_PREDICT_FALSE(...) __VA_ARGS__


typedef int32_t fix16_t;
#if 1
#define fix16_one (1<<16)
//#define fix16_half (1<<15)
//#define fix16_to_int(a) (((a) + ((a) >= 0 ? fix16_half : -fix16_half)) / fix16_one)
#define fix16_to_int(a) ((a) / fix16_one) // TODO rounding?
#define fix16_from_int(a) ((a) * fix16_one)
#define F16(x) fix16_from_int((x))
//#define fix16_to_float(a) ((float)(a) / (float)fix16_one)
//#define fix16_from_float(a) ((fix16_t)((a) * (float)fix16_one))
//#define fix16_sadd(a, b) ((fix16_t)((uint32_t)(a)+(uint32_t)(b)))
//#define fix16_ssub(a, b) ((fix16_t)((uint32_t)(a)-(uint32_t)(b)))
#else
#define fix16_one 1
#define fix16_to_int(a) ((a))
#define fix16_from_int(a) ((a))
#define F16(x) ((x))
//#define fix16_to_float(a) ((float)(a))
//#define fix16_from_float(a) ((fix16_t)(a))
#endif
#define fix16_sadd(a,b) ((a)+(b))
#define fix16_ssub(a,b) ((a)-(b))
//#define fix16_sdiv(a,b) ((a)/(b))
#define fix16_clamp(x, lo, hi) clamp((x), (lo), (hi))
#define fix16_min(a,b) min((fix16_t)(a), (fix16_t)(b))
#define fix16_max(a,b) max((fix16_t)(a), (fix16_t)(b))


#define Xaligned_alloc(x,y) malloc((y))
#define Xaligned_free free
#define ALIGNED_FREE_AND_NULL(x) do { Xaligned_free((x)); (x) = NULL; } while(0)
#define DO_FREE_AND_NULL(var) do { Xfree(var); (var) = NULL; } while (0)
#define Xcalloc calloc
#define Xfree free
#define Xstrdup strdup
#define Xmalloc malloc

#define Bfstat fstat
#define Bexit exit
#define Bfflush fflush
//#define Blrintf(x) ((int32_t)lrintf((x)))
#define Bassert assert
#define Bchdir chdir

void gltexinvalidate (long dapicnum, long dapalnum, long dameth);
#if USE_POLYMOST && USE_OPENGL
#define tileInvalidate invalidatetile
#else
#define tileInvalidate(x,y,z)
#endif



#define ClockTicks long
//#define timerInit inittimer
//#define timerSetCallback installusertimercallback
#define timerInit(tickspersecond) { int tps = (tickspersecond);
#define timerSetCallback(callback) inittimer(tps, (callback)); }
#define timerGetTicks getticks
#define timerGetPerformanceCounter getusecticks
#define timerGetPerformanceFrequency() (1000*1000)
static inline int32_t BGetTime(void) { return (int32_t)totalclock; }

#include "palette.h"

#define in3dmode() (1)
enum rendmode_t
{
	REND_CLASSIC,
	REND_POLYMOST = 3,
	REND_POLYMER
};

#if USE_POLYMOST
#define videoGetRenderMode() ((rendmode_t)getrendermode())
#define videoSetRenderMode(mode) setrendermode((int)(mode)
#else
#define videoGetRenderMode() REND_CLASSIC
#define videoSetRenderMode(mode)
#endif
#define videoNextPage nextpage
 extern char inpreparemirror;
#define renderDrawRoomsQ16(daposx, daposy, daposz, daang, dahoriz, dacursectnum) ({drawrooms((daposx), (daposy), (daposz), fix16_to_int((daang)), fix16_to_int((dahoriz)), (dacursectnum)); inpreparemirror;})
#define renderDrawMasks drawmasks
#define videoCaptureScreen(x,y) screencapture((char *)(x),(y))
#define videoCaptureScreenTGA(x,y) screencapture((char *)(x),(y))
#define engineFPSLimit() 1
#define enginePreInit preinitengine
#define PrintBuildInfo() // already part of preinitengine
#define windowsCheckAlreadyRunning() 1
#define enginePostInit() 0 // TODO
//#define engineLoadBoard loadboard
//#define engineLoadBoardV5V6 loadoldboard
#define artLoadFiles(a,b) loadpics(const_cast<char*>((a)), (b))
#define engineUnInit uninitengine
#define engineInit initengine
#define tileCreate allocatepermanenttile
#define tileLoad loadtile
#define tileDelete(x) // nothing to do here

// common.h
void G_AddGroup(const char *buffer);
void G_AddPath(const char *buffer);
void G_AddDef(const char *buffer);
int32_t G_CheckCmdSwitch(int32_t argc, char const * const * argv, const char *str);


// palette

#if USE_POLYMOST && USE_OPENGL
static inline void hicsetpalettetint_eduke32(int32_t palnum, char r, char g, char b, char sr, char sg, char sb, long effect)
{
	hicsetpalettetint(palnum, r, g, b, effect);
}
#define hicsetpalettetint hicsetpalettetint_eduke32
extern palette_t palookupfog[MAXPALOOKUPS];
extern float palookupfogfactor[MAXPALOOKUPS];
#define videoTintBlood(r,g,b) // TODO
//extern char nofog;
// polymost_setFogEnabled
#define fullscreen_tint_gl(a,b,c,d) // TODO
#endif
#define renderEnableFog() // TODO
#define renderDisableFog() // TODO
#define numshades numpalookups

extern char pow2char[8];
#define renderDrawLine drawline256
#define renderSetAspect setaspect
#if 0
#undef renderSetAspect
#define renderSetAspect(xrange, aspect) do { buildprintf("%s:%d renderSetAspect(%d,%d)\n", __FUNCTION__, __LINE__, (xrange), (aspect)); setaspect((xrange), (aspect)); } while(0)
#endif
typedef walltype *uwallptr_t;
#define tspritetype spritetype
typedef spritetype *tspriteptr_t;
#define videoSetCorrectedAspect() // newaspect not needed
#define renderDrawMapView drawmapview
#define videoClearViewableArea clearview
#define videoSetGameMode(davidoption, daupscaledxdim, daupscaledydim, dabpp, daupscalefactor) setgamemode(davidoption, daupscaledxdim, daupscaledydim, dabpp)
#define videoSetViewableArea setview
//#define videoSetViewableArea(x1, y1, x2, y2) do { setview((x1),(y1),(x2),(y2)); setaspect(65536, yxaspect); } while(0) // TODO HACK!
#define videoClearScreen clearallviews
#define mouseLockToWindow(a) grabmouse((a)-2)
#define videoResetMode resetvideomode
//extern int whitecol, blackcol; // TODO hardcode
#define whitecol 31
#define blackcol 0
#define kopen4loadfrommod kopen4load
#define g_visibility visibility

/*
//   bit 0: 1 = Blocking sprite (use with clipmove, getzrange)       "B"
//   bit 1: 1 = transluscence, 0 = normal                            "T"
//   bit 2: 1 = x-flipped, 0 = normal                                "F"
//   bit 3: 1 = y-flipped, 0 = normal                                "F"
//   bits 5-4: 00 = FACE sprite (default)                            "R"
//             01 = WALL sprite (like masked walls)
//             10 = FLOOR sprite (parallel to ceilings&floors)
//   bit 6: 1 = 1-sided sprite, 0 = normal                           "1"
//   bit 7: 1 = Real centered centering, 0 = foot center             "C"
//   bit 8: 1 = Blocking sprite (use with hitscan / cliptype 1)      "H"
//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
//   bits 10-14: reserved
//   bit 15: 1 = Invisible sprite, 0 = not invisible
*/
#define CSTAT_SPRITE_BLOCK (1)
#define CSTAT_SPRITE_TRANSLUCENT (2)
#define CSTAT_SPRITE_YFLIP (8)
#define CSTAT_SPRITE_BLOCK_HITSCAN (0x100)
#define CSTAT_SPRITE_TRANSLUCENT_INVERT (0x200)
#define CSTAT_SPRITE_INVISIBLE (0x8000)
#define CSTAT_SPRITE_ALIGNMENT_FACING (0)
// wall 16
#define CSTAT_SPRITE_ALIGNMENT_FLOOR (32)
#define CSTAT_SPRITE_ONE_SIDED (64)


/*
//   bit 0: 1 = Blocking wall (use with clipmove, getzrange)         "B"
//   bit 1: 1 = bottoms of invisible walls swapped, 0 = not          "2"
//   bit 2: 1 = align picture on bottom (for doors), 0 = top         "O"
//   bit 3: 1 = x-flipped, 0 = normal                                "F"
//   bit 4: 1 = masking wall, 0 = not                                "M"
//   bit 5: 1 = 1-way wall, 0 = not                                  "1"
//   bit 6: 1 = Blocking wall (use with hitscan / cliptype 1)        "H"
//   bit 7: 1 = Transluscence, 0 = not                               "T"
//   bit 8: 1 = y-flipped, 0 = normal                                "F"
//   bit 9: 1 = Transluscence reversing, 0 = normal                  "T"
//   bits 10-15: reserved
*/
#define CSTAT_WALL_BLOCK (1)
#define CSTAT_WALL_MASKED (16)
#define CSTAT_WALL_BLOCK_HITSCAN (64)

#define clipmove_old clipmove
#define getzrange_old getzrange
#define pushmove_old pushmove


// controls

#ifndef MAXMOUSEBUTTONS // TODO
#define MAXMOUSEBUTTONS 6
#endif
#define CONTROL_ProcessBinds()
#define CONTROL_ClearAllBinds()
extern int CONTROL_BindsEnabled; // TODO
#define KB_UnBoundKeyPressed KB_KeyPressed
extern kb_scancode KB_LastScan;


// math

//#define fPI 3.14159265358979323846f // TODO remove

#ifdef __cplusplus
template <typename T, typename X, typename Y> static inline T clamp(T in, X min, Y max) { return in <= (T) min ? (T) min : (in >= (T) max ? (T) max : in); }
#undef min
#undef max
#include <algorithm>
using std::min;
using std::max;
#else
static inline int32_t clamp(int32_t in, int32_t min, int32_t max) { return in <= min ? min : (in >= max ? max : in); }
#ifndef min
# define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
# define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#endif

#define tabledivide32_noinline(n, d) ((n)/(d))
//#define tabledivide64(n, d) ((n)/(d))
#define tabledivide32(a,b) ((a)/(b))

// defs.c
extern const char *G_DefaultDefFile(void);
extern const char *G_DefFile(void);
extern char *g_defNamePtr;
enum {
	T_EOF = -2,
	T_ERROR = -1,
};
typedef struct { char *text; int tokenid; } tokenlist;
static int getatoken(scriptfile *sf, const tokenlist *tl, int ntokens)
{
	char *tok;
	int i;

	if (!sf) return T_ERROR;
	tok = scriptfile_gettoken(sf);
	if (!tok) return T_EOF;

	for(i=0;i<ntokens;i++) {
		if (!Bstrcasecmp(tok, tl[i].text))
			return tl[i].tokenid;
	}

	return T_ERROR;
}
#define clearDefNamePtr() Xfree(g_defNamePtr)
#define G_AddDefModule(x) // TODO

#define CONSTEXPR constexpr

static inline char *dup_filename(const char *fn)
{
    char * const buf = (char *) Xmalloc(BMAX_PATH);
    return Bstrncpy(buf, fn, BMAX_PATH);
}

// ODS stuff

typedef const osdfuncparm_t *osdcmdptr_t;
#define system_getcvars() // TODO 
#define OSD_SetLogFile buildsetlogfile

// fnlist

struct strllist
{
    struct strllist *next;
    char *str;
};

typedef struct
{
    BUILDVFS_FIND_REC *finddirs, *findfiles;
    int32_t numdirs, numfiles;
}
fnlist_t;

#define FNLIST_INITIALIZER { NULL, NULL, 0, 0 }

void fnlist_clearnames(fnlist_t *fnl);
int32_t fnlist_getnames(fnlist_t *fnl, const char *dirname, const char *pattern, int32_t dirflags, int32_t fileflags);

extern const char *s_buildRev;

extern int32_t clipmoveboxtracenum;

#define MUSIC_Update()
#define OSD_Cleanup() // done via atexit
#define OSD_GetCols() (60) // TODO

// keyboard.h
static inline void keyFlushScans(void)
{
	keyfifoplc = keyfifoend = 0;
}
#define KB_FlushKeyboardQueueScans keyFlushScans

static inline char keyGetScan(void)
{
	char c;
	if (keyfifoplc == keyfifoend) return 0;
	c = keyfifo[keyfifoplc];
	keyfifoplc = ((keyfifoplc+2)&(KEYFIFOSIZ-1));
	return c;
}
#define keyFlushChars KB_FlushKeyboardQueue

extern int32_t win_priorityclass;
#define MAXUSERTILES (MAXTILES-16)  // reserve 16 tiles at the end
#define tileGetCRC32(tile) (0) // TODO

//#include "kplib.h"
static inline int32_t check_file_exist(const char *fn)
{
	int fh;
	if ((fh = openfrompath(fn, BO_BINARY|BO_RDONLY,BS_IREAD)))
	{
		close(fh);
		return 1;
	}

    return 0;
}

static inline int32_t testkopen(const char *filename, char searchfirst)
{
    buildvfs_kfd fd = kopen4load(filename, searchfirst);
    if (fd != buildvfs_kfd_invalid)
        kclose(fd);
    return (fd != buildvfs_kfd_invalid);
}

//char * OSD_StripColors(char *outBuf, const char *inBuf)
#define OSD_StripColors strcpy
#define Bstrncpyz Bstrncpy
#define Bstrstr strstr
//#define Bstrtolower 
#define maybe_append_ext(wbuf, wbufsiz, fn, ext) snprintf((wbuf), (wbufsiz), "%s%s", (fn), (ext)) // TODO


extern int32_t Numsprites; // only used by the editor

#define roundscale scale // TODO rounding?
#define rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, daalpha, dablend, cx1, cy1, cx2, cy2) rotatesprite((sx), (sy), (z), (a), (picnum), (dashade), (dapalnum), (dastat)&0xFF, (cx1), (cy1), (cx2), (cy2))
#define rotatesprite_fs_alpha(sx, sy, z, a, picnum, dashade, dapalnum, dastat, alpha) rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, alpha, 0, 0, 0, xdim-1, ydim-1)
#define rotatesprite_fs(sx, sy, z, a, picnum, dashade, dapalnum, dastat) rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, 0,0,xdim-1,ydim-1)
static inline void rotatesprite_eduke32(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum,
                                int8_t dashade, char dapalnum, int32_t dastat,
                                int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
    rotatesprite_(sx, sy, z, a, picnum, dashade, dapalnum, dastat, 0, 0, cx1, cy1, cx2, cy2);
}
#define rotatesprite rotatesprite_eduke32

/* TODO
enum {
    RS_TRANS1 = 1,
    RS_AUTO = 2,
    RS_YFLIP = 4,
    RS_NOCLIP = 8,
    RS_TOPLEFT = 16,
    RS_TRANS2 = 32,
    RS_TRANS_MASK = RS_TRANS1|RS_TRANS2,
    RS_NOMASK = 64,
    RS_PERM = 128,

    RS_ALIGN_L = 256,
    RS_ALIGN_R = 512,
    RS_ALIGN_MASK = RS_ALIGN_L|RS_ALIGN_R,
    RS_STRETCH = 1024,

    ROTATESPRITE_FULL16 = 2048,
    RS_LERP = 4096,
    RS_FORCELERP = 8192,

    // ROTATESPRITE_MAX-1 is the mask of all externally available orientation bits
    ROTATESPRITE_MAX = 16384,

    RS_CENTERORIGIN = (1<<30),
};
*/
#define RS_LERP 0
#define RS_AUTO 2

extern char const g_keyAsciiTable[128];
extern char const g_keyAsciiTableShift[128];

extern int hitscangoalx;
extern int hitscangoaly;

#define videoEndDrawing()
#define videoBeginDrawing()

#define yax_preparedrawrooms()
#define yax_drawrooms(a, b, c, d)
/*void   preparemirror(int dax, int day, int daz, short daang, int dahoriz, short dawall, short dasector, int *tposx, int *tposy, short *tang);
void   renderPrepareMirror(int32_t dax, int32_t day, int32_t daz, fix16_t daang, fix16_t dahoriz, int16_t dawall,
                           int32_t *tposx, int32_t *tposy, fix16_t *tang);*/
#define renderPrepareMirror(dax, day, daz, daang, dahoriz, dawall, tposx, tposy, tang) \
	do { \
		short shortang; \
		preparemirror((dax), (day), (daz), fix16_to_int((daang)), fix16_to_int((dahoriz)), (dawall), 0, (int *)(tposx), (int *)(tposy), &shortang); \
		*tang = fix16_from_int(shortang); \
	} while(0)
	//preparemirror((dax), (day), (daz), fix16_to_int((daang)), fix16_to_int((dahoriz)), (dawall), 0, (int *)(tposx), (int *)(tposy), (short *)(tang)) // TODO tang fix16_t short
#define renderCompleteMirror completemirror

extern unsigned char picsiz[MAXTILES];

#define EXTERN_INLINE static inline
//#define MV_Lock DisableInterrupts // TODO
//#define MV_Unlock RestoreInterrupts
#define MV_Lock()
#define MV_Unlock()

static inline int FX_PlayRaw_eduke32(char *ptr, uint32_t ptrlength, int rate, int pitchoffset, int vol, int left, int right, int priority, fix16_t volume, intptr_t callbackval)
{
	return FX_PlayRaw(ptr, ptrlength, rate, pitchoffset, vol, left, right, priority, callbackval);
}
#define FX_PlayRaw FX_PlayRaw_eduke32
static inline int FX_PlayLoopedRaw_eduke32(char *ptr, uint32_t ptrlength, char *loopstart, char *loopend, int rate, int pitchoffset, int vol, int left, int right, int priority, fix16_t volume, intptr_t callbackval)
{
	return FX_PlayLoopedRaw(ptr, ptrlength, loopstart, loopend, rate, pitchoffset, vol, left, right, priority, callbackval);
}
#define FX_PlayLoopedRaw FX_PlayLoopedRaw_eduke32

#define FX_Play(ptr, ptrlength, loopstart, loopend, pitchoffset, vol, left, right, priority, volume, callbackval) \
	FX_PlayLoopedAuto(ptr, ptrlength, loopstart, loopend, pitchoffset, vol, left, right, priority, callbackval)

#define MIDI_GetDevice MUSIC_GetCurrentDriver

#define MOUSE_ClearAllButtons() MOUSE_ClearButton(0xFF)
#define JOYSTICK_GetButtons() 0 // TODO
#define JOYSTICK_ClearAllButtons() // TODO
//#define timerGetFractionalTicks() ((double)getusecticks())

// TODO remove this
extern float curgamma;
#define g_videoGamma curgamma
#define DEFAULT_GAMMA 1.0f
//#define GAMMA_CALC ((int32_t)(min(max((float)((g_videoGamma - 1.0f) * 10.0f), 0.f), 15.f)))
#define GAMMA_CALC 0

extern int totalclocklock; // engine.c

// if (picanm[nTile].sf&PICANM_ANIMTYPE_MASK)
// picanm[tilenum]&192

// int const i = (int) totalclocklock >> (picanm[tilenum].sf & PICANM_ANIMSPEED_MASK);
// i = (totalclocklock>>((picanm[tilenum]>>24)&15));

#define PICANM_ANIMTYPE_OSC (64)
#define PICANM_ANIMTYPE_FWD (128)
#define PICANM_ANIMTYPE_BACK (192)

#define Bcrc32(data, length, crc) ({ assert((crc) == 0); (uint32_t)crc32once((unsigned char *)(data), (length)); })

// these are done in loadpalette
#define paletteInitClosestColorScale(r,g,b) 
#define paletteInitClosestColorGrid()
#define paletteInitClosestColorMap(pal)
#define palettePostLoadTables()
#define palettePostLoadLookups()
extern int paletteloaded; // TODO
enum 
{
    PALETTE_MAIN,
    PALETTE_SHADE,
    PALETTE_TRANSLUC
};
extern unsigned char *transluc;
// blendtable transluc
// transluc = kmalloc(65536L)
int getclosestcol(int r, int g, int b);
#define paletteGetClosestColor getclosestcol
#define RESERVEDPALS 4 // TODO

#ifndef MAXVOXMIPS
#define MAXVOXMIPS 5
#endif
extern intptr_t voxoff[MAXVOXELS][MAXVOXMIPS];
extern unsigned char voxlock[MAXVOXELS][MAXVOXMIPS];

#define maxspritesonscreen MAXSPRITESONSCREEN
//int animateoffs(short tilenum, short fakevar);
#define animateoffs_replace qanimateoffs


#define renderSetTarget setviewtotile
#define renderRestoreTarget setviewback

/*static inline int tilehasmodelorvoxel(int const tilenume, int pal)
{
    return
#ifdef USE_OPENGL
    (videoGetRenderMode() >= REND_POLYMOST && mdinited && usemodels && tile2model[Ptile2tile(tilenume, pal)].modelid != -1) ||
#endif
    (videoGetRenderMode() <= REND_POLYMOST && usevoxels && tiletovox[tilenume] != -1);
}*/
#define usevoxels (1)

#define CACHE1D_PERMANENT 255
static inline void tileSetSize(int32_t picnum, int16_t dasizx, int16_t dasizy)
{
	int j, dasiz;

    tilesizx[picnum] = dasizx;
    tilesizy[picnum] = dasizy;
	picanm[picnum] = 0;

    //tileUpdatePicSiz(picnum);

	extern int pow2long[32];
	j = 15; while ((j > 1) && (pow2long[j] > dasizx)) j--;
	picsiz[picnum] = ((unsigned char)j);
	j = 15; while ((j > 1) && (pow2long[j] > dasizy)) j--;
	picsiz[picnum] += ((unsigned char)(j<<4));
}

#ifdef __AMIGA__
static inline size_t strnlen(const char * s, size_t len) {
    size_t i = 0;
    for ( ; i < len && s[i]; ++i);
    return i;
}
#endif

extern uint32_t wrandomseed;

// This aims to mimic Watcom C's implementation of rand
static inline int32_t wrand(void)
{
	wrandomseed = 1103515245 * wrandomseed + 12345;
	return (wrandomseed >> 16) & 0x7FFF;
}

static inline void wsrand(int seed)
{
	wrandomseed = seed;
}

extern int nextvoxid;

#define CLOCKTICKSPERSECOND 120

#define joyGetName getjoyname
#define JOYSTICK_SetDeadZone(axis, dead, satur) do { CONTROL_SetJoyAxisDead((axis), (dead)); CONTROL_SetJoyAxisSaturate((axis), (satur)); } while(0)
#define JOYSTICK_GetControllerButtons() (joyb)
#define CONTROLLER_BUTTON_A joybutton_A
#define CONTROLLER_BUTTON_B joybutton_B
#define CONTROLLER_BUTTON_START joybutton_Start
#define CONTROLLER_BUTTON_DPAD_UP joybutton_DpadUp
#define CONTROLLER_BUTTON_DPAD_DOWN joybutton_DpadDown
#define CONTROLLER_BUTTON_DPAD_LEFT joybutton_DpadLeft
#define CONTROLLER_BUTTON_DPAD_RIGHT joybutton_DpadRight

#ifdef __GNUC__
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#else
# define UNUSED(x) x
#endif

// save-game compatibility
#undef BMAX_PATH
#define BMAX_PATH 260

#endif
