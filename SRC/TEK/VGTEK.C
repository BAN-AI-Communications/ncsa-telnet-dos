/*
vgtek.c by Aaron Contorer 1987 for NCSA
bugfixes by Tim Krauskopf 1988

Takes Tektronix codes as input; sends output to real graphics devices.

CHANGES TO MAKE:
Create a function to make sure a window is attached to a real window.
Calling program will call this whenever switching between active windows.
Pass virtual window number to RG driver so it can call back.
*/

#define FALSE 0
#define TRUE 1
#define MAXVG 20	 /* maximum number of VG windows */
					/* temporary states */
#define HIY 0		/* waiting for various pieces of coordinates */
#define EXTRA 1
#define LOY 2
#define HIX 3
#define LOX 4
#define DONE 5		/* not waiting for coordinates */
#define ENTERVEC 6	/* entering vector mode */
#define CANCEL 7	/* done but don't draw a line */
#define RS 8        /* RS - incremental plot mode */
#define ESCOUT 9	/* when you receive an escape char after a draw command */
#define CMD0 50		/* got esc, need 1st cmd letter */
#define SOMEL 51	/* got esc L, need 2nd letter */
#define IGNORE 52	/* ignore next char */
#define SOMEM 53	/* got esc M, need 2nd letter */
#define IGNORE2 54
#define INTEGER 60	/* waiting for 1st integer part */
#define INTEGER1 61	/* waiting for 2nd integer part */
#define INTEGER2 62	/* waiting for 3rd (last) integer part */
#define COLORINT 70
#define GTSIZE0 75
#define GTSIZE1 76
#define SOMET 80
#define JUNKARRAY 81
#define STARTDISC 82
#define DISCARDING 83
					/* output modes */
#define ALPHA 0
#define DRAW 1
#define MARK 3
#define TEMPDRAW 101
#define TEMPMOVE 102
#define TEMPMARK 103
					/* stroked fonts */
#define CHARWIDE 51		/* total horz. size */
#define CHARTALL 76		/* total vert. size */
#define CHARH 10		/* horz. unit size */
#define CHARV 13		/* vert. unit size */
					/* RG coordinate space dimensions */
#define RGXSIZE 4096
#define RGYSIZE 4096

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "externs.h"
#include "vgtek.h"
#include "vgfont.h"
#include "tekstor.h"

#ifdef MSC
#define mousecl	mousecml
#endif

static void clipvec(int vw, int xa, int ya, int xb, int yb);
static int fontnum(int vw, int n);
static void storexy(int vw, int x, int y);
static int joinup(int hi, int lo, int e);
static void newcoord(int vw);
static void linefeed(int vw);
static int drawc(int vw, char c);
static int clipt(double p, double q, double *t0, double *t1);

extern unsigned char s[550];
extern struct config def;
extern int ftpok, rcpok, temptek, viewmode;
extern int ginon;           /* whether we are in GIN mode or not */
extern int mighty;			/* whether there is a mouse or not hooked up to the system */

struct VGWINTYPE 
{
	int RGdevice, RGnum;
	char mode,modesave; 				/* current output mode */
	char loy,hiy,lox,hix,ex,ey; 		/* current graphics coordinates */
	char nloy,nhiy,nlox,nhix,nex,ney; 	/* new coordinates */
	int curx,cury;						/* current composite coordinates */
	int winbot,wintop,winleft,winright,wintall,winwide; /* position of window in virtual space */
	int textcol;						/* text starts in 0 or 2048 */
	int intin;							/* integer parameter being input */
	int pencolor;						/* current pen color */
	int fontnum,charx,chary; 			/* char size */
	int count; 							/* for temporary use in special state loops */
};

static struct VGWINTYPE VGwin[MAXVG]; 		/* virtual window descriptors */
static char state[MAXVG],savstate[MAXVG];	/* save state in a parallel array for speed */
static STOREP VGstore[MAXVG]; 				/* the store where data for this window is kept */
static char storing[MAXVG]; 				/* are we currently saving data from this window */
static int drawing[MAXVG]; 					/* redrawing or not? */

#define NUMSIZES 6 							/* number of char sizes */
static int charxset[NUMSIZES] = {56,51,34,31,112,168};
static int charyset[NUMSIZES] = {88,82,53,48,176,264};

void showmouse(void)
{
	int m1=1,m2=0,m3=0,m4=0;		/* mouse variables to show the mouse cursor */

	if(mighty) {
		vprint(console->vs, "Mouse Shown\r\n");
		mousecl(&m1,&m2,&m3,&m4);	/* call mouse routine to show mouse cursor */
	}
}									/* end showmouse() */

void hidemouse(void)
{
	int m1=2,m2=0,m3=0,m4=0;		/* mouse variables to hide the mouse cursor */

	if(mighty) {
		vprint(console->vs, "Mouse hidden\r\n");
		mousecl(&m1,&m2,&m3,&m4);	/* call mouse routine to hide mouse cursor */
	}
}									/* end hidemouse() */

/**************************************************************************/
/*  dispgr
*   display graphics menu screen
*/
void dispgr(void)
{
	int c,i,j,k,l;

	c=n_color(current->colors[0]);
	n_clear();
	n_cur(0,0);
	n_puts("ALT-G                           Graphics menu");
	n_puts("<            Press the appropriate function key or ESC to resume        >\n");
	strcpy(s,"   F1 - Write postscript to a file called: ");
	strcat(s,def.psfile);
	n_puts(s);
	n_puts(  "   F2 - Change postscript output file name\n");
	strcpy(s,"   F3 - Write HPGL code to a file called: ");
	strcat(s,def.hpfile);
	n_puts(s);
	n_puts(  "   F4 - Change HPGL output file name\n");
	strcpy(s,"   F5 - Write Tektronix 4014 codes to a file called: ");
	strcat(s,def.tekfile);
	n_puts(s);
	n_puts(  "   F6 - Change Tektronix output file name\n");
	VGwhatzoom(temptek,&i,&j,&k,&l);
	sprintf(s,"        View region is currently: %d,%d,%d,%d",i,j,k,l);
	n_puts(s);
	n_puts("   F7 - Set a new view region (Zoom, Pan)");
	n_puts("   RETURN - draw picture on screen in current zoom factor\n");
	n_puts("   Enter choice:");
	viewmode=8;
	n_color(c);
}

/*******************************************************************/
/*
*	Set font for window 'vw' to size 'n'.
*	Sizes are 0..3 in Tek 4014 standard.
*	Sizes 4 & 5 are used internally for Tek 4105 emulation.
*/
static int fontnum(int vw,int n)
{
	if(n<0||n>=NUMSIZES)
			return(-1);
	VGwin[vw].fontnum=n;
	VGwin[vw].charx=charxset[n];
	VGwin[vw].chary=charyset[n];
	return(0);
}

/* set graphics x and y position */
static void storexy(int vw,int x,int y)
{
	VGwin[vw].curx=x;
	VGwin[vw].cury=y;
}

/* returns the number represented by the 3 pieces */
static int joinup(int hi,int lo,int e)
{
	return (((hi/*&31*/)<<7)|((lo/*&31*/)<<2)|(e/*&3*/));	/* end joinup() */
}

/*
	Replace x,y with nx,ny
*/
static void newcoord(int vw)
{
	VGwin[vw].hiy=VGwin[vw].nhiy;
	VGwin[vw].hix=VGwin[vw].nhix;
	VGwin[vw].loy=VGwin[vw].nloy;
	VGwin[vw].lox=VGwin[vw].nlox;
	VGwin[vw].ey=VGwin[vw].ney;
	VGwin[vw].ex=VGwin[vw].nex;
	VGwin[vw].curx=joinup(VGwin[vw].nhix,VGwin[vw].nlox,VGwin[vw].nex);
	VGwin[vw].cury=joinup(VGwin[vw].nhiy,VGwin[vw].nloy,VGwin[vw].ney);
}

/*
	Perform a linefeed & cr(CHARTALL units) in specified window.
*/
static void linefeed(int vw)
{
/*  int y=joinup(VGwin[vw].hiy,VGwin[vw].loy,VGwin[vw].ey);*/
    int y=VGwin[vw].cury;
	int x;

	if(y>VGwin[vw].chary) 
		y -=VGwin[vw].chary;
	else {
		y= 3119-VGwin[vw].chary;
		VGwin[vw].textcol=2048-VGwin[vw].textcol;
	  }
	x=VGwin[vw].textcol;
	storexy(vw,x,y);
}
	
#ifdef DEBUG
static int drawc(int vw,char c)
{
	putchar(c);
}

#else
/*
	Draw a stroked character at the current cursor location.
	Uses simple 8-directional moving, 8-directional drawing.
*/
static drawc(int vw,char c)
{
	int x,y,savex,savey,strokex,strokey;
	int n;			/* number of times to perform command */
	char *pstroke;	/* pointer into stroke data */
	int hmag,vmag;

	if(c==10) {
		linefeed(vw);
		return(0);
	  }
	if(c==7) {
		(*RG[VGwin[vw].RGdevice].bell)(VGwin[vw].RGnum);
		unstore(VGstore[vw]);
		return(0);
	  }
	savey=y=VGwin[vw].cury;
	savex=x=VGwin[vw].curx;
	if(c==8) {
		if(savex<=VGwin[vw].textcol)
			return(0);
		savex-=VGwin[vw].charx;
		if(savex<VGwin[vw].textcol) 
			savex=VGwin[vw].textcol;
		VGwin[vw].cury=savey;
		VGwin[vw].curx=savex;
		return(0);
	  }
	hmag=VGwin[vw].charx / 10;
	vmag=VGwin[vw].chary / 10;
	if(3119-savey<VGwin[vw].chary) {
		savey=y=3119-VGwin[vw].chary;
	  }
	if(c<32||c>126) 
		return(0);
	c-=32;
	pstroke=VGfont[c];
	while(*pstroke) {
		strokex=x;
		strokey=y;
		n=(*(pstroke++)-48);	/* run length */
		c=*(pstroke++);			/* direction code */
		switch(c) { 			/* horizontal movement: positive=right */
			case 'e': 
			case 'd': 
			case 'c': 
			case 'y': 
			case 'h': 
			case 'n':
				x+=n*hmag;
				break;

			case 'q': 
			case 'a': 
			case 'z': 
			case 'r': 
			case 'f': 
			case 'v':
				x-=n*hmag;
		  }

		switch(c) { 		/* vertical movement: positive=up */
			case 'q': 
			case 'w': 
			case 'e': 
			case 'r': 
			case 't': 
			case 'y':
				y+=n*vmag;
				break;

			case 'z': 
			case 'x': 
			case 'c': 
			case 'v': 
			case 'b': 
			case 'n':
				y-=n*vmag;
		  }

		switch(c) { 		/* draw or move */
			case 'r': 
			case 't': 
			case 'y': 
			case 'f': 
			case 'h':
			case 'v': 
			case 'b': 
			case 'n':
				clipvec(vw,strokex,strokey,x,y);
				break;
		  }
	
	  } 					/* end while not at end of string */

/* Update cursor location to next char position */
    if(savex+2*VGwin[vw].chary<=4096)
        savex+=VGwin[vw].charx;
	else {
		savey-=VGwin[vw].chary;
		if(savey<0) {
            savey=3119-VGwin[vw].chary;
            VGwin[vw].textcol=2048-VGwin[vw].textcol;
		  }
		savex=VGwin[vw].textcol;
	  }
	VGwin[vw].cury=savey;
	VGwin[vw].curx=savex;		 /* end drawc() */
}

#endif

/* To be called only by clipvec() */
static int clipt(double p,double q,double *t0,double *t1)
{
	double r;

	if(p<0.0) {
		r=q/p;
		if(r>*t1) 
            return((int)FALSE);
		else 
			if(r>*t0) 
				*t0=r;
	  }
    else if(p>0.0) {
			r=q/p;
			if(r<*t0) 
                return((int)FALSE);
			else 
				if(r<*t1) 
					*t1=r;
      }
    else
        if(q<0.0)
            return((int)FALSE);
    return((int)TRUE);
}

#ifdef DEBUG
static void clipvec(int vw,int xa,int ya,int xb,int yb)
{
	printf("%d,%d to %d,%d\n",xa,ya,xb,yb);
}

#else
/*
	Draw a vector in vw's window from x0,y0 to x1,y1.
	Zoom the vector to the current visible window,
	and clip it before drawing it.
	Uses Liang-Barsky algorithm from ACM Transactions on Graphics,
		Vol. 3, No. 1, January 1984, p. 7.
*/
static void clipvec(int vw,int xa,int ya,int xb,int yb)
{
	int t,b,l,r;
	double x0,y0,x1,y1,t0,t1,deltay,deltax;
	struct VGWINTYPE *vp;
	vp=&VGwin[vw];

	t=vp->wintop;
	b=vp->winbot;
	l=vp->winleft;
	r=vp->winright;

/* totally visible */
	if(xa<=r&&xb<=r&&xa>=l&&xb>=l&&ya<=t&&yb<=t&&ya>=b&&yb>=b) {
		if (ginon) hidemouse();  /* hide mouse cursor */
		(*RG[vp->RGdevice].drawline)(vp->RGnum,
		(int)((long)(xa - l) * RGXSIZE / vp->winwide),
		(int)((long)(ya- b) * RGYSIZE / vp->wintall),
		(int)((long)(xb - l) * RGXSIZE / vp->winwide),
		(int)((long)(yb- b) * RGYSIZE / vp->wintall));
		if (ginon) showmouse();	 /* show mouse cursor */
		return;
	  }
/* trivially invisible */
	if((xa>r&&xb>r)||(xa<l&&xb<l)||(ya<b&&yb<b)||(ya>t&&yb>t)) 
		return;
/* the clipping algorithm */
	x0=(double)xa;
	y0=(double)ya;
	x1=(double)xb;
	y1=(double)yb;
	t0=0.0;
	t1=1.0;
	deltax=x1-x0;

	if(clipt(-deltax,x0-l,&t0,&t1)) {
		if(clipt(deltax,r-x0,&t0,&t1)) {
			deltay=y1-y0;
			if(clipt(-deltay,y0-b,&t0,&t1)) {
				if(clipt(deltay,t-y0,&t0,&t1)) {
					if(t1<1.0) {
						x1=x0+t1*deltax;
						y1=y0+t1*deltay;
					  }
					if(t0>0.0) {
						x0+=t0*deltax;
						y0+=t0*deltay;
					  }
/* draw the line, it is at least partially visible */
					(*RG[vp->RGdevice].drawline)(vp->RGnum,
						(int)((long)((int)x0-l)*RGXSIZE/vp->winwide),
						(int)((long)((int)y0-b)*RGYSIZE/vp->wintall),
						(int)((long)((int)x1-l)*RGXSIZE/vp->winwide),
						(int)((long)((int)y1-b)*RGYSIZE/vp->wintall));
				  } 
			  } 
		  } 
      } /* end if */
} /* end clipvec() */
#endif

/*******************************************************
********************************************************
All routines given below may be called by the user
program.  No routines given above may be called from
the user program.
********************************************************
*******************************************************/

/*
	Initialize the whole VG environment.  Should be called ONCE
	at program startup before using the VG routines.
*/
void VGinit(void)
{
	int i;
	for(i=0; i<MAXVG; i++)
		VGwin[i].RGdevice=-1; /* no device */
#ifndef DEBUG
	for(i=0; i<MAXRG; i++) 
		(*RG[i].init)();
#endif
}

/*
	Make sure window is completely visible, rather than partly or
	completely invisible.
	This function is to be called whenever a window is made active,
	to ensure that the user can see it.
*/
void VGuncover(int vw)
{
#ifndef DEBUG
	(*RG[VGwin[vw].RGdevice].uncover)(VGwin[vw].RGnum);
#endif
}

#ifdef NOT_USED
/*
	Detach window from its current device and attach it to the
	specified device.  Returns negative number if unable to do so.
	Sample application:  switching an image from #9 to Hercules.
	Must redraw after calling this.
*/
int VGdevice(int vw,int dev)
{
	int newwin;
	newwin=(*RG[dev].newwin)();
	if(newwin<0) 
		return((int)newwin); 		/* unable to open new window */
	(*RG[VGwin[vw].RGdevice].close)(VGwin[vw].RGnum);
	VGwin[vw].RGdevice=dev;
	VGwin[vw].RGnum=newwin;
	VGwin[vw].pencolor=1;
	fontnum(vw,1);
	return(0);
}
#endif

/*
	Create a new VG window and return its number.
	New window will be attached to specified real device.
	Returns -1 if unable to create a new VG window.

    device -     number of RG device to use
*/
int VGnewwin(int device)
{
	int vw=0;
	while(vw<MAXVG&&VGwin[vw].RGdevice!=-1)
		vw++;
	if(vw==MAXVG) {
		return(-1);
	  }
	VGstore[vw]=newstore();
	if(VGstore[vw]==NULL) {
		return(-1);				/* no memory */
	  }
	VGwin[vw].RGdevice=device;
	VGwin[vw].RGnum=(*RG[device].newwin)();
	if(VGwin[vw].RGnum< 0) {	/* no windows available on device */
		freestore(VGstore[vw]);
		return(-1);
	  }
	VGwin[vw].mode=ALPHA;
	state[vw]=DONE;
	storing[vw]=TRUE;
	VGwin[vw].textcol=0;
	drawing[vw]=1;
	fontnum(vw,0);
	(*RG[device].pencolor)(VGwin[vw].RGnum,1);
	storexy(vw,0,3071);
	VGzoom(vw,0,0,4095,3119);
	return(vw);
}

/*
	Clear the store associated with window vw.  
	All contents are lost.
	User program can call this whenever desired.
	Automatically called after receipt of Tek page command.
*/
void VGclrstor(int vw)
{
	freestore(VGstore[vw]);
    VGstore[vw]=newstore(); /* Don't have to check for errors; there was definitely enough memory. */
}

/*
	Successively call the function pointed to by 'func' for each
	character stored from window vw.  Each character will
	be passed in integer form as the only parameter.  A value of -1
	will be passed on the last call to indicate the end of the data.
*/
void VGdumpstore(int vw,void (*func)(int ))
{
	int data;
	STOREP st=VGstore[vw];

	topstore(st);
	while((data=nextitem(st))!=-1)(*func)(data);
	(*func)(-1);
}

/*
	This is the main Tek emulator process.  Pass it the window and
	the latest input character, and it will take care of the rest.
	Calls RG functions as well as local zoom and character drawing
	functions.
*/
void VGdraw(int vw,char c)
{
	char cmd;
	char value;
	char goagain; /* true means go thru the function a second time */
	struct VGWINTYPE *vp;
	vp=&VGwin[vw];

/*** MAIN LOOP ***/
 	do {
		cmd=(char) ((c>>5) & 0x03);
		value=(char) (c&0x1f);
		goagain=FALSE;
		switch(state[vw]) {
			case HIY: 			/* beginning of a vector */
				vp->nhiy=vp->hiy;
				vp->nhix=vp->hix;
				vp->nloy=vp->loy;
				vp->nlox=vp->lox;
				vp->ney =vp->ey;
				vp->nex =vp->ex;
				switch(cmd) {
					case 0:
						if(value==27) {		/* escape sequence */
							state[vw]=ESCOUT;
							savstate[vw]=HIY;
						  }
						else 
							if(value<27) 	/* ignore */
								break;
							else {
								state[vw]=CANCEL;
								goagain=TRUE;
							  }
						break;

					case 1: 				/* hiy */
						vp->nhiy=value;
						state[vw]=EXTRA;
						break;

					case 2: 				/* lox */
						vp->nlox=value;
						state[vw]=DONE;
						break;

					case 3: 				/* extra or loy */
						vp->nloy=value;
						state[vw]=LOY;
						break;
			  	  }
				break;		
	
			case ESCOUT:
				if(value!=13&&value!=10&&value!=27&&value!='~') {	/* skip all EOL-type characters */
					state[vw]=savstate[vw];
					goagain=TRUE;
				  }
				break;

			case EXTRA:						/* got hiy; expecting extra or loy */
				switch(cmd) {
					case 0:
						if(value==27) {		/* escape sequence */
							state[vw]=ESCOUT;
							savstate[vw]=EXTRA;
						  }
						else 
							if(value<27)	/* ignore */
								break;
							else {
								state[vw]=DONE;
								goagain=TRUE;
							  }
							break;

					case 1: 				/* hix */
						vp->nhix=value;
						state[vw]=LOX;
						break;

					case 2: 				/* lox */
						vp->nlox=value;
						state[vw]=DONE;
						break;

					case 3: 				/* extra or loy */
						vp->nloy=value;
						state[vw]=LOY;
						break;
			  	  }
				break;
	
			case LOY: 						/* got extra or loy; next may be loy or something else */
				switch(cmd) {
					case 0:
						if(value==27) {		/* escape sequence */
							state[vw]=ESCOUT;
							savstate[vw]=LOY;
						  }
						else 
							if(value<27) 	/* ignore */
								break;
							else {
								state[vw]=DONE;
								goagain=TRUE;
							  }
						break;

					case 1: 				/* hix */
						vp->nhix=value;
						state[vw]=LOX;
						break;

					case 2: 				/* lox */
						vp->nlox=value;
						state[vw]=DONE;
						break;

					case 3: 				/* this is loy; previous loy was really extra */
						vp->ney=(char) ((vp->nloy>>2)&3);
						vp->nex=(char) ((vp->nlox) & 3);
						vp->nloy=value;
						state[vw]=HIX;
						break;

				  }
				break;
	
			case HIX: 						/* hix or lox */
				switch(cmd) {
					case 0:
						if(value==27) {		/* escape sequence */
							state[vw]=ESCOUT;
							savstate[vw]=HIX;
						  }
						else 
							if(value<27)	/* ignore */
								break;
							else {
								state[vw]=DONE;
								goagain=TRUE;
							  }
						break;

					case 1: 				/* hix */
						vp->nhix=value;
						state[vw]=LOX;
						break;

					case 2:					 /* lox */
						vp->nlox=value;
						state[vw]=DONE;
						break;
				  }
		 		break;
	
			case LOX: 						/* must be lox */
				switch(cmd) {
					case 0:
						if(value==27) {		/* escape sequence */
							state[vw]=ESCOUT;
							savstate[vw]=LOX;
						  }
						else 
							if(value< 27) 	/* ignore */
								break;
							else {
								state[vw]=DONE;
								goagain=TRUE;
							  }
						break;

					case 2:
						vp->nlox=value;
						state[vw]=DONE;
						break;

				  }
				break;
	
			case ENTERVEC:
				if(c==7)
					vp->mode=DRAW;
				if(c<27)
					break;
				state[vw]=HIY;
				vp->mode=TEMPMOVE;
				vp->modesave=DRAW;
				goagain=TRUE;
				break;

			case RS:
				switch(c) {
					case ' ':				/* pen up */
						vp->modesave=vp->mode;
						vp->mode=TEMPMOVE;
						break;

					case 'P':				/* pen down */
						vp->mode=DRAW;
						break;

					case 'D':				/* move up */
						vp->cury++;
						break;

					case 'E':
						vp->cury++;
						vp->curx++;
						break;

					case 'A':
						vp->curx++;
						break;

					case 'I':
						vp->curx++;
						vp->cury--;
						break;

					case 'H':
						vp->cury--;
						break;

					case 'J':
						vp->curx--;
						vp->cury--;
						break;

					case 'B':
						vp->curx--;
						break;

					case 'F':
						vp->cury++;
						vp->curx--;
						break;

					case 27:
						savstate[vw]=RS;
						state[vw]=ESCOUT;
						break;

					default:		/*storexy(vw,vp->curx,vp->cury);*/
						state[vw]=CANCEL;
						goagain=TRUE;
						break;
				  }
				if(vp->mode==DRAW)
					clipvec(vw,vp->curx,vp->cury,vp->curx,vp->cury);
#ifdef DEBUG
				printf("RS: %d,%d\n",vp->curx,vp->cury);
#endif
				break;

			case CMD0: 							/* get 1st letter of cmd */
				switch(c) {
					case 29:					/* GS, start draw */
						state[vw]=DONE;
						goagain=TRUE;
						break;

					case '8':
						fontnum(vw,0);
						state[vw]=DONE;
						break;

					case '9':
						fontnum(vw,1);
						state[vw]=DONE;
						break;

					case ':':
						fontnum(vw,2);
						state[vw]=DONE;
						break;

					case ';':
						fontnum(vw,3);
						state[vw]=DONE;
						break;

					case 12: /* form feed=clrscr */
                        VGpage(vw);     /* changes state[vw] to DONE */
						VGclrstor(vw);				
						break;

					case 'L':
						state[vw]=SOMEL;
						break;

					case 'K':
						state[vw]=IGNORE;
						break;

					case 'M':
						state[vw]=SOMEM;
						break;

					case 'T':
						state[vw]=SOMET;
						break;

					case 26:
						setgin();			/* set the gin stuff up */
						unstore(VGstore[vw]);
						unstore(VGstore[vw]);
						break;

					case 10:
					case 13:
					case 27:
					case '~':
						savstate[vw]=DONE;
						state[vw]=ESCOUT;
						break;		/* completely ignore these after ESC */

					default:
						state[vw]=DONE;
				  } 				/* end switch */
				break;
	
			case SOMET:				/* Got ESC T; now handle 3rd char. */
				switch(c) {
					case 'G':		 /* set surface color map */
						state[vw]=INTEGER;
						savstate[vw]=JUNKARRAY;
						break;

					case 'F':		 /* set dialog area color map */
						state[vw]=JUNKARRAY;
						break;

					default:
						state[vw]=DONE;
				  }			
				break;

			case JUNKARRAY:			/* This character is the beginning of an integer arrayto be discarded.  Get array size. */
				savstate[vw]=STARTDISC;
				state[vw]=INTEGER;
				break;

			case STARTDISC:			/* Begin discarding integers. */
				vp->count=vp->intin + 1;
				goagain=TRUE;
				state[vw]=DISCARDING;
				break;

			case DISCARDING:		/* We are in the process of discarding an integer array. */
				goagain=TRUE;
				if(!(--(vp->count))) 
					state[vw]=DONE;
				else
					if(vp->count==1) {
						state[vw]=INTEGER;
						savstate[vw]=DONE;
					  }
 					else {
						state[vw]=INTEGER;
						savstate[vw]=DISCARDING;
					  }
				break;

			case INTEGER:
				if(c & 0x40) {
					vp->intin=c & 0x3f;
					state[vw]=INTEGER1;
				  } 
				else {
					vp->intin=c&0x0f;
					if(!(c & 0x10))
						vp->intin*=-1;
					state[vw]=savstate[vw];
				  }
				break;

			case INTEGER1:
				if(c & 0x40) {
					vp->intin=(vp->intin<<6)|(c&0x3f);
					state[vw]=INTEGER2;
				  } 
				else {
					vp->intin=(vp->intin<<4)|(c&0x0f);
					if(!(c & 0x10)) 
						vp->intin*=-1;
					state[vw]=savstate[vw];
				  }
				break;

			case INTEGER2:
				vp->intin=(vp->intin<<4)|(c&0x0f);
				if(!(c&0x10)) 
					vp->intin*=-1;
				state[vw]=savstate[vw];
				break;

			case IGNORE: 		/* ignore next char; it's not supported */
				state[vw]=DONE;
				break;
	
			case IGNORE2: 		/* ignore next 2 chars */
				state[vw]=IGNORE;
				break;

			case SOMEL: 		/* now process 2nd letter */
				switch(c) {
					case 'F': 	/* move */
						vp->modesave=vp->mode;
						vp->mode=TEMPMOVE;
						state[vw]=HIY;
						break;

					case 'G':	 /* draw */
						vp->modesave=vp->mode;
						vp->mode=TEMPDRAW;
						state[vw]=HIY;
						break;

					case 'H': 	/* marker */
						vp->modesave=vp->mode;
						vp->mode=TEMPMARK;
						state[vw]=HIY;
						break;

                    case 'Z':   /* clear dialog scroll */
                        VGpage(vw);     /* changes state[vw] to DONE */
                        VGclrstor(vw);              
                        break;

					default:
						state[vw]=DONE;
				  } 			/* end switch */
				break;
	
			case SOMEM:
				switch(c) {
					case 'C': 	/* set graphtext size */
						savstate[vw]=GTSIZE0;
						state[vw]=INTEGER;
						break;

					case 'L': 	/* set line index */
						savstate[vw]=COLORINT;
						state[vw]=INTEGER;
						break;

					default:
						state[vw]=DONE;
				  } 			/* end switch */
				break;
	
			case COLORINT: 		/* set line index; have integer */
				vp->pencolor=vp->intin;
				(*RG[vp->RGdevice].pencolor)
				(vp->RGnum,vp->intin);
				state[vw]=CANCEL;
				goagain=TRUE; 	/* we ignored current char; now process it */
				break;

			case GTSIZE0:		/* discard the first integer; get the 2nd */
				state[vw]=INTEGER; /* get the important middle integer */
				savstate[vw]=GTSIZE1;
				goagain=TRUE;
				break;

			case GTSIZE1:		/* integer is the height */
				if(vp->intin<88)
					fontnum(vw,0);
				else 
					if(vp->intin<149) 
						fontnum(vw,4);
					else 
						fontnum(vw,5);
				state[vw]=INTEGER; 			/* discard last integer */
				savstate[vw]=DONE;
				goagain=TRUE;
				break;


			case DONE: 			/* ready for anything */
				switch(c) {
					case 31:
						vp->mode=ALPHA; 
						state[vw]=CANCEL;
						break;

					case 30:
						state[vw]=RS;
						break;

					case 28:
						if(cmd) {
							vp->mode=MARK;
							state[vw]=HIY;
						  }
						break;

					case 29:
						state[vw]=ENTERVEC;
						break;

					case 27:
						state[vw]=CMD0;
						break;
				
					default:
						if(vp->mode==ALPHA) { 
							drawc(vw,c);
							state[vw]=CANCEL;
						  }
                        else if(vp->mode==DRAW && cmd) {
                            state[vw]=HIY;
                            goagain=TRUE;
                          }
                        else if(vp->mode==MARK && cmd) {
                            state[vw]=HIY;
                            goagain=TRUE;
                          }
                        else if(vp->mode==DRAW &&(c==13||c==10)) {   /* break drawing mode on CRLF */
                            vp->mode=ALPHA;
                            state[vw]=CANCEL;
                          }
                        else
                            state[vw]=CANCEL;   /* do nothing */
                  } /* end switch(c) */
          } /* end switch(state) */
		if(state[vw]==DONE) {
			if(vp->mode==TEMPMOVE) {
				vp->mode=vp->modesave;
				newcoord(vw);
			  }
			else 
				if(vp->mode==DRAW||vp->mode==TEMPDRAW) {
                    clipvec(vw,vp->curx,vp->cury,joinup(vp->nhix,vp->nlox,vp->nex),joinup(vp->nhiy,vp->nloy,vp->ney));
					if(vp->mode==TEMPDRAW) 
						vp->mode=vp->modesave;
					newcoord(vw);
				  }
				else 
					if(vp->mode==MARK||vp->mode==TEMPMARK)		/* draw marker */
						newcoord(vw);
		  } 					/* end if done */
		if(state[vw]==CANCEL) 
			state[vw]=DONE;
	  } while(goagain);
/*** END OF MAIN LOOP ***/
} /* end VGdraw() */

/*  Clear screen and have a few other effects:
	- Return graphics to home position(0,3071)
	- Switch to alpha mode
	This is a standard Tek command; don't look at me.
*/
void VGpage(int vw)
{
#ifndef DEBUG
	(*RG[VGwin[vw].RGdevice].clrscr)(VGwin[vw].RGnum);
	(*RG[VGwin[vw].RGdevice].pencolor)(VGwin[vw].RGnum,1);
#endif
	VGwin[vw].mode=ALPHA;
	state[vw]=DONE;
	VGwin[vw].textcol=0;
	fontnum(vw,0);
	storexy(vw,0,3071);
} /* end page */

/*
	Redraw window 'vw' in pieces to window 'dest'.
	Must call this function repeatedly to draw whole image.
	Only draws part of the image at a time, to yield CPU power.
	Returns 0 if needs to be called more, or 1 if the image
	is complete.  Another call would result in the redraw beginning again.
	User should clear screen before beginning redraw.
*/
int VGpred(int vw,int dest)
{
	int data;
	STOREP st=VGstore[vw];
	int count=0;

	if(drawing[vw]) {					/* wasn't redrawing */
		topstore(st);
		drawing[vw]=0;					/* redraw incomplete */
	  }
	while( ++count < (int)PREDCOUNT && ((data=(int)nextitem(st)) != -1) ) 
		VGdraw(dest,(char)data);
	if(data==-1)
		drawing[vw]=1; 					/* redraw complete */
	return((int)drawing[vw]);
}

#ifdef NOT_USED
/*
	Abort VGpred redrawing of specified window.
	Must call this routine if you decide not to complete the redraw.
*/
void VGstopred(int vw,int dest)
{
	drawing[vw]=1;
	dest=dest;
}

/*
	Redraw the contents of window 'vw' to window 'dest'.
	Does not yield CPU until done.
	User should clear the screen before calling this, to avoid 
	a messy display.
*/
void VGredraw(int vw,int dest)
{
	int data;
	STOREP st=VGstore[vw];

	topstore(st);
	while((data=(int)nextitem(st))!=-1) {
		VGdraw(dest,(char)data);
	  }
}
#endif

/*
	Set new borders for zoom/pan region.
	x0,y0 is lower left; x1,y1 is upper right.
	User should redraw after calling this.
*/
void VGzoom(int vw,int x0,int y0,int x1,int y1)
{
	VGwin[vw].winbot=y0;
	VGwin[vw].winleft=x0;
	VGwin[vw].wintop=y1;
	VGwin[vw].winright=x1;
	VGwin[vw].wintall=y1-y0+1;
	VGwin[vw].winwide=x1-x0+1;
	VGgiveinfo(vw);
}

void VGwhatzoom(int vw,int *px0,int *py0,int *px1,int *py1)
{
	*py0=VGwin[vw].winbot;
	*px0=VGwin[vw].winleft;
	*py1=VGwin[vw].wintop;
	*px1=VGwin[vw].winright;
}

/*
	Set zoom/pan borders for window 'dest' equal to those for window 'src'.
	User should redraw window 'dest' after calling this.
*/
void VGzcpy(int src,int dest)
{
	VGzoom(dest,VGwin[src].winleft, VGwin[src].winbot,
	VGwin[src].winright, VGwin[src].wintop);
}

/*
	Close virtual window.
	Release its real graphics device and its store.
*/
void VGclose(int vw)
{
	(*RG[VGwin[vw].RGdevice].close)(VGwin[vw].RGnum);
	freestore(VGstore[vw]);
	VGwin[vw].RGdevice=-1;
}

/*
	Draw the data pointed to by 'data' of length 'count'
	on window vw, and add it to the store for that window.
	This is THE way for user program to pass Tektronix data.
*/
int VGwrite(int vw,char *data,int count)
{
	char *c=data;
	char *end=&(data[count]);

	if(VGwin[vw].RGdevice==-1 || vw>=MAXVG || vw<0)
		return(-1);		/* window not open */
	if(storing[vw]) {
		while(c!=end) {
			if(*c==24)				/* ASC CAN character */
				return((int)(c-data+1));
			addstore(VGstore[vw], *c);
			VGdraw(vw,*c++);
		  }
	  } 
	else {
		while(c!=end) {
			if(*c==24)
				return((int)(c-data+1));
			else
				VGdraw(vw,*c++);
		  }
	  }
	return(count);
}

/*
	Send interesting information about the virtual window down to
	its RG, so that the RG can make VG calls and display zoom values
*/
void VGgiveinfo(int vw)
{
	(*RG[VGwin[vw].RGdevice].info)(VGwin[vw].RGnum,vw,VGwin[vw].winbot,VGwin[vw].winleft,VGwin[vw].wintop,VGwin[vw].winright);
}

#ifdef NOT_USED
/*
	Return a pointer to a human-readable string
	which describes the specified real device
*/
char *VGrgname(int rgdev)
{
	return(*RG[rgdev].devname)();
}
#endif

/* Put the specified real device into graphics mode */
void VGgmode(int rgdev)
{
#ifndef DEBUG
	(*RG[rgdev].gmode)();
#endif
}

/* Put the specified real device into text mode */
void VGtmode(int rgdev)
{
	(*RG[rgdev].tmode)();
}

/*
	Translate data for output as GIN report.

	User indicates VW number and x,y coordinates of the GIN cursor.
	Coordinate space is 0-4095, 0-4095 with 0,0 at the bottom left of
	the real window and 4095,4095 at the upper right of the real window.
	'c' is the character to be returned as the keypress.
	'a' is a pointer to an array of 5 characters.  The 5 chars must
	be transmitted by the user to the remote host as the GIN report.
#ifndef BEFORE_GIN_FIX
  A temporary fix is needed for routine VGgindata.  There is a bug that
  causees the window parameters in VGwin[vw].winwide and VGwin[vw].wintall
  to be valid only for the first telnet session, and 0 for subsequent
  sessions.  The fix replaces those two variables by the constants 4096
  and 3120, respectively.  I don't have any idea where these variables
  should be initialized.  -- Larry W. Finger  finger@gl.ciw.edu
#endif
*/
void VGgindata(int vw,int x,int y,char c,char *a)
{
	unsigned long x1,y1,x2,y2;
#ifndef BEFORE_GIN_FIX
  unsigned long wide,tall;    /* LWF  11/22/93 */
#endif

	x2=x;
	y2=y;
#ifndef BEFORE_GIN_FIX
  wide = 4096;    /* LWF  11/22/93 */
 	tall = 3120;    /* LWF  11/22/93 */
#endif
#ifdef CGW
	printf("x=%d y=%d\n",x,y);
	printf("x2=%d y2=%d\n",x2,y2);
#endif
	vw-=1;
#ifdef CGW
	printf("winwide=%d wintall=%d ",VGwin[vw].winwide,VGwin[vw].wintall);
	printf("winleft=%d  winbot=%d\n",VGwin[vw].winleft,VGwin[vw].winbot);
#endif
#ifdef OLDWAY
	x2=(((x1*RGXSIZE)/VGwin[vw].winwide)+VGwin[vw].winleft)>>2;
	y2=(((y1*RGYSIZE)/VGwin[vw].wintall)+VGwin[vw].winbot)>>2;
#else
#ifdef BEFORE_GIN_FIX
  x1=(x2 * (unsigned long)VGwin[vw].winwide) / 640;
	y1=(y2 * (unsigned long)VGwin[vw].wintall) / 350;
#else
  x1=(x2 * (unsigned long)wide) / 640;    /* LWF  11/22/93 */
 	y1=(y2 * (unsigned long)tall) / 350;    /* LWF  11/22/93 */
#endif
#ifdef CGW
	printf("1 x2=%d  y2=%d\n",(int) x1,(int) y1);
#endif
	x1+=VGwin[vw].winleft;
	y1+=VGwin[vw].winbot;
#ifdef CGW
	printf("2 x2=%d  y2=%d\n",(int) x1,(int) y1);
#endif
	x1=x1>>2;
	y1=y1>>2;
#ifdef CGW
	printf("3 x2=%d  y2=%d\n",(int) x1,(int) y1);
#endif
#endif

	a[0]=c;
	a[1]=(char)(0x20 + (x1 >> 5));
	a[2]=(char)(0x20 | (x1 & 0x001F));
	a[3]=(char)(0x20 + (y1 >> 5));
	a[4]=(char)(0x20 | (y1 & 0x001F));

 #ifdef CGW   /* conditional added by LWF  11/22/93 */
 	printf("a== %c %x %x %x %x\n",a[0],a[1],a[2],a[3],a[4]);
 #endif       

}


/**********************************************************************
*
* setgin()
*	shows the GIN cursor on the screen
*/
void setgin(void )
{
	int m1=0,m2=0,m3=0,m4=0;		/* mouse variables to show the mouse cursor */

	vprint(console->vs,"GIN set\r\n");
	if(mighty)
		mousecl(&m1,&m2,&m3,&m4);	/* call mouse routine to show mouse cursor */
	showmouse();
	ginon=1;
} /* end setgin() */

/**********************************************************************
*
* resetgin()
*	removes the GIN cursor on the screen
*/
void resetgin(void )
{
	hidemouse();
	vprint(console->vs, "GIN reset\r\n");
	ginon=0;
} /* end resetgin() */
