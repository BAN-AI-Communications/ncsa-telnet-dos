/*
*  rgc.c by Aaron Contorer for NCSA
*  graphics routines for drawing on CGA
*  Input coordinate space = 0..4095 by 0..4095
*  MUST BE COMPILED WITH LARGE MEMORY MODEL!
*/

#include <stdio.h>		/* used for debugging only */
#include <stdlib.h>
#include <dos.h>		/* for the FP_SEG & FP_OFF macros */
#include "externs.h"

#define TRUE 1
#define FALSE 0
#define INXMAX 4096
#define INXSHIFT 12
#define INYMAX 4096
#define INYSHIFT 12
#define MAGNIFY 1000
#define SCRNXHI 639
#define SCRNYHI 199
#define MAXRW 32		/* max. number of real windows */
#define CGAxmax 640		/* max. number of pixels in the x direction */
#define CGAymax 200		/* max. number of pixels in the y direction */
#define CGAbytes 80		/* number of bytes per line */

static int CGAactive;		/* number of currently CGAactive window */
static char *CGAvidram;
static unsigned char CGApbit[SCRNXHI+1];
#ifdef OLD_WAY
static unsigned char power2[8] = {1,2,4,8,16,32,64,128};
#endif
static char *CGAname = "Color Graphics Adaptor, 640 x 200";

static void CGAsetup(void );

/* Current status of an CGA window */
struct CGAWIN {
	char inuse; 		/* true if window in use */
	int pencolor, rotation, size;
	int winbot,winleft,wintall,winwide;
				/* position of the window in virtual space */
};

static struct CGAWIN CGAwins[MAXRW];

/* prepare variables for use in other functions */
static void CGAsetup(void )
{
	int x;
	CGApbit[0]=128; 
	CGApbit[1]=64; 
	CGApbit[2]=32; 
	CGApbit[3]=16;
	CGApbit[4]=8;   
	CGApbit[5]=4;  
	CGApbit[6]=2;  
	CGApbit[7]=1;
	for(x=8; x<=SCRNXHI; x++) 
		CGApbit[x]=CGApbit[x&7];
#if defined(MSC) && !defined(__TURBOC__) && !defined(__WATCOMC__)
	FP_SEG(CGAvidram) = 0xB800;
	FP_OFF(CGAvidram) = 0;
#else
    CGAvidram=MK_FP(0xB800,0);
#endif
}

/* go into CGA graphics mode */
void RGCgmode(void)
{
	n_gmode(6); 				/* 640x200 BW */
}

/* go into CGA 80x25 color text mode */
void RGCtmode(void)
{
	n_gmode(3); 				/* 80x25 color text */
	CGAactive=-1;
}

/* draw a line from (x0,y0) to (x1,y1) */
/* uses Bresenham's Line Algorithm */
void RGCdrawline(int w,int x0,int y0,int x1,int y1)
{
	int x,y,dx,dy,d,temp,
	dx2,dy2,			/* 2dx and 2dy */
	direction;			/*+1 or-1, used for slope */
	char transpose;		/* true if switching x and y for vertical-ish line */

	if (w!=CGAactive) 
		return;
	x0=(int)(((long)x0*CGAxmax)>>INXSHIFT);
	y0=CGAymax-1-(int)(((long)y0*CGAymax)>>INYSHIFT);
	x1=(int)(((long)x1*CGAxmax)>>INXSHIFT);
	y1=CGAymax-1-(int)(((long)y1*CGAymax)>>INYSHIFT);
	if(abs(y1-y0)>abs(x1-x0)) {			/* transpose vertical-ish to horizontal-ish */
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
	if(x1==x0&&y1==y0) { 
		CGAvidram[(y1&1)*8192+(y1>>1)*80+(x1>>3)]|=CGApbit[x1];
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
	if(transpose) {	
				/* CASE OF VERTICALISH (TRANSPOSED) LINES */
		while(x<=x1) {
			if(y>=0&&y<CGAxmax&&x>=0&&x<CGAymax)
				CGAvidram[(x&1)*8192+(x>>1)*80+(y>>3)]|=CGApbit[y];
			while(d>=0) {
				y+=direction;
				d -=dx2;
			  }
			d+=dy2;
			x++;
			  } 
		  } 
	else {
				/* CASE OF HORIZONTALISH LINES */
		while (x <= x1) {
			if(x>=0&&x<CGAxmax&&y>=0&&y<CGAymax)
				CGAvidram[(y&1)*8192+(y>>1)*80+(x>>3)]|=CGApbit[x];
			while(d>=0) {
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  }
	  } 		/* end horizontalish */
} 				/* end CGArasline() */

/*
*	Clear the screen.
*/
void RGCclrscr(int w)
{
	if(w==CGAactive) {
		CGAsetup();
		RGCgmode();
	  }
}

/*
*	Set up a new window; return its number.
*	Returns -1 if cannot create window.
*/
int RGCnewwin(void)
{
	int w=0;

	while(w<MAXRW&&CGAwins[w].inuse) 
		w++;
	if(w==MAXRW)
		return(-1); /* no windows available */
	CGAwins[w].pencolor=7;
	CGAwins[w].winbot=0;
	CGAwins[w].wintall=3120;
	CGAwins[w].winleft=0;
	CGAwins[w].winwide=4096;
	CGAwins[w].inuse=TRUE;
	return(w);
}

void RGCclose(int w)
{
	if(CGAactive==w) {
		RGCclrscr(w);
		CGAactive=-1;
	  }
	CGAwins[w].inuse=FALSE;
}

/* set pixel at location (x,y) -- no range checking performed */
void RGCpoint(int w,int x,int y)
{
	int x2,y2; /* on-screen coordinates */

	if(w==CGAactive) {
		x2=(int)(((long)x*CGAxmax)>>INXSHIFT);
		y2=SCRNYHI-(int)(((long)y*CGAymax)>>INYSHIFT);
#ifdef OLD_WAY
		CGAvidram[y2*80+(x2>>3)]|=CGApbit[x2];
#else
		CGAvidram[(y&1)*8192+(y>>1)*80+(x>>3)]|=CGApbit[x2];
#endif
	  }
}  

/*
	Do whatever has to be done when the drawing is all done.
	(For printers, that means eject page.)
*/
void RGCpagedone(int w)
{
	/* do nothing for CGA */
	w=w;
}

/*
	Copy 'count' bytes of data to screen starting at current
	cursor location.
*/
void RGCdataline(int w,char *data,int count)
{
	/* Function not supported yet. */
	w=w;
	data=data;
	count=count;
}

/*
	Change pen color to the specified color.
*/
void RGCpencolor(int w,int color)
{
	/* Function not supported. */
	w=w;
	color=color;
}

/*
	Set description of future device-supported graphtext.
	Rotation=quadrant.
*/
void RGCcharmode(int w,int rotation,int size)
{
	/* No rotatable device-supported graphtext is available on CGA. */
	w=w;
	rotation=rotation;
	size=size;
}

/* Not yet supported: */
void RGCshowcur(void) {}
void RGClockcur(void) {}
void RGChidecur(void) {}

/* Ring bell in window w */
void RGCbell(int w)
{
	if(w==CGAactive)
		putchar(7);
}

/* return name of device that this RG supports */
char *RGCdevname(void)
{
	return(CGAname);
}

/* initialize all RGC variables */
void RGCinit(void)
{
	int i;
	CGAsetup();
	for(i=0; i<MAXRW; i++)
		CGAwins[i].inuse=FALSE;
	CGAactive=-1;
}

/*
*	Make this window visible, hiding all others.
*	Caller should follow this with clrscr and redraw to show the current
*	contents of the window.
*/
void RGCuncover(int w)
{
	CGAactive=w;
}

/* Needed for possible future functionality */
void RGCinfo(int w,int a,int b,int c,int d,int v)
{
    w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}

