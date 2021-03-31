/*
*
*  rghp.c by Aaron Contorer for NCSA
*
*  Routines for HP-GL plotter output.  Only 1 window output at a time.
*
*/

#include <stdio.h>
#include "externs.h"

#define TRUE 1
#define FALSE 0

static char *HPname = "Hewlett-Packard HP-GL plotter";
static char busy; /* is device already in use */
#ifdef OLD_WAY
static int winbot,winleft,wintall,winwide;	/* position and size of window into virtual space */
#endif
static void (*outfunc)(char *s);	/* the function to call with pointer to strings */
static char HPtext[100];	/* the string containing the HP-GL output text */
static int HPpenx,HPpeny;
static int HPblank;
static int HPcolor;

static void HPbegin(void );
static void signore(char *s);

static void signore(char *s)
{
	s=s;
}

/*
	Specify the function that is to be called with pointers to all
	the HP-GL strings.
*/
void RGHPoutfunc(void (*f)(char *))
{
	outfunc=f;
}

/* set up environment for whole new printout */
static void HPbegin(void)
{
	(*outfunc)("IN;SP1;SC-50,4370,-100,4120;PU0,0;");
	HPpenx=HPpeny=0;
}

int RGHPnewwin(void)
{
	if(busy) 
		return(-1);
	HPtext[0]='\0';
	HPpenx=HPpeny=0;
	HPblank=TRUE;
	HPcolor=100;
	return(0);
}

void RGHPclrscr(int w)
{
	RGHPpagedone(w);
}

void RGHPclose(int w) 
{
	RGHPclrscr(w);
	busy=FALSE;
}

void RGHPpoint(int w,int x,int y) 
{
	(*outfunc)("PD;PU;");
	w=w;
	x=x;
	y=y;
} 

void RGHPdrawline(int w,int x0,int y0,int x1,int y1)
{
	w=w;
	if(HPblank) {
		HPbegin();
		HPblank=FALSE;
	  }
	if(x0!=HPpenx||y0!=HPpeny) {		/* only move pen if not already there */
		sprintf(HPtext,"PU%d,%d;",x0, y0);
		(*outfunc)(HPtext);
	  }
	sprintf(HPtext,"PD%d,%d;",x1, y1);
	(*outfunc)(HPtext);
	HPpenx=x1;
	HPpeny=y1;
}

void RGHPpagedone(int w) 
{
	(*outfunc)("PG;");
	HPblank=TRUE;
	w=w;
}

/* Needed for possible future functionality */
void RGHPdataline(int w,char *data,int count)
{
	w=w;
	data=data;
	count=count;
}


void RGHPpencolor(int w,int color) 
{
	color&=7;
	if(color) {
		sprintf(HPtext,"SP%d;",color);
		(*outfunc)(HPtext);
	  }
	w=w;
}

/* Needed for possible future functionality */
void RGHPcharmode(int w,int rotation,int size)
{
    w=w;
	rotation=rotation;
	size=size;
}

void RGHPshowcur(void) {}
void RGHPlockcur(void) {}
void RGHPhidecur(void) {}

/* Needed for possible future functionality */
void RGHPbell(int w)
{
    w=w;
}

/* Needed for possible future functionality */
void RGHPuncover(int w)
{
    w=w;
}


char *RGHPdevname(void)
{
	return(HPname);
}

void RGHPinit(void)
{
	busy=FALSE;
	outfunc=signore;
}

/* Needed for possible future functionality */
void RGHPinfo(int w,int a,int b,int c,int d,int v)
{
	w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}

void RGHPgmode(void) {}
void RGHPtmode(void) {}
