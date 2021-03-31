/*
*
*  rgh.c by Aaron Contorer for NCSA
*
*  graphics routines for drawing on Hercules monochrome card
*  Input coordinate space = 0..4095 by 0..4095
*  MUST BE COMPILED WITH LARGE MEMORY MODEL!
*
*  RGH = routines callable from outside
*  HGC = routines for internal use only
*
*/
#include <stdio.h>	/* used for debugging only */
#include <stdlib.h>
#include <dos.h>	/* used for HGC init call */
#ifdef MSC
#include <conio.h>
#endif
#include "externs.h"

#ifdef __TURBOC__
#define outpw outport
#endif

#define TRUE 1
#define FALSE 0
#define INXMAX 4096
#define INXSHIFT 12
#define INYMAX 4096
#define INYSHIFT 12
#define MAGNIFY 1000
#define SCRNXHI 719
#define SCRNYHI 347
#define MAXHERC 32		/* max. number of Hercules windows */
#define HGCxmax 720		/* max. number of pixels in the x direction */
#define HGCymax 348		/* max. number of pixels in the y direction */

			/* Hercules control */
#define INDXPORT 0x3b4
#define DATAPORT 0x3b5
#define CTRLPORT 0x3b8
#define CONFPORT 0x3bf
#define SCRN_ON 8

#ifdef QAK
#define GRPH 0x02
#define TEXT 0x20
#endif
#define GRPH 0x06
#define TEXT 0x25
			/* graphics */
#ifdef MSC
static unsigned int HGCgtable[12]={ 
	0x35, 0x2d, 0x2e, 0x07,
	0x5b, 0x02, 0x57, 0x57,
	0x02, 0x03, 0x00, 0x00
};
			/* text */
static unsigned int HGCttable[12]={
	0x61, 0x50, 0x52, 0x0f,
	0x19, 0x06, 0x19, 0x19,
	0x02, 0x0d, 0x0b, 0x0c
};

#else
static unsigned char HGCgtable[12]={ 
	0x35, 0x2d, 0x2e, 0x07,
	0x5b, 0x02, 0x57, 0x57,
	0x02, 0x03, 0x00, 0x00
};
			/* text */
static unsigned char HGCttable[12]={
	0x61, 0x50, 0x52, 0x0f,
	0x19, 0x06, 0x19, 0x19,
	0x02, 0x0d, 0x0b, 0x0c
};
#endif

static int HGCactive;		/* number of currently HGCactive window */
static char *HGCram;
static unsigned char HGCpbit[SCRNXHI+1];
#ifdef OLD_WAY
static unsigned char power2[8] = {1,2,4,8,16,32,64,128};
#endif
static char *HGCname = "Hercules High-Resolution Graphics";
static void HGCrasline(int w,int x0,int y0,int x1,int y1);
static void HGCsetup(void );

/* Current status of an HGC window */
struct HGCWIN {
	char inuse; 				/* true if window in use */
	int pencolor, rotation, size;
	int winbot,winleft,wintall,winwide;	/* position of the window in virtual space */
};

static struct HGCWIN HGCwin[MAXHERC];

/* prepare variables for use in other functions */
static void HGCsetup(void ) 
{
	int x;

	HGCpbit[0]=128; 
	HGCpbit[1]=64; 
	HGCpbit[2]=32; 
	HGCpbit[3]=16;
	HGCpbit[4]=8;   
	HGCpbit[5]=4;  
	HGCpbit[6]=2;  
	HGCpbit[7]=1;
	for(x=8; x<=SCRNXHI; x++) 
		HGCpbit[x]=HGCpbit[x&7];
#if defined(MSC) && !defined(__TURBOC__) && !defined(__WATCOMC__)
	FP_SEG(HGCram) = 0xB000;  FP_OFF(HGCram) = 0;
#else
    HGCram=(char *)MK_FP(0xB000,0x0);
#endif
}

/* go into HGC graphics mode */
void RGHgmode(void)
{
#ifdef MSC
 	unsigned int *hdata=HGCgtable;
	unsigned int port;
#else
 	char *hdata=HGCgtable;
	char port;
#endif
	long *video; 		/* long does 4 chars at a time */
	int memloc;
						/* set video chips */
#ifdef MSC
	outpw(CTRLPORT,GRPH|SCRN_ON);
	outpw(CONFPORT,0x01);
	for(port=0; port<12; port++) {
		outpw(INDXPORT,port);
		outpw(DATAPORT,*(hdata++));
	  }	/* end for */
#else
	outp(CTRLPORT,GRPH|SCRN_ON);
	outp(CONFPORT,0x01);
	for(port=0; port<12; port++) {
		outp(INDXPORT,port);
		outp(DATAPORT,*(hdata++));
	  }
#endif
						/* clear video buffer */
	video=(long *)HGCram;
	for(memloc=0; memloc<8191; memloc++)
		*(video++)=0;
}

/* go into HGC text mode, compatible with IBM Monochrome */
void RGHtmode(void)
{
#ifdef MSC
	unsigned int *hdata=HGCttable;
	unsigned port;
#else
	char *hdata=HGCttable;
	char port;
#endif
	long *video; /* long does 4 chars at a time */
	int memloc;
				/* set video chips */
#ifdef MSC
	outpw(CTRLPORT,TEXT|SCRN_ON);
	outpw(CONFPORT,0x01);
	for(port=0; port<12; port++) {
		outpw(INDXPORT,port);
		outpw(DATAPORT,*(hdata++));
	  }
#else
	outp(CTRLPORT,TEXT|SCRN_ON);
	outp(CONFPORT,0x01);
	for(port=0; port<12; port++) {
		outp(INDXPORT,port);
		outp(DATAPORT,*(hdata++));
	  }
#endif

				/* clear video buffer */
	video=(long *)HGCram;
	for(memloc=0; memloc<1000; memloc++)
		*(video++)=0;
	HGCactive=-1;
}

/* draw a line from (x0,y0) to (x1,y1) */
/* uses Bresenham's Line Algorithm */
static void HGCrasline(int w,int x0,int y0,int x1,int y1)
{
	int x,y,dx,dy,d,temp,
	dx2,dy2,		/* 2dx and 2dy */
	direction;		/* +1 or -1, used for slope */
	char transpose;	/* true if switching x and y for vertical-ish line */

	x0=(int)(((long)x0*HGCxmax)>>INXSHIFT);
	y0=HGCymax-1-(int)(((long)y0*HGCymax)>>INYSHIFT);
	x1=(int)(((long)x1*HGCxmax)>>INXSHIFT);
	y1=HGCymax-1-(int)(((long)y1*HGCymax)>>INYSHIFT);
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
	if(x1==x0&&y1==y0) { 
		HGCram[0x2000*(y1%4)+90*(y1/4)+(x1/8)]|=HGCpbit[x1];
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
	if (transpose) {	
							/* CASE OF VERTICALISH (TRANSPOSED) LINES */
		while(x<=x1) {
			if(y>=0&&y<HGCxmax&&x>=0&&x<HGCymax)
				HGCram[0x2000*(x%4)+90*(x/4)+(y/8)]|=HGCpbit[y];
			while(d>=0) {
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  } 
	  } 
	else {
							/* CASE OF HORIZONTALISH LINES */
		while(x<=x1) {
			if(x>=0&&x<HGCxmax&&y>=0&&y<HGCymax)
				HGCram[0x2000*(y%4)+90*(y/4)+(x/8)]|=HGCpbit[x];
			while(d>=0) {
				y+=direction;
				d-=dx2;
			  }
			d+=dy2;
			x++;
		  }
	  }					/* end horizontalish */
	w=w;
} 						/* end HGCrasline() */

/*
	Clear the screen.
*/
void RGHclrscr(int w)
{
	if(w==HGCactive) {
		HGCsetup();
		RGHgmode();
	  }
}

/*
	Set up a new window; return its number.
	Returns -1 if cannot create window.
*/
int RGHnewwin(void)
{
	int w=0;

	while(w<MAXHERC&&HGCwin[w].inuse) 
		w++;
	if(w==MAXHERC)
		return(-1); 				/* no windows available */
	HGCwin[w].pencolor=7;
	HGCwin[w].winbot=0;
	HGCwin[w].wintall=3120;
	HGCwin[w].winleft=0;
	HGCwin[w].winwide=4096;
	HGCwin[w].inuse=TRUE;
	return(w);
}

void RGHclose(int w)
{
	if(HGCactive==w) {
		RGHclrscr(w);
		HGCactive=-1;
	  }
	HGCwin[w].inuse=FALSE;
}

/* set pixel at location (x,y) -- no range checking performed */
void RGHpoint(int w,int x,int y)
{
	int x2,y2; 		/* on-screen coordinates */
	if(w==HGCactive) {
		x2=(int)(((long)x*HGCxmax)>>INXSHIFT);
		y2=SCRNYHI-(int)(((long)y*HGCymax)>>INYSHIFT);
		HGCram[0x2000*(y2%4)+90*(y2/4)+(x2/8)]|=HGCpbit[x2];
	  }
}  

void RGHdrawline(int w,int x0,int y0,int x1,int y1)
{
    if(w==HGCactive)
		HGCrasline(w,x0,y0,x1,y1);
}

/*
	Do whatever has to be done when the drawing is all done.
	(For printers, that means eject page.)
*/
void RGHpagedone(int w)
{
	/* do nothing for HGC */
	w=w;
}

/*
	Copy 'count' bytes of data to screen starting at HGCactive
	cursor location.
*/
void RGHdataline(int w,char *data,int count)
{
	/* Function not supported yet. */
	w=w;
	count=count;
	data=data;
}

/*
	Change pen color to the specified color.
*/
void RGHpencolor(int w,int color)
{
	/* Function not supported yet. */
	w=w;
	color=color;
}

/*
	Set description of future device-supported graphtext.
	Rotation=quadrant.
*/
void RGHcharmode(int w,int rotation,int size)
{
	/* No rotatable device-supported graphtext is available on HGC. */
	w=w;
	rotation=rotation;
	size=size;
}

/* Not yet supported: */
void RGHshowcur(void) {}
void RGHlockcur(void) {}
void RGHhidecur(void) {}

/* Ring bell in window w */
void RGHbell(int w)
{
    if(w==HGCactive)
        putchar(7);
}

/* return name of device that this RG supports */
char *RGHdevname(void)
{
	return(HGCname);
}

/* initialize all RGH variables */
void RGHinit(void)
{
	int i;

	HGCsetup();
	for(i=0; i<MAXHERC; i++)
		HGCwin[i].inuse=FALSE;
	HGCactive=-1;
}

/*
	Make this window visible, hiding all others.
	Caller should follow this with clrscr and redraw to show the HGCactive
	contents of the window.
*/
void RGHuncover(int w)
{
	HGCactive=w;
}

/* Needed for possible future functionality */
void RGHinfo(int w,int a,int b,int c,int d,int v)
{
	w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}
