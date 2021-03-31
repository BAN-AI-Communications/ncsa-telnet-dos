/*
* 	rgnull.c by Aaron Contorer for NCSA
*	routines for "null" device -- calling these routines
*	has no effect, but they are compatible with all RG calls.
*/

#include <stdio.h>
#include "externs.h"

static char *nullname="Null device--do not display output";

int RG0newwin(void)
{
	return(0);
}

void RG0clrscr(int w) {
/* Needed for possible future functionality */
	w=w;	
}

void RG0close(int w) {
/* Needed for possible future functionality */
	w=w;	
}

void RG0point(int w,int x,int y) {
/* Needed for possible future functionality */
	w=w;
	x=x;
	y=y;
}

void RG0drawline(int w,int a,int b,int c,int d) {
/* Needed for possible future functionality */
	w=w;
	a=a;
	b=b;
	c=c;
	d=d;
}

void RG0pagedone(int w) {
/* Needed for possible future functionality */
	w=w;	
}

void RG0dataline(int w,char *data,int count) {
/* Needed for possible future functionality */
	w=w;
	data=data;
	count=count;
}

void RG0pencolor(int w,int color){
/* Needed for possible future functionality */
	w=w;
	color=color;
}

void RG0charmode(int w,int rotation,int size)
{
/* Needed for possible future functionality */
	w=w;
	rotation=rotation;
	size=size;
}

void RG0showcur(void ) {}

void RG0lockcur(void ) {}

void RG0hidecur(void ) {}

void RG0bell(int w) {
/* Needed for possible future functionality */
	w=w;	
}


char *RG0devname(void)
{
	return(nullname);
}

void RG0uncover(int w) {
/* Needed for possible future functionality */
	w=w;	
}

void RG0init(void ) {}

void RG0info(int w,int a,int b,int c,int d,int v) {
/* Needed for possible future functionality */
	w=w;
	a=a;
	b=b;
	c=c;
	d=d;
	v=v;
}

void RG0gmode(void ) {}
void RG0tmode(void ) {}
