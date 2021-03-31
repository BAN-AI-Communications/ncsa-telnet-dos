/*
*
*  rge.c by Aaron Contorer for NCSA
*
*  Thanks to Bruce Orchard for contributions to this code
*
*  graphics routines for drawing on EGA
*  Input coordinate space = 0..4095 by 0..4095
*  MUST BE COMPILED WITH LARGE MEMORY MODEL!
*
*/

#include <stdio.h>	/* used for debugging only */
#include <stdlib.h>
#include <dos.h>	/* used for EGA init call */
#include "externs.h"

#define TRUE 1
#define FALSE 0
#define INXMAX 4096
#define INXSHIFT 12
#define INYMAX 4096
#define INYSHIFT 12
#define SCRNXHI 639
#define SCRNYHI 349
#define MAXRW 32	/* max. number of real windows */

static int EGAactive;		/* number of currently EGAactive window */
static char *EGAname="Enhanced Graphics Adaptor, 640 x 350";
#define EGAxmax 640
#define EGAymax 350
#ifdef OLD_WAY
static int EGAbytes=80;		/* number of bytes per line */
#endif

/* Current status of an EGA window */
struct EGAWIN {
	char inuse; 		/* true if window in use */
	int pencolor, rotation, size;
	int winbot,winleft,wintall,winwide;
				/* position of the window in virtual space */
};

static struct EGAWIN EGAwins[MAXRW];
static void EGAsetup(void );	 /* prepare variables for use in other functions */

/* prepare variables for use in other functions */
static void EGAsetup(void )
{
}

/* go into EGA graphics mode */
void RGEgmode(void )
{
	n_gmode(16);
}

/* go into EGA 80x25 color text mode */
void RGEtmode(void)
{
	n_gmode(3);
	EGAactive=-1;
}

/*
	Clear the screen.
*/
void RGEclrscr(int w)
{
	if(w==EGAactive) {
		EGAsetup();
		RGEgmode();
	  }
}

/*
	Set up a new window; return its number.
	Returns -1 if cannot create window.
*/
int RGEnewwin(void)
{
	int w=0;

	while(w<MAXRW && EGAwins[w].inuse) 
		w++;
	if(w==MAXRW) 
		return(-1); 			/* no windows available */
	EGAwins[w].pencolor=7;
	EGAwins[w].winbot=0;
	EGAwins[w].wintall=3120;
	EGAwins[w].winleft=0;
	EGAwins[w].winwide=4096;
	EGAwins[w].inuse=TRUE;
	return(w);
}

void RGEclose(int w)
{
	if(EGAactive==w) {
		RGEclrscr(w);
		EGAactive=-1;
	  }
	EGAwins[w].inuse=FALSE;
}

/* set pixel at location (x,y) -- no range checking performed */
void RGEpoint(int w,int x,int y)
{
	int x2,y2; 				/* on-screen coordinates */
	if(w==EGAactive) {
		x2=(int)((EGAxmax*(long)x)>>INXSHIFT);
		y2=SCRNYHI-(int)(((long)y*EGAymax)>>INYSHIFT);
		EGAset(x2, y2, EGAwins[w].pencolor);
	  }
}

/*
	Do whatever has to be done when the drawing is all done.
	(For printers, that means eject page.)
*/
void RGEpagedone(int w)
{
	w=w;
	/* do nothing for EGA */
}

/*
	Copy 'count' bytes of data to screen starting at current
	cursor location.
*/
void RGEdataline(int w,char *data,int count)
{
	/* Function not supported yet. */
	w=w;
	data=data;
	count=count;
}

/*
	Change pen color to the specified color.
*/
void RGEpencolor(int w,int color)
{
	if(!color)
		color=1;
	color&=0x7;
									/* flip color scale */
	EGAwins[w].pencolor=8-color;
}

/*
	Set description of future device-supported graphtext.
	Rotation=quadrant.
*/
void RGEcharmode(int w,int rotation,int size)
{
/* No rotatable device-supported graphtext is available on EGA. */
	w=w;
	rotation=rotation;
	size=size;
}

/* Not yet supported: */
void RGEshowcur(void) {}
void RGElockcur(void) {}
void RGEhidecur(void) {}

/* draw a line from (x0,y0) to (x1,y1) */
/* uses Bresenham's Line Algorithm */
void RGEdrawline(int w,int x0,int y0,int x1,int y1)
{
	int x,y,dx,dy,d,temp,
	dx2,dy2,					/* 2dx and 2dy */
	direction;					/* +1 or -1, used for slope */
	char transpose;				/* true if switching x and y for vertical-ish line */

#ifdef DEBUG
printf("RGEdrawline(): x0=%d, y0%d\n",x0,y0);
printf("x1=%d, y1=%d\n",x1,y1);
#endif
	if(w!=EGAactive) 
		return;
	x0=(int)(((long)x0*EGAxmax)>>INXSHIFT);
	y0=EGAymax-1-(int)((EGAymax*(long)y0)>>INYSHIFT);
	x1=(int)(((long)x1*EGAxmax)>>INXSHIFT);
	y1=EGAymax-1-(int)((EGAymax*(long)y1)>>INYSHIFT);
	if(abs(y1-y0)>abs(x1-x0)) {		/* transpose vertical-ish to horizontal-ish */
		temp=x1; 
		x1=y1; 
		y1=temp;
		temp=x0; 
		x0=y0; 
		y0=temp;
		transpose=TRUE;
	  } 
	else 
		transpose=FALSE;
/* make sure line is left to right */
	if(x1<x0) {
		temp=x1; 
		x1=x0; 
		x0=temp;
		temp=y1; 
		y1=y0; 
		y0=temp;
	  }
/* SPECIAL CASE: 1 POINT */
	if(x1==x0 && y1==y0) {
		EGAset(x1,y1,EGAwins[w].pencolor);
		return;
	  }
/* ANY LINE > 1 POINT */
	x=x0;
	y=y0;
	dx=x1-x0;
	if(y1>=y0) {
		dy=y1-y0;
		direction=1;
	  } 
	else {
		dy=y0-y1;
		direction=-1;
	  }
	dx2=dx<<1;
	dy2=dy<<1;
	d=(dy<<1)-dx;
	if(transpose) {		/* CASE OF VERTICALISH (TRANSPOSED) LINES */
		while(x<=x1) {
			if(y>=0&&y<EGAxmax&&x>=0&&x<EGAymax)
				EGAset(y,x,EGAwins[w].pencolor);
			while(d>=0) {
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  }
	  } 
	else {			/* CASE OF HORIZONTALISH LINES */
		while(x<=x1) {
			if(x>=0&&x<EGAxmax&&y>=0&&y<EGAymax)
				EGAset(x,y,EGAwins[w].pencolor);
			while(d>=0) {
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  }
	  }			 /* end horizontalish */
}   /* end RGEdrawline() */

/* Ring bell in window w */
void RGEbell(int w)
{
	if(w==EGAactive)putchar(7);
}

/* return name of device that this RG supports */
char *RGEdevname(void)
{
	return(EGAname);
}

/* initialize all RGE variables */
void RGEinit(void)
{
	int i;

	EGAsetup();
	for(i=0; i<MAXRW; i++)
		EGAwins[i].inuse = FALSE;
	EGAactive=-1;
}

/*
	Make this window visible, hiding all others.
	Caller should follow this with clrscr and redraw to show the current
	contents of the window.
*/
void RGEuncover(int w)
{
	EGAactive=w;
}

/* Needed for possible future functionality */
void RGEinfo(int w,int a,int b,int c,int d,int v)
{
	w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}
