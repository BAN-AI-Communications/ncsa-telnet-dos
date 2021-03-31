/*
 *
 *	  Virtual Screen Kernel Internal Routines
 *					  (vsintern.c)
 *  National Center for Supercomputing Applications
 *
 *	  by Gaige B. Paulsen
 *
 *	This file contains the private internal calls for the NCSA
 *  Virtual Screen Kernel.
 *
 *		Version Date	Notes
 *		------- ------  ---------------------------------------------------
 *		0.01	861102  Initial coding -GBP
 *		0.50	861113  First compiled edition -GBP
 *		0.70	861114  Internal operation confirmed -GBP
 *		2.1		871130	NCSA Telnet 2.1 -GBP
 *		2.2 	880715	NCSA Telnet 2.2 -GBP
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef MSC
#include <malloc.h>
#endif
#include "vsdata.h"
#include "vskeys.h"
#include "externs.h"

#define AL(x) VSIw->attrst[x]
#define VL(x) VSIw->linest[x]
#define vtp VSIw->top
#define btm VSIw->bottom
#define VSIclrattrib 0
#define Rrt VSIw->Rright
#define Rlt VSIw->Rleft
#define Rtp VSIw->Rtop
#define Rbm VSIw->Rbottom

/* clip a requested area of the screen to the actuall virtual screen window */
int VSIclip(int *x1,int *y1,int *x2,int *y2,int *n,int *offset)
{
	if(*n>=0) { 	/* set the requested right edge of the window */
		*x2=*x1+*n-1; 
		*y2=*y1; 
	  }	/* end if */
	if((*x1>Rrt) || (*y2<Rtp))		/* check for invalid invalid left and bottom edges */
		return(-1);
	if(*x2>Rrt)		/* clip the right edge to the right edge of the virtual screen */
		*x2=Rrt;
	if(*y2>Rbm)		/* clip the bottom edge to the bottom edge of the virtual screen */
		*y2=Rbm;

	*x1-=Rlt; 		/* adjust to represent actual screen coor. */
	*x2-=Rlt;
	*y1-=Rtp; 
	*y2-=Rtp;

	*offset=-*x1; 
	if(*offset<0)
		*offset=0;
	if(*x1<0)		/* clip left edge to top of virtual screen */
		*x1=0;
	if(*y1<0)		/* clip top edge to top of virtual screen */
		*y1=0;
	*n=*x2-*x1+1;	/* set the width of the clipped region */
	if((*n<=0) || (*y2-*y1<0)) /* check whether the clipped region really exists */
		return(-1);
	return(0);
}

int VSIcdellines(int w,int top,int bottom,int n,int scrolled)
{
	int x1=0,
		x2=VSIw->maxwidth,
		tn=(-1),
		offset;

	if(VSIclip(&x1,&top,&x2,&bottom,&tn,&offset))
		return(-1);
	tn=bottom-top;
	if(tn<n) 
		n=tn;
	RSdellines(w,top,bottom,n,scrolled);
	return(0);				/* I delete the whole thing! */
}

int VSIcinslines(int w,int top,int bottom,int n,int scrolled)
{
	int x1=0,
		x2=VSIw->maxwidth,
		tn=(-1),
		offset;

	if(VSIclip(&x1,&top,&x2,&bottom,&tn,&offset))
		return(-1);
	tn=bottom-top;
	if(tn<n) 
		n=tn;
	RSinslines(w,top,bottom,n,scrolled);
	return(0);
}

void VSIcurson(int w,int x,int y,int ForceMove)
{
	int 
#ifdef OLD_WAY
		ox=x,oy=y,
#endif
		x2,y2,
		n=1,
		offset;

	if(!VSIclip(&x,&y,&x2,&y2,&n,&offset)) 
		RScurson(w,x,y);
	else 
		if(ForceMove) {
			x2=Rbm-Rtp;
			if(x2>=VSPBOTTOM)
				VSsetrgn(VSIwn,Rlt,VSPBOTTOM-x2,Rrt,VSPBOTTOM);
			else {
				if(y>0) 
					VSscrolforward(VSIwn,y);
				else	 
					VSscrolback(VSIwn,-y);
			  }	/* end else */
/* Restore from previous */
#ifdef OLD_WAY
			x=ox; 
			y=oy;
			n=1;
			if(!VSIclip(&x,&y,&x2,&y2,&n,&offset)) 
				RScurson(w,x,y);
#endif
		  }	/* end if */
}

void VSIcuroff(int w)
{
	int x=VSIw->x,
		y=VSIw->y,
		x2,y2,
		n=1,
		offset;

	if(!VSIclip(&x,&y,&x2,&y2,&n,&offset)) 
		RScursoff(w);
}

VSline *VSInewline(void )
{
	VSline *t2;
	char *t;

    EXCHECK((t=malloc(VSIw->allwidth+1))==NULL,AllocLineFailed);
    EXCHECK((t2=(VSline *)malloc(sizeof(VSline)))==NULL,AllocVSFailed);

    t2->text=t;
	t2->next=t2->prev=NULL;
	return(t2);

AllocVSFailed:      /* couldn't allocate storage for the line information */
    free(t);
AllocLineFailed:    /* couldn't allocate storage for the line */
    return(NULL);
}

void VSIlistndx(VSline *ts,VSline *as)
{
	int i;

	for(i=0; i<=VSPBOTTOM; i++) {
		AL(i)=as;
		VL(i)=ts;
		ts=ts->next;
		as=as->next;
	  }	/* end for */
}

void VSIscroff(void)
{
	VSline *tmp;
	int i;

	if(VL(VSPBOTTOM)->next !=NULL) {
		for(i=0; i<=VSPBOTTOM; i++) {
			if(VL(VSPBOTTOM)->next==VSIw->buftop)
				VSIw->buftop=VSIw->buftop->next;	/* if we are in old terr. loop */
			VSIw->scrntop=VSIw->scrntop->next;
			VSIlistndx(VSIw->scrntop,AL(1));
		  }
	  }
	else {
		for(i=0; i<=VSPBOTTOM; i++) {
			if((VSIw->numlines<VSIw->maxlines)&&(tmp=VSInewline()) !=NULL ) {
					VL(VSPBOTTOM)->next=tmp;			/* ADD A LINE */
					tmp->prev=VL(VSPBOTTOM);
					VSIw->numlines++;
			  }
			else {
				if(VL(VSPBOTTOM)->next==NULL) {
					VL(VSPBOTTOM)->next=VSIw->buftop;	/* Make it circular */
					VSIw->buftop->prev=VL(VSPBOTTOM);
				  }
				if(VL(VSPBOTTOM)->next==VSIw->buftop)
					VSIw->buftop=VSIw->buftop->next;/* if we are in old terr. loop*/
			  }
			VSIw->scrntop=VSIw->scrntop->next;
			VSIlistndx(VSIw->scrntop,AL(1));
		  }
		RSbufinfo(VSIwn,VSIw->numlines,VSIw->Rtop,VSIw->Rbottom);
      } /* end else */
}

void VSIelo(int s)
{
	char *tt,*ta;
	int i;

/*	VSIwrapnow(&i,&j); */
	if(s<0) 
		s=VSIw->y;
	ta=&AL(s)->text[0];
	tt=&VL(s)->text[0];
	for(i=0; i<=VSIw->allwidth; i++) {
		*ta++=VSIclrattrib;
		*tt++=' ';
	  }	/* end for */
}

void VSIes(void)
{
	int i;
	int x1=0,y1=0,
		x2=VSIw->maxwidth,y2=VSPBOTTOM,
		n=(-1),
		offset;

	if(VSIw->ESscroll)
		VSIscroff();
	for(i=0; i<=VSPBOTTOM; i++)
		VSIelo(i);
	if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset)) 
		RSerase(VSIwn,x1,y1,x2,y2);
	VSIw->vistop=VSIw->scrntop;
}

void VSItabclear(void)
{
	int x=0;

	while(x<=VSIw->allwidth) {
		VSIw->tabs[x]=' ';
		x++;
	  }	/* end while */
}

void VSItabinit(void)
{
	int x=0;

	VSItabclear();

	while(x<=VSIw->allwidth) { 
		VSIw->tabs[x]='x';
		x+=8;
	  }	/* end while */
	VSIw->tabs[VSIw->allwidth]='x';
}

void VSIreset(void)
{
	VSIw->top=0;
	VSIw->bottom=VSPBOTTOM;
	VSIw->parmptr=0;
	VSIw->escflg=0;
	VSIw->DECAWM=0;
	VSIw->DECCKM=0;
	VSIw->DECPAM=0;
/* 	VSIw->DECORG=0;		*/
/* 	VSIw->Pattrib=-1;	*/
	VSIw->IRM=0;
	VSIw->attrib=0;
	VSIw->x=0;
	VSIw->y=0;
	VSIw->charset=0;
	VSIes();
	VSItabinit();
	set_vtwrap(VSIwn,VSIw->DECAWM);		/* QAK - 7/27/90: added because resetting the virtual screen's wrapping flag doesn't reset telnet window's wrapping */
}

void VSIlistmove(VSline *TD,VSline *BD,VSline *TI,VSline *BI)
{
	if(TD->prev!=NULL) 
		TD->prev->next=BD->next;	/* Maintain circularity */
	if(BD->next!=NULL) 
		BD->next->prev=TD->prev;
	TD->prev=TI;					/* Place the node in its new home */
	BD->next=BI;
	if(TI!=NULL) 
		TI->next=TD;				/* Ditto prev->prev */
	if(BI!=NULL) 
		BI->prev=BD;
}

void VSIdellines(int n,int s)
{
	int i,j,
		attop=(VSIw->vistop==VSIw->scrntop);
	char *ta,*tt;
	VSline *as,*ts,
		*TD,*BD,
		*TI,*BI,
		*itt,*ita;

	if(s<0) 
		s=VSIw->y;
	if(s+n-1>VSIw->bottom) 
		n=VSIw->bottom -s +1;
	ts=VL(0)->prev;
	TD=VL(s);
	BD=VL(s+n-1);
	TI=VL(VSIw->bottom);
	BI=TI->next;
	itt=TD;
	if(TI!=BD)
		VSIlistmove(TD,BD,TI,BI);
	if(s==0 || n>VSPBOTTOM) 
		as=AL(n);
	else 
		as=AL(0);
	TD=AL(s);
	BD=AL(s+n-1);
	TI=AL(VSIw->bottom);
	BI=TI->next;
	if(TD!=BI&&TI!=BD)
		VSIlistmove(TD,BD,TI,BI);
	ita=TD;
	for(i=0; i<n; i++) {
		ta=ita->text;
		tt=itt->text;
		for(j=0; j<=VSIw->allwidth; j++) {
			*tt++=' ';
			*ta++=VSIclrattrib;
		  }	/* end for */
		ita=ita->next;
		itt=itt->next;
	  }	/* end for */
	VSIw->scrntop=ts->next;
	if(attop) 
		VSIw->vistop=VSIw->scrntop;
	VSIlistndx(ts->next,as);
	VSIcdellines(VSIwn,s,VSIw->bottom,n,-1);		/* Destroy selection area if this is called */
}

void VSIinslines(int n,int s)
{
	int i,j,
		attop=(VSIw->vistop==VSIw->scrntop);
	char *ta,*tt;
	VSline *as,*ts,
		*TD,*BD,
		*TI,*BI,
		*itt,*ita;

	if(s<0) 
		s=VSIw->y;
	if(s+n-1>VSIw->bottom) 
		n=VSIw->bottom -s +1;
	ts=VL(0)->prev;
	BI=VL(s);
	TI=BI->prev;
	TD=VL(VSIw->bottom-n+1);
	BD=VL(VSIw->bottom);
	itt=TD;
	if(TD!=BI) 
		VSIlistmove(TD,BD,TI,BI);
	if(s==0 || n>VSPBOTTOM) 
		as=AL(VSIw->bottom-n+1);
	else 
		as=AL(0);
	BI=AL(s);
	TI=BI->prev;
	TD=AL(VSIw->bottom-n+1);
	BD=AL(VSIw->bottom);
	if(TD!=BI && TI!=BD)
		VSIlistmove(TD,BD,TI,BI);
	ita=TD;
	for(i=0; i<n; i++) {
		tt=itt->text;
		ta=ita->text;
		for(j=0; j<=VSIw->allwidth; j++) {
			*tt++=' ';
			*ta++=VSIclrattrib;
		  }	/* end for */
		itt=itt->next;
		ita=ita->next;
	  }	/* end for */
	VSIw->scrntop=ts->next;
	if(attop) 
		VSIw->vistop=VSIw->scrntop;
	VSIlistndx(ts->next,as);
	VSIcinslines(VSIwn,s,VSIw->bottom,n,-1);  /* Destroy selection area if this is called tooo */
}

/* scroll the virtual screen up one line */
void VSIscroll(void)
{
	char *temp,*tempa;
	VSline *tmp;
	int i;
	int tx1=0,ty1=0,
		tx2,ty2,
		tn=132,
		offset;

	if(!VSIclip(&tx1,&ty1,&tx2,&ty2,&tn,&offset))
		RSdrawsep(VSIwn,ty1,1);					/* Draw Separator */
	if((!VSIw->savelines) || (VSIw->top!=0) || (VSIw->bottom!=VSPBOTTOM))	/* if we are not using scroll back, or still in the middle of the screen, delete the line which scroll off the top */
		VSIdellines(1,VSIw->top);
	else {
		if((VL(VSPBOTTOM)->next==NULL) && (VSIw->numlines<VSIw->maxlines) && (tmp=VSInewline())!=NULL) {
			VL(VSPBOTTOM)->next=tmp;			/* ADD A LINE to the scroll back list */
			tmp->prev=VL(btm);
			VSIw->numlines++;
			RSbufinfo(VSIwn,VSIw->numlines,VSIw->Rtop,VSIw->Rbottom);
		  }	/* end if */
		else {
			if(VL(btm)->next==NULL) {
				VL(btm)->next=VSIw->buftop;	/* Make it circular */
				VSIw->buftop->prev=VL(btm);
			  }	/* end if */
			if(VL(btm)->next==VSIw->buftop)
				VSIw->buftop=VSIw->buftop->next;	/* if we are in old terr. loop*/
		  }	/* end else */
#ifdef OLD_WAY
		attop=(VSIw->vistop==VSIw->scrntop);		/* set this when scrollback isn't possible yet */
#endif
		VSIw->scrntop=VSIw->scrntop->next;		/* move the top of the screen down one position */
		VSIlistndx(VSIw->scrntop,AL(1));
		if(VSIcdellines(VSIwn,VSIw->Rtop,VSIw->Rbottom,1,1)) {		/* scroll real screen up, but Dont destroy select */
				/* If we did not show on screen then we must do this */
			if(VSIw->Rtop>-VSIw->numlines) {	/* If we are not at the top..... */
				VSIw->Rtop--;					/* Then we should move down  */
				VSIw->Rbottom--;
			  }
			else {								/* But if we're at the top.... */
												/* The region remains the same .. */
				VSIw->vistop=VSIw->vistop->next;/* and move the vistop as */
				RSdellines(VSIwn,0,Rbm-Rtp,1,1);/* we also delete the top line */
			  }								/* well.... (but don't destroy selection)  */
		  }
		else {
			VSIw->vistop=VSIw->vistop->next;
		  }
		tempa=AL(VSPBOTTOM)->text;
		temp=VL(VSPBOTTOM)->text;
		for(i=0;i<=VSIw->allwidth; i++) {
			temp[i]=' ';
			tempa[i]=0;
		  }
	  }	/* end else */
	tx1=ty1=0;
	tn=132;
	if(!VSIclip(&tx1,&ty1,&tx2,&ty2,&tn,&offset))
		RSdrawsep(VSIwn,ty1,1);					/* Draw Separator */
}

/* increment the y position we are on in the virtual screen, and check whether
 *	that means we need to scroll
*/
void VSIindex(void)
{
#ifdef OLD_WAY
	if(++VSIw->y > VSIw->bottom) {
		VSIw->y=VSIw->bottom;
		VSIscroll();
	  }	/* end if */
#endif
 	if(VSIw->y==VSIw->bottom)
 		VSIscroll();
 	else
 		VSIw->y++;
}

void VSIwrapnow(int *xp,int *yp)
{
	if(VSIw->x > VSIw->maxwidth) {
		VSIw->x=0;
		VSIindex();
	  }	/* end if */
	*xp=VSIw->x;
	*yp=VSIw->y;
}

void VSIeeol(void)
{
	char *tt,*ta;
	int x1=VSIw->x,y1=VSIw->y,
		x2=VSIw->maxwidth,y2=VSIw->y,
		n=(-1),
		offset;
    int i;

	VSIwrapnow(&x1,&y1);
	y2=y1;
	ta=&AL(y1)->text[x1];
	tt=&VL(y1)->text[x1];
	for(i=VSIw->allwidth-x1+1; i>0; i--) {
		*ta++=VSIclrattrib;
		*tt++=' ';
	  }	/* end for */
	if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset)) 
		RSerase(VSIwn,x1,y1,x2,y2);
}

void VSIdelchars(int x)
{
	int i;
	int x1=VSIw->x,y1=VSIw->y,
		x2=VSIw->maxwidth,y2=VSIw->y,
		n=(-1),
		offset;
	char *tempa,*temp;

	VSIwrapnow(&x1,&y1);
	y2=y1;
	if(x>VSIw->maxwidth) 
		x=VSIw->maxwidth;
	tempa=VSIw->attrst[y1]->text;
	temp=VSIw->linest[y1]->text;
	for(i=x1; i<=VSIw->maxwidth-x; i++) {
		temp[i]=temp[x+i];
		tempa[i]=tempa[x+i];
	  }	/* end for */
	for(i=VSIw->maxwidth-x+1; i<=VSIw->allwidth; i++) {
		temp[i]=' ';
		tempa[i]=VSIclrattrib;
	  }	/* end for */
	if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset))
		if(VSIw->VSIDC)
			RSdelchars(VSIwn,x1,y1,x);
		else
			RSdraw(VSIwn,x1,y1,VSIw->attrib,n,&VSIw->linest[y1]->text[x1]);
}

void VSIrindex(void)
{
#ifdef OLD_WAY
	if(--VSIw->y < VSIw->top ) {
		VSIw->y=VSIw->top;
		VSIinslines(1,VSIw->top);
	  }	/* end if */
#endif
	if(VSIw->y==VSIw->top)
		VSIinslines(1,VSIw->top);
	else
		VSIw->y--;
}

void VSIebol(void)
{
	char *tt,*ta;
	int x1=0,y1=VSIw->y,
		x2=VSIw->x,y2=VSIw->y,
		n=(-1),
		offset;
	register int i;

	VSIwrapnow(&x2,&y1);
	y2=y1;
	ta=&AL(y1)->text[0];
	tt=&VL(y1)->text[0];
	for(i=0;i<=x2; i++) {
		*ta++=VSIclrattrib;
		*tt++=' ';
	  }	/* end for */
	if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset)) 
		RSerase(VSIwn,x1,y1,x2,y2);
}

void VSIel(int s)
{
	char *tt,*ta;
	int x1=0,y1=s,
		x2=VSIw->maxwidth,y2=s,
		n=(-1),
		offset;
	register int i;

	if(s<0) {
		VSIwrapnow(&x1,&y1);
		s=y2=y1;
		x1=0;
	  }	/* end if */
	ta=&AL(s)->text[0];
	tt=&VL(s)->text[0];
	for(i=0; i<=VSIw->allwidth; i++) {
		*ta++=VSIclrattrib;
		*tt++=' ';
	  }	/* end for */
	if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset)) 
		RSerase(VSIwn,x1,y1,x2,y2);
}

void VSIeeos(void)
{
	register int i;
	int x1=0,y1=VSIw->y+1,
		x2=VSIw->maxwidth,y2=VSPBOTTOM,
		n=(-1),
		offset;

	VSIwrapnow(&x1,&y1);
	y1++;
	x1=0;
	i=y1;
	if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset)) 
		RSerase(VSIwn,x1,y1,x2,y2);
	VSIeeol();
	while(i<=VSPBOTTOM) {
		VSIelo(i);
		i++;
	  }	/* end while */
	if(VSIw->y < VSPBOTTOM && (VSIw->x <= VSIw->maxwidth))
		if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset)) 
			RSerase(VSIwn,x1,y1,x2,y2);
}

void VSIebos(void)
{
	register int i;
	int x1,y1,
		x2=VSIw->maxwidth,y2,
		n=(-1),
		offset;

	VSIwrapnow(&x1,&y1);
	y2=y1-1;
	x1=0;
	y1=0;
	VSIebol();
	i=0;
	while(i<(y2+1))	{			/* Equiv of VSIw->y */
		VSIelo(i);
		i++;
	  }	/* end while */
	if(y2>=0) 						/* Equiv of VSIw->y>0 */
		if(!VSIclip(&x1,&y1,&x2,&y2,&n,&offset)) 
			RSerase(VSIwn,x1,y1,x2,y2);
}

void VSIrange(void)				  /* check and resolve range errors on x and y */
{
	int wrap=0;

	if(VSIw->DECAWM)
		wrap=1;
	if(VSIw->x<0)
		VSIw->x=0;
	if(VSIw->x>(VSIw->maxwidth+wrap))
		VSIw->x=VSIw->maxwidth+wrap;
	if(VSIw->y<0)
		VSIw->y=0;
	if(VSIw->y>VSPBOTTOM)
		VSIw->y=VSPBOTTOM;
}

void VTsendpos(void)		/* send cursor position to host */
{
	char tempbuf[15];
	int x=VSIw->x,
		y=VSIw->y;

	if(x > VSIw->maxwidth) {
		x=0;
		y++;
	  }	/* end if */
	if(y>VSPBOTTOM) 
		y=VSPBOTTOM;
	sprintf(tempbuf,"\033[%d;%dR",y+1,x+1);
	RSsendstring(VSIwn,tempbuf,strlen(tempbuf));
}

void VTsendstat(void)		/* send OK status back to host */
{
	RSsendstring(VSIwn,"\033[0n",4);
}

void VTsendident(void)		/* send terminal capabilities to host */
{
    RSsendstring(VSIwn,"\033[?6c",5);       /* GPO & AVO options supported */
}

void VTalign(void)	/* vt100 alignment, fill screen with 'E's */
{
	char *tt;
	register int i,j;

	VSIes();		/* erase the screen */

	for(j=0; j<VSPBOTTOM; j++) {
		tt=&VL(j)->text[0];
		for(i=0; i<=VSIw->maxwidth; i++)
			*tt++='E';
	  }	/* end for */
	VSredraw(VSIwn,0,0,(VSIw->Rright-VSIw->Rleft),(VSIw->Rbottom-VSIw->Rtop));
}

/* reset all the ANSI parameters back to the default state */
void VSIapclear(void)
{
	for(VSIw->parmptr=5; VSIw->parmptr>=0; VSIw->parmptr--)
		VSIw->parms[VSIw->parmptr]=-1;
	VSIw->parmptr=0;
}

void VSIsetoption(int toggle)
{
	int WindWidth=(VSIw->Rright-VSIw->Rleft);

	switch(VSIw->parms[0]) {
		case -2:
			switch(VSIw->parms[1]) {
				case 1:		/* set/reset cursor key mode */
					VSIw->DECCKM=toggle;
					break;

#ifdef NOT_SUPPORTED
				case 2:		/* set/reset ANSI/vt52 mode */
					break;
#endif

				case 3:		/* set/reset column mode */
                    VSIw->x=VSIw->y=0;      /* Clear the screen, mama! */
					VSIes();
					if(toggle)		/* 132 column mode */
						VSIw->maxwidth=VSIw->allwidth;
					else
						VSIw->maxwidth=79;
					RSmargininfo(VSIwn,VSIw->maxwidth-WindWidth,VSIw->Rleft);
					break;

#ifdef NOT_SUPPORTED
				case 4:		/* set/reset scrolling mode */
				case 5:		/* set/reset screen mode */
				case 6:		/* set/rest origin mode */
					VSIw->DECORG = toggle;
					break;
#endif

				case 7:		/* set/reset wrap mode */
					VSIw->DECAWM=toggle;
					set_vtwrap(VSIwn,VSIw->DECAWM);		/* QAK - 7/27/90: added because resetting the virtual screen's wrapping flag doesn't reset telnet window's wrapping */
					break;

#ifdef NOT_SUPPORTED
				case 8:		/* set/reset autorepeat mode */
				case 9:		/* set/reset interlace mode */
					break;
#endif

				default:
                    break;
			  }	/* end switch */
			break;

		case 4:
			VSIw->IRM=toggle;
			break;

		default:
			break;

	  }	/* end switch */
}

void VSItab(void)
{
	if(VSIw->x>=VSIw->maxwidth)
		VSIw->x=VSIw->maxwidth;
	VSIw->x++;
	while((VSIw->tabs[VSIw->x] != 'x') && (VSIw->x < VSIw->maxwidth)) 
		VSIw->x++;
}

void VSIinschar(int x)
{
	int i,j;
	char *tempa,*temp;

	VSIwrapnow(&i,&j);

	tempa=VSIw->attrst[VSIw->y]->text;
	temp=VSIw->linest[VSIw->y]->text;
	for(i=VSIw->maxwidth-x; i>=VSIw->x; i--) {
		temp[x+i]=temp[i];
		tempa[x+i]=tempa[i];
	  }	/* end for */
	for(i=VSIw->x; i<VSIw->x+x; i++) {
		temp[i]=' ';
		tempa[i]=VSIclrattrib;
	  }	/* end for */
}

void VSIinsstring(int len,char *start)
{
	if(VSIw->VSIDC)
		RSinsstring(VSIwn,VSIw->x-len,VSIw->y,VSIw->attrib,len,start);
	else
		RSdraw(VSIwn,VSIw->x-len,VSIw->y,VSIw->attrib,VSIw->maxwidth-VSIw->x+len+1,start);
}

void VSIsave(void)
{
	VSIw->Px=VSIw->x;
	VSIw->Py=VSIw->y;
	VSIw->Pattrib=VSIw->attrib;
}

void VSIrestore(void)
{
	VSIw->x=VSIw->Px;
	VSIw->y=VSIw->Py;
	VSIrange();
	VSIw->attrib=VSinattr(VSIw->Pattrib);
}

void VSIdraw(int VSIwn,int x,int y,int a,int len,char *c)
{
	int x2,y2,
		offset;

	if(!VSIclip(&x,&y,&x2,&y2,&len,&offset))
		RSdraw(VSIwn,x,y,a,len,c+offset);
}

void VSIstring(unsigned char *c,int ctr)
{
	int sx;
	int insert,
		ocount,
		attrib,
		extra,
		offend;
    char *acurrent,         /* pointer to the attributes for characters drawn */
        *current,           /* pointer to the place to put characters */
		*start;

    while(ctr>0) {
        if(*c<32) {
            switch(*c) {
				case 0x07:		/* CTRL-G found (bell) */
					RSbell(VSIwn);
					break;

				case 0x08:		/* CTRL-H found (backspace) */
					VSIw->x--;
                    if(VSIw->x<0)
						VSIw->x=0;
					break;

				case 0x09:		/* CTRL-I found (tab) */
					VSItab();	  /* Later change for versatile tabbing */
					break;

				case 0x0a:		/* CTRL-J found (line feed) */
				case 0x0b:		/* CTRL-K found (treat as line feed) */
				case 0x0c:		/* CTRL-L found (treat as line feed) */
					VSIindex();
					break;

				case 0x0d:		/* CTRL-M found (carriage feed) */
					VSIw->x=0;
					break;

			  }	/* end switch */
            c++;
            ctr--;
          } /* end if */
        else
            while((ctr>0) && (*c>=32)) {     /* print out printable ascii chars, if we haven't found an ESCAPE char */
                current=start=&VSIw->linest[VSIw->y]->text[VSIw->x];    /* this line is important, both are * chars */
                acurrent=&VSIw->attrst[VSIw->y]->text[VSIw->x];
                attrib=VSIw->attrib;
                insert=VSIw->IRM;          /* boolean */
                ocount=VSIw->x;
                offend=0;
                extra=0;
                sx=VSIw->x;
                if(VSIw->x>VSIw->maxwidth) {
                    if(VSIw->DECAWM) {  /* check for line wrapping on */
                        VSIw->x=0;
                        VSIindex();
                      } /* end if */
                    else                /* no line wrapping */
                        VSIw->x=VSIw->maxwidth;
                    current=start=&VSIw->linest[VSIw->y]->text[VSIw->x];
                    acurrent=&VSIw->attrst[VSIw->y]->text[VSIw->x];
                    ocount=VSIw->x;
                    sx=VSIw->x;
                  } /* end if */
                while((ctr>0) && (*c>=32) && (offend==0)) {
                    if(insert)
                        VSIinschar(1);
                    *current=*c;
                    *acurrent=(char)attrib;
                    c++;
                    ctr--;
                    if(VSIw->x<VSIw->maxwidth) {
                        acurrent++;
                        current++;
                        VSIw->x++;
                      } /* end if */
                    else {
                        if(VSIw->DECAWM) {
                            VSIw->x++;
                            offend=1;
                          } /* end if */
                        else {
                            VSIw->x=VSIw->maxwidth;
                            extra=1;
                          } /* end else */
                      } /* end else */
                  } /* end while */
                if(insert)
                    VSIinsstring(VSIw->x-ocount+offend+extra,start);        /* actually just decides which RS to use */
                else
                    VSIdraw(VSIwn,sx,VSIw->y,VSIw->attrib,VSIw->x-ocount+offend+extra,start);
              } /* end while */
      } /* end while */
}   /* end VSIstring */

