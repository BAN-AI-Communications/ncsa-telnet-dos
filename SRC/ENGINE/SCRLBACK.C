/*
*    SCRLBACK.C
****************************************************************************
*                                                                          *
*      NCSA Telnet for the PC                                              *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
*      This program is in the public domain.                               *
*                                                                          *
****************************************************************************
*   Revisions:
*   6-26-90	 Separated from look.c by Heeren Pathak
*/


#ifdef MSC
#define mousecl mousecml
#endif

#ifdef MOUSE
#include "mouse.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <conio.h>
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif

#include "whatami.h"
#include "nkeys.h"
#include "windat.h"
#include "hostform.h"
#include "protocol.h"
#include "externs.h"

static int its_in(long ,long );

extern int scroll,				/* which way do we scroll */
	cflag,						/* are we copying ? */
	cbuf,						/* do we have things in the buffer? */
	mcflag,						/* mouse button flag */
	vsrow,						/* VS row */
	numline,
	mighty;						/* mouse present? */

extern long size,				/* size of buffer */
	cstart,						/* starting location of VS copy buffer */
	cend,						/* ending location of VS copy buffer */
	cdist;						/* distance to base point */

extern struct config def;
extern char *copybuf,
	*lineend;

/***********************************************************************/
/*  scrollback
*   Take keyboard keys to manipulate the screen's scrollback
*   Written by Heeren Pathak 4/1/89 along with the mouse routinues.
*/
void scrollback(struct twin *tw)
{
	int row,				/* current row */
	    col,				/* current col */
	    c; 					/* input */
	long i,					/* loop index  */
		temp;				/* general purpose */

	if(tw->termstate==TEKTYPE)
		return;
	set_cur(0);
	c=n_chkchar();
#ifdef SCRL_MOUSE
	if(c==-1) {
		 c=chkmouse();
		 mouse=0;
	  } /* end if */
#endif
	switch (scroll) {
		case -1:			/* scroll stuff down */
			if(c==CURDN || c==PGDN || c==E_CURDN || c==E_PGDN) {
				scroll=0;
				c=0;
			}
			row=n_row();
			col=n_col();
			if(vsrow>(-tw->bkscroll)) {		/* have not reached limit of buffer */
				vsrow--;
				row=0;
				VSscrolback(tw->vs,1);
				temp=4;
				if(cflag || cbuf) {
					i=cdist;
					cdist=(long)(vsrow+row)*80+col;
					if(cbuf || i>cdist) {
						temp-=setattr(0,tw->colors[0],tw->colors[2]);
            temp-=setattr(1,tw->colors[0],tw->colors[2]);
					  }
				  }
				for(i=0; i<temp*1000; i++);	/* delay */
				n_cur(0,col);
		    }
			break;

		case 1:				/* scroll stuff up */
			row=n_row();
			col=n_col();
			if(c==CURUP || c==PGUP || c==E_CURUP || c==E_PGUP) {
				scroll=0;
				c=0;
			}
			if(vsrow<0) {
				++vsrow;
				row=numline;
				VSscrolforward(tw->vs,1);
				temp=4;
				if(cflag || cbuf) {
					i=cdist;
					cdist=(long)(vsrow+row)*80+col;
					if(cbuf || cdist>i) {
						temp-=setattr(numline-1,tw->colors[0],tw->colors[2]);
 						temp-=setattr(numline,tw->colors[0],tw->colors[2]);
					  }
				  }
				for(i=0; i<temp*1000; i++);	/* delay */
				n_cur(numline,col);
			  }
			break;				
 	  }	/* end switch */

	switch (c) {			/* just move around */
#ifdef MOUSE
		case CTRLCURRT:
		case E_CTRLCURRT: /* Send a message so speed up mouse */
			nm_mousespeed(-1);
			break;

		case CTRLCURLF:
		case E_CTRLCURLF: /* Send a message so speed up mouse */
			nm_mousespeed(1);
			break;
#endif
        case CTRLPGDN:
		case E_CTRLPGDN:
			col=79; 
			row=numline;
			VSscrolforward(tw->vs,-vsrow);
			vsrow=0;
			cdist=(long)(row+vsrow)*80+col;
			n_cur(row,col);
			if(cflag || cbuf)
				updatescr(tw->colors[2]);
			break;

		case CTRLPGUP:
		case E_CTRLPGUP:
			row=col=0; 
			VSscrolback(tw->vs,(tw->bkscroll - vsrow));
			vsrow=(-tw->bkscroll);
			cdist=(long)(row+vsrow)*80+col;
			n_cur(row,col);
			if(cflag || cbuf)
				updatescr(tw->colors[2]);
			break;

		case HOME:
		case E_HOME:
			row=n_row();
			col=n_col();
			if(col) {
				col=0;
				c=0;
			  }	/* end if */
			else {
				row=0;
				c=1;
			  }	/* end else */
			cdist=(long)(row+vsrow)*80+col;
			n_cur(row,col);
			if(cflag || cbuf)
				if(c)
					updatescr(tw->colors[2]);
				else
					setattr(row,tw->colors[0],tw->colors[2]);
			break;

		case ENDKEY:
		case E_ENDKEY:
			row=n_row();
			col=n_col();
			if(col!=79) {
				col=79;
				c=0;
			  }	/* end if */
			else {
				row=numline;
				c=1;
			  }	/* end else */

			cdist=(long)(row+vsrow)*80+col;

			if(c)
				n_cur(numline,col);
			else
				n_cur(row,col);

			if(cflag || cbuf)
				if(c)
					updatescr(tw->colors[2]);
				else
					setattr(row,tw->colors[0],tw->colors[2]);
			break;										 

		case CURLF:
		case E_CURLF:
			row=n_row();
			col=n_col();
			if(col>0) {
				col--;
				cdist=(long)(row+vsrow)*80+col;
				if(cflag || cbuf) {
					if(is_in(row,vsrow,col+1)) 
						(*attrptr)(tw->colors[2]);
					else 
						(*attrptr)(tw->colors[0]);
				  }
				n_cur(row,col);
				if(cflag || cbuf) {
					if(is_in(row,vsrow,col))
						(*attrptr)(tw->colors[2]);
					else 
						(*attrptr)(tw->colors[0]);
				  }
			  }
			break;

		case CURRT:
		case E_CURRT:
			row=n_row();
			col=n_col();
			if(col<79) {
				col++;
				cdist=(long)(row+vsrow)*80+col;
				if(cflag || cbuf) {
					if(is_in(row,vsrow,col-1)) 
						(*attrptr)(tw->colors[2]);
					else 
						(*attrptr)(tw->colors[0]);
				  }
				n_cur(row,col);
				if(cflag || cbuf) {
					if(is_in(row,vsrow,col)) 
						(*attrptr)(tw->colors[2]);
					else 
						(*attrptr)(tw->colors[0]);
				  }
			  }
			break;

		case CURUP:
		case E_CURUP:
			row=n_row();
			col=n_col();
			if(row>=1) {
				row--;
				if(cflag) {
					cdist=(long)(vsrow+row)*80+col;
					setattr(row+1,tw->colors[0],tw->colors[2]);
					setattr(row,tw->colors[0],tw->colors[2]);
				  }
			  }
			else {
				if(vsrow>(-tw->bkscroll)) {
					if (def.autoscroll)
						scroll=-1;
					vsrow--;
					row=0;
					VSscrolback(tw->vs,1);
					if(cflag || cbuf) {
						cdist=(long)(vsrow+row)*80+col;
						setattr(1,tw->colors[0],tw->colors[2]);
						setattr(0,tw->colors[0],tw->colors[2]);
					  }
				  }
				else 
					if(row>0){
						row--;
						if(cflag || cbuf) {
							cdist=(long)(vsrow+row)*80+col;
							setattr(row+1,tw->colors[0],tw->colors[2]);
							setattr(row,tw->colors[0],tw->colors[2]);
						  }
					  }				
			  }
			n_cur(row,col);
			break;

		case CURDN:
		case E_CURDN:
			row=n_row();
			col=n_col();
			if(row<numline) {
				++row;
				if(cflag) {
					cdist=(long)(vsrow+row)*80+col;
					setattr(row-1,tw->colors[0],tw->colors[2]);
					setattr(row,tw->colors[0],tw->colors[2]);
				  }
			  }
			else{
				if(vsrow<0) {
					if (def.autoscroll)
						scroll=1;
					++vsrow;
					row=numline;
					VSscrolforward(tw->vs,1);
					if(cflag || cbuf) {
						cdist=(long)(vsrow+row)*80+col;
						setattr(numline-1,tw->colors[0],tw->colors[2]);
						setattr(numline,tw->colors[0],tw->colors[2]);
					  }
				  }
				else 
					if(row<numline) {
						++row;
						if(cflag || cbuf) {
							cdist=(long)(vsrow+row)*80+col;
							setattr(row-1,tw->colors[0],tw->colors[2]);
							setattr(row,tw->colors[0],(int)tw->colors[2]);
						  }
					  }
			  }
			n_cur(row,col);
			break;

		case PGUP:
		case E_PGUP:
			row=n_row();
			col=n_col();
			VSscrolback(tw->vs,numline+1);
			vsrow-=numline+1;
			if(vsrow<(-tw->bkscroll)) 
				vsrow=(-tw->bkscroll);
			cdist=(long)(vsrow+row)*80+col;
			if(cflag || cbuf)
				updatescr(tw->colors[2]);
			n_cur(row,col);
			break;

		case PGDN:
		case E_PGDN:
			row=n_row();
			col=n_col();
			VSscrolforward(tw->vs,numline+1);
			vsrow+=numline+1;
			if(vsrow>0) 
				vsrow=0;
			cdist=(long)(vsrow+row)*80+col;
			if(cflag || cbuf)
				updatescr(tw->colors[2]);
			n_cur(row,col);
			break;

		case (int)' ':
		case (int)'\n':
			row=n_row();
			col=n_col();
			if(!cflag) {
				if((n_flags()&3)) {
					cflag=1;
					cbuf=0;
					remark(row,col,vsrow,tw);
				  }
				else {
					if(cbuf)
						resetscr(tw);
					cflag=1;
					cbuf=0;
          cstart=(long)(vsrow+row)*80+col;
					cdist=cstart;
					setattr(row,tw->colors[0],tw->colors[2]);
				  }
			  }
			else {
				cflag=0;
				cbuf=1;
				cend=(long)(vsrow+row)*80+col;
			  }
			n_cur(row,col);
      /* Why mark text and not copy it??  rmg 930429 */
      if(!cbuf) break;

		case ALTC:
			if(cend<cstart) {
				temp=cend;
				cend=cstart;
				cstart=temp;
			  }
			size=((cend-cstart+2));
			if(size>32700)
				break;
			if(copybuf!=NULL) 
				free(copybuf);
			if((copybuf=(char *)malloc((unsigned int)(size+1)))==NULL)
				break;
#ifdef OLD_WAY
      size=VSgettext(tw->vs,(int)(cstart%80)-1,(int)(cstart/80),(int)(cend%80),(int)(cend/80),copybuf,size,lineend,0);
#else
      /* Contribution by Hsiao-yang Cheng ++ sycheng@csie.nctu.edu.tw */
      /* When we scroll back, the "cstart" and "cend" are negative numbers.
         In C language, The remainder of a negative number is also a negative
         number.  So the result of VSgettext(...,cstart%80, ...., cend%80,...)
         will not be the expected result.  This also can be fixed in function
         VSgettext(), but I solve it in scrollback(). */ /* Thanks much,  rmg 931111 */
      {
        int t1,t2,t3,t4;

        t1=(int)(cstart%80); t2=(int)(cstart/80);
        if (t1<0) {
            t1+=80; t2--;
        }
        t3=(int)(cend%80); t4=(int)(cend/80);
        if (t3<0) {
            t3+=80; t4--;
        }
        size=VSgettext(tw->vs,t1-1,t2,t3,t4,copybuf,size,lineend,0);
      }
#endif
			copybuf[size]='\0';
			break;
										
		default:
			break;
	  }
	set_cur(1);
}

/*************************************************
*
* setattr()
* sets the attribute of a line 
*/
int setattr(int row,int attrn,int attrh)
{
	int i,
		col;

	if(is_in(row,vsrow,0) || is_in(row,vsrow,79) || its_in((long)(row+vsrow-1)*80,(long)(row+vsrow+1)*80+79 )) {
		n_row();
		col=n_col();
		for(i=0; i<80;i++) {
			n_cur(row,i);
			if(is_in(row,vsrow,i))
				(*attrptr)((char)attrh);
			else
				(*attrptr)((char)attrn);
		  }
		n_cur(row,col);
		return(1);
	  }
	return(0);
}

/************************************************
*
* resetscr(struct twin *tw)
* clear marked text from screen
*/

void resetscr(struct twin *tw)
{
	int row,col,r,c;

	r=n_row();
	c=n_col();
	for(row=0; row<numline+1; row++)
		for(col=0; col<80; col++) {
			n_cur(row,col);
			(*attrptr)((char)(tw->colors[0]));
		  }
	n_cur(r,c);
}

/*************************************************
*
* updatescr()
* marks screen
*/
void updatescr(int attr)
{
	int row,
		col,
		r,
		c;

	r=n_row();
	c=n_col();
	for(row=0;row<numline+1;row++)
		for(col=0;col<80;col++) {
			if(is_in(row,vsrow,col)) {
				n_cur(row,col);
				(*attrptr)(attr);
			  }
		  }
	n_cur(r,c);
}

/**********************************************************************
*
* chkmouse()
* gets mouse movement and button info
*/

#ifdef MOUSE
#ifdef SCRL_MOUSE
int num_right = 0;
int num_down = 0;
#endif
#define MIN_MOVE 6
#endif

#ifdef SCRL_MOUSE
int chkmouse(void )
{
	int m1=3, m2=0 ,m3=0 ,m4=0;		/* mouse variables */

	printf("using chkmouse\n");
	if(!mighty) 
		return(-1);

	m1=11;

	mousecl(&m1,&m2,&m3,&m4);			/* read absolute positions */

#ifndef MOUSE
	if(m3>2)
		return(CURRT);
	if(m3<-2)
		return(CURLF);
	if(m4>2)
		return(CURDN);
	if(m4<-2)
		return(CURUP);
#else
	num_right += m3;
	num_down += m4;
	if (abs(num_right)>abs(num_down)) {
		if(num_right >= MIN_MOVE) {
			num_right-=MIN_MOVE;
			return(CURRT);
		 }	/* end if */
		if(num_right <= -MIN_MOVE) {
			num_right+=MIN_MOVE;
			return(CURLF);
		 }	/* end if */
	  } /* end if */
	if(num_down >= MIN_MOVE) {
		num_down-=MIN_MOVE;
		return(CURDN);
	 }	/* end if */
	if(num_down <= -MIN_MOVE) {
		num_down+=MIN_MOVE;
		return(CURUP);
	  } /* end if */
#endif

	m1=3;								/* read mouse buttons */
	mousecl(&m1,&m2,&m3,&m4);

	if(m2 & 1) 
		m2=1;
	else 
		m2=0;
	if(m2!=mcflag) {
		mcflag=m2;
		return((int)' ');
	  }
	return(-1);
}
#endif

#ifndef MOUSE
/*****************************************************************
*
* initmouse()
* check to see if we have a mouse
*/
int initmouse(void )
{
	int m1=0, m2=0, m3=0, m4=0;  			/* mouse variables */

	m1=0;	 					
	mousecl(&m1,&m2,&m3,&m4);				/* initialize mouse driver */
	if(!m1) 
		return(0);
	m1=2;
	mousecl(&m1,&m2,&m3,&m4);				/* turn off software cursor */
	return(1);
}
#endif

/**************************************
*
* is_in()
* check to see if cursor is at marked location
*/
int is_in(int row,int vs_row,int col)
{
	long vsloc,
		c,
		end,
		start;

    start=cstart;
    if(cbuf) 
		end=cend;
    else 
		if(cflag) 
			end=cdist;
	 	else 
			return(0);
    if(end<start) {
		c=end;
		end=start;
		start=c;
      }
	vsloc=(long)(row+vs_row)*80+col;
	if((start<=vsloc) && (end>=vsloc)) 
		return(1);
	return(0);
}

/*******************************************
*
* its_in()
*
* checks to see if marked area is between two points
*/
static int its_in(long vsstart,long vsend)
{
	long start,end;

    start=cstart;
    if(cbuf) 
		end=cend;
    else 
		if(cflag) 
			end=cdist;
		else 
			return(0);
	if(((vsstart<=start) && (vsend>=start)) || ((vsstart<=end) && (vsend>=end)))
		return(1);
	return(0);
}

/********************************************
*
* remark()
*
* it resets the the end points of the marked text
*
*/
void remark(int row,int col,int vs_row,struct twin *tw)
{
	long vsloc, dstart, dend;
	int x, y;

	vsloc=(long)(row+vs_row)*80+col;
	dstart=vsloc-cstart;
	dend=vsloc-cend;
	if(dstart<0) 
		dstart=-dstart;
	if(dend<0) 
		dend=-dend;
	if(dstart<dend) {
		cstart=cend;
		cdist=vsloc;
	  }
	else 
		cdist=vsloc;
	for(y=0; y<numline+1; y++)
		for(x=0; x<80; x++) {
			if(is_in(y,vs_row,x)) {
				n_cur(y,x);		
				(*attrptr)(tw->colors[2]);
			  }	/* end if */
			else {
				n_cur(y,x);		
				(*attrptr)(tw->colors[0]);
			  }	/* end else */
		  }	/* end for */
	n_cur(row,col);
}
