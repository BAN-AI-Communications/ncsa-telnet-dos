/*
*    LOOK.C
*    User interface code for NCSA Telnet
****************************************************************************
*                                                                          *
*      NCSA Telnet for the PC                                              *
*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
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
*   10/1/87  Initial source release, Tim Krauskopf
*   7/5/88   Version 2.2 Tim Krauskopf
*   5/8/89   Version 2.3 Heeren Pathak & Quincey Koziol
*   1/21/91  Version 2.3 Chris Wilson & Quincey Koziol
*   6/19/91  Version 2.3 - Linemode reworked - Jeff Wiedemeier
*/

//#define UM
#define WINMASTER
#define REALTIME

#ifndef NOTEK
#define USETEK
#endif
/* #define USERAS */
#define HTELNET 23
/*#define DEBUG*/


#ifndef USETEK
#define leavetek() 0
#endif

#ifdef MSC
#define mousecl mousecml
#endif

#ifdef __TURBOC__
#include "turboc.h"
#endif
#ifdef MOUSE
#include "mouse.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <direct.h>
#include <time.h>
#include <conio.h>
#include <process.h>
#include <stdarg.h>
#include <bios.h>
#ifdef MSC
#include <malloc.h>
#endif
#ifdef CHECKNULL
#include <dos.h>
#endif

#include "whatami.h"
#include "nkeys.h"
#include "windat.h"
#include "hostform.h"
#include "protocol.h"
#include "data.h"
#include "confile.h"
#include "telopts.h"
#include "vidinfo.h"
#include "version.h"
#include "externs.h"

#include "map_out.h"

FILE *tekfp;
#ifdef MAL_DEBUG
FILE *mem_file;
#endif
FILE *console_fp;       /* file pointer for the console output when active */

//extern int default_mapoutput; /* Apparantly no longer used. */
extern int beep_notify;
extern int bypass_passwd;  /* flag to bypass the password check for ftp'ing back to our own machine */

long size,            /* size of buffer */
  cstart,             /* starting location of VS copy buffer */
  cend,               /* ending location of VS copy buffer */
  cdist,              /* distance to base point */
	temp;

int stand=0,          /* whether telnet is operating in 'standalone' mode (remains active when all sessions are closed) */
    sound_on=0,
    basetype=VTTYPE,
    ftpact=0,					/* ftp transfer is active */
    rcpact=0,					/* rcp transfer is active */
    viewmode=0,       /* What are we looking at? */
    prevmode=0,       /* What were we looking at? (before the help screen) */
    ftpok=1,					/* is file transfer enabled? */
    ftpwok=1,         /* incoming ftp */
    rcpok=1,					/* remote copying enabled? */
    scroll,           /* which way do we scrool */
    cflag=0,					/* are we copying ? */
    cbuf=0,						/* do we have things in the buffer? */
    mcflag,						/* mouse button flag */
    ginon=0,					/* the tektronics Graphics INput (GIN) mode flag */
    vsrow=0,					/* VS row */
    mighty,						/* mouse present? */
    temptek,					/* where drawings go by default */
    indev,outdev,     /* for copying tek images */
    rgdevice=0,       /* tektronix device to draw on */
    vton=1,
    capon=0,					/* overall capture on or not */
    foundbreak=0,     /* trap Ctrl-C */
    machparm=255,     /* the argument on the command line where the first machine is located */
    use_mouse=0,      /* use mouse for scrolling toggle */
    console_file_wanted=0,      /* should console be written to file ? */
    lpt_port=0;       /* port 0 == lpt1 (currently a hardwire) Also check out vsem.c */

int save_screen = TRUE;

unsigned char s[550],
    parsedat[256],
	path_name[_MAX_DRIVE+_MAX_DIR],		/* character storage for the path name */
    colors[NCOLORS]={4,2,0x70,0},       /* base colors */
	myipnum[4],
#ifdef CHECKNULL
    null_done=0,                /* flag to indicate that the null area has been snapshotted */
    nullbuf[1024],              /* buffer for debugging NULL overwrites */
#endif
    *copybuf=NULL,
    *lineend={"\r\n"},
    *blankline={"                                                                                       "};

unsigned int time_check=0;      /* variable to try to minimize the time spent checking the time */
time_t oldtime;					/* the last time the clock was updated */

long int lastt;

extern struct config def;		/* Default settings obtained from host file */

struct machinfo *mp=NULL;		/* Pointer to a machine information structure */

void CDECL (*attrptr)(int );    /* pointer to the routine to change the attribute of a character on the screen */

#define PSDUMP 128

#ifdef IP_ENHANCEMENTS					/* UNFINISHED message screen enhancements */
char *source_ip_num;					/* for error messages */
int port_num;               /* for error messages */
#endif

#define  USAGE   printf("\n\nUsage: telnet [-s] [-n] [-t] [-c color] [-h hostfile] [machinename] ...\n")

main(int argc,char *argv[])
{
    char temp_str[128];
    int i,j;
	char *config;

extern char keyboard_type;

    /* Check for CONFIG.TEL environment variable */
	config=(getenv("CONFIG.TEL"));
	if(config)
		Shostfile(config);

  /* work on parms */
  examineCommandLine(argc,argv);

  /* Initialize the video configuration, and retrieve it */
  initvideo();
  getvconfig(&tel_vid_info);
  getvstate(&tel_vid_state);

  if(save_screen)     /* save the screen if we are supposed to */
    init_text();

#ifdef __TURBOC__
	fnsplit(argv[0],path_name,s,temp_str,parsedat);		/* split the full path name of telbin.exe into it's components */
#else
	_splitpath(argv[0],path_name,s,temp_str,parsedat);	/* split the full path name of telbin.exe into it's components */
#endif
	strcat(path_name,s);	/* append the real path name to the drive specifier */

	install_keyboard();		/* determine what kind of keyboard this is */

  save_break();           /* preserve the status of the DOS BREAK setting */

  install_break((int *)&foundbreak);      /* install our BREAK handler */

  time(&oldtime);
#ifndef MOUSE
	mighty=initmouse();
#else
	mighty=nm_initmouse();
#endif
  n_clear();              /* do you know where your BP is? */
	n_cur(0,0);
  n_window(0,0,numline,79);   /* vt100 size */
	n_color(2);
  printf("%s, reading configuration file . . .\n",TEL_VERSION);

/*
* initialize the default keyboard settings
*/
#ifdef OLD_WAY /* Keyboard inits  rmg 931100 */
    if((j=initkbfile())<0) {    /* check for correct keyboard initialization */
        puts("Error reading default settings from keymap file.");
		getch();
    } /* end if */
#else
  memset(key_map_flags,0,1024);   /* initialize all the keyboard mapped flags to zero (not mapped) */
  memset(key_special_flags,0,1024); /* initialize all the keyboard special flags to zero (not special) */
#endif

  initoutputfile();   /* initialize the output mapping (to no mapping) */

/*
* initialize network stuff
*/
    if(j=Snetinit()) {  /* Read in the config.tel file and fire up the network */
		errhandle();
        printf("Error initializing network or getting configuration file\r\n");
		switch (j) {
			case -1:
                puts("Cannot initialize board.");
				break;

			case -5:
                puts("Error in config.tel file.");
				break;

			case -2:
                puts("RARP failed!!");
				break;

			case -3:
                puts("BOOTP failed!!");
				break;

			case -4:
                puts("X25 Initialization Failed!!");
				break;

			default:
                printf("Error from Snetinit()=%d\n",j);
				break;
          } /* end switch */
		if(j<-1)	/* network initialized, special case */
			netshut();
        remove_break();             /* restore the previous ctrl-c interupt handler */
        exit(1);
    }   /* end if */

#ifdef CHECKNULL
    /* Note: we snapshot the NULL area here, because some ethernet drivers modify the interrupt table when initialized */
    memcpy(nullbuf,MK_FP(0x0,0x0),1024);    /* make a copy of the NULL area */
    null_done=1;            /* set the flag for the NULL area */
#endif

    netgetip(myipnum);      /* what is my ip number? */
    Sgetconfig(&def);       /* get information provided in hosts file */

    ftpok=def.ftp;          /* what types of files transfers are allowed? */
    rcpok=def.rcp;
    ftpwok=def.ftpw;        /* Allow FTP writes? */
    for(i=0;i<3;i++)        /* set console colors to what's in config.tel */
      colors[i]=def.color[i];

    /* Check to see if we need to enter 43 or 50 line mode */
    if(def.ega43==1) {
		numline=41;
        ega43();
      } /* end if */
    else if(def.ega43==2) {
		numline=48;
        vga50();
      } /* end if */
	else {
		numline=23;				/***** NEED TO ADD SUPPORT FOR MODE CURRENT */
#ifndef AUX
    ega24();
#endif
	}

    n_window(0,0,numline,79);   /* vt100 size */

    if(def.cursortop!=-1 && def.cursorbottom!=-1)
		install_cursor((unsigned int)((def.cursortop<<8) | def.cursorbottom));

	if(Scmode())
		attrptr=n_attr;
	else
		attrptr=n_biosattr;

    n_clear();  /* clear the screen */

    for(i=0; i<NPORTS; i++)
		wins[i]=NULL;					/* we are using no window yet */

/*
* create console window for errors, informative messages, etc.
*/
    if(0>VSinit(NUM_WINDOWS))  {             /* initialize GPs virtual screens */
		n_puts("Virtual screen initialization failed.");
		exit(1);
    } /* end if */
    if(NULL==(console=creatwindow())) {
		n_puts("Console memory allocation failed");
		exit(1);
    }
    VSsetrgn(console->vs,0,0,79,numline);
    strcpy(console->mname,"Console");
/*
* introductions on the console window
*/
    i=console->vs;
    RSvis(-1);
    /* swapped around  rmg 931100 */
    //tprintf(i,"NCSA Telnet\r\n");
    vhead(i);
    tprintf(i,"\n\nConsole messages:\r\n");

#ifdef USETEK
/*
*  install tektronix
*/
	if(Stmode())
		tekinit(def.video);
#endif
#ifdef USERAS
	if(!VRinit())
        tprintf(i,"Error initializing raster support\r\n");
#endif
/*
*  Display my Ethernet (or Appletalk) and IP address for the curious people
*/
	pcgetaddr(&s[200],def.address,def.ioaddr);
    tprintf(console->vs,"My Ethernet address: %x:%x:%x:%x:%x:%x\r\n",s[200],s[201],s[202],s[203],s[204],s[205]);
    tprintf(console->vs,"My IP address: %d.%d.%d.%d\n\r\n",myipnum[0],myipnum[1],myipnum[2],myipnum[3]);

    Stask();                    /* any packets for me? (return ARPs) */
/*
*   With the parameters left on the command line, open a connection
*   to each machine named.  Start up all of the configuration on open
*   connections. 
*/
    for(i=machparm; i<argc; i++)
		addsess(argv[i]);

    if(current==NULL) {         /* special viewmode for server mode */
		current=console;		/* special enter sequence avoids flicker */
		viewmode=6;
      } /* end if */

    atexit(endall);             /* set the function to call when exitting */
    wrest(current);             /* paint the first session's screen */
    while(EXIT_TELNET!=dosessions());  /* serve all sessions, events */

    exit(0);
}   /* end main() */

void examineCommandLine(int argc, char *argv[])
{
    int i,j,size=0;

    for(i=1; i<argc; i++) {         /* look at each parm */
		if(*argv[i]=='-') {
			switch(*(argv[i]+1)) {
                case 'c':   /* set foreground color */
					i++;
					for(j=0; j<NCOLORS && *(argv[i]+j*2); j++)
						colors[j]=(unsigned char)hexbyte(argv[i]+j*2);
					break;

                case 'd':
					console_file_wanted=1;
                    console_fp=fopen("console.dmp","wb+");
#ifdef OLD_WAY
                    setbuf(console_fp,NULL);    /* unbuffer the console output */
#else
                    setvbuf(console_fp,NULL,_IONBF,0);
#endif
					break;

                case 'e':
                    if(commandLineOveride) {
                      size=strlen(commandLineOveride);
#ifdef AUX /* RMG */
fprintf(stdaux," commandLineOverride is on ");
#endif
                    }
                    commandLineOveride=(char *)realloc(commandLineOveride,strlen(commandLineOveride)+strlen(argv[++i])+3);
                    *(commandLineOveride+size)=(char)NULL;
                    strcat(commandLineOveride,argv[i]);
                    *(commandLineOveride+strlen(commandLineOveride)+1)=(char)NULL;
                    *(commandLineOveride+strlen(commandLineOveride))=';';
                    break;

                case 'n':   /* disable screen restore */
                    save_screen = FALSE;
                    break;

                case 's':   /* set standalone/server flag */
					stand=1;
					break;

                case 't':   /*  Disable direct writes to screen  */
					Scwritemode(0);
					break;

                case 'h':   /* set new name for host file */
                    Shostfile(argv[++i]);   
					break;

                case '?':   /* help screen */
				default:
					USAGE;
                    printf("\n\n -c 172471    sets the basic color scheme for console screen\n");
                    printf(" -h file      full path specification of host information file\n");
                    printf(" -s           standalone (server) mode for rcp and ftp\n");
                    printf(" -t           disable direct writes to screen\n");
                    printf(" -n           disable screen restore\n");
                    exit(1);
              } /* end switch() */
          } /* end if */
		else {
            machparm=i;         /* where first machine name is */
            break;
          } /* end else */
      } /* end for */
}   /* end examineCommandLine() */

/************************************************************************/
/* vhead
*  place the header for new sessions in.
*/
void vhead(int v)
{
    tprintf(v,"\r\nNational Center for Supercomputing Applications\r\n");
    tprintf(v,"%s for the PC\r\n",TEL_VERSION);
    tprintf(v,"\nAlt-H presents a summary of special keys \n\r\n");
}   /* end vhead() */

/************************************************************************/
/*  dosessions
*   dosessions is an infinite loop serving three sources of external
*   input in order of precedence:
*   - changes to the video display properties
*   - keyboard strokes
*   - network events
*
*   What the user sees is top priority followed by servicing the keyboard
*   and when there is time left over, looks for network events.
*
*   viewmode is a state variable which can determine if the screen should
*   be re-drawn, etc.
*/
int dosessions(void)
{
    static int scroll_row,scroll_col;   /* the row and column saved while in scroll back */
	time_t newtime;					/* variable to hold the new time for the clock */
	int clock_row,clock_col,		/* the row and column saved while updating the clock */
        i=0,j,ch,
        c,                          /* the color saved */
		cl,dat,cvs;
#ifdef BEFORE_GIN_FIX
    int m1,m2,m3,m4;        /* mouse variables */
#else
    int m1,m2,m3,m4,m3a,m4a;        /* mouse variables */
#endif
    int x1,x2,y1,y2;        /* temp variables for VSgetrgn test when exiting scrlback mode */
  unsigned int sw;
#ifdef BEFORE_GIN_FIX
  char gindata[5];    /* variable to store gin data in when returning fron VGgindata */
#else
  char gindata[6];    /* variable to store gin data in when returning fron VGgindata */
#endif
	struct twin *t1;
	static char lch=-1;
    char *p=NULL;
#ifdef CHECKNULL
    static unsigned int null_check=0;        /* don't try to check NULL every time */

/* If we are running in NULL debugging mode, check that */
    if(++null_check%8==0)
        check_null_area();
#endif

#ifdef USETEK
    if(ginon && current->termstate==TEKTYPE) { /* check for GIN mode for tektronics */
#ifdef BEFORE_GIN_FIX
      ch=n_chkchar();
      m1=3;
      m2=0;
      m3=0;
      m4=0;        /* mouse variables */
      mousecl(&m1,&m2,&m3,&m4);
      if((ch!=(-1)) || (m2&1)) {
        if((ch!='\r') && (ch!='\n') && (ch>0))
          lch=(char)ch;
        if((lch!=(-1)) && (m2&1)) {
          VGgindata(current->vs,m3,m4,lch,gindata);

#else /* this is gin_fix stuff */
      do {
        ch=kbhit();
        m1=3;
        m2=0;
        m3=0;
        m4=0;        /* mouse variables */
        mousecl(&m1,&m2,&m3,&m4);
      } while ((ch == 0) && (m2 == 0));  /* loop until mouse button or key pressed */

      if(ch==0)
        lch=(char)' ';  /* Mouse button pressed - return space */
      else
        lch=(char)getch();

    /* RMG ??? 940103 */
      if(ch==0)
        do {
          m1=3;
          m2=0;
          m3a=0;
          m4a=0;
          mousecl(&m1,&m2,&m3a,&m4a);
        } while (m2 != 0);  /*hold until mouse button released */

      VGgindata(current->vs,m3,m4,lch,gindata);       /* send the mouse cursor */
                                                      /* position to be translated */
                                                      /* by VGgindata with the */
                                                      /* information returned */
                                                      /* in gindata */
      gindata[5]='\r';                                /* add new-line char */
#endif /* before_gin_fix */

      RSsendstring(current->vs,gindata,6);    /* return GIN data across network */
      vt100key(0);

      lch=-1;                 /* clear last char buffer */
      resetgin();               /* reset GIN mode */
#ifdef BEFORE_GIN_FIX
              } /* end if */
          } /* end if */
#endif
    } /* end if */
#endif /* usetek */

	switch(viewmode) {
		case 0:					/* no special mode, just check scroll lock */
		default:
			if(n_scrlck()) {	/* scroll lock prevents text from printing */
				viewmode=1;
				vsrow=0;
				cstart=0;
				cend=0;
				cflag=0;
				cbuf=0;
				mcflag=0;
				scroll_row=n_row();
				scroll_col=n_col();
				c=n_color(current->colors[1]);
				n_cur(numline+1,61);
				n_draw("Scrl",4);	/* status in lower left */
				n_color(c);
				n_cur(scroll_row,scroll_col);
              } /* end if */
/*
*  This gives precedence to the keyboard over the network.
*/
            if((ginon) && (current->termstate==TEKTYPE))
                break;
            while(0<=(c=newkey(current)))   /* do all key translations */
				if(c==EXIT_TELNET)
          return(EXIT_TELNET);
			break;			/* no special viewing characterisitics */

		case 1:						/* scrollock is active */
			if(!n_scrlck()) {
				VSgetrgn(current->vs,&x1,&y1,&x2,&y2);
        if(y1!=0)
          VSsetrgn(current->vs,0,0,79,numline);
				viewmode=0;			/* set back if appropriate */
        if(y1==0)
          wrest(current);
				scroll=0;
				statline();
				n_cur(scroll_row,scroll_col);
      } /* end if */
/* 
*  In scroll lock mode, take keys only for the scrollback, 
*  The scrollback routine will never block, so we keep servicing events.
*/
      scrollback(current);
			break;

		case 2:						/* console is visible */
			if(n_scrlck()) {	/* scroll lock prevents text from printing */
				viewmode=13;
				vsrow=0;
				cstart=0;
				cend=0;
				cflag=0;
				cbuf=0;
				mcflag=0;
				scroll_row=n_row();
				scroll_col=n_col();
				c=n_color(console->colors[1]);
				n_cur(numline+1,61);
				n_draw("Scrl",4);	/* status in lower left */
				n_color(c);
				n_cur(scroll_row,scroll_col);
              } /* end if */
            if(0<n_chkchar())
				viewmode=10;	
			break;

		case 3:						/* help screen view1 */
		case 4:						/* help screen view2 */
			while(0<=(c=n_chkchar())) {	
				if(c==EXIT_TELNET)
					return(EXIT_TELNET);
				if(viewmode==3 && c==27) {
					viewmode=4;
          help2();
        } /* end if */
				else { 	 			/* restore view 0 */
          if(c==' ')
            viewmode=10;
#ifdef NOT /* rmg 931122 */
          else
            if(c>128)
              dokey(current,c); /* dokey might change view, if so, don't reset view 0 */
#endif
        } /* end else */
      } /* end while */
			break;

		case 5:						/* DOS screen view */
			viewmode=9;				/* wait for keypress */
			break;

		case 6:						/* server mode */
      statline();
      tprintf(console->vs,"\r\nServer mode, press ESC to exit or ALT-A to begin a session\r\n");
			viewmode=7;

		case 7:						/* server mode 2 */
			j=n_chkchar();
			switch(j) {
				case 27:
          tprintf(console->vs,"\n\r\n Ending server mode \r\n");
					return(EXIT_TELNET);	/* leave the program */

				case ALTA:
          if(!addsess(NULL))   /* start up a new one */
						viewmode=10;
          else {
						current=console;
						viewmode=6;
          } /* end else */
					break;

        case ALTE:      /* shell to dos */
					leavetek();
					n_window(0,0,numline+1,79);
					i=n_color(current->colors[0]);
					dosescape();
					wrest(console);
					viewmode=7;
					n_color(i);
					break;

        case ALTH:      /* help display (added to console window)  rmg 931100 */
          leavetek();
          prevmode=7;
          viewmode=3;
          helpmsg();
          break;

#ifdef FTP
        /* modified ALTW to query  rmg 931100 */
        case ALTW:        /* send our password to the machine we are connected to, for use in ftp'ing back to oneself */
          leavetek();
          if(bypass_passwd) {
            n_puts("\n OK then, we won't bypass the next FTP password after all.");
            bypass_passwd=0;
          }
          else {
            n_puts("\n Are you sure you want to bypass the FTP password once? Y/N");
            c=nbgetch();        /* get the answer */
            if(tolower(c)=='y') {
              n_puts("\n The next FTP connection will accept any password and give root access.");
              bypass_passwd=1;  /* set the flag to bypass the password check */
            } /* end if */
          }
          n_puts("\n Press any key to return");
          c=nbgetch();        /* get the answer */
          wrest(console);
          viewmode=7;      /* redraw screen */
          break;
#endif

				case -1:
					break;		/* no keypress */

				default:
          tprintf(console->vs,"\r\nYou must have an open session to use other commands.\r\n");
					viewmode=6;
					break;
      } /* end switch */
			break;

#ifdef USETEK
		case 8:
			if(graphit())				/* graphics menu screen */
				viewmode=10;
			break;
#endif

		case 9:							/* reset current screen on keypress */
			if(0<n_chkchar())
				viewmode=10;	
			break;

		case 10:						/* Display current screen */
			wrest(current);
			viewmode=0;					/* return to standard mode */
      /* for help screen in server window  rmg 931100 */
      if(prevmode==7) {
        viewmode=prevmode;
        prevmode=0;
      }
//      statline();
			break;

		case 11:						/* paste copy buffer to current session */
			temp-=netwrite(current->pnum,(char *)&copybuf[(int)(size-temp)],(int)temp);
			if(!temp)
				viewmode=0;
			break;

		case 13:
			if(!n_scrlck()) {
				VSsetrgn(current->vs,0,0,79,numline);
				viewmode=2;			/* set back if appropriate */
				scroll=0;
				wrest(console);
				statline();
				n_cur(scroll_row,scroll_col);
              } /* end if */
/* 
*  In scroll lock mode, take keys only for the scrollback, 
*  The scrollback routine will never block, so we keep servicing events.
*/
			scrollback(console);
			break;
      } /* end switch */

/*
*  Check for any relevant events that need to be handled by me
*/
	if(0<(i=Sgetevent(USERCLASS | CONCLASS | ERRCLASS, &cl, &dat))) {
		sw=cl*256+i;				/* class and event combination */
		cvs=console->vs;
		switch(sw) {
			case CONCLASS*256+CONOPEN:	/* a connection has just opened */
			    t1=wins[dat];			/* where the window pointer is stored */
				if(!t1)
					break;

				t1->sstat='/';			/* connection status */
				netpush(dat);

				/* Start negotiation on network */
                start_negotiation(t1,cvs);

				if(current!=t1) {
					current=t1;
					viewmode=10;
				  }
				break;

			case CONCLASS*256+CONCLOSE:	/* connection is closing */
				if(0<netqlen(dat))
					netputuev(CONCLASS,CONCLOSE,dat);  /* call me again */
					/* drop through, process any data */

			case CONCLASS*256+CONDATA:
				if(viewmode) {			/* not ready for it */
					netputuev(CONCLASS,CONDATA,dat);
					break;
				  }
			    t1=wins[dat];			/* where the window pointer is stored */
				if(!t1)
					break;

				if(inprocess(t1))
					return(EXIT_TELNET);
				break;

			case CONCLASS*256+CONFAIL:	/* can't open connection */
			    t1=wins[dat];			/* where the window pointer is stored */
				if(!t1)
					break;				/* this don't count */
                tprintf(cvs,"\r\nCan't open connection, timed out\r\n");
				netclose(dat);			/* close out attempt */
				if(!t1->next) {
					wrest(console);
					return(EXIT_TELNET);
				  }
				if(t1==current) {
					current=current->next;
					viewmode=10;
				  }
				delwindow(t1,1);
				statline();
				break;

/*
*  domain nameserver results.
*/
			case USERCLASS*256+DOMFAIL:			/* domain failed */
				mp=Slooknum(dat);				/* get machine info */
        tprintf(cvs,"\r\nDOMAIN lookup failed for: ");
				if(mp && mp->hname)
          tprintf(cvs,"%s\r\n",mp->hname);
				else 
					if(mp && mp->sname)
            tprintf(cvs,"%s\r\n",mp->sname);
				break;

			case USERCLASS*256+DOMOK:
				mp=Slooknum(dat);				/* get machine info */
				if(mp) {
          tprintf(cvs,"\r\nDOMAIN lookup OK for: ");   /* print session name and host name */
					if(mp->hname)
            tprintf(cvs,mp->hname);
					if(mp->sname) {
            tprintf(cvs," - %s",mp->sname);
						if(mp->port!=HTELNET) {
							char *s;

							if((s=(char *) malloc(strlen(mp->sname) + 6))!=NULL) {
								sprintf(s,"%s#%d",mp->sname,mp->port);
								mp->port=HTELNET;
								addsess(s);
								free(s);
							  }	/* end if */
						  }	/* end if */
						else
							addsess(mp->sname);
					  }
					else
						if(mp->port!=HTELNET) {
							char *s;

							if((s=(char *) malloc(strlen(mp->hname) + 6))!=NULL) {
								sprintf(s,"%s#%d",mp->hname,mp->port);
								mp->port=HTELNET;
								addsess(s);
								free(s);
							  }	/* end if */
						  }
						else
							addsess(mp->hname);
              tprintf(console->vs,"\r\nDomain Port is:%d\r\n",mp->port);
					viewmode=10;
				  }
				break;

/*
*  FTP status events.
*/
#ifdef FTP
			case USERCLASS*256+FTPBEGIN:		/* data connection */
                {
                    unsigned char host[4];

                    ftpact=dat;
                    Sftpname(s);                    /* get name */
                    Sftphost(host);
                    if (SftpDirection())
                        tprintf(cvs,"FTP: RECEIVING %s from host ", s);
                    else
                        tprintf(cvs,"FTP: SENDING %s to host ", s);
                    if((NULL==(mp=Slookip(host))) || (NULL==mp->sname))
                        tprintf(cvs,"%d.%d.%d.%d\r\n",host[0],host[1],host[2],host[3]);
                    else
                        tprintf(cvs,"%s\r\n",mp->sname);
                    ftpstart((char)(ftpact+2),s);
                    lastt=n_clicks();
                    break;
                }

			case USERCLASS*256+FTPLIST:			/* LIST or NLST */
        tprintf(cvs,"FTP directory beginning\r\n");
				break;

			case USERCLASS*256+FTPEND:			/* data connection ending */
				ftpact=0;
				statline();
        tprintf(cvs,"FTP transfer done\r\n");
				break;

			case USERCLASS*256+FTPCOPEN:		/* command connection */
        tprintf(cvs,"FTP server initiated from host: ");
				Sftphost(s);
				if((NULL==(mp=Slookip(s))) || (NULL==mp->sname)) 
                    tprintf(cvs,"%d.%d.%d.%d\r\n",s[0],s[1],s[2],s[3]);
				else
                    tprintf(cvs,"%s\r\n",mp->sname);
				break;

			case USERCLASS*256+FTPUSER:			/* user name entered */
                tprintf(cvs,"FTP user ");
        Sftpuser(s);
                tprintf(cvs,">> %s << login request\r\n",s);
				break;

      case USERCLASS*256+FTPANON:     /* passwordless user name entered */
                tprintf(cvs,"FTP user ");
				Sftpuser(s);
        Sftppass(parsedat);
                tprintf(cvs,">ANON< login request as: %s passwd: >> %s <<\r\n",s,parsedat);  /* parsedat is a rarely used global string */
				break;

      case USERCLASS*256+FTPPWOK:     /* user password verified */
                tprintf(cvs,"FTP Password verified\r\n");
				break;

      case USERCLASS*256+FTPPWSK1:     /* user password verified */
                tprintf(cvs,"FTP Password Skipped (no password file)\r\n");
				break;

      case USERCLASS*256+FTPPWSK2:     /* user password verified */
                tprintf(cvs,"FTP Password Skipped with Alt-W\r\n");
				break;

      case USERCLASS*256+FTPPWWT:     /* user password verified */
                tprintf(cvs,"FTP Password includes Write access\r\n");
				break;

      case USERCLASS*256+FTPPWRT:     /* user password verified */
                tprintf(cvs,"FTP Root access gained\r\n");
				break;

      case USERCLASS*256+FTPPWNO:     /* user password failed */
                tprintf(cvs,"FTP Password failed verification\r\n");
				break;

			case USERCLASS*256+FTPCLOSE:		/* command connection ends */
                tprintf(cvs,"FTP server ending session\r\n");
				break;
#endif

#ifdef RCP
			case USERCLASS*256+RCPBEGIN:		/* rcp starting */
                tprintf(cvs,"rcp file transfer\r\n");
				rcpact=1;
				break;

			case USERCLASS*256+RCPEND:			/* rcp ending */
                tprintf(cvs,"rcp ending\r\n");
				rcpact=0;
				break;
#endif

#ifdef USETEK
			case USERCLASS*256+PSDUMP:			/* dump graphics screen */
				if(VGpred(indev,outdev)) {
					if(dat) {
						endump();
                        tprintf(cvs,"Graphics writing finished\r\n");
						wrest(console);
						viewmode=2;
					  }
				  }
				else
					netputevent(USERCLASS,PSDUMP,dat);	/* remind myself */
				break;

#endif

      case ERRCLASS*256+ERR1:                     /* error message */
				p=neterrstring(dat);
				VSwrite(cvs,p,strlen(p));
                VSwrite(cvs,"\r\n",2);
                if(dat==407)
                    inv_port_err(0,0,NULL);

            default:
				break;
          } /* end switch */
      } /* end if */
/*
*	update the FTP spinner if we are in ftp, and update the clock
*/
	else {
#ifdef FTP
		if(ftpact && (n_clicks()>lastt + 10)) {
			ftpstart((char)(ftpact+2),s);
			lastt=n_clicks();
    }
#endif
		if(def.clock) {
      if((++time_check%4)==0 && (current->termstate<TEKTYPE) && (!ftpact)) { /* only check the time every four calls to dosessions to try to minimize the timer impact */
				time(&newtime);
				if(oldtime!=newtime) {
					strcpy(s,ctime(&newtime));
					clock_row=n_row();
					clock_col=n_col();
					c=n_color(current->colors[0]);
					n_cur(numline+1,72);
					set_cur(0);
					if(Scmode())	  /* check which screen writing mode to use */
						n_cheat(&s[11],9);
					else
						n_draw(&s[11],9);
					n_cur(clock_row,clock_col);
					n_color(c);
					oldtime=newtime;
					set_cur(1);
        } /* end if */
      } /* end if */
    } /* end if */
#ifdef NOT /* cRMG */
    if(scrolllocked) {
      ;
    }
#endif
  } /* end else */
	return(c);
}   /* end dosessions() */

/*********************************************************************/
/* inprocess
*  take incoming data and process it.  Close the connection if it
*  is the end of the connection.
*/
int inprocess(struct twin *tw)
{
	int cnt;

	cnt=netread(tw->pnum,s,200);	/* get some from incoming queue */
	if(cnt<0) {					/* close this session, if over */
		netclose(tw->pnum);

		if(tw->capon) {
			fclose(tw->capfp);		/* close the capture file */
			tw->capon=capon=0;
		  }
		n_color(tw->colors[0]);
		if(tw->next==NULL) 		/* if this is the last one */
			if(stand) {
				wins[tw->pnum]=NULL;
				freewin(tw);
				current=console;
				viewmode=6;
				wrest(current);
				return(0);
			  }	/* end if */
			else
				return(-1);		/* signal no sessions open */

#ifdef USETEK
		leavetek();					/* make Tek inactive */
#endif
		if(tw!=current)
			wrest(tw);

		if(!def.wingoaway)		/* check whether the window just goes away, or sticks around for a keypress */
			n_puts("\nConnection closed, press a key . . .");
		if(tw==current)
			current=tw->next;
		delwindow(tw,1);
		if(!def.wingoaway)		/* check whether the window just goes away, or sticks around for a keypress */
			viewmode=9;
		else
			viewmode=10;
		return(0);
      } /* end if */

    if(cnt)
        parse(tw,s,cnt);    /* display on screen, etc.*/
	return(0);
}   /* end inprocess() */

/*********************************************************************/
/* endall
*  clean up and leave
*/
void endall(void )
{
    netshut();      /* close down the network */

	n_cur(numline+1,0);			/* go to bottom of screen */
	n_color(7);
	n_draw(blankline,80);		/* blank it out */
	if(def.ega43>0)				/* restore screen */
		ega24();

	remove_break();				/* restore the previous ctrl-c interupt handler */
	restore_break();			/* restore the BREAK status */
    setvstate(&tel_vid_state);  /* restore the video state */
    if(save_screen)     /* is we saved the screen, restore it */
        end_text();
    if(console_file_wanted) /* is we were debugging the console, close the file */
        fclose(console_fp);
}   /* end endall() */

/*********************************************************************/
/*  errhandle
*   write error messages to the console window
*/
void errhandle(void )
{
	char *errmsg;
	int i,j;

	while(ERR1==Sgetevent(ERRCLASS,&i,&j)) {
		errmsg=neterrstring(j);
        tprintf(console->vs,"%s\r\n",errmsg);
        if(j==407)
			inv_port_err(0,0,NULL);
      } /* end while */
}   /* end errhandle() */

/*********************************************************************/
/*  vprint
*   print to a virtual screen
*/
void vprint(int w,char *str)
{
#ifdef CONSOLEDEBUG
	VSwrite(current->vs,str,strlen(str));		/* dump all messages to the current screen */
#else
    if(w==console->vs && console_file_wanted)
        fputs(str,console_fp);
    VSwrite(w,str,strlen(str));
#endif
}   /* vprint() */

/*********************************************************************/
/*  tprintf
*	print a formatted string to a virtual screen
*/
int tprintf(int w,char *fmt,...)
{
    char temp_str[256];     /* this may be a problem, if the string is too long */
    va_list arg_ptr;
    int str_len;            /* length of the formatted string */

	va_start(arg_ptr,fmt);		/* get a pointer to the variable arguement list */
    str_len=vsprintf(temp_str,fmt,arg_ptr); /* print the formatted string into a buffer */
	va_end(arg_ptr);
    if(str_len>0) {
        if(w==console->vs) {    /* write stuff to the console screen without going through the overhead of looking through the string for VT100 escape codes */
            if(console_file_wanted) {
                fputs(temp_str,console_fp);
                fflush(console_fp);
              } /* end if */
            VSdirectwrite(w,temp_str,str_len);
          } /* end if */
        else
#ifdef CONSOLEDEBUG
            VSwrite(current->vs,temp_str,str_len);     /* dump all messages to the current screen */
#else
            VSwrite(w,temp_str,str_len);
#endif
      } /* end if */
    return(str_len);
}   /* tprintf() */

/*********************************************************************/
/*  parsewrite
*   write out some chars from parse
*   Has a choice of where to send the stuff
*/
void parsewrite(struct twin *tw,char *dat,int len)
{
    char localdata[32];      /* local data for strings */
	int i;
/*
*  send the string where it belongs
*  1. Check for a capture file.  If so, echo a copy of the string
*  2. Check for dumb terminal type, convert special chars if so
*  3. Check for Tektronix mode, sending Tek stuff
*  3b. check for raster color type
*  4. or, send to virtual screen anyway
*/

#ifdef UM  /* #define UM statement at top of file is commented out */
	if (localprint) {
    for(i=0; i<len-3; i++) {
      if ((dat[i]==27)&&(dat[i+1]=='[')&&(dat[i+2]=='4')&&(dat[i+3]=='i')) {
        localprint=0;
      }
		}
		if (localprint)
			for(i=0; i<len; i++)
#ifdef MSC
        _bios_printer(_PRINTER_WRITE,lpt_port,dat[i]);
#elif __TURBOC__
				biosprint(0,ptr[i],0);
#endif
	}
#endif
	if(tw->capon)						/* capture to file? */
		fwrite(dat,len,1,tw->capfp);
/*
* raw mode for debugging, passes through escape sequences and other
* special characters as <27> symbols
*/
	if(tw->termstate==DUMBTYPE) {
		for (i=0; i<len; i++, dat++) 
			if(*dat==27 || *dat>126) {
                sprintf(localdata,"<%d>",*dat);
                VSwrite(tw->vs,localdata,strlen(localdata));
			  }
			else
				VSwrite(tw->vs,dat,1);
	  }
	else {
#ifdef USETEK
		if(tw->termstate==TEKTYPE) {
			i=VGwrite(temptek,dat,len);
			if(i<len) {
				leavetek();
				viewmode=10;
				parsewrite(tw,dat+i,len-i);
			  }
		  }				
		else 
#endif
#ifdef USERAS
			if(tw->termstate==RASTYPE) {
				i=VRwrite(dat,len);
				if(i<len) {
					tw->termstate=VTEKTYPE;
					parsewrite(tw,dat+i,len-i);
				  }
			  }
			else
#endif
        VSwrite(tw->vs,dat,len);  /* send to virtual VT102 */

	  }	/* end else */
}   /* end parsewrite() */

/*********************************************************************/
/* newkey
*  filter for command key sequences
*/
int newkey(struct twin *t1)
{
	int c;

	if(foundbreak) {
		foundbreak=0;
        c='\003';       /* ctrl-c */
        if(t1->lmflag) {       /* Telnet line mode is on */
            if(LMinterp_char(t1,c)) /* check if this character is a special line mode character */
                c=0;    /* don't want ^C on beginning of next line */
        } /* end if */
      } /* end if */
    else {
        if(ginon)
            c=0;
        else {
            if(t1->lmedit) {       /* Telnet line mode editting is on */
                c=LMgets(t1);
                if(c>0) {       /* check for an actual character */
#ifdef QAK
tprintf(console->vs,"Linemode on, c=%d, t1->linemode=%s\r\n",(int)c,t1->linemode);
#endif
                    if(LMinterp_char(t1,c))  /* check for a special line mode character */
                        c=0;    /* we handled this character */
                    else {  /* linemode routines didn't interpret this character */
                        if(t1->linemode[0]) {
#ifdef QAK
tprintf(console->vs,"c=%d, t1->linemode=%s\r\n",(int)c,t1->linemode);
#endif
                            netpush(t1->pnum);
                            netwrite(t1->pnum,t1->linemode,strlen(t1->linemode));
                            t1->linemode[0]='\0';
                          } /* end if */
                      } /* end else */
                  } /* end if */
              } /* end if */
            else {      /* no linemode */
                if(t1->echo)
                    c=n_chkchar();          /* a char available ? */
                else {
                    if(t1->halfdup) {       /* half duplex */
                        c=n_chkchar();
                        if(c==13) {
                            parse(t1,"\r\n",2); /* echo crlf */
                            vt100key(13);
                            c=10;
                          } /* end if */
                        else
                            if(c>0 && c<128)
                                parse(t1,(char *)&c,1);     /* echo char */
                      } /* end if */
                    else {                      /* kludge linemode */
                        c=RSgets(t1->vs,t1->linemode,79,1);
                        if(c==13) {         /* pressed return */
                            parse(t1,"\r\n",2);     /* echo the return */
                            strcat(t1->linemode,"\r\n");
                            netpush(t1->pnum);
                            netwrite(t1->pnum,t1->linemode,strlen(t1->linemode));
                            t1->linemode[0]='\0';
                            c=0;
                          } /* end if */
                        else {
                            if(c>0) {       /* write string, pass c to command interp */
                                if(t1->linemode[0]) {
                                    netpush(t1->pnum);
                                    netwrite(t1->pnum,t1->linemode,strlen(t1->linemode));
                                    t1->linemode[0]='\0';
                                  } /* end if */
                              } /* end if */
                          } /* end else */
                      } /* end else */
                  } /* end else */
#ifdef QAK
if(c!=(-1))
    tprintf(console->vs,"c=%u:%x\r\n",(unsigned int)c,(unsigned int)c);
#endif
              } /* end else */
          } /* end else */
      } /* end else */

	if(c<=0)
		return(c);
	return(dokey(t1,c));
}   /* end newkey() */

/************************************************************************/
/*  screendump
*   dump the contents of the current screen into the capture file
*
*/
void screendump(struct twin *t1,int whole_scrollback)
{
	int y,			/* variable to count the line of the screen we are dumping */
        line_num,   /* the number of lines to dump */
		len;		/* the length of the line with trailing blanks eliminated */
	unsigned char *c,*d;	/* temporary pointers into the text for a line */

	if(t1->capfp) {		/* make certain the file is open */
        if(VSvalids(t1->vs))    /* make certain there is a virtual screen to dump */
			return;
		capstat("Dump",1);
        if(whole_scrollback)        /* check if we want to dump the entire scroll scrollback */
            line_num=VSIw->lines;
        else
            line_num=numline+1;
        for(y=0; y<line_num; y++) {    /* dump each line */
			c=&VSIw->linest[y]->text[0];	/* get the address of the text line */
			d=c+VSIw->allwidth;			/* allwidth =79 for an 80 width window */
			while((*d==' ' || *d==0) && d>c)		/* don't print trailing blanks */
				d--;
			len=d-c+1;
            fprintf(t1->capfp,"%*.*s\r\n",len,len,c);   /* store the line */
		  }	/* end for */
        fprintf(t1->capfp,"\f");    /* print formfeed */
    statline();   /* re-print the statline */
	  }	/* end if */
}	/* end screendump() */

/************************************************************************/
/*  dokey
*   translates and sends keys and filters for command keys
*
*/
int dokey(struct twin *t1,int c)
{
	int i;

#ifdef QAK
tprintf(console->vs,"t1=%p, c=%d\n",t1,c);
#endif
    switch(c) {
		case BACKSPACE:		/* backspace */
			c=t1->bksp;		/* allows auto del instead of bs */
			break;

		case 13:			/* different CR mappings */
                  /*   even more different now - rmg 931100 */
      if(!t1->crfollow) {   /* most common case avoids switch statemant */
        vt100key(13);
        vt100key(10);
      }
      else {
        switch(t1->crfollow) {
          case 1:
            vt100key(13);
            break;
          case 2:
            vt100key(10);
            break;
          case 3:
            vt100key(13);
            vt100key(0);
            break;
        }
      }
			c=0;
			break;

		case 127:
			c=t1->del;		/* switch bs around, too */
			break;

		case THENUL:		/* user wants the true NUL char */
			c=0;
			netwrite(t1->pnum,(char *)&c,1);	/* write a NUL */
			break;

#ifdef USETEK
		case CTRLHOME:		/* tek clear screen */
		case E_CTRLHOME:	/* tek clear screen */
			if(def.tek) {	/* check whether we are allowed to go into tek mode */
                if(current->termstate!=TEKTYPE) {
					current->termstate=TEKTYPE;
					VGgmode(rgdevice);
					VGuncover(temptek);
                }
                VGwrite(temptek,"\033\014",2);  /* clear storage and screen */
				c=0;
			  }	/* end if */
			break;

		case HOME:			/* clear to text */
		case E_HOME:		/* clear to text */
			if(def.tek) {	/* check whether tektronix is enabled */
				if(leavetek()) {
					viewmode=10;
					c=0;
				  }
			  }	/* end if */
			break;
#endif

    case ALTA:        /* add session */
			c=0;
			if(0>addsess(NULL)) {		/* start up a new one */
        tprintf(console->vs,"\r\nPress any key to continue . . .");
				viewmode=9;
      }
			else
				viewmode=10;
			break;

		case ALTB:						/* session switch backwards */
			c=0;
			leavetek();
			if(current->prev==NULL)   /* are we only one ? */
				break;
			current=current->prev;
			viewmode=10;
			break;

		case ALTC:							/* toggle capture */
			if(capon && current->capon) {	/* already on */
				capstat("    ",0);
				fclose(current->capfp);		/* close the capture file */
				current->capon=capon=0;
			  }	/* end if */
			else 
				if(!capon) {				/* I want one */
					if(NULL==(current->capfp=Sopencap())) {
                        tprintf(console->vs,"\r\nCannot open capture file ");
						break;
					  }	/* end if */
					capstat("Capt",1);
					current->capon=capon=1;
				  }	/* end if */
				else {
                    tprintf(console->vs,"\r\nAnother session has capture file open, cannot open two at once\r\n");
					wrest(console);
					viewmode=2;
				  }	/* end else */
			c=0;
			break;

		case ALTD:		/* dump screen to capture file */
			c=0;
			if(!capon) {	/* if the capture file is not already open */
                if((current->capfp=Sopencap())==NULL) { /* try to open the capture file */
                    tprintf(console->vs,"\r\nCannot open capture file for screendump ");
					wrest(console);
					viewmode=2;
					break;
				  }	/* end if */
                screendump(current,0);    /* dump the current screen */
				fclose(current->capfp);
				current->capfp=NULL;
				break;
			  }	/* end if */
			if(current->capon && current->capfp)	/* ok, capture if on & the file is open */
                screendump(current,0);    /* dump the screen */
			else {
                tprintf(console->vs,"\r\nAnother session has a capture file open, cannot screendump\r\n");
				wrest(console);
				viewmode=2;
			  }	/* end else */
			break;

    case ALTE:    /* shell to commmand processor (DOS) */
			leavetek();
			n_window(0,0,numline+1,79);
			i=n_color(current->colors[0]); 
      if(!dosescape())
        viewmode=10;
      else
        viewmode=5;  /* error, pause */
			n_color(i);
			c=0;
			break;

#ifdef FTP
		case ALTF:							/* an ftp command */
			strcpy(s,"ftp ");
#ifdef AUX /* RMG */
fprintf(stdaux," ftpoptions %s ",current->ftpopts);
#endif
      //if((!Sneedpass()) && (current->ftpopts)) {  What's needpass() here for??
      if(current->ftpopts) {
				strcat(s,current->ftpopts);
				strcat(s," ");
      } /* end if */
      sprintf(&s[strlen(s)],"%d.%d.%d.%d\r\n",myipnum[0],myipnum[1],myipnum[2],myipnum[3]);
			netwrite(t1->pnum,s,strlen(s));
      if(!t1->echo)
        parse(t1,s,strlen(s));     /* echo the string */
			c=0;
			break;
#endif

#ifdef USETEK
		case ALTG:			/* graphics manipulation */
			if(Stmode()) {		/* make certain that tektronix has been initialized */
				c=0;
				leavetek();
				dispgr();
			  } /* end if */
			break;
#endif

		case ALTH:				/* help display */
			if(viewmode!=3) {
        leavetek();
        viewmode=3;
        helpmsg();
        c=0;
      } /* end if */
			break;

		case ALTI:							/* my internet address */
      sprintf(s,"%d.%d.%d.%d\r\n",myipnum[0],myipnum[1],myipnum[2],myipnum[3]);
			netwrite(t1->pnum,s,strlen(s));
      if(!t1->echo)
        parse(t1,s,strlen(s));      /* echo the string */
			c=0;
      break;

        case ALTJ:      /* for debuggin' purposes */
#ifdef MAL_DEBUG
            mem_file=fopen("c:\mem_dump","w");
            if(mem_file) {
                Mem_Display(mem_file);
                tprintf(console->vs,"wrote mem_file.\r\n");
                fclose(mem_file);
              } /* end if */
            else
                tprintf(console->vs,"failed mem_file write.\r\n");
#endif
#ifdef QAK
            mcb();
#endif
#ifdef QAK
            for(i=0; i<256; i++)
                fprintf(console_fp,"%d: %d -> %d\n",i,i,(int)outputtable[i]);
#endif
#ifdef CHECKNULL
            check_null_area();
#endif
            tprintf(console->vs,"numline=%d\n",numline);
            c=0;
			break;

		case ALTK:				/* erase char */
			netpush(t1->pnum);
			netwrite(t1->pnum,"\377\367",2);
			c=0;
            break;

        case ALTL:
            sound_on=!sound_on;
            c=0;
            break;
				
        case ALTM:                  /* mouse control on/off */
			c=0;
			use_mouse=!use_mouse;
			break;

		case ALTN:						/* session switch forwards */
			c=0;
			leavetek();
			if(current->next==NULL)   /* are we only one ? */
				break;
			current=current->next;
			viewmode=10;
			break;

		case ALTO:				/* abort output */
			netpush(t1->pnum);
			netwrite(t1->pnum,"\377\365",2);
			c=0;
			break;
				
		case ALTP:				/* change a parameter */
			parmchange();
			c=0;
			break;

		case ALTQ:				/* are you there? */
			netpush(t1->pnum);
			netwrite(t1->pnum,"\377\366",2);
			c=0;
			break;

		case ALTR:				/* reset screen values */
			if(!leavetek()) {
				if(current->capon) {
					fclose(current->capfp);
					current->capon=capon=0;
                  } /* end if */
				VSreset(current->vs);		/* reset's emulator */
              } /* end if */
			wrest(current);
			c=0;
			break;

		case ALTS:						/* skip to end */
			c=0;
			RSvis(0);
            while(0<(i=netread(t1->pnum,s,500)))
                parse(t1,s,i);
			viewmode=10;
			break;

    case ALTT:        /* reload keyboard file */
      c=0;
      if(read_keyboard_file(Scon.keyfile)<0) { /* rmg  940216 */
        leavetek();
        n_puts("\nError reading settings from keymap file.");
        n_puts("\n Press any key to return");
        nbgetch();
        wrest(console);
        viewmode=7;
      } /* end if */
      break;

		case ALTU:				/* erase line */
			netpush(t1->pnum);
			netwrite(t1->pnum,"\377\370",2);
			c=0;
			break;
				
		case ALTV:				/* paste clipboard into buffer */
			if(copybuf!=NULL) {
				temp=size;
				viewmode=11;
              } /* end if */
			c=0;
			break;

#ifdef FTP
    case ALTW:				/* send our password to the machine we are connected to, for use in ftp'ing back to oneself */
      leavetek();
      if(bypass_passwd) {
        n_puts("\n OK then, we won't bypass the next FTP password after all.");
        bypass_passwd=0;
      }
      else {
        n_puts("\n Are you sure you want to bypass the FTP password once? Y/N");
        c=nbgetch();        /* get the answer */
        if(tolower(c)=='y') {
          n_puts("\n The next FTP connection will accept any password and give root access.");
          bypass_passwd=1;  /* set the flag to bypass the password check */
        } /* end if */
      }
      n_puts("\n Press any key to return");
      c=nbgetch();        /* get the answer */
      viewmode=10;      /* redraw screen */
      c=0;
      break;
#endif

		case ALTX:						/* close the connection */
			leavetek();
			n_puts("\n Are you sure you want to close the connection? Y/N");
			c=nbgetch();				/* get the answer */
			if(tolower(c)=='y') {
				n_puts("\n Attempting to close . . .");
				netclose(t1->pnum);
				Stask();
				netputuev(CONCLASS,CONCLOSE,t1->pnum);
      } /* end if */
			else
				viewmode=10;			/* redraw screen */
			c=0;
			break;

		case ALTY:				/* interrupt */
			netpush(t1->pnum);
			netwrite(t1->pnum,"\377\364",2);
			t1->timing=1;
			netwrite(t1->pnum,"\377\375\006",3);
			c=0;
			break;

		case ALTZ:					/* view console */
			c=0;
			leavetek();
			wrest(console);
			viewmode=2;				/* console view mode */
			break;

#ifdef DEBUG
		case AF9:
      statcheck();
			break;
#endif

		case EXIT_TELNET:			/* abort Telnet */
			return(EXIT_TELNET);

		default:
			break;
	  }	/* end switch */

	if(c>0)
		vt100key(c);			/* send it, with VT100 translation */
	return(c);
}   /* end dokey() */

/***************************************************************************/
/*  dosescape
*  escape to dos for processing
*  put the connections to automated sleep while in DOS
*  As telnet has grown, this feature has deteriorated in practicality.
*/
int dosescape(void )
{
	int i;
	char *command_shell;		/* the command shell to shell out to */
  char oldDir[_MAX_PATH];
  struct vidstate shell_vid_state;   /* the video state before we shelled out */

	if(ftpact || rcpact) {
		n_puts("Please wait until file transfer is finished");
		n_puts("\nPress any key to continue");
    return 1;
      } /* end if */

    getcwd(oldDir,_MAX_PATH);

    getvstate(&shell_vid_state);   /* get the current video state */
    setvstate(&tel_vid_state);     /* restore the user's previous video state */

    n_clear();
	n_cur(0,0);
	n_puts("Warning, some programs will interfere with network communication and can");
    n_puts("cause lost connections.  Do not run any network programs from this shell.");
	n_puts("Type 'EXIT' to return to NCSA Telnet");

/*
*  invoke a put-to-sleep routine which calls netsleep every 8/18ths of a sec
*  Also:  disable ftp,rcp when asleep and suppress error messages
*/
	restore_break();		/* restore the break status */
    remove_break();         /* restore the previous ctrl-c interupt handler */

    netsleep(0);

    tinst();  /* Will crash if Stack checking is on */

  command_shell = getenv("COMSPEC");    /* search for the user's COMSPEC */

    if(command_shell!=NULL)     /* make certain that the COMSPEC exists */
      i=system(command_shell);        /* call DOS */
    else    /* tell the user if it doesn't */
      i=-1;

    tdeinst();

    chgdir(oldDir);

    install_break((int *)&foundbreak);      /* install our ctrl-c interupt handler */
    save_break();           /* save the break state again */

    setvstate(&shell_vid_state);    /* restore the telnet graphics state */

/* RMG */
#ifdef AUX
  i= -1;
#endif

    if(i<0) {
      n_puts("\n\nError loading DOS shell");
      n_puts("Make sure DOS shell is specified under COMSPEC.");
      n_puts(command_shell);
      //n_puts("It must also be in a directory which is in your PATH statement.");
      n_puts("Telnet does not always leave enough free memory for a DOS shell.");
      sprintf(s,"%d",_memavl());
      n_puts(s);
      n_row();
      n_puts("\nPress any key to return to telnet");
    } /* end if */

#ifdef Pauseme
    n_row();
    n_puts("\nPress any key to return to telnet"); /** rmg: no dont ***/
#endif

    viewmode=10;

#ifdef CHECKNULL
    for(i=0x88; i<=0x8b; i++)   /* patch to correct the change DOS makes to the INT 22h address when returning to the program */
        nullbuf[i]=*((unsigned char far *)i);
#endif

return(i<0);
}   /* end dosescape() */

/***********************************************************************/

void wrest(struct twin *t)
{
	RSvis(t->vs);
	statline();						/* done before, moves cursor */
    VSredraw(t->vs,0,0,79,numline);     /* redisplay, resets cursor correctly */
}   /* end wrest() */

/***************************************************************************/
void statline(void )
{
	struct twin *t1,*t2;
	int wn;
	int i,c,sm,rw,cl;

  if(current==NULL || current->termstate==TEKTYPE)
		return;
	c=n_color(current->colors[0]);			/* save current color */
	if(current->sstat!='*')
		current->sstat=254;					/* this is current one */
	rw=n_row();
	cl=n_col();
	t1=t2=current;
	wn=0;
	sm=Scmode();
  if(current!=console)  /* rmg 931122 */
    do {
      n_cur(numline+1,wn*15);
      n_color(t1->colors[2]);
      if(t1->sstat==254 && t1!=current)
        t1->sstat=176;
      n_putchar((char)t1->sstat);
      n_putchar(' ');
      i=strlen(t1->mname);
      if(i>=13) {   /* check for too long of a name */
        if(sm)
          n_cheat(t1->mname,13);        /* machine name of connection */
        else
          n_draw(t1->mname,13);       /* machine name of connection */
        } /* end if */
      else {
        if(sm) {
          n_cheat(t1->mname,i);       /* machine name of connection */
          n_cheat(blankline,13-i);    /* fill out to 13 spaces */
          }
        else {
          n_draw(t1->mname,i);        /* machine name of connection */
          n_draw(blankline,13-i);     /* fill out to 13 spaces */
          }
        } /* end else */
      if(t1->next)            /* if not the only window open */
        t1=t1->next;
      wn++;
    } while(t1!=current && wn<4);
	n_color(current->colors[0]);
	n_cur(numline+1,wn*15);
	if(def.clock)	/* if we have a clock, then only fill up to the edge of the capture indicator */
		if(sm)
			n_cheat(blankline,71-wn*15);			/* fill to edge of screen */
		else
			n_draw(blankline,71-wn*15); 			/* fill to edge of screen */
	else		/* for no clock, fill all the way across the bottom of the screen */
		if(sm)
			n_cheat(blankline,80-wn*15);			/* fill to edge of screen */
		else
			n_draw(blankline,80-wn*15);				/* fill to edge of screen */

	if(wn>3 && (t1!=t2)) {		/* check whether to display the '\' */
		i=176;		/* display a '\' if any of the windows has data and those windows don't appear on the statline, (windows>4) */
        while(t1!=t2 && i!=14) {
            if(t1->sstat!=254 && t1->sstat!=176)    /* check for a different status character */
				i=t1->sstat;
			if(t1->next)	/* go to the next window if there is one */
				t1=t1->next;
   } /* end while */
		n_color(current->colors[0]);
		n_cur(numline+1,71);
		n_putchar((char)i);
	  }	/* end if */
	else {	/* just display a blank */
		n_color(current->colors[0]);
        n_cur(numline+1,71);
		n_putchar((char)' ');
      } /* end else */

	if(current->capon)	  						/* put capture flag status */
		capstat("Capt",1);
	else
		capstat("    ",0);

	n_color(c);
	n_cur(rw,cl);
}   /* end statline() */

/***********************************************************************/
/*  creatwindow
*   returns a pointer to a new window
*/
struct twin *creatwindow(void )
{
	struct twin *p;
	int i;

	p=(struct twin *)malloc(sizeof(struct twin));
	if(p==NULL)
		return(NULL);
	p->pnum=-1;
	p->telstate=0;
	p->substate=0;
	p->termsent=0;
	if(vton)
		p->termstate=basetype;
	else
		p->termstate=DUMBTYPE;
	p->linemode[0]=0;
#ifdef OLD_WAY
	p->echo=1;
#else
    p->echo=0;
#endif
    p->ibinary=0;       /* I'm sending NVT ASCII data */
    p->iwantbinary=0;   /* I'm not the one who asked for binary transmission from me */
    p->ubinary=0;       /* Server is sending NVT ASCII data */
    p->uwantbinary=0;   /* I'm not the one who asked for server to begin binary transmission */
    p->ugoahead=0;      /* we want goahead suppressed */
	p->igoahead=0;		/* we want goahead suppressed */
	p->timing=0;
	p->capon=0;
	p->next=NULL;
	p->prev=NULL;
    p->lmflag=0;
    p->lmedit=0;        /* turn off the telnet linemode editting initially */
    p->litflag=0;       /* turn off the literal flag initially */
    p->litecho=0;       /* turn off the literal echo initially */
    p->softtab=0;       /* turn off the soft tabbing initially */
    p->trapsig=0;       /* turn off the signal trapping initially */
    p->linemode_mask=0; /* set the linemode mask to remote editting and signal processing */
    for(i=1; i<=NUMLMODEOPTIONS; i++) {
		p->slc[i]=-1;
        p->slm[i]=SLC_NOSUPPORT;
      } /* end for */
    p->rows=numline+1;
    p->slm[SLC_BRK]|=SLC_SUPPORTED;     /* set the supported flag for the SLC functions we can handle */
    p->slm[SLC_IP]|=SLC_SUPPORTED;
    p->slm[SLC_AO]|=SLC_SUPPORTED;
    p->slm[SLC_AYT]|=SLC_SUPPORTED;
    p->slm[SLC_ABORT]|=SLC_SUPPORTED;
    p->slm[SLC_EOF]|=SLC_SUPPORTED;
    p->slm[SLC_SUSP]|=SLC_SUPPORTED;
    p->slm[SLC_EC]|=SLC_SUPPORTED;
    p->slm[SLC_EL]|=SLC_SUPPORTED;
    p->slm[SLC_EW]|=SLC_SUPPORTED;
    p->slm[SLC_RP]|=SLC_SUPPORTED;
    p->slm[SLC_LNEXT]|=SLC_SUPPORTED;
    p->slm[SLC_XON]|=SLC_SUPPORTED;
    p->slm[SLC_XOFF]|=SLC_SUPPORTED;
    p->slm[SLC_FORW1]|=SLC_SUPPORTED;
    p->slm[SLC_FORW2]|=SLC_SUPPORTED;
    p->sstat='*';               /* connection not opened yet */

	if(mp==NULL) {
		p->bksp=127;
		p->del=8;
    p->crfollow=0;
		p->halfdup=0;
		p->vtwrap=1;
#ifdef OLD_WAY
		p->bkscroll=0;
#else
    p->bkscroll=500;
#endif
		p->width=80;
    p->slc[SLC_EC]=127;
    p->ftpopts=NULL;
    p->condebug=0;
    p->mapoutput=0;     /* turn off output mapping for console */

		for(i=0; i<3; i++)					/* start default colors */
			p->colors[i]=colors[i];

    /****!*!*!*!*!*!*!*!*!*!*!*!*****/
		i=VSnewscreen(500,1,80,0);			/* create a new virtual screen */
    if(i<0) {
        free(p);
        return(NULL);
      } /* end if */
    p->vs=i;
    screens[i]=p;                       /* we need to know where it is */
    VSsetlines(i,numline+1);
		VSsetrgn(i,0,0,79,numline);
  } /* end if */
	else {
		p->bksp=mp->bksp;
		if(p->bksp==127)
			p->del=8;
		else
			p->del=127;
		p->crfollow=mp->crmap;
		p->halfdup=mp->halfdup;
		p->vtwrap=mp->vtwrap;
		p->bkscroll=mp->bkscroll;
		p->width=mp->vtwidth;
		p->colors[0]=mp->nfcolor + (mp->nbcolor<<4);
		p->colors[1]=mp->ufcolor + (mp->ubcolor<<4);
		p->colors[2]=mp->bfcolor + (mp->bbcolor<<4);
    p->slc[SLC_EC]=mp->bksp;
    p->ftpopts=mp->ftpoptions;
    p->condebug=mp->consoledebug;
    p->mapoutput=mp->mapoutflag;     /* turn output mapping on if specified */

		i=VSnewscreen(mp->bkscroll,1,80,0);	/* create a new virtual screen */
        if(i<0) {
            free(p);
            return(NULL);
          } /* end if */
        p->vs=i;
        screens[i]=p;                       /* we need to know where it is */
        VSsetlines(i,numline+1);
		VSsetrgn(i,0,0,79,numline);

		if(i>=0 && mp->vtwrap)
			VSwrite(i,"\033[?7h",5);		/* turn wrap on */
		VSscrolcontrol(i,-1,mp->clearsave);	/* set clearsave flag */
      } /* end else */

#ifdef OLD_WAY
	if(i<0)
		return(NULL);
	p->vs=i;
	screens[i]=p;						/* we need to know where it is */
#endif
	return(p);
}   /* end creatwindow() */

/***********************************************************************/
/*  inswindow
*	insert a window into the circular linked list
*
*   current is used as a reference point for the new entry, the new entry
*   is put next in line past "current"
*/
void inswindow(struct twin *t,int wtype)
{
	struct twin *p,*q;

/*
*   put it into the port number array
*/
	if(wtype)
		wins[t->pnum]=t;

/*
*  check for the NULL case for current.
*/
	if(current==NULL || current==console) {
		current=t; 
		statline();
		return;
	  }
	p=current;					/* find surrounding elements */
	if(p->prev==NULL) {			/* only one now, we are adding 2nd */
		t->next=p;
		t->prev=p;
		p->next=t;
		p->prev=t;
	  }
	else {							/* adding third or more */
		q=p->next;				/* find next one */
		t->prev=p;
		t->next=q;				/* insert it as next after current */
		q->prev=t;
		p->next=t;
	  }
}   /* end inswindow() */

/***********************************************************************/
/*  delwindow()
*   take a window out of the linked list
*/
void delwindow(struct twin *t,int wtype)
{
	struct twin *p,*q;

	if(wtype)	
		wins[t->pnum]=NULL;		/* take out of array */
	p=t->prev;
	q=t->next;
	if(p==NULL) {				/* is only node */
		freewin(t);
		current=console;
		return;
	  }	/* end if */
	if(p==q) {				/* two in list */
		p->next=NULL;
		p->prev=NULL;
	  }	/* end if */
	else {
		q->prev=p;
		p->next=q;			/* merge two links */
	  }	/* end else */
	freewin(t);				/* release the space */
}	/* end delwindow() */

/************************************************************************/
/*  freewin
*   deallocate and detach all associated memory from a window
*/
void freewin(struct twin *t)
{
	VSdetatch(t->vs);
	free(t);
}	/* end freewin() */

/************************************************************************/
/*
*  hexbyte
*   return a byte taken from a string which contains hex digits
*/
int hexbyte(char *st)
{
	int i;

	if(*st>='A')
		i=((*st|32)-87)<<4;
	else
		i=(*st-48)<<4;
	st++;
	if(*st>'A')
		i|=(*st|32)-87;
	else
		i+=(*st-48);
	return(i);
}   /* end hexbyte() */

#ifdef USETEK

/***********************************************************************/
/* tekinit
*  tektronix initialization
*/
int tekinit(char *dev)
{
	if(strlen(dev)<1)
		return(0);
#ifdef OLD_WAY
	if(0>VGinit()) {
        tprintf(console->vs,"\r\nCannot initialize Tektronix driver\r\n");
		return(-1);
	  }
	else
        tprintf(console->vs,"\r\nTektronix initialized\r\n");
#else
	VGinit();
    tprintf(console->vs,"\r\nTektronix initialized\r\n");
#endif
	if(!strcmp(dev,"vga")) 
		rgdevice=7;
	  	/* end if */
	else if(!strcmp(dev,"no9"))
		rgdevice=4;
	else if(!strcmp(dev,"ega"))
		rgdevice=1;
	else if(!strcmp(dev,"hercules"))
		rgdevice=3;
	else if(!strcmp(dev,"cga") || !strcmp(dev,"pga"))
		rgdevice=5;
	else
		rgdevice=0;				/* null device */
	basetype=VTEKTYPE;
	temptek=VGnewwin(rgdevice);	/* default for drawing */
	return(0);
}   /* end tekinit() */

/***********************************************************************/
/*  function to write to file
*/
void fdump(char *str)
{
	fputs(str,tekfp);
}   /* end fdump() */

void fdumpc(int c)
{
	fputc(c,tekfp);
}   /* end fdumpc() */

void endump(void )
{
	VGclose(outdev);
	if(indev!=temptek) {
		VGclose(indev);
		if(tekfp) {
			fclose(tekfp);
			tekfp=NULL;
		 }
	  }
}   /* end endump() */

/***********************************************************************/
/* graphit
*  Get some user choices and execute them
*/
int graphit(void )
{
	int i,j,k,l,c;

	c=n_chkchar();
	if(c==27)
		return(1);
	if(c<0)
		return(0);
	switch (c) {
		case F2:
		case F4:
		case F6:	/* prompting for file name */
			n_puts("\nEnter new file name:");
			nbgets(s,50);
			if(s[0] && s[0]!=' ') {
				switch(c) {
					case F2:
						Snewpsfile(s);
						break;

					case F4:
						Snewhpfile(s);
						break;

					case F6:
						Snewtekfile(s);
						break;
				  }
				Sgetconfig(&def);
			  }
			dispgr();						/* leave in graphit mode */
			return(0);

		case F1:			/* postscript dump */
			if(*def.psfile=='+') {
				if(NULL==(tekfp=fopen(&def.psfile[1],"a")))
					return(1);
				fseek(tekfp,0L,2);      /* to end */
			  }
			else 
				if(NULL==(tekfp=fopen(def.psfile,"w"))) 
					return(1);
#ifdef MSC
			RGPoutfunc(fdump);		/* set function */
#else
			RGPoutfunc(&fdump);		/* set function */
#endif
			outdev=VGnewwin(2);
			indev=temptek;
			temptek=VGnewwin(rgdevice);
			VGgmode(2);
			VGzcpy(indev,temptek);
			VGzcpy(indev,outdev);
			VGuncover(outdev);
			VGpage(outdev);
			if(VGpred(indev,outdev))
				endump();
			else
				netputevent(USERCLASS,PSDUMP,1);		/* remind myself */
			return(1);
					
		case F3:			/* HPGL dump */
			if(*def.hpfile=='+') {			/* append feature */
				if(NULL==(tekfp=fopen(&def.hpfile[1],"a"))) 
					return(1);
				fseek(tekfp,0L,2);
			  }
			else 
				if(NULL==(tekfp=fopen(def.hpfile,"w"))) 
					return(1);
#ifdef MSC
			RGHPoutfunc(fdump);		/* set function */
#else
			RGHPoutfunc(&fdump);		/* set function */
#endif
			outdev=VGnewwin(6);
			indev=temptek;
			temptek=VGnewwin(rgdevice);
			VGgmode(6);
			VGzcpy(indev,temptek);
			VGzcpy(indev,outdev);
			VGuncover(outdev);
			VGpage(outdev);
			if(VGpred(indev,outdev))
				endump();
			else
				netputevent(USERCLASS,PSDUMP,1);		/* remind myself */
			return(1);

		case F5:			/* tektronix dump */
			if(*def.tekfile=='+') {
				if(NULL==(tekfp=fopen(&def.tekfile[1],"ab"))) 
					return(1);
				fseek(tekfp,0L,2);
			  }
			else 
				if(NULL==(tekfp=fopen(def.tekfile,"wb"))) 
					return(1);
			fputs("\033\014",tekfp);
#ifdef MSC
			VGdumpstore(temptek,fdumpc);
#else
			VGdumpstore(temptek,&fdumpc);
#endif
			fclose(tekfp);
			return(1);

		case F7:			/* tek view region */
			n_puts("\nEnter 0-4095 for lower left xy, upper right xy.");
			n_puts("\nExample:  0,0,4095,3119 is full view. (default if you leave it blank)");
			nbgets(s,30);
			if(4!=sscanf(s,"%d,%d,%d,%d",&i,&j,&k,&l))
				VGzoom(temptek,0,0,4096,3119);
			else
				VGzoom(temptek,i,j,k,l);
			dispgr();						/* leave in graphit mode */
			return(0);

		case 13:
			if (!def.tek) break;
			current->termstate=TEKTYPE;
			VGgmode(rgdevice);
			VGuncover(temptek);
			outdev=temptek;				/* redraw to itself */
			indev=temptek;
			tekfp=NULL;
			if(!VGpred(indev,outdev))
				netputevent(USERCLASS,PSDUMP,0);		/* remind myself */
 			viewmode=0;					/* normal logon state */
			break;

		default:
			break;
	}
	return(0);
}   /* end graphit() */
#endif /* usetek */

/*************************************************************************/
/*  addsess
*   Add a session to a named machine, or prompt for a machine to be named.
*/
int addsess(char *st)
{
	int i,new,cv,pflag=0,port=0;
	struct twin *newin;

	leavetek();
	cv=console->vs;
	if(st==NULL) {			/* no machine yet */
		wrest(console);
    tprintf(cv,"\n\r\nEnter new machine name/address, ESC to return: \r\n");
		s[0]='\0';
		while(0>=(i=RSgets(cv,s,70,1)))
			Stask();
		if(i==27 || !s[0])
			return(1);
        tprintf(cv,"\n\r\n");    /* skip down a little */
		st=s;					/* make a copy of the pointer to s */
  }
/*
*	Find out what port to open to
*/
    for(i=0; (st[i]!=' ') && (st[i]!='#') && (st[i]!='\0'); i++);

	if((st[i]=='#') || (st[i]==' ')) {
		st[i++]='\0';
		new=i;
		pflag=1;
    for( ; (st[i]!='\0') && isdigit(st[i]); i++);
 		if(st[i]!='\0') 
			pflag=0;
		if(pflag) 
			port=(unsigned int)atoi(&st[new]);
  } /* end if */
	mp=Sgethost(st);			/* gain access to host information */
	errhandle();
	if(!mp) {
		if(pflag)
			st[strlen(st)]='#';	/* Append port number */
		if(Sdomain(st)>0) 
      tprintf(cv,"\r\nQuerying the DOMAIN name server\r\n");
		else {
      tprintf(cv,"\r\nNo nameserver, cannot resolve IP address\r\n");
			return(-1);
    }
  } /* end if */
	else {
/*
*   tell user about it on the console
*/
    tprintf(cv,"\r\nTrying to open TCP connection to: %s\r\n",st);
		if(!pflag) 
			port=mp->port;

	/* try to serve the request */
    if(0>(new=Snetopen(mp,port))) {
			errhandle();
      tprintf(cv,"\r\nCould not open new connection to: %s\r\n",st);
			return(-1);
    } /* end if */
		newin=creatwindow();
		if(!newin) {				/* mem error */
      tprintf(console->vs,"\r\nMemory Allocation error for window\r\n");
      return(-1);
    } /* end if */
		newin->pnum=new;
		strncpy(newin->mname,st,14);
		newin->mname[14]='\0';
		inswindow(newin,1);
		vhead(newin->vs);
  } /* end else */
	return(0);
}   /* end addsess() */

#ifdef USETEK
int leavetek(void )
{
    if(current!=NULL && current->termstate==TEKTYPE) {
		VGwrite(temptek,"\037",1);			/* force to alpha */
		current->termstate=VTEKTYPE;
		VGtmode(rgdevice);
		resetgin();				/* make sure to reset the GIN mode */
								/* clear graphics mode */
        if(def.ega43==1)
			ega43();
        else if(def.ega43==2)
            vga50();
		return(1);
    }
	return(0);					/* we did nothing */
}   /* end leavtek() */
#endif

/***********************************************************************/
/*  capstat
*	Print the capture status on the screen
*/
void capstat(char *str,int i)
{
	int r,c,color;

	r=n_row();
	c=n_col();
	color=n_color(current->colors[i]);
	n_cur(numline+1,66);
	n_draw(str,strlen(str));
	n_color(color);
	n_cur(r,c);
}   /* end capstat() */

/***********************************************************************/
/*  set_vtwrap
*	QAK - 7/29/90
*	set the vtwrap parameter for any telnet window, added because changing
*	a virtual screen's auto-wrapping had no way to change the telnet window's
*	wrapping state
*/
void set_vtwrap(int vs_win,int value)
{
	struct twin *temp_window;

	temp_window=screens[vs_win];		/* get a pointer to the window for the virtual screen */
	if(temp_window!=NULL)
		temp_window->vtwrap=(unsigned char)value;		/* set the virtual screen's wrapping */
}	/* end set_vtwrap() */

/***********************************************************************/
/*  strchar
*   QAK - 2/24/92
*   Append a character to a string
*/
void strchar(char *s,char c)
{
    while(*s)
        s++;    /* find the terminating zero in the string */
    *s++=c;     /* replace the terminating zero with the character */
    *s='\0';    /* terminate the new string */
}   /* end strchar() */

#ifdef CHECKNULL
void check_null_area(void)
{
    unsigned u;
    unsigned char *s;

    if(null_done && memcmp(nullbuf,MK_FP(0x0,0x0),1024)) {
        puts("NULL overwritten!");
        s=MK_FP(0x0,0x0);
        for(u=0; u<1024; u++,s++)
            if(*s!=nullbuf[u])
                printf("s=%p, u=%u, val=%u\n",s,u,(unsigned int)*s);
        getch();
      } /* end if */
}   /* end check_null_area() */
#endif

#ifdef DEBUG
void heapdump(void)
{

	struct _heapinfo hinfo;
	long free_mem=0;
	int heapstatus;

	hinfo._pentry = NULL;
	while ((heapstatus = _heapwalk(&hinfo)) == _HEAPOK){
		free_mem+= (hinfo._useflag == _USEDENTRY ? 0 : hinfo._size);
		printf("%6s block at %p of size %4.4X\n",
				(hinfo._useflag == _USEDENTRY ? "USED" : "FREE"),
				hinfo._pentry, hinfo._size);

		}
	switch(heapstatus){
		case _HEAPEMPTY:
			printf("OK - empty heap\n\n");
			break;
		case _HEAPEND:
			printf("OK - end of heap \n\n");
			break;
		case _HEAPBADPTR:
			printf("ERROR - bad pointer to heap \n\n");
			break;
		case _HEAPBADBEGIN:
			printf("ERROR - bad start of heap \n\n");
			break;
		case _HEAPBADNODE:
			printf("ERROR - bad node in heap \n\n");
	}
	printf("free_mem=%ld\n",free_mem);
}   /* end heapdump() */

void print_windowlist(void )
{

	struct twin *p, *q;

	wrest(console);
	n_clear();
	p=q=current;
	printf("List of Machines following NEXT list\n");
	do{
		printf("Host is %s\n",p->mname);
		p=p->next;
		} while(p!=q);
	printf("Hit any key for prev list\n");
	getch();
	n_clear();
	p=q=current;
	printf("List of Machines following PREV list\n");
	do{
		printf("Host is %s\n",p->mname);
		p=p->prev;
		} while(p!=q);
	printf("Hit any key to return to normal\n");
	getch();
}   /* end print_windowlist() */
#endif

