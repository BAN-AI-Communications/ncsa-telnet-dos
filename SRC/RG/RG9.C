/*
*
*  rg9.c by Aaron Contorer for NCSA
*
*  graphics routines for drawing on Number Nine card
*  Input coordinate space = 0..4095 by 0..4095
*  MUST BE COMPILED WITH LARGE MEMORY MODEL!
* 
*  RG9 = routines callable from outside
*  NO9 = routines for internal use only
*
*/

#include <stdio.h>	/* used for debugging only */
#include <stdlib.h>
#include <dos.h>	/* used for NO9 init call */
#if defined(MSC) && !defined(__TURBOC__) && !defined(__WATCOMC__)
#include <memory.h>
#else
#include <string.h>
#endif
#include "externs.h"

#define TRUE 1
#define FALSE 0
#define INXMAX 4096
#define INXSHIFT 12
#define INYMAX 4096
#define INYSHIFT 12
#define MAGNIFY 1000
#define SCRNXHI 511
#define SCRNYHI 479
#define MAXWIN 32		/* max. number of windows */
#define NO9xmax 512 	/* max. number of pixels in the x direction */
#define NO9ymax 480		/* max. number of pixels in the y direction */

static int NO9active;		/* number of currently NO9active window */
static unsigned char *NO9ram;
static int *NO9bank; 		/* bank control registers */
#ifdef OLD_WAY
static unsigned char power2[8]={1,2,4,8,16,32,64,128};
#endif
static char *NO9name="Number Nine, 512 x 480";
static void clrbuf(unsigned int bank);
static void NO9point(int ,int );
static void NO9setup(void );

/* Current status of an NO9 window */
struct NO9WIN 
{
	char inuse; 							/* true if window in use */
	int pencolor, rotation, size;
	int winbot,winleft,wintall,winwide;		/* position of the window in virtual space */
};

static struct NO9WIN NO9win[MAXWIN];

static void NO9setup(void )         /* prepare variables for use in other functions */
{
#if defined(MSC) && !defined(__TURBOC__) && !defined(__WATCOMC__)
	FP_SEG(NO9ram) = 0xA000;  FP_OFF(NO9ram) = 0;
	FP_SEG(NO9bank) = 0xC070;  FP_OFF(NO9bank) = 5;
#else
    NO9ram=(char *)MK_FP(0xA000,0);
    NO9bank=(int *)MK_FP(0xC070,5);
#endif
}

static void clrbuf(unsigned int bank)
{
	*NO9bank=bank;
	memset(NO9ram,0,0xFFFF);
	NO9ram[0xFFFF]=0;
}

void RG9gmode(void)         /* go into NO9 graphics mode -- not yet implemented */
{
	clrbuf(0x0000);
	clrbuf(0x00ff);
	clrbuf(0xff00);
	clrbuf(0xffff);
}

void RG9tmode(void)         /* go into NO9 text mode -- not supported */
{ }

/* draw a line from (x0,y0) to (x1,y1) */
/* uses Bresenham's Line Algorithm */
void RG9drawline(int w,int x0,int y0,int x1,int y1)
{
	int x,y,dx,dy,d,temp,
	dx2,dy2,			/* 2dx and 2dy */
	direction;		/* +1 or -1, used for slope */
	char transpose;	/* true if switching x and y for vertical-ish line */

	if(w!=NO9active) 
		return;
	x0=(int)(((long)x0*NO9xmax)>>INXSHIFT);
	y0=NO9ymax-1-(int)(((long)y0*NO9ymax)>>INYSHIFT);
	x1=(int)(((long)x1*NO9xmax)>>INXSHIFT);
	y1=NO9ymax-1-(int)(((long)y1*NO9ymax)>>INYSHIFT);
	if(abs(y1-y0)>abs(x1-x0)) {				/* transpose vertical-ish to horizontal-ish */
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
	if(x1<x0){						/* make sure line is left to right */
		temp=x1; 
		x1=x0; 
		x0=temp; 
		temp=y1; 
		y1=y0; 
		y0=temp;
	  }		
/* SPECIAL CASE: 1 POINT */
	if(x1==x0&&y1==y0) { 
		NO9point(x1,y1);
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
			if(y>=0&&y<NO9xmax&&x>=0&&x<NO9ymax)
				NO9point(y,x);
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
			if(x>=0&&x<NO9xmax&&y>=0&&y<NO9ymax)
				NO9point(x,y);
			while(d>=0) {
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  }
	  }				 /* end horizontalish */
}   /* end NO9rasline() */

/*
	Clear the screen.
*/
void RG9clrscr(int w)
{
	if(w==NO9active) {
		NO9setup();
		RG9gmode();
	  }
}

/*
	Set up a new window; return its number.
	Returns -1 if cannot create window.
*/
int RG9newwin(void)
{
	int w=0;

	while(w<MAXWIN && NO9win[w].inuse) 
		w++;
	if(w==MAXWIN) 
		return(-1); 			/* no windows available */
	NO9win[w].pencolor=7;
	NO9win[w].winbot=0;
	NO9win[w].wintall=3120;
	NO9win[w].winleft=0;
	NO9win[w].winwide=4096;
	NO9win[w].inuse=TRUE;
	return(w);
}

void RG9close(int w)
{
	if(NO9active==w) {
		RG9clrscr(w);
		NO9active=-1;
	  }
	NO9win[w].inuse=FALSE;
}

/*
	set point at real pixel x,y
*/
static void NO9point(int x,int y)
{
	unsigned int bank,loc;

	y+=16; 					/* map 480 lines onto 512 lines */
	if(y&256) 
		bank=0xff00;
	else
		 bank=0;
	if(y&128)
		bank|=0x00ff;
	*NO9bank=bank;		
	loc=((y&0x7f)<<9)+x;
	NO9ram[loc]=255; 		/* only drawing in color 255 */
}

/* set pixel at location (x,y) -- no range checking performed */
void RG9point(int w,int x,int y)
{
	int x2,y2; 				/* on-screen coordinates */

	if(w==NO9active) {
		x2=(int)(((long)x*NO9xmax)>>INXSHIFT);
		y2=SCRNYHI-(int)(((long)y*NO9ymax)>>INYSHIFT);
		NO9point(x2,y2);
	  }
}  

/*
	Do whatever has to be done when the drawing is all done.
	(For printers, that means eject page.)
*/
void RG9pagedone(int w)
{
	/* do nothing for NO9 */
	w=w;
}

/*
	Copy 'count' bytes of data to screen starting at current
	cursor location.
*/
void RG9dataline(int w,char *data,int count)
{
	/* Function not supported yet. */
	w=w;
	data=data;
	count=count;
}

/*
	Change pen color to the specified color.
*/
void RG9pencolor(int w,int color)
{
	/* Function not supported yet. */
	w=w;
	color=color;
}

/*
	Set description of future device-supported graphtext.
	Rotation=quadrant.
*/
void RG9charmode(int w,int rotation,int size)
{
	/* No rotatable device-supported graphtext is available on NO9. */
	w=w;
	rotation=rotation;
	size=size;
}


/* Not yet supported: */
void RG9showcur(void ) {}
void RG9lockcur(void ) {}
void RG9hidecur(void ) {}

/* Ring bell in window w */
void RG9bell(int w)
{
	if(w==NO9active) 
		putchar(7);
}

/* return name of device that this RG supports */
char *RG9devname(void )
{
	return(NO9name);
}

/* initialize all RG9 variables */
void RG9init(void )
{
	int i;
	NO9setup();
	for(i=0; i<MAXWIN; i++) {
		NO9win[i].inuse=FALSE;
	  }
	NO9active=-1;
}

/*
	Make this window visible, hiding all others.
	Caller should follow this with clrscr and redraw to show the current
	contents of the window.
*/
void RG9uncover(int w)
{
	NO9active=w;
}

/* Needed for possible future functionality */
void RG9info(int w,int a,int b,int c,int d,int v)
{
	w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}
