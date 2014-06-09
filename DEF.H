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

#include "ID_HEADS.H"
#include <MATH.H>
#include <VALUES.H>

//#define PROFILE

#define DEBUG_KEYS_AVAILABLE 0

/*
=============================================================================

						 GLOBAL CONSTANTS

=============================================================================
*/

//
// SOFTLIB GFX FILENAME
//
#define 	SLIB_GFX			"ARM_SLIB."EXT


#define USE_INERT_LIST false


//
// MAX_BASE - represents 100 percent in 1st base
// MAX_PERC - represents 100 percent in 2nd base
// PERC - fractional portion of 2nd base
// SCALE - arbitrary scaling value (bigger number means more accurate)
//
// ex: PERCENTAGE(320,16,8,7)    returns  160
//
// Make sure values used won't overflow a WORD! In general, if largest number
// to be used (320 in ex: above) * (1<<SCALE) is greater than 65535, use
// LONG_PERCENTAGE or a lower SCALE. Using a SCALE of 8 in the example
// above would overflow a WORD in some circumstances!
//
// LONG_PERCENTAGE is to be used for larger SCALEs, thus, giving you
// massive accuracy!
//
#define PERCENTAGE(MAX_BASE,MAX_PERC,PERC,SCALE) ((unsigned)(MAX_BASE*((PERC<<SCALE)/MAX_PERC))>>SCALE)
#define LONG_PERCENTAGE(MAX_BASE,MAX_PERC,PERC,SCALE) (((long)MAX_BASE*(((long)PERC<<SCALE)/MAX_PERC))>>SCALE)

#define PI	3.141592657

//#define SIZE_TEST 		65000
#define SIZE_TEST 		0


#define FL_QUICK 			0x01
#define FL_NOMEMCHECK	0x02
#define FL_HELPTEST		0x04

#define FL_CLEAR		(FL_QUICK|FL_NOMEMCHECK|FL_HELPTEST)

#if 0
#define GEM_SHIFT		2
#define FL_RGEM		0x04
#define FL_GGEM		0x08
#define FL_BGEM		0x10
#define FL_YGEM		0x20
#define FL_PGEM		0x40
#endif

#define FL_DEAD		0x80



#define MAXBOLTS		10
#define MAXNUKES		10
#define MAXPOTIONS	10

#define NUKE_COST		(1000)
#define BOLT_COST    (1200)
#define POTION_COST	(1300)

#define NUKE_COST_TXT		("1000")		// Allows for Q&D positioning..
#define BOLT_COST_TXT    	("1200")
#define POTION_COST_TXT		("1300")

#define RADARX	31															// bytes
#define RADARY	11                                           // pixels
#define RADAR_WIDTH     51                                  //   "
#define RADAR_HEIGHT		51                                  //   "
#define RADAR_XCENTER	((RADARX*8)+(RADAR_WIDTH/2)+3)      //   "
#define RADAR_YCENTER	((RADARY-8)+(RADAR_HEIGHT/2)+5)       //   "
#define MAX_RADAR_BLIPS	60


#define RADAR_RADIUS			17
#define RADAR_RADIUS_NSEW	15
#define RADAR_X_IRADIUS		(113/5)
#define RADAR_Y_IRADIUS		(113/7)
#define RADAR_ICON_CENTER	4						// Center offset into icon.

#define NAMESTART	180
#define REMOVED_DOOR_TILE	NAMESTART

#define NEXT_LEVEL_CODE		0xff
#define REMOVE_DOOR_CODE	0xfe
#define CANT_OPEN_CODE		0xfd
#define EXP_WALL_CODE		0xfc


#define UNMARKGRCHUNK(chunk)	(grneeded[chunk]&=~ca_levelbit)

#define MOUSEINT	0x33

#define EXPWALLSTART	8
#define NUMEXPWALLS	7
#define WALLEXP		59
#define WATEREXP	62
#define NUMFLOORS	80	//71

#define NUMLATCHPICS	 (FIRSTWALLPIC-FIRSTLATCHPIC) //+5
#define NUMSCALEPICS	 (FIRSTWALLPIC-FIRSTSCALEPIC) //+5
#define NUMSCALEWALLS (LASTWALLPIC-FIRSTWALLPIC) //+5


#define FLASHCOLOR	12
#define FLASHTICS	4


#define NUMLEVELS	32		//21

#define VIEWX		0		// corner of view window
#define VIEWY		0
#define VIEWWIDTH	(40*8)		// size of view window				// 33
#define VIEWHEIGHT	(15*8)												// 18
#define VIEWXH		(VIEWX+VIEWWIDTH-1)
#define VIEWYH		(VIEWY+VIEWHEIGHT-1)

#define CENTERX		(VIEWX+VIEWWIDTH/2-1)	// middle of view window
#define CENTERY		(VIEWY+VIEWHEIGHT/2-1)

#define GLOBAL1		(1l<<16)
#define TILEGLOBAL  GLOBAL1
#define TILESHIFT	16l

#define MINDIST		(2*GLOBAL1/5)
#define FOCALLENGTH	(TILEGLOBAL)	// in global coordinates

#define ANGLES		360		// must be divisable by 4

#define MAPSIZE		64		// maps are 64*64 max
#define MAXACTORS	100		// max number of tanks, etc / map

#define NORTH	0
#define EAST	1
#define SOUTH	2
#define WEST	3

#define SIGN(x) ((x)>0?1:-1)
#define ABS(x) ((int)(x)>0?(x):-(x))
#define LABS(x) ((long)(x)>0?(x):-(x))

#define	MAXSCALE	(VIEWWIDTH/2)


#define MAXBODY		100
#define MAXSHOTPOWER	56

#define SCREEN1START	0
#define SCREEN2START	8320

#define STATUSLEN			0xc80
#define PAGELEN			0x1700

#define PAGE1START	STATUSLEN
#define PAGE2START	(PAGE1START+PAGELEN)
#define PAGE3START	(PAGE2START+PAGELEN)
#define FREESTART		(PAGE3START+PAGELEN)

#define PIXRADIUS		512

#define STATUSLINES		(200-VIEWHEIGHT)

enum bonusnumbers {B_BOLT,B_NUKE,B_POTION,B_RKEY,B_YKEY,B_GKEY,B_BKEY,B_SCROLL1,
 B_SCROLL2,B_SCROLL3,B_SCROLL4,B_SCROLL5,B_SCROLL6,B_SCROLL7,B_SCROLL8,
 B_GOAL,B_CHEST,B_RGEM,B_GGEM,B_BGEM,B_YGEM,B_PGEM};


#define MAX_DOOR_STORAGE 5

#define GEM_DELAY_TIME	(120*60)

#define ROTATE_SPEED		(6)

#define WALL_SKELETON_CODE 6

#define MAXREALTICS (2*60)

#define MAXFREEZETIME			(100*30)				// 50 secs (100 half)

#define INVISIBLEWALL			0x46

#define USE_STRIPS		FALSE

/*
=============================================================================

						   GLOBAL TYPES

=============================================================================
*/

enum {BLANKCHAR=9,BOLTCHAR,NUKECHAR,POTIONCHAR,KEYCHARS,SCROLLCHARS=17,
	NUMBERCHARS=25};

typedef long fixed;

typedef struct {int x,y;} tilept;
typedef struct {fixed x,y;} globpt;

typedef struct
{
  int	x1,x2,leftclip,rightclip;// first pixel of wall (may not be visable)
  unsigned	height1,height2,color,walllength,side;
	long	planecoord;
} walltype;

typedef enum
  {nothing,playerobj,bonusobj,succubusobj,batobj,skeletonobj,fatdemonobj,godessobj,
  mageobj,pshotobj,bigpshotobj,mshotobj,inertobj,bounceobj,grelmobj,sshotobj,
  gateobj,zombieobj,antobj,wetobj,expobj,eyeobj,wallskelobj,eshotobj,treeobj,
  gshotobj,reddemonobj,freezeobj,solidobj,cloudobj,dshotobj,hbunnyobj,bunnyobj,
  realsolidobj} classtype;

typedef enum {north,east,south,west,northeast,southeast,southwest,
		  northwest,nodir} dirtype;		// a catacombs 2 carryover

typedef struct	statestruct
{
	int		shapenum;
	int		tictime;
	void	(*think) ();
	struct	statestruct	far *next;
} statetypestruct;

#define statetype statetypestruct far

#define of_shootable		0x01
#define of_damagedone	0x02
#define of_forcefield	0x40		// defines a solid object as a forcefield???????????
#define of_tree			0x80		// used to identify between a tree and a statue --
											//			last minute changes for Greg

typedef struct objstruct
{

	int ticcount;						//
	statetype *state;					// THESE MEMBERS MUST BE IN THE SAME
	fixed x,y;                    // ORDER AS THE MEMBERS DEFINED IN
	int viewx;                    // IOBJSTRUCT OR ALL HELL WILL BREAK
	unsigned tilex,tiley;         // LOOSE!!
	unsigned viewheight;          //
	unsigned size;                //
	struct	objstruct	*next;   //


  struct objstruct *prev;
  enum {no,noalways,yes,always}	active;
  classtype	obclass;

  unsigned char flags;

  long		distance;
  dirtype	dir;

  int 		angle;
  int		hitpoints;
  long		speed;

  fixed		xl,xh,yl,yh;	// hit rectangle

  int		temp1,temp2;
} objtype;

#if USE_INERT_LIST

#define MAXINERTOBJ 20

typedef struct iobjstruct {
	int ticcount;
	statetype *state;
	fixed x,y;
	int viewx;
	unsigned tilex,tiley;
	unsigned viewheight;
	unsigned size;
	struct iobjstruct *next;
} inertobjtype;

#endif


typedef	enum	{ex_stillplaying,ex_died,ex_warped,ex_resetgame
	,ex_loadedgame,ex_victorious,ex_turning,ex_abort} exittype;


typedef enum {S_NONE,     S_HELP,    S_SND,     S_SAVING,   S_RESTORING,
			S_JOYSTICK, S_RETREAT, S_ADVANCE, S_SIDESTEP, S_QTURN,
			S_MISSLE,   S_ZAPPER,  S_XTER,    S_CURING,   S_READ,
			S_VIEWING,  S_ITEMDES,  S_DAMAGE,  S_TURN,    S_TIMESTOP} status_flags;

typedef struct {
	char x,y;
	unsigned ondoor,underdoor;
} doorinfo;

typedef struct {
	char x,y;
	short angle;
	doorinfo doors[MAX_DOOR_STORAGE];
} levelinfo;

typedef	struct
{
	int		difficulty;
	int		mapon;
	int		bolts,nukes,potions,keys[4],scrolls[8];

	int		gems[5];				// "int allgems[5]" is used for 1:1 comparison
										// in play loop for radar... CHANGE IT, TOO!

	long	score;
	int		body,shotpower;
	short mapwidth,mapheight;
//	levelinfo levels[NUMLEVELS];
} gametype;

/*
=============================================================================

						 C3_MAIN DEFINITIONS

=============================================================================
*/

extern	char inlevel[][2];
extern	char		str[80],str2[20];
extern	unsigned	tedlevelnum;
extern	boolean		tedlevel;
extern	gametype	gamestate;
extern	exittype	playstate;
extern 	char SlowMode;
extern   unsigned Flags;
extern	boolean LoadShapes;
extern	boolean EASYMODEON;


void NewGame (void);
boolean	SaveTheGame(int file);
boolean	LoadTheGame(int file);
void ResetGame(void);
void ShutdownId (void);
void InitGame (void);
void Quit (char *error, ...);
void TEDDeath(void);
void DemoLoop (void);
void SetupScalePic (unsigned picnum);
void SetupScaleWall (unsigned picnum);
void SetupScaling (void);
void main (void);
void Display320(void);
void Display640(void);
void PrintHelp(void);

/*
=============================================================================

						 C3_GAME DEFINITIONS

=============================================================================
*/

extern	unsigned	latchpics[NUMLATCHPICS];
extern	unsigned	tileoffsets[NUMTILE16];
extern	unsigned	textstarts[27];


#define	L_CHARS		0
#define L_NOSHOT	1
#define L_SHOTBAR	2
#define L_NOBODY	3
#define L_BODYBAR	4


void ScanInfoPlane (void);
void ScanText (void);
void SetupGameLevel(void);
void Victory (boolean playsounds);
void Died (void);
void NormalScreen (void);
void DrawPlayScreen (void);
void LoadLatchMem (void);
void FizzleFade (unsigned source, unsigned dest,
	unsigned width,unsigned height, boolean abortable);
void FizzleOut (int showlevel);
void FreeUpMemory (void);
void GameLoop (void);


/*
=============================================================================

						 C3_PLAY DEFINITIONS

=============================================================================
*/

#define BGF_NIGHT			0x01		// it is officially night
#define BGF_NOT_LIGHTNING		0x02		// lightning flash has ended

extern byte BGFLAGS,bcolor;

extern unsigned *skycolor,*groundcolor;

extern	ControlInfo	control;
extern	boolean		running,slowturn;

extern	int			bordertime;

extern	byte		tilemap[MAPSIZE][MAPSIZE];
extern	objtype		*actorat[MAPSIZE][MAPSIZE];
extern	byte		spotvis[MAPSIZE][MAPSIZE];

extern	objtype 	objlist[MAXACTORS],*new,*obj,*player;

extern	unsigned	farmapylookup[MAPSIZE];
extern	byte		*nearmapylookup[MAPSIZE];
extern	byte		update[];

extern	boolean		godmode,singlestep;
extern	int			extravbls;

extern	int			mousexmove,mouseymove;
extern	int			pointcount,pointsleft;
extern	status_flags    status_flag;
extern  int             status_delay;

extern 	objtype 		dummyobj;
extern 	short 		BeepTime;
extern  unsigned	scolor,gcolor;

void CenterWindow(word w,word h);
void DebugMemory (void);
void PicturePause (void);
int  DebugKeys (void);
void CheckKeys (void);
void InitObjList (void);
void GetNewObj (boolean usedummy);
void RemoveObj (objtype *gone);
void PollControlls (void);
void PlayLoop (void);
void InitBgChange(short stimer, unsigned *scolors, short gtimer, unsigned *gcolors, byte flag);

void DisplayStatus (status_flags *stat_flag);

/*
=============================================================================

						 C3_STATE DEFINITIONS

=============================================================================
*/

//void SpawnNewObj (unsigned x, unsigned y, statetype *state, unsigned size);
//void SpawnNewObjFrac (long x, long y, statetype *state, unsigned size);

void Internal_SpawnNewObj (unsigned x, unsigned y, statetype *state, unsigned size, boolean UseDummy, boolean PutInActorat);
void Internal_SpawnNewObjFrac (long x, long y, statetype *state, unsigned size,boolean UseDummy);

#define DSpawnNewObj(x, y, state, size)				Internal_SpawnNewObj(x,y,state,size,true,true)
#define SpawnNewObj(x, y, state, size)					Internal_SpawnNewObj(x,y,state,size,false,true)
#define ASpawnNewObj(x, y, state, size)					Internal_SpawnNewObj(x,y,state,size,false,false)
#define SpawnNewObjFrac(x, y, state, size,Dummy)	Internal_SpawnNewObjFrac(x, y, state, size,false)
#define DSpawnNewObjFrac(x, y, state, size)			Internal_SpawnNewObjFrac(x, y, state, size,true)

boolean CheckHandAttack (objtype *ob);
void T_DoDamage (objtype *ob);
boolean Walk (objtype *ob);
void ChaseThink (objtype *obj, boolean diagonal);
void MoveObj (objtype *ob, long move);
boolean Chase (objtype *ob, boolean diagonal);

extern	dirtype opposite[9];

/*
=============================================================================

						 C3_TRACE DEFINITIONS

=============================================================================
*/

int FollowTrace (fixed tracex, fixed tracey, long deltax, long deltay, int max);
int BackTrace (int finish);
void ForwardTrace (void);
int FinishWall (void);
void InsideCorner (void);
void OutsideCorner (void);
void FollowWalls (void);

extern	boolean	aborttrace;

/*
=============================================================================

						 C3_DRAW DEFINITIONS

=============================================================================
*/

#define MAXWALLS	50
#define DANGERHIGH	45

#define	MIDWALL		(MAXWALLS/2)

//==========================================================================

extern	tilept	tile,lasttile,focal,left,mid,right;

extern	globpt	edge,view;

extern	unsigned screenloc[3];
extern	unsigned freelatch;

extern	int screenpage;

extern	boolean		fizzlein;

extern	long lasttimecount;

extern	int firstangle,lastangle;

extern	fixed prestep;

extern	int traceclip,tracetop;

extern	fixed sintable[ANGLES+ANGLES/4],*costable;

extern	fixed	viewx,viewy,viewsin,viewcos;			// the focal point
extern	int	viewangle;

extern	fixed scale,scaleglobal;
extern	unsigned slideofs;

extern	int zbuffer[VIEWXH+1];

extern	walltype	walls[MAXWALLS],*leftwall,*rightwall;


extern	fixed	tileglobal;
extern	fixed	focallength;
extern	fixed	mindist;
extern	int		viewheight;
extern	fixed scale;

extern	int	far walllight1[NUMFLOORS];
extern	int	far walldark1[NUMFLOORS];

extern unsigned topcolor,bottomcolor;

extern char wall_anim_info[NUMFLOORS];
extern char wall_anim_pos[NUMFLOORS];

//==========================================================================

void	DrawLine (int xl, int xh, int y,int color);
void	DrawWall (walltype *wallptr);
void	TraceRay (unsigned angle);
fixed	FixedByFrac (fixed a, fixed b);
void	TransformPoint (fixed gx, fixed gy, int *screenx, unsigned *screenheight);
fixed	TransformX (fixed gx, fixed gy);
int	FollowTrace (fixed tracex, fixed tracey, long deltax, long deltay, int max);
void	ForwardTrace (void);
int	FinishWall (void);
int	TurnClockwise (void);
int	TurnCounterClockwise (void);
void	FollowWall (void);

void	NewScene (void);
void	BuildTables (void);


/*
=============================================================================

						 C3_SCALE DEFINITIONS

=============================================================================
*/


#define COMPSCALECODESTART	(65*6)		// offset to start of code in comp scaler

typedef struct
{
	unsigned	codeofs[65];
	unsigned	start[65];
	unsigned	width[65];
	byte		code[];
}	t_compscale;

typedef struct
{
	unsigned	width;
	unsigned	codeofs[64];
}	t_compshape;


extern unsigned	scaleblockwidth,
		scaleblockheight,
		scaleblockdest;

extern	byte	plotpix[8];
extern	byte	bitmasks1[8][8];
extern	byte	bitmasks2[8][8];


extern	t_compscale _seg *scaledirectory[NUMSCALEPICS];
extern	t_compshape _seg *shapedirectory[NUMSCALEPICS];
extern	memptr			walldirectory[NUMSCALEWALLS];
extern	unsigned	shapesize[NUMSCALEPICS];

void 		DeplanePic (int picnum);
void ScaleShape (int xcenter, t_compshape _seg *compshape, unsigned scale);
unsigned	BuildCompShape (t_compshape _seg **finalspot);


/*
=============================================================================

						 C3_ASM DEFINITIONS

=============================================================================
*/

extern	unsigned	wallheight	[VIEWWIDTH];
extern	unsigned	wallwidth	[VIEWWIDTH];
extern	unsigned	wallseg		[VIEWWIDTH];
extern	unsigned	wallofs		[VIEWWIDTH];
extern	unsigned	screenbyte	[VIEWWIDTH];
extern	unsigned	screenbit	[VIEWWIDTH];
extern	unsigned	bitmasks	[64];

extern	long		wallscalecall;

void	ScaleWalls (void);

/*
=============================================================================

						 C3_WIZ DEFINITIONS

=============================================================================
*/

#define MAXHANDHEIGHT	72

extern statetype s_pshot_exp1;
extern statetype s_pshot_exp2;
extern statetype s_pshot_exp3;

extern	long	lastnuke;
extern   int lasttext;
extern	int		handheight;
extern	int		boltsleft,bolttimer;
extern short RadarXY[][3];

extern short RotateAngle;
extern short FreezeTime;

//void FaceDir(short x, short y, boolean StopTime);
//short CalcAngle(short dx, short dy);

void FaceAngle(short DestAngle);
void RotateView();
void InitRotate(short DestAngle);
short FaceDoor(short x, short y);

char DisplayMsg(char *text,char *choices);
char DisplaySMsg(char *text,char *choices);

extern statetype s_explode;

void SpawnExplosion(fixed x, fixed y,short Delay);
void T_ExpThink(objtype *obj);
void SpawnBigExplosion(fixed x, fixed y, short Delay, fixed Range);


/*
=============================================================================

						 C3_ACT1 DEFINITIONS

=============================================================================
*/

int EasyHitPoints(int NrmHitPts);
int EasyDoDamage(int Damage);

#define zombie_mode	ob->temp1
#define zombie_delay	ob->temp2

enum zombie_modes {zm_wait_for_dark,zm_wait_to_rise,zm_active};

enum eye_modes {em_other1,em_player1,em_other2,em_player2,em_other3,em_player3,em_other4,em_player4,em_dummy};

#define MSHOTDAMAGE	2
#define MSHOTSPEED	10000

#define ESHOTDAMAGE	1
#define ESHOTSPEED	5000

#define SSHOTDAMAGE	3
#define SSHOTSPEED	6500

#define RANDOM_ATTACK 20

extern dirtype dirtable[];
extern short other_x[],	other_y[];
extern short zombie_base_delay;

extern statetype s_fatdemon_ouch;
extern statetype s_fatdemon_blowup1;

extern statetype s_succubus_ouch;
extern statetype s_succubus_death1;

extern	statetype s_godessouch;
extern	statetype s_godessdie1;

extern	statetype s_mageouch;
extern	statetype s_magedie1;

extern	statetype s_grelouch;
extern	statetype s_greldie1;

extern	statetype s_batdie1;

extern	statetype s_zombie_death1;
extern	statetype s_zombie_ouch;

extern statetype s_zombie_rise1;
extern statetype s_zombie_rise2;
extern statetype s_zombie_rise3;
extern statetype s_zombie_rise4;

extern statetype s_ant_wait;
extern statetype s_ant_egg;
extern statetype s_ant_walk1;
extern statetype s_ant_walk2;
extern statetype s_ant_walk3;
extern statetype s_ant_attack1;
extern statetype s_ant_pause;
extern statetype s_ant_ouch;
extern statetype s_ant_die1;
extern statetype s_ant_die2;
extern statetype s_ant_die3;

extern statetype s_skel_pause;
extern statetype s_skel_1;
extern statetype s_skel_2;
extern statetype s_skel_3;
extern statetype s_skel_4;
extern statetype s_skel_attack1;
extern statetype s_skel_attack2;
extern statetype s_skel_attack3;
extern statetype s_skel_ouch;
extern statetype s_skel_die1;
extern statetype s_skel_die2;
extern statetype s_skel_die3;

extern statetype s_wet_pause;

extern statetype s_wet_bubbles1;
extern statetype s_wet_bubbles2;
extern statetype s_wet_bubbles3;
extern statetype s_wet_bubbles4;

extern statetype s_wet_peek;

extern statetype s_wet_rise1;
extern statetype s_wet_rise2;
extern statetype s_wet_rise3;
extern statetype s_wet_rise4;
extern statetype s_wet_rise5;

extern statetype s_wet_sink1;
extern statetype s_wet_sink2;
extern statetype s_wet_sink3;

extern statetype s_wet_walk1;
extern statetype s_wet_walk2;
extern statetype s_wet_walk3;
extern statetype s_wet_walk4;

extern statetype s_wet_attack1;
extern statetype s_wet_attack2;
extern statetype s_wet_attack3;
extern statetype s_wet_attack4;

extern statetype s_wet_ouch;

extern statetype s_wet_die1;
extern statetype s_wet_die2;
extern statetype s_wet_die3;
extern statetype s_wet_die4;
extern statetype s_wet_die5;

extern	statetype s_obj_gate1;
extern	statetype s_obj_gate2;
extern	statetype s_obj_gate3;
extern	statetype s_obj_gate4;

extern statetype s_eye_pause;

extern statetype s_eye_1;
extern statetype s_eye_2;
extern statetype s_eye_3;
extern statetype s_eye_4;

extern statetype s_eye_ouch;
extern statetype s_eye_ouch2;

extern statetype s_eye_die1;
extern statetype s_eye_die2;
extern statetype s_eye_die3;

extern	statetype s_mshot1;
extern	statetype s_mshot2;

extern statetype s_bonus_die;

extern	statetype s_red_demonouch;
extern	statetype s_red_demondie1;

extern statetype s_bunny_death1;
extern statetype s_bunny_ouch;

extern statetype s_tree_death1;
extern statetype s_tree_ouch;

extern statetype s_force_field_die;