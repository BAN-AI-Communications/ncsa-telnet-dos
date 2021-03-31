/***********************************************************************/
/*  menu support
*   New 5/22/88 - TK
*   Provide menus with a reasonable user interface for the user to
*   select colors or other parameters.
*
*   This menu system provides two types of entries.
*   1. Up to nine static choices, 0-8.  Each static choice is encoded
*      into the data structure and automatically rotated by the user.
*      The user cannot select an illegal value.
*   2. A string choice.  A maximum string length is honored.
*   There must be at least 20 characters open for each field, longer
*   if the field is longer.  Static choices cannot be longer than 20 chars.
*
*	Code Cleanup for 2.3 release - 6/89 QK
*/

/*
*	Includes
*/
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
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
#include "menu.h"
#include "telopts.h"
#include "version.h"
#include "externs.h"

#ifndef USETEK
#define leavetek() 0
#endif

extern unsigned char s[550];
extern struct config def;
extern int ftpok, ftpwok, rcpok, temptek, viewmode;

#define noalpha				/* for alpha test, both alpha and beta must be defined */
#define nobeta

#define LO 4
static struct pt pc[]={		/* session colors */
	{LO+0,44,0,0,"black","blue","green","cyan","red","magenta","yellow","white","BLACK","BLUE","GREEN","CYAN","RED","MAGENTA","YELLOW","WHITE",NULL},
	{LO+1,44,0,0,"black","blue","green","cyan","red","magenta","yellow","white","BLACK","BLUE","GREEN","CYAN","RED","MAGENTA","YELLOW","WHITE",NULL},
	{LO+2,44,0,0,"black","blue","green","cyan","red","magenta","yellow","white","BLACK","BLUE","GREEN","CYAN","RED","MAGENTA","YELLOW","WHITE",NULL},
	{LO+3,44,0,0,"black","blue","green","cyan","red","magenta","yellow","white","BLACK","BLUE","GREEN","CYAN","RED","MAGENTA","YELLOW","WHITE",NULL},
	{LO+4,44,0,0,"black","blue","green","cyan","red","magenta","yellow","white","BLACK","BLUE","GREEN","CYAN","RED","MAGENTA","YELLOW","WHITE",NULL},
	{LO+5,44,0,0,"black","blue","green","cyan","red","magenta","yellow","white","BLACK","BLUE","GREEN","CYAN","RED","MAGENTA","YELLOW","WHITE",NULL},
/* things which also apply to this session */
	{LO+6,44,0,0,"Local echo","Remote echo",NULL},
	{LO+7,44,1,0,"Backspace","Delete",NULL},
	{LO+8,44,0,20,NULL,"                    ",NULL},
	{LO+9,44,0,0,"VT102 and Tek4014","Dumb TTY","VT102 only",NULL},
	{LO+10,44,0,0,"Wrapping Off","Wrapping On",NULL},
	{LO+11,44,0,0,"Mapping Off","Mapping On",NULL},
/* things which apply over all of telnet */
	{LO+13,44,0,35,NULL,"                                   ",NULL},
	{LO+14,44,1,0,"Use BIOS","Direct to screen",NULL},
	{LO+15,44,0,0,"Disabled","Enabled",NULL},
	{LO+16,44,0,0,"Disabled","Enabled",NULL},
	{LO+17,44,0,0,"Disabled","Enabled",NULL},
	{LO+18,44,0,0,"Disabled","Enabled",NULL},
    {LO+0,0}
};

/************************************************************************/
/* menuit
*  draw the current line at the required position.
*  If the field length (replen) is longer than 20 chars, fill in the
*  entire field width.  All fields are padded with spaces to their
*  length when printed, but stored without padding.
*/
void menuit(struct pt p[],int n)
{
	int i;
	char fmt[12];

	n_cur(p[n].posx,p[n].posy);
	if((i=p[n].replen)<20) 			/* i=larger of replen and 20 */
		i=20;
	sprintf(fmt,"%%-%d.%ds",i,i);				/* create format string */
	sprintf(s,fmt,p[n].vals[p[n].choice]);		/* put out value */
	n_puts(s);
}

/************************************************************************/
/*  makechoice
*   Allow the user to travel between choices on the screen, selecting
*   from a list of legal options.
*   Arrow keys change the selections on the screen.  The data structure
*   must be set up before entering this procedure.
*/
int makechoice(struct pt p[],int maxp,int spec)
{
	int i,oldln,ln,c;
	
	oldln=ln=0;
	n_color(7);
	for(i=1; i<maxp; i++) 		/* print the opening selections */
		menuit(p,i);
/*
*  For each keystroke, repaint the current line and travel around the
*  data structure until the exit key is hit.
*/
	do {
		if(oldln!=ln) {
			n_color(7);					/* re-write old line in normal color */
			menuit(p,oldln);
		  }
		n_color(0x70);
		menuit(p,ln);					/* write the current line in reverse */
		if(spec)
			makespecial();				/* display special requirements */
		n_cur(p[ln].posx,p[ln].posy);	/* reset cursor to current entry */
		oldln=ln;
		c=nbgetch();
		switch (c) {			/* act on user's key */
			case CURUP:
			case E_CURUP:
				if(ln)
					ln--;
				else
					ln=maxp-1;
				break;

			case CURDN:			/* up and down change current field */
			case E_CURDN:
				if(++ln>=maxp)
					ln=0;
				break;

			case PGUP:
			case HOME:
			case E_PGUP:
			case E_HOME:
				ln=0;
				break;

			case PGDN:
			case ENDKEY:
			case E_PGDN:
			case E_ENDKEY:
				ln=maxp-1;
				break;

			case 32:				/* space, tab and arrows change field contents */
			case TAB:
			case CURRT:
			case E_CURRT:
				i=++p[ln].choice;		/* if blank or non-existant, reset to 0 */
				if(!p[ln].vals[i] || !(*p[ln].vals[i]) || ' '==*p[ln].vals[i])
					p[ln].choice=0;
				break;

			case CURLF:
			case E_CURLF:
				if(p[ln].choice)
					p[ln].choice--;
				else {				/* if at zero, search for highest valid value */
					i=0;
					while(p[ln].vals[i] && *p[ln].vals[i] && ' ' != *p[ln].vals[i])
						i++;
					if(i)
						p[ln].choice=i-1;
				  }
				break;

			case BACKSPACE:
			case 21:
			case 13:				 	/* BS, Ctrl-U, or return */
/*
*  if allowed, the user can enter a string value.
*  prepare the field, by printing in a different color to set it apart.
*/
				if(p[ln].replen) {
					char *temp_val;		/* temporary storage for the field value */

					p[ln].choice=1;
					n_color(1);						/* underline color */
					n_cur(p[ln].posx,p[ln].posy);
					for(i=0; i<p[ln].replen; i++)  /* blank out field */
						n_putchar(' '); 
					n_cur(p[ln].posx,p[ln].posy);
					temp_val=(char *)malloc((unsigned int)p[ln].replen+1);	/* allocate space for the temporary field */
                    if(temp_val!=NULL) {
                        strcpy(temp_val,p[ln].vals[1]);     /* store copy of old line */
                        if(nbgets(p[ln].vals[1],p[ln].replen)==NULL)    /* check for user escaping */
                            strcpy(p[ln].vals[1],temp_val); /* restore backup copy */
                        free(temp_val);
                      } /* end if */
				  }
				break;

			default:
				break;
	   	  }
	  } while(c!=F1 && c!=F10 && c!=27);
	return(c==F1);
}

/************************************************************************/
/*  makespecial
*  Apart from standard menuing, we want to show an example of what the
*  text is going to look like in each of the three attributes.
*/
void makespecial(void)
{
	n_cur(LO-1,15);
	n_color(pc[0].choice+(pc[1].choice <<4));
	n_puts("normal");
	n_cur(LO-1,25);
	n_color(pc[2].choice+(pc[3].choice <<4));
	n_puts("reverse");
	n_cur(LO-1,35);
	n_color(pc[4].choice+(pc[5].choice <<4));
	n_puts("underline");
}

/************************************************************************/
/*  parmchange
*   ALT-P from the user calls this routine to prompt the user for
*   telnet parameter values.
*   Set up the menuing system with the current values for this session.
*   Call the menuing routines, then analyze the results when it returns.
*
*   Affects the settings of:
*   session color, name, local echo, backspace/del, terminal type
*   overall file transfer enable, capture file name, screen access method
*
*/
void parmchange(void)
{
	int i,colsave;
/*
*  set up the screen for the menus
*  Positions of text interlock with fields of menu routines
*/
	leavetek();
	colsave=n_color(7);
	n_clear();
	n_cur(0,0);
	n_puts("ALT-P                         Parameter menu ");
	n_puts("   Arrows select, Enter clears \"*>\" fields.  F1 to accept, ESC to abort.");
	n_puts("    -------------- Color setup and session parameters ----------------- ");
	n_puts(" Text: ");
	n_puts("          Normal Foreground (nfcolor) - ");
	n_puts("          Normal Background (nbcolor) - ");
	n_puts("         Reverse Foreground (rfcolor) - ");
	n_puts("         Reverse Background (rbcolor) - ");
	n_puts("       Underline Foreground (ufcolor) - ");
	n_puts("       Underline Background (ubcolor) - ");
	n_puts("        Use remote echo or local echo - ");
	n_puts("                  Backspace key sends - ");
	n_puts("                         Session name *>");
	n_puts("                        Terminal type - ");
	n_puts("                        Line wrapping - ");
	n_puts("                       Output Mapping - ");
	n_puts("   -------------- Parameters which apply to all sessions -------------- ");
	n_puts("                    Capture file name *>");
	n_puts(" Screen mode (for BIOS compatibility) - ");
	n_puts("                     File transfer is - ");
	n_puts("    Incoming File transfer writes are - ");
	n_puts("                    Remote Copying is - ");
	n_puts("                             Clock is - ");
/*
*  set values for menus from our telnet-stored values
*/
	i=current->colors[0];		/* session colors */
	pc[0].choice=i&15;
	pc[1].choice=i>>4;
	i=current->colors[2];
	pc[2].choice=i&15;
	pc[3].choice=i>>4;
	i=current->colors[1];
	pc[4].choice=i&15;
	pc[5].choice=i>>4;
	pc[6].choice=current->echo;
	if(current->bksp==8)		/* backspace setting */
		pc[7].choice=0;
	else
		pc[7].choice=1;
	pc[8].vals[0]=current->mname;		/* session name */
    pc[9].choice=current->termstate-1;  /* terminal type */
    pc[10].choice=current->vtwrap;      /*line wrapping */
	pc[11].choice=current->mapoutput;	/* output mapping on for this session */
	pc[12].vals[0]=def.capture; 	/* capture file name */
	pc[13].choice=Scmode(); 		/* screen write mode */
	pc[14].choice=ftpok;      /* filetransfer enable */
  pc[15].choice=ftpwok;     /* allow FTP writes */
	pc[16].choice=rcpok;			/* remote copying enable */
	pc[17].choice=def.clock;		/* is the clock on? */
	if(makechoice(pc,18,1)) {		/* call it and check the results */
/*
*  Work on results, only if user pressed 'F1', if ESC, this is skipped.
*/
/*
*  assign new colors
*/
        i=pc[0].choice+(pc[1].choice<<4);   /* normal color */
        if(i!=(int)(current->colors[0])) {
			current->colors[0]=i;
			RSsetatt(127,current->vs);		/* seed the current screen */
			n_color(current->colors[0]);	/* seed ncolor */
          } /* end if */
        current->colors[1]=pc[4].choice+(pc[5].choice<<4);
        current->colors[2]=pc[2].choice+(pc[3].choice<<4);
/*
*  check remote or local echo mode
*/
        if((pc[6].choice)!=(int)(current->echo)) {
			if(current->echo=pc[6].choice) {		/* assign=is on purpose */
				netpush(current->pnum);
                netprintf(current->pnum,"%c%c%c",IAC,DOTEL,ECHO);   /* telnet negotiation */
              } /* end if */
			else {
				netpush(current->pnum);
                netprintf(current->pnum,"%c%c%c",IAC,DONTTEL,ECHO);
              } /* end else */
          } /* end if */
/*
*  check function of backspace or delete
*/
		if(pc[7].choice) {
			current->bksp=127;		/* backspace/delete are swapped */
			current->del=8;
          } /* end if */
		else {
			current->bksp=8;			/* are normal */
			current->del=127;
          } /* end else */
/*
*  check new session name
*/
		if(pc[8].choice) {
			strcpy(s,pc[8].vals[1]);
			if(s[0]!=' ' && s[0]) {		/* limit of 14 chars stored */
				strncpy(current->mname,s,15);
				current->mname[14]=0;
              } /* end if */
			*pc[8].vals[1]=0;
			pc[8].choice=0;
          } /* end if */
/*
*  check terminal type
*/
		if(pc[9].choice!=current->termstate-1) 
			current->termstate=pc[9].choice+1;
/*
*    check for line wrapping
*/
        if((pc[10].choice)!=(int)(current->vtwrap)) {
			current->vtwrap=(unsigned char)pc[10].choice;
			if(pc[10].choice)
				VSwrite(current->vs,"\033[?7h",5);
			else 	
				VSwrite(current->vs,"\033[?7l",5);
          } /* end if */
/*
*	 check for output mapping
*/
        if((pc[11].choice)!=(int)(current->mapoutput)) {
            netpush(current->pnum);
            if(current->mapoutput=pc[11].choice) {   /* assign=is on purpose */
                netprintf(current->pnum,"%c%c%c",IAC,DOTEL,BINARY);   /* telnet negotiation */
                netprintf(current->pnum,"%c%c%c",IAC,WILLTEL,BINARY);
              } /* end if */
			else {
                netprintf(current->pnum,"%c%c%c",IAC,DONTTEL,BINARY);
                netprintf(current->pnum,"%c%c%c",IAC,WONTTEL,BINARY);
              } /* end else */
            current->uwantbinary=1;  /* set the flag indicating we wanted server to start transmitting binary */
            current->iwantbinary=1;  /* set the flag indicating we want to start transmitting binary */
          } /* end if */
/*
*  check for new capture file name
*/
        if(pc[12].choice) {
			strcpy(s,pc[12].vals[1]);
			if(s[0] && s[0]!=' ') {	/* no NULL names */
				Snewcap(s);
				Sgetconfig(&def);
              } /* end if */
			*pc[12].vals[1]=0;
			pc[12].choice=0;
          } /* end if */
/*
*  check for new screen mode, BIOS or not
*/
		if(pc[13].choice!=Scmode())
			Scwritemode(pc[13].choice); /* set to whatever choice is */
/*
*  check whether to enable or disable file transfers
*/
#ifdef FTP
    if(pc[14].choice!=ftpok) {
      ftpok=pc[14].choice;
      Sftpmode(ftpok);
          } /* end if */
    if(pc[15].choice!=ftpwok) { /* for incoming writes to disk  rmg 931100 */
      ftpwok=pc[15].choice;
      setftpwrt(ftpwok);
          } /* end if */
    if(pc[16].choice!=rcpok) {
      rcpok=pc[16].choice;
      Srcpmode(rcpok);
          } /* end if */
#endif
/*
*	check whether to enable or disable the clock
*/
        if((pc[17].choice)!=(int)(def.clock))
			def.clock=pc[17].choice;
      } /* end if */
/*
*  go back to telnet
*/
	n_color(colsave);
	wrest(current);
}   /* end parmchange() */

/*********************************************************************/
void helpmsg(void )
{
	int i;

/*  leavetek();  */
  i=n_color(7);  /* easy to see.  rmg 931100 */
	n_clear();
	n_cur(0,0);
/* RMG these should be reorganized */
  n_puts("Keyboard usage for NCSA telnet:");
	n_puts("Alt-A     Add a Session                    Alt-Y     Interrupt Process");
	n_puts("Alt-N     Next Session                     Alt-B     Previous Session");
	n_puts("Alt-D     Dump Screen to Capture File      Alt-O     Abort Output");
	n_puts("Alt-M     Toggle mouse control             Alt-Q     Are you there?");
	n_puts("Alt-E     Escape to COMMAND shell          Alt-U     Erase line");
	n_puts("Alt-G     Graphics Menu                    Alt-K     Erase Kharacter");
  n_puts("Alt-C     Toggle Capture On/Off            Alt-V     Paste clipbord copy");
	n_puts("Alt-R     Reset VT102 Screen               HOME      Exit Graphics Mode");
  n_puts("Alt-H     This Help Screen                 Ctrl-HOME Clear/Enter Graphics mode");
  n_puts("Alt-Z     Messages Screen                  Alt-T     Reload keymap file");
  n_puts("ScrLock   Enter/Exit Scroll-Back Mode (also pauses/restarts screen)");
  n_puts("            in Scroll-Back: block begin/end= <space>, Alt-C= copy");
  n_puts("            to later paste text copied to the clipbord, use Alt-V");
  n_puts("Alt-F     Start File Transfer as if typed: ftp [my internet address]");
	n_puts("Alt-I     Send My Internet Address to host as if typed");
	n_puts("Alt-S     Skip Scrolling, Jump Ahead");
  n_puts("Alt-P     Change a Parameter, one of: colors, permissions, capture file name,");
  n_puts("            backspace, session name, screen mode...");
	n_puts("Alt-X	  Close Connection");
	n_puts("Ctrl-Shift-F3    Abort program completely.  STRONGLY discouraged");
  n_puts("\nPress ESC for information page, space bar to return to session:");
	n_color(i);
}   /* end helpmsg() */

void help2(void )
{
	int i;
  char line[80];

  i=n_color(7);
	n_clear();
	n_cur(0,0);
  sprintf(line,"%s, For the IBM PC.  %s",TEL_VERSION,TEL_REV_VERSION);
  n_puts(line);
	n_puts("\nNational Center for Supercomputing Applications, University of Illinois");
	n_puts("written by Tim Krauskopf, Gaige B. Paulsen, Aaron Contorer, Heeren Pathak");
    n_puts("    Kurt Mahan, Quincey Koziol, Chris Wilson, & Ryan Grant\n");
#ifndef beta
	n_puts("This program is in the public domain.");
	n_puts("Please retain the following notice:");
	n_puts("\n  Portions developed by the National Center for Supercomputing Applications");
	n_puts("  at the University of Illinois, Urbana-Champaign.");
#else
#ifndef alpha
	n_puts("Authorized beta test version - open to technical users at your own risk.");
  n_puts("Please report all problems that you wish fixed before VERSION_DEADLINE");
	n_puts("All rights reserved.");
#else
	n_puts("Authorized alpha test version - open to technical users at your own risk.");
  n_puts("Please report all problems that you wish fixed before VERSION_DEADLINE");
	n_puts("All rights reserved.");
#endif
#endif
  n_puts("\n\nFor information or for disks and hardcopy manuals (there is a handling fee),");
	n_puts("contact NCSA at:");
	n_puts("152 Computing Applications Building");
	n_puts("605 E. Springfield Ave.");
	n_puts("Champaign, IL 61820");
  n_puts("\nbugs and suggestions to pctelnet@ncsa.uiuc.edu");
	n_puts("\nPress space bar to return to session");
	n_color(i);
}   /* end help2() */
