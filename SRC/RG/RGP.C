/*
*
*  rgp.c by Aaron Contorer for NCSA
*
* Routines for PostScript output.  Only 1 window output at a time.
*
*/

#include <stdio.h>
#include "externs.h"

#define TRUE 1
#define FALSE 0

static char *psname="PostScript output";
static char busy; 				/* is device already in use */
#ifdef OLD_WAY
static int winbot,winleft,wintall,winwide;	/* position and size of window into virtual space */
#endif
static void (*outfunc)(char *s);		/* the function to call with pointer to strings */
static char pstext[100];		/* the string containing the PostScript output text */
static char PSblank, PSnopath;

static void signore(char *s);
static void psbegin(void );
static void PSenv(void );
static void stroke(void );

static void signore(char *s)
{
	s=s;
}

/*
	Specify the function that is to be called with pointers to all
	the PostScript strings.
*/
void RGPoutfunc(void (*f)(char *))
{
	outfunc=f;
}

static void stroke(void )
{
	if(!PSnopath) 
		(*outfunc)(" S ");
}

static void PSenv(void )			/* set up PostScript environment for new page */
{
						/* Map 4k x 4k graphics onto 11x8 inch paper space,leaving margins and preserving 4x3 aspect ratio. */
	(*outfunc)("533 72 translate\n");
	(*outfunc)("90 rotate\n");
	(*outfunc)("newpath\n 1 setlinewidth\n 0.16 0.12 scale\n");
}

static void psbegin(void )		/* set up PS environment for whole new printout */
{
	(*outfunc)("%! PostScript code from NCSA software\n");
	(*outfunc)("% National Center for Supercomputing Applications\n");
	(*outfunc)("% at the University of Illinois\n\n");
	(*outfunc)("/M { moveto } def\n");
	(*outfunc)("/L { lineto } def\n");
	(*outfunc)("/N { newpath } def\n");
	(*outfunc)("/S { stroke } def\n");
	(*outfunc)("/R { rlineto } def\n");
	(*outfunc)("/H { 0 0 moveto newpath } def\n");
}

int RGPnewwin(void )
{
	if(busy) 
		return(-1);
	else {
		busy=TRUE;
		psbegin();
		PSnopath=TRUE;
		pstext[0]='\0';
		PSblank=TRUE;
		return(0);
	  }
}

void RGPclrscr(int w)
{
	RGPpagedone(w);
}

void RGPclose(int w) 
{
	RGPclrscr(w);
	busy=FALSE;
}

void RGPpoint(int w,int x,int y) 
{
	(*outfunc)("3 0 R -3 0 R\n");
	PSblank=FALSE;
	PSnopath=FALSE;
/* Needed for possible future functionality */

	w=w;
	x=x;
	y=y;
} 

void RGPdrawline(int w,int x0,int y0,int x1,int y1)
{
	stroke();
	if(PSblank) {
		PSenv();
		PSblank=FALSE;
	  }
	sprintf(pstext,"H %d %d M %d %d L\n",x0,y0,x1,y1);
	(*outfunc)(pstext);
	PSnopath=FALSE;
	w=w;
}

void RGPpagedone(int w) 
{
	if(!PSblank) {
		stroke();
		(*outfunc)("showpage\n");
		(*outfunc)("% ++ DONE\n");
	  }
	PSblank=TRUE;
	w=w;
}

/* Needed for possible future functionality */
void RGPdataline(int w,char *data,int count)
{
	w=w;
	data=data;
	count=count;
}

/* Needed for possible future functionality */
void RGPpencolor(int w,int color)
{
	w=w;
	color=color;
}

/* Needed for possible future functionality */
void RGPcharmode(int w,int rotation,int size)
{
	w=w;
	rotation=rotation;
	size=size;
}

void RGPshowcur(void ) {}
void RGPlockcur(void ) {}
void RGPhidecur(void ) {}

/* Needed for possible future functionality */
void RGPbell(int w)
{
	w=w;
}


/* Needed for possible future functionality */
void RGPuncover(int w)
{
    w=w;
}


char *RGPdevname(void ) 
{
	return(psname);
}

void RGPinit(void ) 
{
	busy=FALSE;
	PSblank=TRUE;
	PSnopath=TRUE;
	outfunc=signore;
}

/* Needed for possible future functionality */
void RGPinfo(int w,int a,int b,int c,int d,int v)
{
	w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}

void RGPgmode(void ) {}
void RGPtmode(void ) {}
