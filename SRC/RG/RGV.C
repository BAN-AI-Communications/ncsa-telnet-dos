/*
* 	rgv.c by Quincey Koziol for NCSA
*
*  graphics routines for drawing on EGA
*  Input coordinate space = 0..4095 by 0..4095
*  MUST BE COMPILED WITH LARGE MEMORY MODEL!
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>		/* used for VGA init call */
#include "externs.h"

#define TRUE 1
#define FALSE 0
#define INXMAX 4096
#define INXSHIFT 12
#define INYMAX 4096
#define INYSHIFT 12
#define SCRNXHI 639
#define SCRNYHI 479
#define VGAxmax 640			/* max. number of pixels in the x direction */
#define VGAymax 480			/* max. number of pixels in the y direction */
#define VGAbytes 80 		/* number of bytes per line */
#define MAXRW 32			/* max. number of real windows */

static int VGAactive;		/* number of currently EGAactive window */
static char *VGAname="Video Graphics Array, 640 x 480";

/* Current status of an VGA window */
struct VGAWIN {
	char inuse; 		/* true if window in use */
	int pencolor, rotation, size;
	int winbot,winleft,wintall,winwide;
				/* position of the window in virtual space */
};

static struct VGAWIN VGAwins[MAXRW];

/*
	Set up a new window; return its number.
	Returns -1 if cannot create window.
*/
int RGVnewwin(void)
{
	int w=0;

	while(w<MAXRW && VGAwins[w].inuse) 
		w++;
	if(w==MAXRW) 
		return(-1); 			/* no windows available */
	VGAwins[w].pencolor=7;
	VGAwins[w].winbot=0;
	VGAwins[w].wintall=4096;
	VGAwins[w].winleft=0;
	VGAwins[w].winwide=4096;
	VGAwins[w].inuse=TRUE;
	return(w);
}

/*
	Clear the screen.
*/
void RGVclrscr(int w)
{
	if(w==VGAactive)
		n_gmode(18);
}

void RGVclose(int w)
{
	if(VGAactive==w) {
		RGVclrscr(w);
		VGAactive=-1;
	  }
	VGAwins[w].inuse=FALSE;
}

/* set pixel at location (x,y) -- no range checking performed */
void RGVpoint(int w,int x,int y)
{
	int x2,y2; 				/* on-screen coordinates */
	if(w==VGAactive) {
		x2=(int)((VGAxmax*(long)x)>>INXSHIFT);
		y2=SCRNYHI-(int)(((long)y*VGAymax)>>INYSHIFT);
		VGAset(x2,y2,VGAwins[w].pencolor);
	  }
}

/* draw a line from (x0,y0) to (x1,y1) */
/* uses Bresenham's Line Algorithm */
void RGVdrawline(int w,int x0,int y0,int x1,int y1)
{
	int x,y,dx,dy,d,temp,
	dx2,dy2,					/* 2dx and 2dy */
	direction;					/* +1 or -1, used for slope */
	char transpose;				/* true if switching x and y for vertical-ish line */

#ifdef DEBUG
printf("RGVdrawline(): x0=%d, y0%d\n",x0,y0);
printf("x1=%d, y1=%d\n",x1,y1);
#endif
	if(w!=VGAactive) 
		return;
	x0=(int)(((long)x0*VGAxmax)>>INXSHIFT);
	y0=VGAymax-1-(int)((VGAymax*(long)y0)>>INYSHIFT);
	x1=(int)(((long)x1*VGAxmax)>>INXSHIFT);
	y1=VGAymax-1-(int)((VGAymax*(long)y1)>>INYSHIFT);
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
		VGAset(x1,y1,VGAwins[w].pencolor);
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
			if(y>=0 && y< VGAxmax && x>=0 && x<VGAymax)
				VGAset(y,x,VGAwins[w].pencolor);
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
			if(x>=0 && x<VGAxmax && y>=0 && y<VGAymax)
				VGAset(x,y,VGAwins[w].pencolor);
			while(d>=0) {
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  }
	  }			 /* end horizontalish */
}   /* end RGVdrawline() */

/*
	Do whatever has to be done when the drawing is all done.
	(For printers, that means eject page.)
*/
void RGVpagedone(int w)
{
	/* do nothing for VGA */
	w=w;
}

/*
	Copy 'count' bytes of data to screen starting at current
	cursor location.
*/
void RGVdataline(int w,char *data,int count)
{
	/* Function not supported yet. */
	w=w;
	data=data;
	count=count;
}

/*
	Change pen color to the specified color.
*/
void RGVpencolor(int w,int color)
{
	if(!color)
		color=1;
	color&=0x7;
									/* flip color scale */
	VGAwins[w].pencolor=8-color;
}

/*
	Set description of future device-supported graphtext.
	Rotation=quadrant.
*/
void RGVcharmode(int w,int rotation,int size)
{
	/* No rotatable device-supported graphtext is available on VGA. */
	w=w;
	rotation=rotation;
	size=size;
}

/* Not yet supported: */
void RGVshowcur(void) {}
void RGVlockcur(void) {}
void RGVhidecur(void) {}

/* Ring bell in window w */
void RGVbell(int w)
{
	if(w==VGAactive)
		putchar(7);
}

/* return name of device that this RG supports */
char *RGVdevname(void )
{
	return(VGAname);
}

/*
	Make this window visible, hiding all others.
	Caller should follow this with clrscr and redraw to show the current
	contents of the window.
*/
void RGVuncover(int w)
{
	VGAactive=w;
}

/* initialize all RGV variables */
void RGVinit(void)
{
	int i;
	for(i=0; i<MAXRW; i++)
		VGAwins[i].inuse = FALSE;
	VGAactive=-1;
}

void RGVinfo(int w,int a,int b,int c,int d,int v)
{
	w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}

/* go into VGA graphics mode */
void RGVgmode(void )
{
	n_gmode(18);
}

/* go into VGA 80x25 color text mode */
void RGVtmode(void)
{
	n_gmode(3);
	VGAactive=-1;
}
