/* Catacomb Armageddon Source Code
 * Copyright (C) 1993-2014 Flat Rock Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "SL_FILE.h"


///////////////////////////////////////////////////////////////////////////
//
// Defines
//

#define ANIM_USES_SHAPE_TABLE false
#define PI_MAX_ANIMS 10
#define PI_MAX_PAGES	40

#define SAVEVER_DATA "0.01"
#define FILENAME_LEN 15

#define  GAMENAME		"CATACOMB ARMAGEDDON 3-D"
#define	VERSION		"V1.02"
#define  REVISION		"  rev 1 "

//#define BOBLIST 1                          //SP - Undefine if not using BOBList

#define AUDIO_DISK		(2)
#define VIDEO_DISK		(1)
#define LEVEL_DISK		(2)

#define BIO_BUFFER_LEN	(512)

#define TrashProg Quit

//   #define AMIGA


typedef struct Sample {
		char *filename;
		memptr *data;
} Sample;

typedef enum {ged_none, ged_SoundSource,ged_SoundBlaster} AudioDeviceType;

//typedef struct {
//	memptr textptr;
//	char far *pages[MAX_TEXT_PAGES];
//	short totalpages;
//} textinfo;

typedef struct {
	int handle;			// handle of file
	memptr buffer;		// pointer to buffer
	word offset;		// offset into buffer
	word status;		// read/write status
} BufferedIO;

typedef enum ANIMINFO {at_NONE,at_INIT,at_WAIT,at_ONCE,at_CYCLE,
							  at_REBOUND,at_EXTRA,
							  at_FWD,at_REV
} ANIMINFO;

struct BitMapHeader {
	unsigned int	w,h,x,y;
	unsigned char	d,trans,comp,pad;
};

struct BitMap {
	unsigned int Width;
	unsigned int Height;
	unsigned int Depth;
	unsigned int BytesPerRow;
	char far *Planes[8];
};

struct Shape {
	memptr Data;
	long size;
	unsigned int BPR;
	struct BitMapHeader bmHdr;
};

#ifdef AMIGA
typedef struct {
	char *script[PI_MAX_PAGES];
	XBitMap **shapes;
	XBitMap **font;
	short xl,yl,xh,yh;
	struct BitMap *dst;
	char numpages,bgcolor;
} PresenterInfo;
#else
typedef struct {
	char far *script[PI_MAX_PAGES];
	memptr scriptstart;
	short xl,yl,xh,yh;
	char numpages,bgcolor,ltcolor,dkcolor;
} PresenterInfo;
#endif


///////////////////////////////////////////////////////////////////////////
//
// Externs
//

extern char Filename[], ID[], VER[];
extern boolean ge_textmode;
extern short PPT_LeftEdge,PPT_RightEdge;
//extern boolean ConserveMemory;
extern BufferedIO lzwBIO;
extern short wall_anim_delay,wall_anim_time;



///////////////////////////////////////////////////////////////////////////
//
// Function prototypes
//
void WaitKeyVBL(short key, short vbls);
void CalibrateJoystick(short joynum);
void MoveScreen(short x, short y);
void MoveGfxDst(short x, short y);
void DoPiracy(void);
void PrintPropText(char far *text);
//void DisplayText(textinfo *textinfo);
//long LoadTextFile(char *filename,textinfo *textinfo);
//void FreeTextFile(textinfo *textinfo);
//void InitTextFile(textinfo *textinfo);
long Verify(char *filename);
void GE_SaveGame(void);
boolean GE_LoadGame(void);
int GE_HardError(word errval,int ax,int bp,int si);

#ifdef BOBLIST


boolean UpdateBOBList(objtype *obj,struct Simple_Shape *Shape,shapeclass Class, short priority, spriteflags sprflags);
boolean RemoveBOBShape(objtype *obj, shapeclass Class);
void RemoveBOBList(objtype *obj);
void InitBOBList(objtype *obj, struct BOB_Shape *BOB_Shape, short NumElements);
void RefreshBOBList(objtype *obj);
#endif


unsigned long BLoad(char *SourceFile, memptr *DstPtr);
void lzwDecompressFromRAM(byte far *SrcPtr, byte far *DstPtr, longword SrcLen);
void lzwDecompressFromFile(BufferedIO *SrcPtr, byte far *DstPtr, longword SrcLen);
byte readch(int handle);

memptr InitBufferedIO(int handle, BufferedIO *bio);
void FreeBufferedIO(BufferedIO *bio);
byte bio_readch(BufferedIO *bio);
void bio_fillbuffer(BufferedIO *bio);


void SwapLong(long far *Var);
void SwapWord(unsigned int far *Var);
int LoadShape(char *Filename,struct Shape *SHP);
void FreeShape(struct Shape *shape);
int UnpackEGAShapeToScreen(struct Shape *SHP,int startx,int starty);
char GetKeyChoice(char *choices,boolean clear);
boolean AnimateObj(objtype *obj);
void AdvanceAnimFWD(objtype *obj);
void AdvanceAnimREV(objtype *obj);

void LoadASArray(struct Sample *ASArray);
void FreeASArray(struct Sample *ASArray);
//void SelectDigiAudio(AudioDeviceType Device);
void PlaySample(unsigned SampleNum);
void GE_FreeAllDigiSounds(void);
void GE_LoadAllDigiSounds(void);
void DisplayGameList(short winx, short winy, short list_width, short list_height);
void ReadGameList(void);
void CheckStack(void);
void CenterObj(objtype *obj, unsigned x, unsigned y);
void cachein(short s,short e);
void cacheout(short s,short e);
void FizzleFade (unsigned source, unsigned dest,unsigned width,unsigned height, boolean abortable);
void mprintf(char *msg, ...);
boolean FindFile(char *filename,char *disktext,char disknum);
void CacheAV(char *title);
void BlackPalette(void);
void ColoredPalette(void);
void Presenter(PresenterInfo *pi);
unsigned PI_VALUE(char far *ptr,char num_nybbles);
long LoadPresenterScript(char *filename,PresenterInfo *pi);
void FreePresenterScript(PresenterInfo *pi);
void InitPresenterScript(PresenterInfo *pi);

void AnimatePage(short numanims);
short BoxAroundPic(short x1, short y1, unsigned picnum, PresenterInfo *pi);
void PurgeAllGfx(void);
void CachePage(char far *script);


void AnimateWallList(void);
