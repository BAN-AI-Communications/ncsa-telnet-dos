/*
*
*  rgep.c by Aaron Contorer for NCSA
*
*  graphics routines for drawing on Epson printer
*  Input coordinate space = 0..4095 by 0..4095
*  MUST BE COMPILED WITH LARGE MEMORY MODEL!
*
*/

#include <stdio.h>	/* used for debugging only */
#include <stdlib.h>
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#include "externs.h"

#define TRUE 1
#define FALSE 0
#define INXMAX 4096
#define INYMAX 4096
#define SCRNXHI 719
#define ROWS 90
#define SCRNYHI 929
#define LINEBYTE 930
#define MAXRW 1		/* max. number of real windows */

static int EPSactive; /* number of currently active window */
static unsigned char EPSpbit[SCRNXHI+1];
#ifdef OLD_WAY
static unsigned char power2[8]={1,2,4,8,16,32,64,128};
#endif
static char *EPSname="Epson, IBM, or compatible printer";
static int EPSxmax=720;
static int EPSymax=LINEBYTE;
#ifdef OLD_WAY
static int EPSbytes=80;	/* number of bytes per line */
#endif
static unsigned char *EPSram[ROWS];
static void (*EPSoutfunc)(char c);
static void signore(char c);
static void EPSpoint(int x,int y);
static void EPSdump(void );
static void EPSsetup(void );

/* Current status of an EPS window */
struct EPSWIN 
{
	char inuse; /* true if window in use */
	int pencolor, rotation, size;
	int winbot,winleft,wintall,winwide;
		/* position of the window in virtual space */
};

static struct EPSWIN EPSwins[MAXRW];

/* prepare variables for use in other functions */
static void EPSsetup(void )
{
	int x;
	EPSpbit[0]=128; 
	EPSpbit[1]=64; 
	EPSpbit[2]=32; 
	EPSpbit[3]=16;
	EPSpbit[4]=8;   
	EPSpbit[5]=4;  
	EPSpbit[6]=2;  
	EPSpbit[7]=1;
	for(x=8; x<=SCRNXHI; x++) 
		EPSpbit[x]=EPSpbit[x&7];
}

/* go into EPS graphics mode */
void RGEPgmode(void )
{
}

/* go into EPS text mode */
void RGEPtmode(void )
{
}

/*
	Clear the screen.
*/
void RGEPclrscr(int w)
{
	if(w==EPSactive)
		EPSsetup();
}

/*
	Set up a new window; return its number.
	Returns -1 if cannot create window.
*/
int RGEPnewwin(void )
{
	int w=0;
	int x,y;

	for(x=0; x<ROWS; x++) {
		if((EPSram[x]=malloc(LINEBYTE))==NULL) {	/* ran out of memory, free what we have now */
			for(; x>=0; x--)
				free(EPSram[x]);
			return(-1);
		  }	/* end if */
		for(y=0; y<LINEBYTE; y++) 
			(EPSram[x])[y]=0;
	  }
	while(w<MAXRW&&EPSwins[w].inuse) 
		w++;
	if(w==MAXRW) 
		return(-1); /* no windows available */
	EPSwins[w].pencolor=7;
	EPSwins[w].winbot=0;
	EPSwins[w].wintall=3120;
	EPSwins[w].winleft=0;
	EPSwins[w].winwide=4096;
	EPSwins[w].inuse=TRUE;
	return(w);
}

void RGEPclose(int w)
{
	if(EPSactive==w) {
		RGEPclrscr(w);
		EPSactive=-1;
	  }
	EPSdump();
	EPSwins[w].inuse=FALSE;
}

/* set pixel at location (x,y) -- no range checking performed */
void RGEPpoint(int w,int x,int y)
{
	int x2,y2; 			/* on-screen coordinates */

	if(w==EPSactive) {
		x2=(int)((long)x*EPSxmax/INXMAX);
		y2=(int)((long)y*EPSymax/INYMAX);
		EPSpoint(x2,y2);
	  }
}  

/*
	Do whatever has to be done when the drawing is all done.
	(For printers, that means eject page.)
*/
void RGEPpagedone(int w)
{
	EPSdump();
	w=w;
}

/*
	Copy 'count' bytes of data to screen starting at current
	cursor location.
*/
void RGEPdataline(int w,char *data,int count)
{
	/* Function not supported yet. */
	w=w;
	data=data;
	count=count;
}

/*
	Change pen color to the specified color.
*/
void RGEPpencolor(int w,int color)
{
	/* Function not supported. */
	w=w;
	color=color;
}

/*
	Set description of future device-supported graphtext.
	Rotation=quadrant.
*/
void RGEPcharmode(int w,int rotation,int size)
{
	/* No rotatable device-supported graphtext is available on EPS. */
	w=w;
	rotation=rotation;
	size=size;
}

/* Not yet supported: */
void RGEPshowcur(void) {}
void RGEPlockcur(void) {}
void RGEPhidecur(void) {}

/*
	Set bit at x,y
*/
static void EPSpoint(int x,int y)
{
	(EPSram[x>>3])[y]|=EPSpbit[x];
}

/* draw a line from (x0,y0) to (x1,y1) */
/* uses Bresenham's Line Algorithm */
void RGEPdrawline(int w,int x0,int y0,int x1,int y1)
{
	int x,y,dx,dy,d,temp,
	dx2,dy2,		/* 2dx and 2dy */
	direction;		/* +1 or -1, used for slope */
	char transpose;	/* true if switching x and y for vertical-ish line */

	if(w!=EPSactive) 
		return;
	x0=(int)((long)x0*EPSxmax/INXMAX);
	y0=(int)((long)y0*EPSymax/INYMAX);
	x1=(int)((long)x1*EPSxmax/INXMAX);
	y1=(int)((long)y1*EPSymax/INYMAX);

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
		transpose=FALSE;			/* make sure line is left to right */
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
		EPSpoint(x1,y1);
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
			if(y>=0&&y<EPSxmax&&x>=0&&x<EPSymax)
				EPSpoint(y,x);
			while(d>=0) {
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  } 
	  } 
	else {				/* CASE OF HORIZONTALISH LINES */
		while(x<=x1) {
			if(x>=0&&x<EPSxmax&&y>=0&&y<EPSymax)
				EPSpoint(x,y);
			while(d>=0){
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  }
	  } 				/* end horizontalish */
}   /* end RGEPdrawline() */

/* Ring bell in window w */
void RGEPbell(int w)
{
	w=w;
}

/* return name of device that this RG supports */
char *RGEPdevname(void )
{
	return(EPSname);
}

/* Ignore the character pointer passed here. */
static void signore(char s)
{
	s=s;
}

/* initialize all RGEP variables */
void RGEPinit(void )
{
	int i;
	EPSsetup();
	for(i=0; i<MAXRW; i++) 
		EPSwins[i].inuse=FALSE;
	EPSactive=-1;
	EPSoutfunc=signore;
}

/*
	Specify the function that is to be called with pointers to all
	the printout strings.
*/
void RGEPoutfunc(void (*f)(char ))
{
	EPSoutfunc=f;
}

/*
	Make this window visible, hiding all others.
	Caller should follow this with clrscr and redraw to show the current
	contents of the window.
*/
void RGEPuncover(int w)
{
	EPSactive=w;
}

/* Needed for possible future functionality */
void RGEPinfo(int w,int a,int b,int c,int d,int v)
{
    w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}

/*
*	Dump the contents of the buffer to the specified function.
*/
static void EPSdump(void )
{
	int x,y;
	int size=EPSymax*2;

	for(x=0;x<90;x++) {				/* init graphics mode */
		(*EPSoutfunc)(033);
		(*EPSoutfunc)('L');
		(*EPSoutfunc)((char)(size&255));
		(*EPSoutfunc)((char)(size>>8));		/* send a line of bit image data */
		for(y=0; y<size; y++) 
			(*EPSoutfunc)((EPSram[x]) [y]);
									/* go to next line */
		(*EPSoutfunc)(13);
		(*EPSoutfunc)(033);
		(*EPSoutfunc)('J');
		(*EPSoutfunc)(24);
	  }
	(*EPSoutfunc)(12); /* form feed */
}
