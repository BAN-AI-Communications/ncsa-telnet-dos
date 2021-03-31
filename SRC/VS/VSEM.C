/*
 *
 *	  Virtual Screen Kernel Emulation Routines
 *					 (vsem.c)
 *
 *   National Center for Supercomputing Applications
 *	  by Gaige B. Paulsen
 *
 *	This file contains the private emulation calls for the NCSA
 *  Virtual Screen Kernel.
 *
 *	  Version   Date	Notes
 *	  -------   ------  ---------------------------------------------------
 *	    0.01	861102	Initial coding -GBP
 *	    0.10	861111	Added/Modified VT emulator -GBP
 *	    0.50	861113	First compiled edition -GBP
 *		2.1		871130	NCSA Telnet 2.1 -GBP
 *		2.2	880715	NCSA Telnet 2.2 -GBP
 *		2.3		900930	NCSA Telnet	2.3 -QAK
 */

#include <stdio.h>
#include <bios.h>
#include "vsdata.h"
#include "vskeys.h"
#include "externs.h"

#ifdef OLD_WAY
int localprint=0;
#endif

/*  U of Michigan Changes here  and flagged @UM */
/* sends data to local print until we encounter an ESC [ 4 i */
/* the return value is set the number of chars processed     */

/* now accepts <esc>[?4i too  rmg 931119 */
/* allow consecutive 5i escape sequences sent out by Manman.  MPCo.  rmg 931119 */

static int send_localprint(char *,int );

static int send_localprint(char *dat,int len)
{
  char p_esc1[] = "\33[4i";     /* @UM */
  char p_esc2[] = "\33[?4i";    /* rmg */
  char p_esc3[] = "\33[5i";     /* MPCo.  rmg */
  int  p_esc1_len=3, s1;
  int  p_esc2_len=4, s2;
  int  p_esc3_len=3, s3;
  int  i,j,k;
  char c;

  for(i=j=0,s1=s2=s3=1; i<len; i++)  {
    c=dat[i];
    if(s1 && c==p_esc1[j])
      if(j == p_esc1_len) {
        VSIw->localprint=0;   /* we are now turned off */
        i++;
        break;
      }
      else
        ;
    else
      s1=0;

    if(s2 && c==p_esc2[j])
      if(j == p_esc2_len) {
        VSIw->localprint=0;   /* we are now turned off */
        i++;
        break;
      }
      else
        ;
    else
      s2=0;

    if(s3 && c==p_esc3[j])  /* for MPCo. */
      if(j == p_esc3_len) {
        VSIw->localprint=1;   /* we are now left on */
        i++;
        break;
      }
      else
        ;
    else
      s3=0;

    j++;

    if(!s1 && !s2 && !s3) {
      j--;
      for(k=0; k<j; k++)
#ifdef __TURBOC__
        biosprint(0,p_esc1[k],0);
      biosprint(0,c,0);
#elif MSC
        _bios_printer(_PRINTER_WRITE,0,p_esc1[k]); /* lpt_port == 0 */
      _bios_printer(_PRINTER_WRITE,0,c);
#else
        putchar(p_esc1[k]);
      putchar(c);
#endif
      j=0;
      s1=s2=s3=1;
    }
  }

return i;
} /* send_localprint */


/**********************************************************************
*  Function :   VSemchar
*  Purpose  :   Send a character to the virtual screen with no translantion
*  Parameters	:
*           c - character to send to the virtual screen
*  Returns  :   none
*  Calls    :
*  Called by    :   VSem()
**********************************************************************/
static void VSemchar(unsigned char c)
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
    if(insert)
        VSIinschar(1);
    *current=c;
    *acurrent=(char)attrib;
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
    if(insert)
        VSIinsstring(VSIw->x-ocount+offend+extra,start);        /* actually just decides which RS to use */
    else
        VSIdraw(VSIwn,sx,VSIw->y,VSIw->attrib,VSIw->x-ocount+offend+extra,start);
}   /* end VSemchar() */



void VSem(unsigned char *c,int ctr)
{
	int pcount;
    int escflg;             /* vt100 escape level */

    escflg=VSIw->escflg;

/* @UM */
    if(VSIw->localprint && (ctr>0)) {    /* see if printer needs anything */
        pcount=send_localprint(c,ctr);
        /* echo to screen too  rmg 931100 */
#ifdef NO_ECHO
        if(VSIw->localprint == 1) { /* is 2 for echo  RMG 940121 */
          ctr-=pcount;
          c+=pcount;
        }
#endif
      } /* end if */
/* @UM */

	while(ctr>0) {
        while((*c<32) && (escflg==0) && (ctr>0)) {      /* look at first character in the vt100 string, if it is a non-printable ascii code */
			switch(*c) {
				case 0x1b:		/* ESC found (begin vt100 control sequence) */
					escflg++;
					break;

#ifdef CISB
				case 0x05:		/* CTRL-E found (answerback) */
					bp_ENQ();
					break;

#endif
				case 0x07:		/* CTRL-G found (bell) */
					RSbell(VSIwn);
					break;

				case 0x08:		/* CTRL-H found (backspace) */
					VSIw->x--;
                    if(VSIw->x<0)
						VSIw->x=0;
					break;

                case 0x09:          /* CTRL-I found (tab) */
                    VSItab();       /* Later change for versatile tabbing */
					break;

                case 0x0a:          /* CTRL-J found (line feed) */
                case 0x0b:          /* CTRL-K found (treat as line feed) */
                case 0x0c:          /* CTRL-L found (treat as line feed) */
					VSIindex();
					break;

				case 0x0d:		/* CTRL-M found (carriage feed) */
					VSIw->x=0;
					break;

				case 0x0e:		/* CTRL-N found (invoke Graphics (G1) character set) */
					if(VSIw->G1)
						VSIw->attrib=VSgraph(VSIw->attrib);
					else
						VSIw->attrib=VSnotgraph(VSIw->attrib);
					VSIw->charset=1;
					break;

				case 0x0f:		/* CTRL-O found (invoke 'normal' (G0) character set) */
					if(VSIw->G0)
						VSIw->attrib=VSgraph(VSIw->attrib);
					else
						VSIw->attrib=VSnotgraph(VSIw->attrib);
					VSIw->charset=0;
					break;

#ifdef CISB
				case 0x10:		/* CTRL-P found (undocumented in vt100) */
					bp_DLE( c, ctr);
					ctr=0;
					break;
#endif

#ifdef NOT_USED
                case 0x11:      /* CTRL-Q found (XON) (unused presently) */
                case 0x13:      /* CTRL-S found (XOFF) (unused presently) */
				case 0x18:		/* CTRL-X found (CAN) (unused presently) */
				case 0x1a:		/* CTRL-Z found (SUB) (unused presently) */
					break;
#endif
			  }	/* end switch */
			c++;		/* advance to the next character in the string */
			ctr--;		/* decrement the counter */
		  }	/* end while */
        if(escflg==0) {  /* check for normal character to print */
            while((ctr>0) && (*c>=32)) {     /* print out printable ascii chars, if we haven't found an ESCAPE char */
                int sx;
                int insert,
                    ocount,
                    attrib,
                    extra,
                    offend;
                char *acurrent,         /* pointer to the attributes for characters drawn */
                    *current,           /* pointer to the place to put characters */
                    *start;

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
          } /* end if */
		while((ctr>0) && (escflg==1)) { 	/* ESC character was found */
			switch(*c) {
				case 0x08:		/* CTRL-H found (backspace) */
					VSIw->x--;
					if(VSIw->x<0)
						VSIw->x=0;
					break;

				case '[':               /* mostly cursor movement options, and DEC private stuff following */
					VSIapclear();
          escflg=2;
					break;

        case '#':               /* various screen adjustments */
					escflg=3;
					break;

				case '(':               /* G0 character set options */
					escflg=4;
					break;

				case ')':               /* G1 character set options */
					escflg=5;
					break;

				case '>':               /* keypad numeric mode (DECKPAM) */
					VSIw->DECPAM=0;
					escflg=0;
					break;

				case '=':               /* keypad application mode (DECKPAM) */
					VSIw->DECPAM=1;
					escflg=0;
					break;

				case '7':               /* save cursor (DECSC) */
					VSIsave();
					escflg=0;
					break;

				case '8':               /* restore cursor (DECRC) */
					VSIrestore();
					escflg=0;
					break;

				case 'c':               /* reset to initial state (RIS) */
					VSIreset();
					escflg=0;
					break;

				case 'D':               /* index (move down one line) (IND) */
					VSIindex();
					escflg=0;
					break;

                case 'E':               /*  next line (move down one line and to first column) (NEL) */
					VSIw->x=0;
					VSIindex();
					escflg=0;
					break;

				case 'H':               /* horizontal tab set (HTS) */
					VSIw->tabs[VSIw->x]='x';
					escflg=0;
					break;

#ifdef CISB
				case 'I':               /* undoumented in vt100 */
					bp_ESC_I();
					break;

#endif
				case 'M':               /* reverse index (move up one line) (RI) */
					VSIrindex();
					escflg=0;
					break;

				case 'Z':               /* identify terminal (DECID) */
					VTsendident();
					escflg=0;
					break;

				default:
                    VSemchar(0x1b); /* put the ESC character into the VS */
                    VSemchar(*c);   /* put the next character into the VS */
					escflg=0;
					break;

			  }	/* end switch */
			c++;
			ctr--;
		  }	/* end while */
		while((escflg==2) && (ctr>0)) { 	/* '[' handling */
			switch(*c) {
				case 0x08:		/* backspace */
					VSIw->x--;
					if(VSIw->x<0)
						VSIw->x=0;
					break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':               /* numeric parameters */
					if(VSIw->parms[VSIw->parmptr]<0)
						VSIw->parms[VSIw->parmptr]=0;
					VSIw->parms[VSIw->parmptr]*=10;
					VSIw->parms[VSIw->parmptr]+=*c-'0';
					break;

				case '?':               /* vt100 mode change */
					VSIw->parms[VSIw->parmptr++]=(-2);
					break;

				case ';':               /* parameter divider */
					VSIw->parmptr++;
					break;

				case 'A':               /* cursor up (CUU) */
					if(VSIw->parms[0]<1)
						VSIw->y--;
					else
						VSIw->y-=VSIw->parms[0];
					if(VSIw->y<VSIw->top)
						VSIw->y=VSIw->top;
					VSIrange();
					escflg=0;
					break;

				case 'B':               /* cursor down (CUD) */
					if(VSIw->parms[0]<1)
						VSIw->y++;
					else
						VSIw->y+=VSIw->parms[0];
					if(VSIw->y>VSIw->bottom)
						VSIw->y=VSIw->bottom;
					VSIrange();
					escflg=0;
					break;

				case 'C':               /* cursor forward (right) (CUF) */
					if(VSIw->parms[0]<1)
						VSIw->x++;
					else
						VSIw->x+=VSIw->parms[0];
					VSIrange();
					if(VSIw->x>VSIw->maxwidth)
						VSIw->x=VSIw->maxwidth;
					escflg=0;
					break;

				case 'D':               /* cursor backward (left) (CUB) */
					if(VSIw->parms[0]<1)
						VSIw->x--;
					else
						VSIw->x-=VSIw->parms[0];
					VSIrange();
					escflg=0;
					break;

				case 'f':               /* horizontal & vertical position (HVP) */
				case 'H':               /* cursor position (CUP) */
#ifdef OLD_WAY
					if(VSIw->parmptr!=1) {		/* if there weren't enough parameters, perform default action of homing the cursor */
						VSIw->x=0;
#ifdef NOT_SUPPORTED
						if(VSIw->DECORG)
							VSIw->y=VSIw->top;		/* origin mode relative */
						else
							VSIw->y=0;
#endif
						VSIw->y=0;
					  }	/* end if */
					else {		/* correct number of parameters, so move cursor */
						VSIw->x=VSIw->parms[1]-1;
#ifdef NOT_SUPPORTED
						if(VSIw->DECORG)
							VSIw->y=VSIw->parms[0]-1 + VSIw->top;		/* origin mode relative */
						else
							VSIw->y=VSIw->parms[0]-1;
#endif
						VSIw->y=VSIw->parms[0]-1;
						VSIrange();
					  }	/* end else */
#else
					VSIw->x=VSIw->parms[1]-1;
#ifdef NOT_SUPPORTED
					if(VSIw->DECORG)
						VSIw->y=VSIw->parms[0]-1 + VSIw->top;		/* origin mode relative */
					else
						VSIw->y=VSIw->parms[0]-1;
#endif
					VSIw->y=VSIw->parms[0]-1;
#ifdef OLD_WAY /* Caused a resize to report size of 1 due to wrap  rmg 931100 */
          VSIrange();     /* make certain the cursor position is valid */
#else
          if(VSIw->x<0)
            VSIw->x=0;
          if(VSIw->x>(VSIw->maxwidth))
            VSIw->x=VSIw->maxwidth;
          if(VSIw->y<0)
            VSIw->y=0;
          if(VSIw->y>VSPBOTTOM)
            VSIw->y=VSPBOTTOM;
#endif /* resize fix -- Aren't all these ifdefs pretty?! */

#endif
					escflg=0;
					break;

				case 'J':               /* erase in display (ED) */
					switch(VSIw->parms[0]) {
						case -1:
						case 0: 	/* erase from active position to end of screen */
							VSIeeos();
							break;

						case 1: 	/* erase from start of screen to active position */
							VSIebos();
							break;

						case 2: 	/* erase whole screen */
							VSIes();
							break;

						default:
							break;
					  }	/* end switch */
					escflg=0;
					break;

				case 'K':               /* erase in line (EL) */
					switch(VSIw->parms[0]) {
						case -1:
						case 0: 	/* erase to end of line */
							VSIeeol();
							break;

						case 1: 	/* erase to beginning of line */
							VSIebol();
							break;

						case 2: 	/* erase whole line */
							VSIel(-1);
							break;

						default:
							break;
					  }	/* end switch */
					escflg=0;
					break;

                case 'L':               /* insert n lines preceding current line (IL) */
					if(VSIw->parms[0]<1)
						VSIw->parms[0]=1;
					VSIinslines(VSIw->parms[0],-1);
					escflg=0;
					break;

				case 'M':               /* delete n lines from current position downward (DL) */
					if(VSIw->parms[0]<1)
						VSIw->parms[0]=1;
					VSIdellines(VSIw->parms[0],-1);
					escflg=0;
					break;

				case 'P':               /* delete n chars from cursor to the left (DCH) */
					if(VSIw->parms[0]<1)
						VSIw->parms[0]=1;
					VSIdelchars(VSIw->parms[0]);
					escflg=0;
					break;

#ifdef NOT_NEEDED
				case 'R':               /* receive cursor position status from host */
					break;
#endif
				case 'c':               /* device attributes (DA) */
					VTsendident();
					escflg=0;
					break;

				case 'g':               /* tabulation clear (TBC) */
					if(VSIw->parms[0]==3)	/* clear all tabs */
						VSItabclear();
					else
						if(VSIw->parms[0]<=0)	/* clear tab stop at active position */
							VSIw->tabs[VSIw->x]=' ';
					escflg=0;
					break;

				case 'h':               /* set mode (SM) */
					VSIsetoption(1);
					escflg=0;
					break;

				case 'i':               /* toggle printer */
          if(VSIw->parms[VSIw->parmptr]==5) {
            if(VSIw->parms[VSIw->parmptr - 1]==(-2))  /* RMG */
{
              VSIw->localprint=2;  /* echo */
          /* RMG */
#ifdef AUX
fprintf(stdaux," echo printing ");
#endif
}
            else
{
              VSIw->localprint=1;  /* no echo */
#ifdef AUX
fprintf(stdaux," nonecho printing ");
#endif
}
          }

          else if(VSIw->parms[VSIw->parmptr]==4)
            VSIw->localprint=0;
					escflg=0;
					break;

				case 'l':               /* reset mode (RM) */
					VSIsetoption(0);
					escflg=0;
					break;

				case 'm':               /* select graphics rendition (SGR) */
					{
						int temp=0;

						while(temp<=VSIw->parmptr) {
#ifdef OLD_WAY
							if(VSIw->parms[temp]<1)
								VSIw->attrib&=128;
							else
								VSIw->attrib|=(1<<(VSIw->parms[temp]-1));
#else
              /* Contribution from njtaylor@cix.compulink.co.uk
                 to turn individual attributes off */ /* rmg 930917 */
              if(VSIw->parms[temp]<1) VSIw->attrib&=128;
							else if (VSIw->parms[temp]<8)
								 VSIw->attrib|=(1<<(VSIw->parms[temp]-1));
							else if (VSIw->parms[temp]==22) VSIw->attrib&=0xfffe;
							else if ((VSIw->parms[temp]>21) && (VSIw->parms[temp]<28))
								VSIw->attrib&=~(1<<(VSIw->parms[temp]-21));
#endif
							temp++;
						  }	/* end while */
					  }	/* end case */
					escflg=0;
					break;

				case 'n':               /* device status report (DSR) */
					switch(VSIw->parms[0]) {
#ifdef NOT_SUPPORTED
						case 0: /* response from vt100; ready, no malfunctions */
						case 3: /* response from vt100; malfunction, retry */
#endif

						case 5: /* send status */
							VTsendstat();
							break;

						case 6: /* send active position */
							VTsendpos();
							break;

					  }	/* end switch */
					escflg=0;
					break;

				case 'q':               /* load LEDs (unsupported) (DECLL) */
					escflg=0;
					break;

				case 'r':               /* set top & bottom margins (DECSTBM) */
					if(VSIw->parms[0]<0)
						VSIw->top=0;
					else
						VSIw->top=VSIw->parms[0]-1;
					if(VSIw->parms[1]<0)
						VSIw->bottom=VSPBOTTOM;
					else
						VSIw->bottom=VSIw->parms[1]-1;
					if(VSIw->top<0)
						VSIw->top=0;
					if(VSIw->top>VSPBOTTOM-1)
						VSIw->top=VSPBOTTOM-1;
					if(VSIw->bottom<1)
						VSIw->bottom=VSPBOTTOM;
					if(VSIw->bottom>VSPBOTTOM)
						VSIw->bottom=VSPBOTTOM;
					if(VSIw->top>=VSIw->bottom) {	/* check for valid scrolling region */
						if(VSIw->bottom>=1)		/* assume the bottom value has precedence, unless it is as the top of the screen */
							VSIw->top=VSIw->bottom-1;
						else				/* totally psychotic case, bottom of screen set to the very top line, move the bottom to below the top */
							VSIw->bottom=VSIw->top+1;
					  }	/* end if */
					VSIw->x=0;
					VSIw->y=0;
#ifdef NOT_SUPPORTED
					if (VSIw->DECORG)
						VSIw->y=VSIw->top;	/* origin mode relative */
#endif
					escflg=0;
					break;

#ifdef NOT_SUPPORTED
				case 'x':                       /* request/report terminal parameters (DECREQTPARM/DECREPTPARM) */
				case 'y':                       /* invoke confidence test (DECTST) */
					break;
#endif
				default:			/* Dag blasted strays... */
					escflg=0;
					break;

			  }	/* end switch */
			c++;
			ctr--;

/* @UM */
            if(VSIw->localprint && (ctr>0)) {    /* see if printer needs anything */
                pcount=send_localprint(c,ctr);
#ifdef NO_ECHO
        ctr-=pcount;
        c+=pcount;
#endif
              } /* end if */
/* @UM */
    } /* end while */
		while((escflg==3) && (ctr>0)) { /* #  Handling */
			switch(*c) {
				case 0x08:		/* backspace */
					VSIw->x--;
					if(VSIw->x<0)
						VSIw->x=0;
					break;

#ifdef NOT_SUPPORTED
				case '3':               /* top half of double line (DECDHL) */
				case '4':               /* bottom half of double line (DECDHL) */
				case '5':               /* single width line (DECSWL) */
				case '6':               /* double width line (DECDWL) */
					break;
#endif

				case '8':               /* screen alignment display (DECALN) */
					VTalign();
					escflg=0;
					break;

				default:
					escflg=0;
					break;

			  }	/* end switch */
			c++;
			ctr--;
		  }	/* end while */
		while((escflg==4) && (ctr>0)) { /* ( Handling (GO character set) */
			switch(*c) {
				case 0x08:		/* backspace */
					VSIw->x--;
					if(VSIw->x<0)
						VSIw->x=0;
					break;

				case 'A':               /* united kingdom character set (unsupported) */
				case 'B':               /* ASCII character set */
				case '1':               /* choose standard graphics (same as ASCII) */
					VSIw->G0=0;
					if(!VSIw->charset)
						VSIw->attrib=VSnotgraph(VSIw->attrib);
					escflg=0;
					break;

				case '0':               /* choose special graphics set */
				case '2':               /* alternate character set (special graphics) */
					VSIw->G0=1;
					if(!VSIw->charset)
						VSIw->attrib=VSgraph(VSIw->attrib);
					escflg=0;
					break;

				default:
					escflg=0;
					break;
			  }	/* end switch */
			c++;
			ctr--;
		  }	/* end while */
		while((escflg==5) && (ctr>0)) { /* ) Handling (G1 handling) */
			switch(*c) {
				case 0x08:		/* backspace */
					VSIw->x--;
					if(VSIw->x<0)
						VSIw->x=0;
					break;

				case 'A':               /* united kingdom character set (unsupported) */
				case 'B':               /* ASCII character set */
				case '1':               /* choose standard graphics (same as ASCII) */
					VSIw->G1=0;
					if(VSIw->charset)
						VSIw->attrib=VSnotgraph(VSIw->attrib);
					escflg=0;
					break;

				case '0':               /* choose special graphics set */
				case '2':               /* alternate character set (special graphics) */
					VSIw->G1=1;
					if(VSIw->charset)
						VSIw->attrib=VSgraph(VSIw->attrib);
					escflg=0;
					break;

				default:
					escflg=0;
					break;
			  }	/* end switch */
			c++;
			ctr--;
		  }	/* end while */
    if((escflg>2) && (ctr>0)) {
      escflg=0;
      c++;
      ctr--;
      } /* end if */
	  }	/* end while */
  VSIw->escflg=escflg;
}   /* end VSem() */

