/*
*	RSpc.c
*
*   real screen interface for Gaige Paulsen's 
*   Virtual screen driver
*
*   Tim Krauskopf
*
*	Date		Notes
*	--------------------------------------------
*	11/25/86	Start -TKK
*	6/89		code cleanup for 2.3 release -QAK
*/

/*
* Includes
*/
#ifdef MOUSE
#include "mouse.h"
#endif
#ifdef __TURBOC__
#include "turboc.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bios.h>		/* for printer functions */
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#include "whatami.h"
#include "windat.h"
#include "vskeys.h"
#include "hostform.h"
#include "nkeys.h"
#include "externs.h"
#include "keymap.h"
#include "map_out.h"

/*
*	Global Variables
*/
extern struct config def;
extern int numline;				/* numline of lines in display */
extern int beep_notify;         /* musical note display flag */

#ifdef NOT
#ifndef FTP
extern int _Sftpstat;
extern int _Sftpname;
#endif
#endif

/*
*	Local Variables
*/
static int lastatt=255,			/* last attribute value written to screen*/
	lastw=255,
	thevis=0;					/* which window is visible */

int transtable[]={      /*  Graphics translation set  */
	 32,  4,177,  9, 12, 13, 10,248,
	241, 10, 10,217,191,218,192,197,
	196,196,196,196, 95,195,180,193,
	194,179,243,242,227,168,156,250,
	 32, 32, 32, 32, 32, 32, 32, 32,
	 32, 32, 32, 32 };


/***************************************************************************/
/*   map_output:
*	   Takes a character and converts it to the IBM PC extended character
*   set which, in the outputtable array, is mapped to the output mapping
*   set up in telnet.out
*/
#define map_output(ch)  ((int)outputtable[ch-95])

/***************************************************************************/
/*   translate:
*	   Takes a character and converts it to the IBM PC extended character
*   set which, in the transtable array, is mapped to the VT100 graphics
*   characters, with minor conflicts.
*/
#define translate(ch)   ((int)((ch>94) ? transtable[ch-95] : ch))

#ifdef NOT_USED
/*************************************************************************/
/*
*	RSokmem()
*
*	returns whether memory is ok (QAK ?)
*/
unsigned int RSokmem(int size)
{
#ifdef MSC
#if defined (__TURBOC__) | defined(__WATCOMC__)
	return(1);
#else
	return(_freect(size));
#endif
#else
	return(1);
#endif
}
#endif

/*************************************************************************/
/*
*	RSbell()
*
*	rings a bell
*/
void RSbell(int w)
{
/*
*  might add something to indicate which window
*/
	n_sound(1000,12);		/* PC bell routine */
    if (beep_notify) {
        screens[w]->sstat=14;
        statline();
    }
}

/*************************************************************************/
/*
*	RSvis()
*
*	sets the visible window to be the window number passed to it
*/
void RSvis(int w)
{
	thevis=w;				/* this is visible window */
}

void RSinitall() {}

void RSinsstring(int w,int x,int y,int attrib,int len,char *s)
{
/* Needed for possible future functionality */
	w=w;
	x=x;
	y=y;
	attrib=attrib;
	len=len;
	s=s;
}

void RSdelchars(int w,int x,int y,int n)
{
/* Needed for possible future functionality */
	w=w;
	x=x;
	y=y;
	n=n;
}

void RSbufinfo(int w,int numlines,int top,int bottom)
{
/* Needed for possible future functionality */
	w=w;
	numlines=numlines;
	top=top;
	bottom=bottom;
}

void RSdrawsep(int w,int y,int data)
{
/* Needed for possible future functionality */
	w=w;
	y=y;
	data=data;
}

void RSmargininfo(int w,int x,int data)
{
/* Needed for possible future functionality */
	w=w;
	x=x;
	data=data;
}

void RSdelcols(int w,int n)
{
/* Needed for possible future functionality */
	w=w;
	n=n;
}

void RSinscols(int w,int n)
{
/* Needed for possible future functionality */
	w=w;
	n=n;
}


/*************************************************************************/
void RScursoff(int w)
{
	w=w;
		/* do nothing, MAC routine */
}

/*************************************************************************/
/*
*	RScurson()
*
*	move the cursor to a x,y position on the real screen
*/
void RScurson(int w,int y,int x)
{
	if(w!=thevis)
		return;							/* not visible */
/*
*  this is really the cursor positioning routine.  If cursor is turned off,
*  then it needs to be turned back on.
*/
	n_cur(x,y);
/*  add code to save cursor position for a given window */
}

/*************************************************************************/
/*
*	RSdraw()
*
*	put a string at a position on the real screen
*/

/* Contribution from njtaylor@cix.compulink.co.uk */  /* rmg 930917 */
/* First time the characters are
translated to IBM Character set, If Help Screen displayed on the re-draw of
the screen the characters are translated a secord time to different characters
The draw routine, has been updated to use a local character string draw_char 
for the translations, leaving the characters passed via the argument unchanged.
*/

static unsigned char draw_char[133];

void RSdraw(int w,int y,int x,int a,int len,unsigned char *ptr)
{
	int i;

	if(w!=thevis) {	/*  indicate that something happened */
		x=n_row();
		y=n_col();
		if(screens[w]->sstat!='*') {
			if(screens[w]->sstat==47)
				screens[w]->sstat=92;
			else
                if(screens[w]->sstat!=14)
                    screens[w]->sstat=47;
		  }
		statline();
		n_cur(x,y);							/* restore cursor where belongs */
		return;
	  }
/*
* call my own draw routine
*/
  if(w!=lastw || a!=lastatt)    /* need to parse attribute bit */
		RSsetatt(a,w);
	n_cur(x,y);
	if(VSisgrph(lastatt))
	{
		for(i=0; i<len; i++)
			draw_char[i]=(char)translate((int)ptr[i]);
		ptr = draw_char;
	}
	else {		/* ok, we're not in graphics mode, check whether we are mapping the output for this session */
		if(current->mapoutput) {	/* check whether we should map the characters output for this session */
			for(i=0; i<len; i++)	/* go through the entire line, mapping the characters output to the screen */
        draw_char[i]=outputtable[(unsigned char)ptr[i]];
			ptr = draw_char;
		  }	/* end if */
	  }	/* end else */
	if(Scmode())
		n_cheat(ptr,len);
	else
		n_draw(ptr,len); 
}

/*************************************************************************/
/*
*	RSsetatt()
*
*	set the attribute for real screen printing
*/
void RSsetatt(int a,int w)
{
	int c;

	if(VSisundl(a))
		c=screens[w]->colors[1];
	else
		if(VSisrev(a))
			c=screens[w]->colors[2];
		else
			c=screens[w]->colors[0];
	if(VSisblnk(a))
		c |= 128;				/* set blink bit */
	if(VSisbold(a))
		c |= 8;					/* set brightness bit */
	n_color(c);
	lastatt=a;
	lastw=w;
}

/*************************************************************************/
/*
*	RSdellines()
*
*	blank out a section of a series of lines
*/
void RSdellines(int w,int t,int b,int n,int select)
{
	int c;

	if(w!=thevis || n<1)
		return;							/* not visible */
	c=n_color(screens[w]->colors[0]);
	n_scrup(n,t,0,b,79);
	n_color(c);
	select=select;
}

/*************************************************************************/
/*
*	RSerase
*
*	blank out a reactangular section of the screen
*/
void RSerase(int w,int y1,int x1,int y2,int x2)
{
	int c;

	if(w!=thevis)
		return;							/* not visible */
	c=n_color(screens[w]->colors[0]);
	n_scrup(0,x1,y1,x2,y2);
	n_color(c);
}

/*************************************************************************/
/*
*	RSinslines()
*
*	scroll down a section of the screen in order to fit lines above them
*/
void RSinslines(int w,int t,int b,int n,int select)
{
	int c;

	if(w!=thevis || n<1)
		return;							/* not visible */
	c=n_color(screens[w]->colors[0]);
	n_scrdn(n,t,0,b,79);
	n_color(c);
	select=select;
}

/*************************************************************************/
/*
*	RSsendstring()
*
*	send a string over the network
*/
void RSsendstring(int w,char *ptr,int len)
{
	netwrite(screens[w]->pnum,ptr,len);
}


/*	Keyboard translations from the PC to VT100
*		Tim Krauskopf			Sept. 1986
*
*	original: ISP 1984
*	Re-written to handle arbitrary keyboard re-mapping,
*		 Quincey Koziol		Aug. 1990
*/

/***************************************************************************/
/*  takes a key value and checks whether it is a mapped key,
*		then checks whether it is a 'special' key (i.e. vt100 keys, and
*		maybe other kermit verbs), pass the characters on to the
*		TCP port in pnum.
*/
void vt100key(unsigned int c)
{
	key_node *temp_key;		/* pointer to the key map node which matches the character code */

	if(!IS_KEY_MAPPED(c)) {		/* if the key is not mapped, just send it */
    if(c<128 || (current->ibinary))       /* make certain it is just an ascii char which gets sent, unless we are transmitting binary data */
    netwrite(current->pnum,(char *)&c,1);
  } /* end if */
	else {		/* key is mapped */
   if(IS_KEY_SPECIAL(c)) {   /* check whether this is a special key code (i.e. a kermit verb) */
			if((temp_key=find_key(c))!=NULL)	/* get the pointer to the correct key mapping node */
				VSkbsend(current->vs,(unsigned char)(*temp_key).key_data.vt100_code,(int)!(current->echo));	/* send the vt100 sequence */
		  }	/* end if */
		else {		/* just pass along the string the key is mapped to */
			if((temp_key=find_key(c))!=NULL) {	/* get the pointer to the correct key mapping node */
				if((*temp_key).key_data.key_str!=NULL) {	/* verify that the string has been allocated */
					RSsendstring(current->vs,(*temp_key).key_data.key_str,strlen((*temp_key).key_data.key_str));
					if(!(current->echo))
						VSwrite(current->vs,(*temp_key).key_data.key_str,strlen((*temp_key).key_data.key_str));
				  }	/* end if */
			  }	/* end if */
		  }	/* end else */
	  }	/* end else */
}   /* end vt100key() */

/***********************************************************************/
/*  non-blocking RSgets()
*   This routine will continually add to a string that is re-submitted
*   until a special character is hit.  It never blocks.
*
*   As long as editing characters (bksp, Ctrl-U) and printable characters
*   are pressed, this routine will update the string.  When any other special
*   character is hit, that character is returned.  
*/
static char bk[]={8,' ',8};

int RSgets(int w,char *s,int lim,char echo)
{
	int c,count,i;
	char *save;

	count=strlen(s);
	save=s;
	s+=count;
	while(0<(c=n_chkchar())) {
        switch(c) {                 /* allow certain editing chars */
			case 8:					/* backspace */
			case BACKSPACE:			/* backspace */
				if(count) {
                    if(echo)
						VSwrite(w,bk,3);
					count--;		/* one less character */
					s--;			/* move pointer backward */
                  } /* end if */
				break;

			case 21:
                if(echo)
					for(i=0; i<s-save; i++)
                        VSwrite(w,bk,3);
				s=save;
				break;

			case 13:
			case 9:
			case TAB:
				*s='\0';			/* terminate the string */
				return(c);

			default:
				if(count==lim) {			/* to length limit */
                    RSbell(w);
					*s='\0';				/* terminate */
					return(0);
				  }
                if(c>31 && c<127) { /* check for printable ASCII */
                    if(echo)
						VSwrite(w,(char *)&c,1);
					*s++=(char)c;			/* add to string */
					count++;				/* length of string */
				  }
				else {
                    if(c>0 && c<27) {   /* print ASCII control char */
                        c+=64;
                        if(echo) {
							VSwrite(w,"^",1);
							VSwrite(w,(char *)&c,1);
                          }
                        c-=64;
					  }
					*s='\0';			/* terminate the string */
					return(c);
				  }
			break;
		  }
	  }
	*s='\0';			/* terminate the string */
	return(c);
}

/***********************************************************************/
/*  non-blocking gets()
*   This routine will call netsleep while waiting for keypresses during
*   a gets.  Replaces the library gets.
*
*   As long as editing characters (bksp, Ctrl-U) and printable characters
*   are pressed, this routine will continue.  When any other special
*   character is hit, NULL is returned.  the return key causes a normal return.
*/
char *nbgets(char *s,int lim)
{
	int c,count,i;
	char *save;

	count=0;
	save=s;
	while(1) {
		c=n_chkchar();
		if(c<=0) {
			Stask();			/* keep communications going */
			continue;			/* check for a character immediately */
		  }
		switch (c) {				/* allow certain editing chars */
			case 8:					/* backspace */
			case BACKSPACE:			/* backspace */
				if(count) {
					n_putchar((char)8);
					n_putchar(' ');
					n_putchar((char)8);
					count--;		/* one less character */
					s--;			/* move pointer backward */
				  }
				break;

			case 13:					/* carriage return,=ok */
				n_puts("");
				*s='\0';				/* terminate the string */
				return((char *)save);	/* return ok */

			case 21:		/* ctrl-u */
				for(i=0; i<s-save; i++) {
					n_putchar(8);
					n_putchar(' ');
					n_putchar(8);
				  }
				s=save;
				break;

			default:
				if(c>31 && c<127) {
					if(count<lim) {
						n_putchar((char)c);
						*s++=(char)c;			/* add to string */
						count++;				/* length of string */
					  }
				  }
				else {
					n_puts("");
					*s='\0';			/* terminate the string */
					return((char *)NULL);
				  }
			break;
		  }
	  }
}

/************************************************************************/
/*  nbgetch
*   check the keyboard for a character, don't block to wait for it,
*   but don't return to the caller until it is there.
*/
int nbgetch(void)
{
	int c;

	while(0>=(c=n_chkchar())) 		/* there is a key? */
		Stask();					/* no key yet, update everything */
	return(c);
}

#ifdef NOT_USED
/************************************************************************/
/* nbget
*   demux at least one packet each time, check the keyboard for a key, 
*   return any key pressed to the caller.
*/
int nbget(void)
{
	int c;

	demux(0);				/* only one packet */
	if(0>=(c=n_chkchar()))
		return(-1);			/* no key ready */
	return(c);
}
#endif

#ifdef FTP
/***********************************************************************/
/* ftpstart
*  update status line with new file length remaining
*/
void ftpstart(char dir,char *buf)
{
	int r,c,cl;
	long int fpos;

	r=n_row();
	c=n_col();
	cl=n_color(current->colors[0]);
	if(dir)
		dir='<';
	else
		dir='>';
	Sftpname(&buf[100]);	/* get file name */
	Sftpstat(&fpos);		/* get position in file */
	n_cur(numline+1,49);
	sprintf(buf,"FTP %c %14s %10lu",dir,&buf[100],fpos);
	if(Scmode()) 
		n_cheat(buf,strlen(buf));
	else
		n_draw(buf,strlen(buf));
	n_color(cl);
	n_cur(r,c);
}
#endif /* FTP */
