/*
*  User FTP
*  6/8/87
****************************************************************************
*                                                                          *
*      by Tim Krauskopf and Swami Natarajan                                *
*			 Microsoft port by Heeren Pathak							   *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
*                                                                          *
****************************************************************************
*
*/
 
/*
 *Thanks to Jyrki Kuoppala(jkp@hutcs.hut.fi) for the basis of changes for
 * MSC 5.1 
*/

#define FTPMASTER

#ifdef __TURBOC__
#include "turboc.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <io.h>
#include <errno.h>
#include <string.h>
#include <conio.h>

#ifdef	MSC
#define	O_RAW O_BINARY
#include <dos.h>
#ifndef _TURBOC_
#include <direct.h>
#endif
#include <malloc.h>
#endif

#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif
#include "nkeys.h"
#include "hostform.h"
#include "whatami.h"
#include "ftppi.h"              /* list of commands, help strings */
#include "defines.h"
#include "vidinfo.h"
#include "version.h"
#include "externs.h"

#define FASCII  0
#define FIMAGE  1
#define HFTP   21

#define FALSE		0
#define TRUE		1
#define SUCCESS		2
#define HAVEDATA    4
#define ERROR      -1
#define NONE	   -2
#define ABORT	   -3
#define INCOMPLETE -4
#define AMBIGUOUS  -5

int xp=0,					/* general pointer */
	towrite=0,				/* file transfer pointer */
	len=0,					/* file transfer length */
	ftpnum,					/* current command port */
	ftpdata=-1,				/* current data port */
	ftpfh,					/* file handle for ftp */
	ftpstate=0,				/* state for background process */
  fcnt=0,                 /* counter for ftpd */
	ftpfilemode=0,			/* file open mode for transfer */
	foundbreak=0,			/* cntrl-break pending */
	connected=0,			/* not connected */
	debug=0,				/* debug level */
	hash=0,					/* hash mark printing */
	sendport=1,				/* to send ports or not */
	verbose=1,				/* informative messages */
	redirect=1,				/* check for ouput redirection */
	bell=0,					/* sound bell */
	autologin=1,			/* login on connect */
	prompt=1,				/* check on multiple commands */
	tempprompt=0,			/* temp var for Yes/No/All/Quit convenience */
	glob=1,					/* expand wildcards */
	slashflip=1,			/* change \ to / */
	capture=0,				/* capture data or not */
	usemore=0,				/* use |more */
  numlines=24,      /* number of lines for |more */
  lineslft=24,      /* number of lines left for |more */
	ttypass=0,				/* use interactive for password */
	fromtty=1,				/* default input from tty */
	f_enter_login=FALSE,	/* noninteractive but ask for user and pass */
	display_init=FALSE,		/* TRUE if title line has been given */
  scrsetup=0,       /* FALSE if need to clear screen */
	dos_color;				/* storage for the DOS color setup */

FILE *fromfp=NULL;			/* file pointer for input */
unsigned int curftpprt=0;	/* port to use */
unsigned char destname[50]={0,0},	/* who to connect to */
	s[550],					/* temporary string */
	path_name[_MAX_DRIVE+_MAX_DIR],		/* character storage for the path name */
	captlist[2001],			/* response string */
	ftpcommand[200];		/* command to execute */
char printline[2001];		/* line to display */

struct config def;

char *config;

#ifdef _TURBOC_
int use_mouse=0;
#endif

#ifdef _TURBOC_
#define BUFFERS 10000		/* size of buffer */
#else
#define BUFFERS 20000		/* size of buffer */
#endif
#define READSIZE 256		/* how much to read */

static unsigned char xs[BUFFERS+10];	/* buffer space for file transfer */

long start=0L,				/* timing var */
	lengthfile=0L;			/* length of current file for transfer */

/* function prototypes for local functions */
int main(int argc,char *argv[]);
static char *stpblk(char *ch);
static void breakstop(void );
static unsigned int ftpport(void );
static char *stptok(char *p,char *toword,int len,char *delim);
static void nputs(char *line);
static int printerr(void );
static int ftppi(char *command);
static int putstring(char *string);
static int ftpgets(char *s,int lim,int echo);
static int ftpdo(char *s,char *ofile);
static int checkevent(void );
static void nputchar(char ch);
static int dumpcon(int cnum);
static void telnet(int cnt);
static int getword(char *string,char *word);
static int finduniq(char *name,char *list[],int listsize);
static int checkoredir(char *command,char *filename,int slashflip);
static void flip_slashes(char *command,char *filename);
static void getdir(int drive,char *path);
static int ftpreplies(int cnum,int *rcode);
static int getnname(char *string,char *word);
static int rgetline(int cnum);
static void userftpd(void );
int numrows(void);
void give_args(void);
int domore(void);
static char *check_file_name (char *in_fname, char *out_fname);

/************************************************************************/
/* main-main procedure.  Displays opening message, parses arguments,
/*	  initializes network, reads user commands and executes them, and
/*	  cleans up.
/************************************************************************/
main(int argc,char *argv[])
{
	int i,c;
	static char Configfile[50]="config.tel";
	static char fromfile[20]="";

    /* Initialize the video configuration, and retrieve it */
    initvideo();
    getvconfig(&tel_vid_info);

    n_window(0,0,tel_vid_info.rows-1,tel_vid_info.cols-1);   /* to correctly handle non standard screen sizes (changed to work with n_puts  rmg 931100) */
#ifdef __TURBOC__
	fnsplit(argv[0],path_name,s,captlist,printline);   /* split path up */
#else
    _splitpath(argv[0],path_name,s,captlist,printline); /* split the full path name of telbin.exe into it's components */
#endif
	strcat(path_name,s);	/* append the real path name to the drive specifier */

    config=getenv("CONFIG.TEL");
    if(config)
        strcpy(Configfile,config);

	destname[0]='\0';		/* destination unknown */

    for(i=1; i<argc; i++) {     /* parse arguments */
        if(argv[i][0]=='-' || argv[i][0]=='/') {
			switch(tolower(argv[i][1])) {
				case 'r':		/* turn off output redirection */
					redirect=FALSE;
					break;

				case 'v':		/* display informative messages */
					verbose=TRUE;
					break;

				case 'n':		/* do not login on connect */
					autologin=FALSE;
					break;

				case 'i':		/* interactive prompting off */
					prompt=FALSE;
					break;

				case 'g':		/* wildcard expansion off */
					glob=FALSE;
					break;

				case 'd':		/* debug, optional level */
					if(sscanf(argv[++i],"%d",&debug)<=0)
						debug=TRUE;
					break;

				case 's':		/* do not change \ to / */
					slashflip=FALSE;
					break;

				case 'h':		/* host file name */
					strcpy(Configfile,argv[++i]);
					break;

				case 'm':		/* use built in |more */
					printf("\nusing more...");
					usemore=TRUE;
          lineslft=numlines=tel_vid_info.rows-1;  /* rmg 940108 */
					printf("\nusing more...");
					break;

				case 'e':		/* noninteractive commands with interactive login */
                    f_enter_login=TRUE;

				case 'p':		/* noninteractive with interactive password */
					ttypass=TRUE;

				case 'f':						/* noninteractive, optional filename */
          fromtty=FALSE;  /* noninteractive input */
					strcpy(fromfile,argv[++i]);
					if(fromfile[0]) {
			   			fromfp=fopen(fromfile,"r");
			   			if(fromfp==NULL) {
                sprintf(printline,"Could not open file: %s",fromfile);
                nputs(printline);
                return(1);
              } /* end if */
          } /* end if */
					break;

				case '?':
					give_args();

				default:						/* unknown option */
					sprintf(printline,"Unrecognized option -%c ignored",argv[i][1]);
					nputs(printline);
					break;
              } /* end switch */
          } /* end if */
        else
            sscanf(argv[i],"%s",destname);     /* destination host */
      } /* end for */

#ifdef OLD_WAY
    *strrchr(argv[0],'\\')==0;    /* put null at end of pathname string */
	Shostpath(argv[0]);				/* and set the path */
#endif
	Shostfile(Configfile);		/* open host file */


/*
*  initialize network
*/
    if(c=Snetinit()) {      /* cannot initialize network */
        printerr();         /* display TCP/IP message */
		nputs("Error initializing network");
		if(c==-3)	/* check for BOOTP server not responding */
			netshut();	/* release network */
		return(1);
      } /* end if */

    install_break((int *)&foundbreak);      /* install our BREAK handler */

    Sgetconfig(&def);               /* get information provided in hosts file */
	if(destname[0]) 
		sprintf(ftpcommand,"open %s",destname);	/* if destination specified, connect to it */
  nputs("");                  /* initializes screen if not done */

	do {
    if(*ftpcommand) {
      ftppi(ftpcommand);      /* if command available, execute it */
      if(debug>4)
        nputs("after returning from ftppi() ");
    } /* end if */
#ifdef NOT
    if(fromtty)
      n_cur(n_row(),0);        /* in case screen messed up */
#endif
    checkevent(); /* be sure closed */
    userftpd();   /* be sure stats seen */
    putstring("ftp> ");       /* prompt */
    lineslft=numlines;              /* for |more */
    c=ftpgets(ftpcommand,200,1);  /* read cmd from user */
  } while(c!=ABORT);                /* Alt-F3 aborts */
	netclose(ftpnum);					/* close command connection */
	netshut();							/* terminate network stuff */
    remove_break();                     /* restore previous break handler */

	return(0);
}   /* end main() */

void give_args(void) {
	nputs("FTP [-dfghimnprsv?]  [hostname]");
    nputs("     d-Debug [level]");
    nputs("     f-input File <filename>");
    nputs("     g-Global wildcard expansion off");
    nputs("     h-Host file <filename>");
    nputs("     i-Interactive prompting off");
    nputs("     m-use built in More");
    nputs("     n-No login on connect");
    nputs("     p-input file with interactive Password <filename>");
    nputs("     r-turn off command line output Redirection");
    nputs("     s-turn off Slash flip (/\\)");
    nputs("     v-Verbose");
    nputs("     ?-This list");
	exit(1);
}   /* end give_args() */

/**********************************************************************
* ftpgets-read a line from the keyboard
*       returns ABORT if aborted, non-zero on success
* char *s;            where to put the line
* int lim,echo;       max chars to read, echo?
************************************************************************/
static int ftpgets(char *s,int lim,int echo)
{
	int c,count,i;
	char *save, *ret;

	count=0;		/* none read */
	save=s;			/* beginning of line */

	if(foundbreak) {
        foundbreak=0;
		*s='\0';
		nputs("");
		return(ABORT);
      } /* end if */
    if(!fromtty) {
        if(fromfp==NULL)
            ret=fgets(s,lim,stdin);
        else
            ret=fgets(s,lim,fromfp);
        if(ret==NULL) {
            nputs("EOF or error on read from file\n");
            if(connected) {
                ftpdo("QUIT","");
                connected=FALSE;
              } /* end if */
            netclose(ftpnum);                   /* close command connection */
            netshut();
            remove_break();
#ifdef NOT
            if(display_init) {                 /* Restore DOS' color scheme */
                n_color(dos_color);
                n_clear();          /* clear screen */
#ifdef NOT
                n_wrap(1);          /* cursor positioning */
#endif
                n_cur(0,0);
              } /* end if */
#endif
            exit(1);
          } /* end if */
        s[strlen(s)-1]='\0';     /* remove newline */
        if(echo && fromfp)
            nputs(s);
        return(strlen(s));
      } /* end if */
    while(1) {
        if(foundbreak) {        /* abort */
            s=save;             /* reset line */
			*s='\0';			/* null line */
      nputs("");      /* newline */
			foundbreak=0;		/* break processed */
			return(ABORT);	
		  }
		/* SAR 9/13/90 */
        if(!kbhit()) {         /* if no char available */
			checkevent();		/* check event queue */
            c=0;
          } /* end if */
        else
            c=getch();         /* else get character */
		switch(c) {				/* allow certain editing chars */
			case BACKSPACE:
			case 8:				/* backspace */
				if(count) {
					if(echo) {
            n_putchar((char)8);
            n_putchar(' ');
            n_putchar((char)8);
                      } /* end if */
					count--;	/* one less character */
					s--;		/* move pointer backward */
                  } /* end if */
				break;

			case EOF:
			case 13:			/* carriage return,=ok */
				nputs("");		/* newline */
				*s='\0';		/* terminate the string */
				return(c);		/* return ok */

			case 10:			/* line feed */
				break;			/* ignore */

			case 21:			/* kill line */
				for(i=0; i<s-save; i++) {	/* length of line */
					if(echo) {				/* erase */
						nputchar(8);
						nputchar(' ');
						nputchar(8);
                      } /* end if */
                  } /* end for */
				s=save;	/* reset line */
				break;

			case 0:				/* do nothing */
				break;

			default:						/* not special char */
				if(c>31&&c<127) {			/* printable */
					if(echo) nputchar((char)c);	/* display */
					*s++=(char)c;				/* add to string */
					count++;	/* length of string */
                  } /* end if */
				else			/* acts as eol */
					return(c);	/* value of special char */
				if(count==lim) {			/* to length limit */
					*s='\0';	/* terminate */
					return(c);	
                  } /* end if */
			break;
          } /* end switch */
      } /* end while */
}   /* end ftpgets() */

/************************************************************************
* dumpcon
*  take everything from a connection and send it to the screen
*  return -1 on closed connection, else 0, 1 if paused
************************************************************************/

static int dumpcon(int cnum)
{
	int cnt;

	if(fromtty && n_scrlck())
		return(TRUE);				/* if paused, nothing to do */
#ifdef AUX /* RMG */
fprintf(stdaux," D ");
#endif
  do {
    cnt=netread(cnum,s,64);   /* get some from queue */
    if(verbose)
      telnet(cnt);      /* display on screen, etc.*/
  } while(cnt>0);
	return(cnt);					/* 0 normally, -1 if connection closed */
}   /* end dumpcon() */

/***********************************************************************
*  telnet
*   filter telnet options on incoming data
************************************************************************/
static void telnet(int cnt)
{
	int i;

#ifdef AUX /* RMG */
fprintf(stdaux," T(%d)",cnt);
#endif

#ifdef nAUX /* RMG */
    nputs("");
    nputs(" T++ ");
    sprintf(printline,"s=%s cnt=%d ",s,cnt);
    nputs(printline);
    nputs(" T-- ");
#endif
  if(debug>4) {
    sprintf(printline,"telnet(): cnt=%d ",cnt);
    nputs(printline);
  } /* end if */
	for(i=0; i<cnt; i++) {					/* put on screen */
		if(s[i] & 128) {					/* if over ASCII 128 */
      sprintf(printline," %d-",(int)s[i]);  /* show as number */
			nputs(printline);
    } /* end if */
		else
			nputchar(s[i]);
  } /* end for */
  if(debug>4) {
    nputs("telnet(): leaving ");
  } /* end if */
}   /* end telnet() */

/********************************************************************
* ftppi()
*  Protocol interpreter for user interface commands
*  Will permit any command to be abbreviated uniquely.
*  Recognizes commands, translates them to the protocol commands to be
*  sent to the other server, and uses userftpd, the daemon, to do data
*  transfers.
***********************************************************************/
static int ftppi(char *command)
{
	int retry;
  int cmdno,i,j;
  char cmdname[50],
       word[50],
       line[100],
       answer[20],
       ofilename[50];
  char *p;
  struct machinfo *mp;

if(debug>1) {           /* print command */
    sprintf(printline,"command: %s",command);
    nputs(printline);
  } /* end if */

  if(!getword(command,cmdname))   /* removes first word from command */
    return(FALSE);

	strlwr(cmdname);
	cmdno=finduniq(cmdname,ftp_cmdlist,NCMDS);	/* search cmdlist for prefix */
	if(cmdno==AMBIGUOUS) {		/* not unique abbreviation */
		nputs("?Ambiguous command");
		return(FALSE);
      } /* end if */
	if(cmdno==NONE) {		/* not a prefix of any command */
		nputs("?Invalid command");
		return(FALSE);
      } /* end if */

/* change \ to / and check if command output redirected */
	if(cmdno!=BANG) {		/* don't alter shell escape */
		if(redirect) {		/* check for ouput redirection selected */
			if(cmdno!=LLS)		/* do not flip slashes for LLS */
				checkoredir(command,ofilename,slashflip);	/* check redirection, flip \ */
			else
				checkoredir(command,ofilename,FALSE);	/* check redirection */
		  }	/* end if */
		else {				/* no output redirection */
			if(cmdno!=LLS && slashflip)		/* don't flip slashes for LLS and check for slashflipping here and assume it in routine */
				flip_slashes(command,ofilename);
		  }	/* end else */
      } /* end if */

    switch(cmdno) {     /* process commands */
		case QMARK:
		case HELP:
			if(!command[0]) {						/* no argument */
				nputs("Commands may be abbreviated. Commands are:\n");
/* display command list */
				printline[0]='\0';
				for(i=0; i<NCMDS; i++) {
					sprintf(word,"%-16s",ftp_cmdlist[i]);	/* get word from list */
					strcat(printline,word);			/* add to line */
					if(i%5==4) {					/* display line */
						printline[79]='\0';
						nputs(printline);
						printline[0]='\0';
                      } /* end if */
                  } /* end for */
				if(i%5!=4)
					nputs(printline);				/* last line */
				return(TRUE);
              } /* end if */
/* help for specific commands */
			else {
				while(getword(command,word)) {		/* loop for all args */
					i=finduniq(word,ftp_cmdlist,NCMDS);   	/* which command? */
					if(i==AMBIGUOUS)						/* non-unique command name */
						sprintf(printline,"?Ambiguous help command %s",word);
					else 
						if(i==NONE)					/* no such command */
							sprintf(printline,"?Invalid help command %s",word);
						else							/* display help string */
							sprintf(printline,"%s",helpstrings[i-1]);
					nputs(printline);
                  } /* end while */
				return(TRUE);
              } /* end else */
			break;

#ifndef NO_WRT
    case BANG:              /* shell escape */
			fflush(stdout);
			if(*(stpblk(command))) {		/* command specified */
        system(command);      /* execute command */
        n_cur(n_row(),0);   /* in case screen is messed up  rmg 940108 */
        return(TRUE);
      } /* end if */
			dosescape();					/* subshell */
      n_cur(n_row(),0);   /* in case screen is messed up  rmg 940108 */
      return(TRUE);
#endif

		case BELL:
			if(getword(command,word)) {		/* scan arg */
				strlwr(word);
				if(!strcmp(word,"off"))
					 bell=FALSE;
				else 
					if(!strcmp(word,"on"))
						bell=TRUE;
				else 
					bell=!bell;
              } /* end if */
			else
				 bell=!bell;
			if(bell) 
				nputs("Bell mode on.");
			else 
				nputs("Bell mode off.");
			return(TRUE);

		case BYE:
		case QUIT:
			if(connected) {
				ftpdo("QUIT",ofilename);
				connected=FALSE;
              } /* end if */
			netclose(ftpnum);					/* close command connection */
			netshut();
      remove_break();
      if(fromtty)
            if(display_init) {                 /* Restore DOS' color scheme */
                n_color(dos_color);
                n_clear();          /* clear screen */
#ifdef NOT
                n_wrap(1);          /* cursor positioning */
#endif
                n_cur(0,0);
              } /* end if */
			exit(0);

		case DEBUG:				/* turn on/off debugging, optional level */
			if(sscanf(command,"%d",&i)>0)
				debug=i;							/* level */
			else
				if(getword(command,word)) {			/* scan arg */
					strlwr(word);
					if(!strcmp(word,"off"))
						debug=FALSE;
					else 
						if(!strcmp(word,"on"))
							debug=TRUE;
						else
							debug=!debug;
                  } /* end if */
			else
				debug=!debug;
			if(debug) {
				sprintf(printline,"Debugging on(debug=%d).",debug);
				nputs(printline);
              } /* end if */
			else 
				nputs("Debugging off.");
			return(TRUE);

		case GLOB:		/* wildcard expansion */
			if(getword(command,word)) {
				strlwr(word);
				if(!strcmp(word,"off")) 
					glob=FALSE;
				else 
					if(!strcmp(word,"on")) 
						glob=TRUE;
					else 
						glob=!glob;
              } /* end if */
			else 
				glob=!glob;
			if(glob) 
				nputs("Globbing on.");
			else 
				nputs("Globbing off.");
			return(TRUE);

		case HASH:		/* hash mark printing */
			if(getword(command,word)) {
				strlwr(word);
				if(!strcmp(word,"off")) 
					hash=FALSE;
				else 
					if(!strcmp(word,"on")) 
						hash=TRUE;
					else 
						hash=!hash;
              } /* end if */
			else 
				hash=!hash;
			if(hash) 
				nputs("Hash mark printing on(1024 bytes/hash mark).");
			else 
				nputs("Hash printing off.");
			return(TRUE);

		case INTERACTIVE:	/* prompting on multiple transfers */
			prompt=TRUE;
			nputs("Interactive mode on.");
			return(TRUE);

		case LCD:		/* change local directory */
			if(command[1]==':') {		/* if disk specified */
#ifdef	MSC
#ifdef __TURBOC__
				setdisk(tolower(command[0])-'a');
#else
                unsigned int bogus;
				_dos_setdrive( tolower( command[0])-'a'+1,&bogus);
#endif
#else
				chgdsk(tolower(command[0])-'a');
#endif
				strcpy(command,&command[2]);
              } /* end if */
            if(*(stpblk(command))&&chdir(command))     /* CD */
				nputs("Unable to change directory");
			getdir(0,line);		/* current directory */
			sprintf(printline,"Local directory now %s",line);
			nputs(printline);
			return(TRUE);

		case LLS:		/* local DIR */
			sprintf(line,"DIR %s",command);
			fflush(stdout);
			system(line);
			return(TRUE);

		case MORE:		/* toggle use of MORE */
            if(getword(command,word)) {
				strlwr(word);
                if(!strcmp(word,"off"))
					usemore=FALSE;
                else if(!strcmp(word,"on"))
					usemore=TRUE;
              } /* end if */
            else
                usemore=!usemore;
            if(usemore)
                nputs("Use of built in | more on.");
            else
                nputs("Use of built in | more off.");
            lineslft=numlines=tel_vid_info.rows-1;  /* rmg 940108 */
			break;

		case NONINTERACTIVE:	/* turn off interactive prompting */
			prompt=FALSE;
			nputs("Interactive mode off.");
			return(TRUE);

		case OPEN:		/* open connection to host */
			if(connected) {
				nputs("Already connected.");
				return(FALSE);
              } /* end if */
			while(!(*(stpblk(command)))) {		/* no argument */
				putstring("To: ");
				if(ftpgets(command,100,1)==ABORT) 
					return(FALSE);
              } /* end while */
            getword(command,destname);  /* host name */
			mp=Sgethost(destname);		/* get host info */
            if(foundbreak)
                return(FALSE);          /* abort */
			if(mp==NULL) {				/* try domain serving */
				Sdomain(destname);
				while(mp==NULL) {
					switch(checkevent()) {
						case ABORT:
							return(FALSE);	/* abort */

						case DOMFAIL:
							printerr();
							sprintf(printline,"Unknown host: %s\n",destname);
							nputs(printline);
							return(FALSE);

						case DOMOK:
							mp=Slooknum(ftpnum);	/* get host info */
							break;

						default:
							break;
                      } /* end switch */
                  } /* end while */
              } /* end if */
      display_init=TRUE;
      def.color[0]=(unsigned char)(mp->nfcolor+(mp->nbcolor<<4));
      scrsetup=FALSE;
      nputs("");
      if(sscanf(command,"%d",&i)>0) /* port number specified */
				ftpnum=Snetopen(mp,i);
            else
				ftpnum=Snetopen(mp,HFTP);		/* default port */
            if(foundbreak)
                return(FALSE);       /* abort */
			if(ftpnum<0) {				/* error on open */
				printerr();
				sprintf(printline,"Unable to connect to %s",destname);
				nputs(printline);
				return(FALSE);
              } /* end if */
            j=ftpreplies(ftpnum,&i);      /* response from other end */
            if(j==FALSE || j==ERROR || j==NONE || j==ABORT) {
                return(FALSE);
              } /* end if */
            if(foundbreak)
                return(FALSE);
			connected=TRUE;
			if(autologin) {		/* execute login command */
				strcpy(command,"user");
				ftppi(command);
              } /* end if */
			return(TRUE);

		case PROMPT:			/* interactive prompting */
			if(getword(command,word)) {
				strlwr(word);
				if(!strcmp(word,"off")) 
					prompt=FALSE;
				else 
					if(!strcmp(word,"on")) 
						prompt=TRUE;
                    else
                        prompt=!prompt;
              } /* end if */
			else 
				prompt=!prompt;
			if(prompt) 
				nputs("Interactive mode on.");
			else 
				nputs("Interactive mode off.");
			return(TRUE);

		case SENDPORT:			/* send PORT commands for each transfer */
			if(getword(command,word)) {
				strlwr(word);
				if(!strcmp(word,"off")) 
					sendport=FALSE;
				else 
					if(!strcmp(word,"on")) 
						sendport=TRUE;
					else 
						sendport=!sendport;
              } /* end if */
			else 
				sendport=!sendport;
			if(sendport) 
				nputs("Use of PORT cmds on.");
			else 
				nputs("Use of PORT cmds off.");
			return(TRUE);

		case SLASHFLIP:			/* change \ to / */
			if(getword(command,word)) {
				strlwr(word);
				if(!strcmp(word,"off")) 
					slashflip=FALSE;
				else 
					if(!strcmp(word,"on")) 
						slashflip=TRUE;
					else 
						slashflip=!slashflip;
              } /* end if */
			else 
				slashflip=!slashflip;
			if(slashflip) 
				nputs("Slash translation on.");
			else 
				nputs("Slash translation off.");
			return(TRUE);

		case STATUS:		/* display status info */
			if(connected) {
				sprintf(printline,"Connected to %s",destname);
				nputs(printline);
              } /* end if */
			if(ftpfilemode==FASCII) 
				nputs("Transfer mode is ascii.");
			else 
				nputs("Transfer mode is binary.");
			if(bell) 
				nputs("Bell on."); else nputs("Bell off.");
			if(debug) {
				sprintf(printline,"Debugging on.(Debug=%d)",debug);
				nputs(printline);
              } /* end if */
			else 
				nputs("Debugging off.");
			if(glob) 
				nputs("Filename globbing on."); 
			else 
				nputs("Filename globbing off.");
			if(hash) 
				nputs("Hash-mark printing on.");
			else 
				nputs("Hash-mark printing off.");
			if(prompt)
				nputs("Interactive prompting on."); 
			else 
				nputs("Interactive prompting off.");
			if(sendport) 
				nputs("Sending of port commands on.");
			else 
				nputs("Sending of PORT cmds off.");
			if(slashflip) 
				nputs("Flipping \\ to / on.");
			else
				nputs("Flipping \\ to / off.");
			if(verbose) 
				nputs("Verbose mode on."); 
			else 
				nputs("Verbose mode off.");
      if(usemore)
				nputs("Use of built in | more on.");
			else
				nputs("Use of built in | more off.");
			if(connected) {		/* send STAT command */
				nputs("\nRemote status:");
				ftpdo("STAT",ofilename);
              } /* end if */
			return(TRUE);

		case VERBOSE:		/* display informative messages */
			if(getword(command,word)) {
				strlwr(word);
				if(!strcmp(word,"off")) 
					verbose=FALSE;
				else 
					if(!strcmp(word,"on")) 
						verbose=TRUE;
					else 
						verbose=!verbose;
              } /* end if */
			else 
				verbose=!verbose;
			if(verbose) 
				nputs("Verbose mode on.");
			else 
				nputs("Verbose mode off.");
			return(TRUE);

		default:		/* The other commands valid only if connected */
			if(!connected) {
				nputs("Not connected.");
				return(FALSE);
              } /* end if */

            switch(cmdno) {
                case ASCII:         /* transfer mode */
                    ftpdo("TYPE A",ofilename);
                    return(TRUE);

#ifndef NO_WRT
                case BGET:          /* get file in binary mode */
                    i=ftpfilemode;  /* save current mode */
                    if(i==FASCII)
                        ftpdo("TYPE I",ofilename);
                    while(!(*(stpblk(command)))) {
                        putstring("File: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);
                      } /* end while */
                    sprintf(line,"RETR %s",command);
                    ftpdo(line,ofilename);      /* get file */
                    if(i==FASCII)
                        ftpdo("TYPE A",ofilename);  /* restore mode */
                    return(TRUE);
#endif

                case BINARY:        /* binary mode */
                    ftpdo("TYPE I",ofilename);
                    return(TRUE);

                case BPUT:          /* put file in binary mode */
                    i=ftpfilemode;
                    if(i==FASCII)
                        ftpdo("TYPE I",ofilename);
                    while(!(*(stpblk(command)))) {      /* if no arg */
                        putstring("File: ");    /* get from user */
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);
                      } /* end while */
                    sprintf(line,"STOR %s",command);
                    for(retry=0; retry<5; retry++) {
                        if((i=ftpdo(line,ofilename))==TRUE) {
                            if(i==FASCII)
                                ftpdo("TYPE A",ofilename);
                            return(TRUE);
                          } /* end if */
                        else
                            if(i!=FALSE) {
                                if(i==FASCII)
                                    ftpdo("TYPE A",ofilename);
                                return(ERROR);
                              } /* end if */
                        printf("\nRetrying...");
                      } /* end for */
                    printf("Maximum retry count reached.  Aborting this file transfer...");
                    return(FALSE);

                case CD:        /* change remote directory */
                    while(!(*(stpblk(command)))) {      /* if no arg, get from user */
                        putstring("To: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end while */
                    getword(command,word);
                    if(!strcmp(word,"..")) {    /* special case */
                        i=ftpdo("CDUP",ofilename);
                        if(i!=ERROR)
                            return(TRUE);       /* if CDUP understood */
                        nputs("Trying again...");
                        i=ftpdo("XCUP",ofilename);  /* try alternative */
                        if(i!=ERROR)
                            return(TRUE);
                        nputs("Trying again...");       /* else try usual CD */
                      } /* end if */
                    sprintf(line,"CWD %s",word);        /* try CWD */
                    i=ftpdo(line,ofilename);
                    if(i!=ERROR)
                        return(TRUE);
                    nputs("Trying again...");
                    sprintf(line,"XCWD %s",word);       /* try XCWD */
                    ftpdo(line,ofilename);
                    return(TRUE);

                case CLOSE:             /* drop connection */
                    ftpdo("QUIT",ofilename);
                    connected=FALSE;
                    return(TRUE);

                case DEL:
                case RM:
                    getword(command,word);
                    while(!word[0]) {   /* get arg from user */
                        putstring("File: ");
                        if(ftpgets(word,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end while */
                    if(prompt) {        /* check interactively */
                        sprintf(printline,"Delete %s? ",word);
                        putstring(printline);
                        ftpgets(answer,20,1);
                        if(tolower(*(stpblk(answer)))!='y')
                            return(TRUE);
                      } /* end if */
                    sprintf(line,"DELE %s",word);
                    ftpdo(line,ofilename);
                    return(TRUE);

                case DIR:       /* get list of remote files */
                    i=ftpfilemode;  /* save mode */
                    if(i==FIMAGE)
                        ftpdo("TYPE A",ofilename);
                    if(getword(command,word)) { /* do DIR */
                        sprintf(line,"LIST %s",word);
                        ftpdo(line,ofilename);
                      } /* end if */
                    else
                        ftpdo("LIST",ofilename);
                    if(i==FIMAGE)
                        ftpdo("TYPE I",ofilename);
                    return(TRUE);

#ifndef NO_WRT
                case GET:
                case RECV:      /* get remote file */
                    while(!(*(stpblk(command)))) {      /* if no arg */
                        putstring("File: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end while */
                    sprintf(line,"RETR %s",command);
                    ftpdo(line,ofilename);
                    return(TRUE);
#endif

                case LS:        /* get remote file list-short */
                    i=ftpfilemode;
                    if(i==FIMAGE)
                        ftpdo("TYPE A",ofilename);
                    if(getword(command,word)) {
                        sprintf(line,"NLST %s",word);
                        ftpdo(line,ofilename);
                      } /* end if */
                    else
                        ftpdo("NLST",ofilename);
                    if(i==FIMAGE)
                        ftpdo("TYPE I",ofilename);
                    return(TRUE);

                case MDELETE:
                    while(!(*(stpblk(command)))) {      /* no arg */
                        putstring("Files: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end while */
                    while(getword(command,word)) {      /* for each arg */
                        if(glob) {      /* wildcard expansion */
                            sprintf(line,"NLST %s",word);
                            capture=TRUE;
                             ftpdo(line,ofilename);  /* put exapnsion in captlist */
                            capture=FALSE;
                          } /* end if */
                        else
                            strcpy(captlist,word);  /* captlist has name(s) now */
                        while(getnname(captlist,word)) {    /* for each name */
                            if(prompt) {    /* check */
                                sprintf(printline,"mdelete %s? ",word);
                                putstring(printline);
                                if(ftpgets(answer,20,1)==ABORT) {   /* abort */
                                    command[0]='\0';    /* no more processing */
                                    break;          /* quit immediately */
                                  } /* end if */
                                if(tolower(*(stpblk(answer)))!='y')
                                    continue;
                              } /* end if */
                            sprintf(line,"DELE %s",word);   /* delete */
                            ftpdo(line,ofilename);
                          } /* end while */
                      } /* end while */
                    return(TRUE);

                case MDIR:      /* remote multiple DIR */
                    i=ftpfilemode;  /* save mode */
                    if(i==FIMAGE)
                        ftpdo("TYPE A",ofilename);
                    while(!(*(stpblk(command)))) {      /* no arg */
                        putstring("Directories: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);    /* abort */
                      } /* end while */
                    while(getword(command,word)) {      /* for each arg */
                        if(glob) {      /* expand wildcards */
                            sprintf(line,"NLST %s",word);
                            capture=TRUE;
                            ftpdo(line,ofilename);
                            capture=FALSE;
                          } /* end if */
                        else
                            strcpy(captlist,word);
                        while(getnname(captlist,word)) {    /* for each name */
                            if(prompt) {    /* check */
                                sprintf(printline,"mdir %s? ",word);
                                putstring(printline);
                                if(ftpgets(answer,20,1)==ABORT) {   /* abort */
                                    command[0]='\0';    /* no more processing */
                                    break;          /* quit immediately */
                                  } /* end if */
                                if(tolower(*(stpblk(answer)))!='y')
                                    continue;
                              } /* end if */
                            sprintf(line,"LIST %s",word);   /* DIR */
                            ftpdo(line,ofilename);
                          } /* end while */
                      } /* end while */
                    if(i==FIMAGE)
                        ftpdo("TYPE I",ofilename);
                    return(TRUE);

#ifndef NO_WRT
                case MGET:      /* get multiple files */
#ifdef OLD_WAY
                    getword(command,line);
#else
                    strcpy(line,command);
#endif
                    while(!line[0]) {   /* no arg */
                        putstring("Files: ");
                        if(ftpgets(line,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end if */
                    while(getword(line,word)) { /* for each arg */
                        if(glob) {      /* expand wildcards */
                          sprintf(command,"NLST %s",word);
                          capture=TRUE;
                          ftpdo(command,ofilename);
                          capture=FALSE;
                        } /* end if */
                        else
                          strcpy(captlist,word);
                        while(getnname(captlist,word)) {    /* for each name */
                            if(prompt) {    /* check */
                                sprintf(printline,"mget %s? (Yes/No/All/Quit) ",word);
                                putstring(printline);
                                if((ftpgets(answer,20,1)==ABORT) || (tolower(*(stpblk(answer)))=='q')) {  /* abort */
                                    command[0]='\0';    /* no more processing */
                                    break;          /* quit immediately */
                                  } /* end if */
                                if(tolower(*(stpblk(answer)))=='a') {   /* All */
                                    prompt=FALSE;
                                    tempprompt=TRUE;
                                  } /* end if */
                                if((tolower(*(stpblk(answer)))!='y') && (tolower(*(stpblk(answer)))!='a'))
                                    continue;
                              } /* end if */
                            sprintf(command,"RETR \"%s\"",word);
                            ftpdo(command,ofilename);
                          } /* end while */
                        if(tempprompt==TRUE) {
                            prompt=TRUE;
                            tempprompt=FALSE;
                          } /* end if */
                      } /* end while */
                    return(TRUE);
#endif

                case MKDIR:     /* create directory */
                    while(!(*(stpblk(command)))) {      /* no arg */
                        putstring("Directory: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end while */
                    sprintf(line,"XMKD %s",command);    /* try XMKD */
                    i=ftpdo(line,ofilename);
                    if(i!=ERROR)
                        return(TRUE);
                    nputs("Trying again...");
                    sprintf(line,"MKD %s",command);     /* else try MKD */
                    ftpdo(line,ofilename);
                    return(TRUE);

                case MLS:
                    i=ftpfilemode;
                    if(i==FIMAGE)
                        ftpdo("TYPE A",ofilename);
                    while(!(*(stpblk(command)))) {      /* no arg */
                        putstring("Directories: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end while */
                    while(getword(command,word)) {      /* for each arg */
                        if(glob) {      /* exapnd wildcards */
                            sprintf(line,"NLST %s",word);
                            capture=TRUE;
                            ftpdo(line,ofilename);
                            capture=FALSE;
                          } /* end if */
                        else
                            strcpy(captlist,word);
                        while(getnname(captlist,word)) {    /* for each name */
                            if(prompt) {        /* check */
                                sprintf(printline,"mls %s? ",word);
                                putstring(printline);
                                if(ftpgets(answer,20,1)==ABORT) {  /* abort */
                                    command[0]='\0';    /* no more processing */
                                    break;          /* quit immediately */
                                  } /* end if */
                                if(tolower(*(stpblk(answer)))!='y')
                                    continue;
                              } /* end if */
                            sprintf(line,"NLST %s",word);   /* DIR */
                            ftpdo(line,ofilename);
                          } /* end while */
                      } /* end while */
                    if(i==FIMAGE)
                        ftpdo("TYPE I",ofilename);
                    return(TRUE);

                case MODE:      /* set stream mode */
                    getword(command,word);
                    strlwr(word);
                    if(strncmp(word,"stream",strlen(word)))
                        nputs("We only support stream mode, sorry.");
                    else
                        nputs("Mode is stream.");
                    return(TRUE);

                case MPUT:      /* put multiple files */
#ifdef OLD_WAY
                    getword(command,line);
#else
                    strcpy(line,command);
#endif
                    while(!line[0]) {   /* no arg */
                        putstring("Files: ");
                        if(ftpgets(line,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end while */
                    p=NULL; /* no names expanded yet */
                    while(getword(line,word)) { /* for each arg */
                        do {        /* for each name */
                            if(glob) {      /* local wildcard expansion */
                                if(p==NULL) {   /* if no expansions yet */
                                    p=firstname(word,0);    /* get first name, with no file information attached */
                                    if(p==NULL) {   /* if no expansions */
                                        sprintf(printline,"No match for %s",word);
                                        nputs(printline);
                                        continue;
                                      } /* end if */
                                  } /* end if */
                                else {      /* not first name */
                                    p=nextname(0);      /* get next name, with no file information attached */
                                    if(p==NULL)
                                        continue;   /* if no names, next arg */
                                  } /* end else */
                              } /* end if */
                            else
                                p=word;     /* no expansion */
                            if(prompt) {    /* check */
                                sprintf(printline,"mput %s? (Yes/No/All/Quit) ",p);
                                putstring(printline);
                                if((ftpgets(answer,20,1)==ABORT)||(tolower(*(stpblk(answer)))=='q')) {  /* abort */
                                    command[0]='\0';    /* no more processing */
                                    break;          /* quit immediately */
                                  } /* end if */
                                if(tolower(*(stpblk(answer)))=='a') {
                                    prompt=FALSE;
                                    tempprompt=TRUE;
                                  } /* end if */
                                if((tolower(*(stpblk(answer)))!='y')&&(tolower(*(stpblk(answer)))!='a'))
                                    continue;
                              } /* end if */
                            sprintf(command,"STOR \"%s\"",p);   /* name may have special chars */
                            ftpdo(command,ofilename);
                          } while(glob && p!=NULL);       /* Not last expansion */
                        if(tempprompt) {
                            prompt=TRUE;
                            tempprompt=FALSE;
                          } /* end if */
                      } /* end while */
                    return(TRUE);

                case PUT:
                case SEND:      /* put file */
                    while(!(*(stpblk(command)))) {      /* no args */
                        putstring("File: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);
                      } /* end while */
                    sprintf(line,"STOR %s",command);    /* put file */
                    for(retry=0; retry<5; retry++) {
                        if((i=ftpdo(line,ofilename))==TRUE)
                            return(TRUE);
                        else if(i!=FALSE)
                            return(ERROR);
                        printf("\nRetrying...");
                      } /* end for */
                    printf("\nMaximum retry count reached.  Aborting file transfer...");
                    return(ERROR);

                case PWD:
                    i=ftpdo("XPWD",ofilename);      /* try XPWD */
                    if(i!=ERROR)
                        return(TRUE);
                    nputs("Trying again...");
                    ftpdo("PWD",ofilename);         /* else try PWD */
                    return(TRUE);

                case QUOTE:
                    while(!(*(stpblk(command)))) {      /* no arg */
                        putstring("Command: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);
                      } /* end while */
                    ftpdo(command,ofilename);       /* send command */
                    return(TRUE);

                case REMOTEHELP:                /* get help */
                    if(*(stpblk(command))) {        /* for specific command */
                        sprintf(line,"HELP %s",command);
                        ftpdo(line,ofilename);
                      } /* end if */
                    else
                        ftpdo("HELP",ofilename);        /* generic help */
                    return(TRUE);

                case RENAME:        /* rename remote file */
                    while(!(*(stpblk(command)))) {      /* no arg */
                        putstring("From: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);
                      } /* end if */
                    getword(command,word);
                    sprintf(line,"RNFR %s",word);
                    ftpdo(line,ofilename);      /* send rename from name */
                    while(!(*(stpblk(command)))) {      /* no second arg */
                        putstring("To: ");
                        if(ftpgets(command,100,1)==ABORT) {
                            ftpdo("ABOR",ofilename);
                            return(FALSE);
                          } /* end if */
                      } /* end while */
                    sprintf(line,"RNTO %s",command); /* send rename to name */
                    ftpdo(line,ofilename);
                    return(TRUE);

                case RMDIR:         /* remove remote dir */
                    while(!(*(stpblk(command)))) {      /* no arg */
                        putstring("Directory: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);
                      } /* end while */
                    sprintf(line,"XRMD %s",command);    /* try XRMD */
                    i=ftpdo(line,ofilename);
                    if(i!=ERROR)
                        return(TRUE);
                    nputs("Trying again...");
                    sprintf(line,"RMD %s",command);     /* try RMD */
                    ftpdo(line,ofilename);
                    return(TRUE);

                case STRUCT:        /* set structure type-only file */
                    getword(command,word);
                    strlwr(word);
                    if(strncmp(word,"file",strlen(word)))
                        nputs("We only support file structure, sorry.");
                    else
                        nputs("Structure is file.");
                    return(TRUE);

                case TYPE:      /* set transfer type */
                    if(!getword(command,word)) {    /* no arg, just show */
                        if(ftpfilemode==FASCII)
                            nputs("Transfer type is ascii.");
                        else
                            nputs("Transfer type is binary.");
                      } /* end if */
                    strlwr(word);
                    if(!strncmp(word,"ascii",strlen(word)))
                        ftpdo("TYPE A",ofilename);
                    else
                        if(!strncmp(word,"binary",strlen(word))||!strncmp(word,"image",strlen(word)))
                            ftpdo("TYPE I",ofilename);
                        else {
                            sprintf(printline,"Unrecognized type: %s",word);
                            nputs(printline);
                          } /* end else */
                    return(TRUE);

                case USER:          /* login to remote machine */
                    if(!fromtty && ttypass)
                        fromtty=TRUE; /* go interactive */
                    if(f_enter_login) {
                        fromtty=1;        /* fake interactive mode */
                        n_cur(n_row(),0);   /* in case screen is messed up */
                      } /* end if */
                    if(!(*(stpblk(command)))) { /* null response to prompt ok */
                        putstring("Username: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);
                      } /* end if */
                    sprintf(line,"USER %s",command);    /* username */
                    switch(ftpdo(line,ofilename)) {
                        case TRUE:
                            return (TRUE);
                        case FALSE:
                        case ERROR:
                            return(FALSE);
                        case ABORT:
                            return(ABORT);
                        default:
                            break;
                      } /* end switch */
                    putstring("Password: ");
                    if(ftpgets(word,40,0)==ABORT)
                        return(FALSE);      /* no echoing */
                    if(f_enter_login)      /* restore to non-interactive mode */
                        fromtty=0;
                    sprintf(line,"PASS %s",word);       /* password */
                    if(ftpdo(line,ofilename)==INCOMPLETE) { /* if account needed */
                        do {
                            putstring("Account: ");
                            if(ftpgets(command,100,1)==ABORT)
                                return(FALSE);
                          } while(!(*(stpblk(command))));
                        sprintf(line,"ACCT %s",command);
                        ftpdo(line,ofilename);
                      } /* end if */
                    if(ttypass)
                      fromtty=FALSE;     /* back to batch */
                    return(TRUE);

                case ACCOUNT:
                    if(!(*(stpblk(command)))) {   /* null response to prompt OK */
                        putstring("Account: ");
                        if(ftpgets(command,100,0)==ABORT)
                            return(FALSE); /* no echo */
                        while (!(*(stpblk(command))));
                        sprintf(line,"ACCT %s",command);
                        ftpdo(line,ofilename);
                      } /* end if */
                    return(TRUE);

                case SITE:      /* site commands */
                    while(!(*(stpblk(command)))) {      /* if no arg */
                        putstring("Site: ");
                        if(ftpgets(command,100,1)==ABORT)
                            return(FALSE);  /* abort */
                      } /* end while */
                    sprintf(line,"SITE %s",command);
                    ftpdo(line,ofilename);
                    return(TRUE);

                default:    /* unknown command */
                    sprintf(printline,"***Program error: Unknown command no: %d",cmdno);
                    nputs(printline);
                    break;
              } /* end switch */
      } /* end switch */
}   /* end ftppi() */

/************************************************************************
* ftpdo
*  Do whatever command is sent from the user interface using
*  userftpd, the background file handler
*  Returns code from ftpreplies
************************************************************************/

static int ftpdo(char *s,char *ofile)
{
  int  i,
       rcode;
  char name[50],
       name2[50];

  for(i=0; i<4; i++)
    s[i]=(char)toupper(s[i]);  /* command to upper case */

#ifdef AUX /* RMG */
fprintf(stdaux," ...as we enter the mysterious magical world of the ftpdodoo ");
#endif

	if(!strncmp(s,"STOR",4)) {	/* put file */
    getword(&s[5],name);    /* first arg-local file */
		if(!s[5]) 
			strcpy(&s[5],name);		/* if only one argument */
		else {
      getword(&s[5],name2);   /* second arg-removes quotes etc. */
			strcpy(&s[5],name2);	/* copy back into command */
    } /* end else */
#ifdef MSC
		if(0>(ftpfh=open(name,O_RDONLY | O_BINARY))) {	/* open local file */
#else
		if(0 >(ftpfh=open(name,O_RAW))) {	/* open local file */
#endif	
			nputs(" Cannot open file to transfer.");
			return(-1);
    } /* end if */
		ftpdata=netlisten(ftpport());		/* open data connection */
		ftpstate=20;
  } /* end if */

  else if(!strncmp(s,"RETR",4)) {  /* get file */
    getword(&s[5],name);            /* remote file */
    if(s[5])        /* two args present */
      getword(&s[5],name2);   /* local file */
    else
      check_file_name (name, name2);
#ifdef  MSC
    ftpfh=open(name2,O_BINARY|O_CREAT+O_WRONLY,S_IREAD|S_IWRITE);
#else
    ftpfh=creat(name2,O_RAW);              /* open local file */
#endif
    if(ftpfh<0) {
      printf(printline,"Cannot open file to receive: %s\n",name);
      nputs(printline);
      return(-1);
    } /* end if */
    strcpy(&s[5],name); /* put remote name back into command */
    ftpdata=netlisten(ftpport());       /* open data connection */
    ftpstate=30;
  } /* end if */

  else if(!strncmp(s,"LIST",4) || !strncmp(s,"NLST",4)) {
    if(capture)
      captlist[0]='\0';   /* where to put incoming data */
    if(debug>4) {
      nputs("ftpdo(): before calling ftpport() ");
    } /* end if */
/* cRMG */
    /* ftppport is interesting 'cause does more then return a port numnber */
    ftpdata=netlisten(ftpport());       /* data connection */
    if(debug>4) {
      nputs("ftpdo(): after calling ftpport() ");
    } /* end if */
    ftpstate=40;
  } /* end if */

  else if(!strncmp(s,"TYPE",4)) {
    if(toupper(s[5])=='I')
      ftpfilemode=FIMAGE; /* remember mode */
    else
      if(toupper(s[5])=='A')
        ftpfilemode=FASCII;
  } /* end if */

	dumpcon(ftpnum);			/* clear command connection */
  netpush(ftpnum);
	netwrite(ftpnum,s,strlen(s));		/* send command */
	netwrite(ftpnum,"\r\n",2);		/* <CRLF> terminates command */
	if(!capture && ofile[0]) {		/* command redirected */
		if((ftpstate!=20) &&(ftpstate!=30)) {	/* not get or put */
#ifdef	MSC
		 	if(0>(ftpfh=open(ofile,O_CREAT|O_APPEND|O_WRONLY,S_IWRITE|S_IREAD)))
#else
		 	if(0>(ftpfh=open(ofile,O_CREAT|O_APPEND|O_WRONLY,S_IWRITE)))
#endif
				nputs(" Cannot open output file.");
			else
        if(ftpdata>-1)
					ftpstate=30;	/* act as get, since data goes into file */
				else {
					close(ftpfh);
          ftpfh=0;
        } /* end else */
    } /* end if */
  } /* end if */
  if(debug) {
    sprintf(printline,"---> %s",s); /* show command sent */
    nputs(printline);
    if(debug>=7) {
      for(i=0; i<(int)strlen(s); i++)
        sprintf(&printline[4*i],"%3d ",s[i]);
      nputs(printline);
    }
  }
  i=ftpreplies(ftpnum,&rcode);    /* get remote response */
  dumpcon(ftpnum);      /* clear command connection */
  if(debug) {
    nputs("after ftpreplies() call, in ftpdo ");
  }	/* end if */
	if((i==NONE) && strncmp(s,"QUIT",4)) {	/* unexpected connection drop */
		nputs("lost connection");
		connected=FALSE;
      } /* end if */
    if(i==ABORT || i==NONE || i==ERROR || i==FALSE)  {
		ftpstate=0;		/* if error, no transfer */
		if(ftpdata>-1)
			netclose(ftpdata);
		if(ftpfh!=0)
			close(ftpfh);
		ftpdata=-1;
		ftpfh=0;
      } /* end if */
	return(i);
}   /* end ftpdo() */

/************************************************************************
*   ftpport
*   return a new port number so that we don't try to re-use ports
*   before the mandatory TCP timeout period. (lifetime of a packet)
*   use a time-based initial port selection scheme.
************************************************************************/
static unsigned int ftpport(void)
{
    unsigned int i,
        rcode;
	unsigned char hostnum[5];
	char sendline[60];		/* for port command */

	if(!sendport)			/* default port */
		return(HFTP-1);

  if(curftpprt<0x4000) {   /* restart cycle */
		i=(unsigned int)time(NULL);
    curftpprt=(unsigned int)(0x4000+(i&0x3fff));
  } /* end if */
	i=curftpprt--;			/* get port, update for next time */
	netgetip(hostnum);		/* get my ip number */
	sprintf(sendline,"PORT %d,%d,%d,%d,%d,%d\r\n",hostnum[0],hostnum[1],hostnum[2],hostnum[3],i/256,i&255);	/* full port number */
	netpush(ftpnum);		/* empty command connection */
	netwrite(ftpnum,sendline,strlen(sendline));	/* send PORT command */
  /* check result of command, make sure port was okay */
  /* return 0 on error */   /* ????? */

  dumpcon(ftpnum);
#ifdef AUX
fprintf(stdaux," ftpport a dumpcon ");
#endif
  ftpreplies(ftpnum,&rcode);  /* get response */
#ifdef AUX
fprintf(stdaux," ftpport a ftpreplies ");
#endif
  return(i);    /* port number */
}   /* end ftpport() */

/************************************************************************
* ftpreplies
* get responses to commands to server
* return TRUE on successful completion, FALSE on transient negative
* completion, INCOMPLETE if more commands needed for operation,
* NONE on lost connection, ABORT on user abort and ERROR on failure
*
************************************************************************/
static int ftpreplies(int cnum,int *rcode)
{
	int cnt,ev,j=0;
  int continuation_line=0;            /* rmg 940108 */

	while(1) {
    if(debug>4) {
      nputs("ftpreplies(): before rgetline() call ");
    }
    cnt=rgetline(cnum);               /* get line from remote host */

    if(debug>4) {
      sprintf(printline,"s=%s cnt=%d ",s,cnt);
      nputs(printline);
    } /* end if */
    if(cnt==NONE)
      return(NONE);                   /* lost connection */
    if(cnt==ABORT) {                  /* user abort */
			netpush(cnum);
			netwrite(cnum,"ABOR\r\n",6);		/* send abort */
			return(ABORT);
    } /* end if */

    if(!sscanf(s,"%d",rcode))
      *rcode = -1;                    /* continuation line */

    if(s[3]=='-')                     /* line with continuations */
      continuation_line=1;
    else
      continuation_line=0;

    if((*rcode/100)==2 && !continuation_line) {     /* positive completion */
      dumpcon(ftpnum);                /* clear command connection */
      while(ftpdata>=0) {             /* wait till transfers complete */
#ifdef AUX /* RMG */
fprintf(stdaux," please hold ");
#endif
        ev=checkevent();
        if(debug>4) {
          sprintf(printline,"ev=%d ",ev);
          nputs(printline);
        } /* end if */
        if(ev==NONE)
          return(NONE);               /* lost connection */
				if(ev==ABORT)
          return(ABORT);              /* user abort */

				if(ev==HAVEDATA)
          dumpcon(ftpnum);            /* msg on command connection */
      } /* end while */
    } /* end if */

    if(verbose || (*rcode == -1) || (*rcode>500))
      telnet(cnt);                    /* informative/error msg or display on */

    if(continuation_line)             /* line with continuations */
        continue;

    if(debug>4) {
      sprintf(printline,"*rcode=%d ",*rcode);
      nputs(printline);
    }
    switch((int)(*rcode/100)) {       /* first digit */
      case 1:                         /* preliminary */
				continue;
	
      case 2:                         /* positive completion */
#ifdef nAUX
  fprintf(stdaux," p completion");
#endif
        return(TRUE);
				break;

      case 3:                         /* intermediate */
#ifdef nAUX
  fprintf(stdaux," i completion");
#endif
				return(INCOMPLETE);
				break;
		
      case 4:                         /* transient negative completion */
				return(FALSE);
				break;

      case 5:                         /* Permanent negative completion */
				return(ERROR);
				break;

			default:
				nputs("Server response not understood. Terminating command\n");
				return(ERROR);
		  }	/* end switch */
	  }	/* end while */
}   /* end ftpreplies() */

/************************************************************************
* rgetline-get a line from remote server
* return ABORT on user ABORT, NONE on lost connection,
* length of received line on success
*
************************************************************************/
static int rgetline(int cnum)
{
    int cnt,
        i=0,
        ev;

	while(1) {
		ev=checkevent();

		switch(ev) {
			case ABORT:				/* user abort */
			case NONE:				/* lost connection */
				return(ev);

			case HAVEDATA:
				if(fromtty && n_scrlck())
					cnt=0;					/* if paused, nothing to do */
				else 
					while(1) {
						cnt=netread(cnum,&s[i],1);	/* get some from queue */
						if(!cnt) 
							break;			/* nothing available */
						if(s[i++]=='\n') {	/* end of line */
							s[i]='\0';
#ifdef AUX /* RMG */
fprintf(stdaux," R");
#endif
							return(i);		/* return line length */
            } /* end if */
          } /* end while */
				break;

			default:		/* ignore other events */
				break;
    } /* end switch */
  } /* end while */
}   /* end rgetline() */

#ifdef OLD_WAY
/************************************************************************
* breakstop
* handle cntrl-break
************************************************************************/
static void breakstop(void)
{
#ifdef MSC
	signal(SIGINT,SIG_IGN);		/* ignore interrupts while processing this routine */
#endif
	foundbreak=1;
#ifdef MSC
	signal(SIGINT,breakstop);	/* reset Microsoft interception of break */
#endif
}   /* end breakstop() */
#endif

/************************************************************************
* userftpd
*  FTP receive and send file functions
************************************************************************/
static void userftpd(void )
{
    int i,
        r1,
        r2;
  static int stats_to_go=0;
	char lastchar;
	double rate;
	static int stopcapture=FALSE;	/* bytes xferred % 1024 */
	static long tbytes;

	switch(ftpstate) {
		default:					/* unknown */
			break;

		case 40:				/* start LIST */
      if(!netest(ftpdata)) {
				start=time(NULL);	/* current time */
				tbytes=0;		/* received so far */
				ftpstate=41;
              } /* end if */
			break;

/* RMG */
		case 41:		/* get started */
			do {
        if(capture && !stopcapture) { /* into captlist */
#ifdef AUX /* RMG */
fprintf(stdaux," using capture schtuff ");
#endif
          fcnt=netread(ftpdata,xs,READSIZE);
          if(fcnt>0) {    /* make certain a negative length string is not appended */
            if(strlen(captlist)+fcnt>=2000) { /* full */
              if(!fromtty || !n_scrlck())
                nputs(&xs[2001-strlen(captlist)]);  /* display excess chars */
              strncat(captlist,xs,2000-strlen(captlist));
              nputs("Error: capture list too long");
              stopcapture=TRUE;
            } /* end if */
            else
              strncat(captlist,xs,fcnt);    /* append */
          } /* end if */
          if(debug>2) {
            sprintf(printline,"xs %s fcnt %d ",xs,fcnt);
            nputs(printline);
            sprintf(printline,"tbytes %ld strlen(captlist) %d ",tbytes,strlen(captlist));
            nputs(printline);
          }
        } /* end if (capture) */
        else {
          if(fromtty && n_scrlck())
            break;  /* paused */
          fcnt=netread(ftpdata,xs,READSIZE);
#ifdef AUX /* RMG */
fprintf(stdaux," L41");
#endif

          for(i=0; i<fcnt; i++) {
            nputchar(xs[i]);    /* display */
            if(xs[i]==10)
              domore();          /* |more */
          } /* end for */
        } /* end else (not capture) */
        if(fcnt>0)
          tbytes+= fcnt;                     /* how much */
      } while(fcnt>0);              /* till no more input */
      if(debug>1) {
        sprintf(printline,"tbytes %ld ",tbytes);
        nputs(printline);
        sprintf(printline,"captlist %s ",captlist);
        nputs(printline);
      }
      break;

		case 30:		/* receive */
			if(!netest(ftpdata)) {		/* connection made */
				start=time(NULL);
                ftpstate=31;
				len=xp=0;
				tbytes=0;
              } /* end if */
			break;

		case 31:
/*
* file has already been opened, take everything from the connection
* and place into the open file: ftpfh
*/
			do {		/* wait until xs is full before writing to disk */
        if(len<=0) {
          if(xp) {
            if((write(ftpfh,xs,xp)==-1) && (errno==ENOSPC)) {
              netwrite(ftpdata,"ABOR\r\n",6); /* send abort */
              netwrite(ftpdata,"452 Disk full error.\r\n",22);
              printf("Local disk full error.\n");
            } /* end if */
            else
              xp=0;
          } /* end if */
          len=BUFFERS;    /* expected or desired len to go */
        } /* end if */
/* how much to read */
        if(len<READSIZE)
          i=len;
				else
					i=READSIZE;
//        if(!fcnt<0)
          fcnt=netread(ftpdata,&xs[xp],i);
				if(fcnt>0) {	/* adjust counts */
          len-=fcnt;
          xp+=fcnt;
          tbytes+=fcnt;
         } /* end if */
if(debug>1) {
    sprintf(printline,"len %d xp %d fcnt %d",len,xp,fcnt);
    nputs(printline);
  }
        if(fcnt<0) {          /* connection closed */
          write(ftpfh,xs,xp);     /* write last block */
#ifdef NOT  /* RMG 940107 */
					if(ftpfilemode==FASCII) 
						write(ftpfh,"\032",1);	/* EOF char */
#endif
					close(ftpfh);
					ftpfh=0;
                  } /* end if */
				if(hash) {						/* hash mark printing */
          for(i=(int)(((tbytes-(long)fcnt)%1024L+(long)fcnt)/1024L); i; i--)
            if(fromtty)
              nputchar('#');
            else
              putc('#',stderr);
        } /* end if */
      } while(fcnt>0);
      break;

		case 20:	/* send */
			if(!netest(ftpdata)) {				/* connection made */
				start=time(NULL);
				ftpstate=21;
				lengthfile=lseek(ftpfh,0L,SEEK_END);   /* how long is file? */
				lseek(ftpfh, -1L,SEEK_CUR); 		   /* position at last character*/
        if(read(ftpfh, &lastchar, 1)==0) {
          if(debug>4)
            printf("LC Test failed.\n");
          lastchar='\0';
        }
        else {
          if(debug>4)
            printf("Last character test returned %d.\n",(int) lastchar);
        }
				lseek(ftpfh,0L,SEEK_SET);			   /* back to beginning */
        if((ftpfilemode==FASCII) && (lastchar=='\26'))       /* drop cntr-Z for ascii transfer */
					lengthfile--;				/* leave off ctrl-Z */
				towrite=0;
				xp=0;
				tbytes=0;
      } /* end if */
			break;

		case 21:
/*
*  transfer file(s) to the other host via ftp request
*  file is already open=ftpfh
*/
      if(debug>4)
        printf("lengthfile=%d  towrite=%d  xp=%d  i=%d\n",lengthfile,towrite,xp,i);

			if(towrite<=xp) {			/* need to read again */
        if(debug>4)
          printf("towrite<=xp\n");
        if(lengthfile<(long)BUFFERS)
					i=(int)lengthfile;
				else
					i=BUFFERS;
        if(debug>4)
          printf("i=%d\n",i);
				towrite=read(ftpfh,xs,i);
        if(debug>4)
          printf("towrite=%d\n",towrite);
				xp=0;
      } /* end if */
      if(towrite>=xp)
				i=netwrite(ftpdata,&xs[xp],towrite-xp);
			else 
				i=netwrite(ftpdata,&xs[xp],0);
			if(i>0) {					/* send successful, adjust counts */
				xp+=i;
        lengthfile-=i;
				tbytes+=i;
      } /* end if */
      if(debug>1) {
        sprintf(printline,"i %d xp %d towrite %d",i,xp,towrite);
        nputs(printline);
      } /* end if */
			if(hash) {					/* hash printing */
        for(r1=(int)(((tbytes-(long)i)%1024L+(long)i)/1024L); r1; r1--)
          if(fromtty)
            nputchar('#');
          else
            putc('#',stderr);
          } /* end if */
/*
*  done if:  the file is all read from disk and all sent
*  or other side has ruined connection
*/
      if((lengthfile<=0L && xp>=towrite) || netest(ftpdata))
				ftpstate=22;
			break;

		case 22:			/* send done */
/* wait for other side to accept everything and then close */
			if(0>=(r1=netpush(ftpdata)))
				fcnt=-1;
if(debug>1) {
    sprintf(printline,"fcnt %d r1 %d\n",fcnt,r1);
    nputs(printline);
} /* end if */
      break;

  } /* end switch */

/*
*  after reading from connection, if the connection is closed,
*  reset up shop.
*/
  if(stats_to_go) {
    if(hash)
      if(fromtty)
        nputs("");
      else
        fputs("\n",stderr);
    if(verbose)
      nputs(printline);
    stats_to_go=0;
  }

  if(fcnt<0) {    /* connection lost */
		if(ftpfh>0) {	/* close file */
			close(ftpfh);
			ftpfh=0;
          } /* end if */
		ftpstate=0;	/* done */
		fcnt=0;
		i=(int)(time(NULL)-start);	/* how long to transfer */
		if(!i) 
            rate=((double)tbytes)/1024.0;
		else 
            rate=((double)tbytes)/(i*1024.0);
        r1=(int)rate;              /* integer part of rate */
        r2=(int)((rate-(double)r1)*1000);
    sprintf(printline,"Transferred %ld bytes in %d seconds(%d.%03d Kbytes/sec)",tbytes,i,r1,r2);
    stats_to_go=1;
    netclose(ftpdata);      /* close connection */
		ftpdata=-1;
		if(bell) 
			nputchar(7);
      } /* end if */
}   /* end userftpd() */

/************************************************************************
* getword: remove a word from a string.  Things within quotes are
* assumed to be one word.
* return TRUE on success, FALSE on end of string
************************************************************************/
static int getword(char *string,char *word)
{
	char *p,*q;
	int i=0;

if(debug>4) {
    sprintf(printline,"getword: string is %s",string);
    nputs(printline);
  }
	p=stpblk(string);			/* skip leading blanks */
	if(!(*p)) {				/* no words in string */
		word[0]='\0';
		return(FALSE);
      } /* end if */
	if(*p=='!') {				/*! is a word */
		word[0]=*p;
		word[1]='\0';
		strcpy(string,++p);
		return(TRUE);
      } /* end if */
	if(*p=='\"') {				/* word delimited by quotes */
        while(p[++i] && p[i]!='\"')
            word[i-1]=p[i];
		word[i-1]='\0';
		if(!p[i]) 
			nputs("Missing \". Assumed at end of string.");
		else 
            i++;
		q=p+i;
      } /* end if */
	else 
        q=stptok(p,word,50," \t\r\n");  /* get word, max len 50 */
	p=stpblk(q);				     	/* remove trailing blanks */
	strcpy(string,p);					/* remove extracted stuff */
	return(TRUE);
}   /* end getword() */

/************************************************************************/
/* flip_slashes: change \ to /											*/
/************************************************************************/
static void flip_slashes(char *command,char *filename)
{
    int i;                 /* local counting variable */

	filename[0]='\0';				/* don't do anything with the filename */
    for(i=0; !command[i]; i++) {    /* loop over entire string */
		if(command[i]=='\\') 		/* found a \ ? */
			command[i]='/';			/* flip the \ to a / */
      } /* end for */
}	/* end flip_slashes() */

/************************************************************************
* checkoredir: check for output redirection.  If the command contains a
* >, assume a filename follows and extract it.  Remove the redirection
* from the original command.
* Also change \ to /
* return TRUE if redirection specified, FALSE otherwise 
************************************************************************/
static int checkoredir(char *command,char *filename,int slashflip)
{
	int i;

	filename[0]='\0';
    for(i=0; (command[i]!='>'); i++) {   /* process command part */
        if(command[i]=='\\' && slashflip)
			command[i]='/';
		if(!command[i]) 
			return(FALSE);				/* no redirection */
      } /* end for */
	getword(&command[i+1],filename);	/* get redirected filename */
	command[i]='\0';
	return(TRUE);
}   /* end checkoredir() */

/************************************************************************
* getdir: get current directory.  Finds current drive and current path
* on drive, returns a string.
*
************************************************************************/
static void getdir(int drive,char *path)
{
#ifdef	MSC
	getcwd(path,100);
	drive=drive;			/* To get rid of annoying "Unreferenced formal parameter" warning. */
#else
	char partpath[64];
	if(!drive) drive=getdsk();				/* current disk */
		getcd(drive+1,partpath);			/* current dir */
	sprintf(path,"%c:\\%s",'A'+drive,partpath);
#endif
}   /* end getdir() */

/************************************************************************
* finduniq: find name that is a unique prefix of one of the entries in
* a list.  Return position of the entry, NONE if none, AMBIGUOUS if more
* than one.
*
************************************************************************/
static int finduniq(char *name,char *list[],int listsize)
{
    int i,
        j=NONE,
        len;

	len=strlen(name);
	for(i=0; i<listsize; i++) {
		if(!strncmp(name,list[i],len)) {		/* prefix */
			if(len==(int)strlen(list[i]))
				return(i+1);					/* exact match */
			if(j!=NONE) 
				j=AMBIGUOUS;					/* more than one match */
            else
                j=i+1;                              /* note prefix found */
          } /* end if */
      } /* end for */
	return(j);									/* prefix */
}   /* end finduniq() */

/************************************************************************
* checkevent
* get and process network events
* returns ABORT on user abort, HAVEDATA if data available on command
* connection, NONE if connection lost, DOMOK if domain search succeeds,
* DOMFAIL if domain search fails, TRUE if no relevant event.
************************************************************************/
static int checkevent(void)
{
  int ev,
      class=0,
      data;

	kbhit();			/* check for cntrl-break */
  if(foundbreak)
    return(ABORT);
	userftpd(); 		/* do ftp stuff */
	Stask();			/* keep connections alive */
  ev=Sgetevent(CONCLASS|ERRCLASS|USERCLASS,&class,&data);
  if(class==CONCLASS) {
		if(data==ftpnum) {		/* command connection */
			if(ev==CONCLOSE) {	/* connection lost */
				netclose(ftpnum);
        if(!netest(ftpdata))
					netclose(ftpdata);	/* close data connection */
				connected=FALSE;
        return(NONE);
      } /* end if */
			if(ev==CONDATA)
				return(HAVEDATA);	/* data received */
    } /* end if */
    /* rmg 930610 while fixing 0 byte transfers */
    if(data==ftpdata) {       /* data connection */
      if(ev==CONCLOSE) {    /* connection lost */
        netclose(ftpdata);  /* close data connection */
        ftpdata= -1;
        return(FALSE);
      } /* end if */
    }
  } /* end if */
	else 
		if(class==USERCLASS) {		
			if(ev==DOMOK) {		/* domain search succeeded */
				ftpnum=data;
				return(DOMOK);
      } /* end if */
			else 
				if(ev==DOMFAIL) 
					return(DOMFAIL);	/* domain search failed */
    } /* end if */

/* else if(class==ERRCLASS&&ev==ERR1) nputs(neterrstring(data)); */
	return(TRUE);
}   /* end checkevent() */

/************************************************************************
* putstring:
*   display string using vt100 emulation routines
************************************************************************/
static int putstring(char *string)
{
    for(; *string; string++)
        nputchar(*string);
	return(TRUE);
}   /* end putstring() */

/************************************************************************
* printerr:
*   display TCP error messages-disabled
************************************************************************/
static int printerr(void )
{
#ifdef OLD_WAY
    int data,class;

    while(ERR1==Sgetevent(ERRCLASS,&class,&data))
        nputs(neterrstring(data));
#endif
    return(TRUE);
}   /* end printerr() */

/************************************************************************
* getnname: get next name from captured list
* names delimited by newlines-<CR> or <LF>
************************************************************************/
static int getnname(char *string,char *word)
{
	char *s;

	s=string;
    while((*string=='\n') || (*string=='\r'))
		string++;						/* skip initial newlines */
	if(!(*string)) 
		return(FALSE);		/* end of captlist */
    while((*string!='\n') && (*string!='\r') && (*string))
        *(word++)=*(string++);
    while((*string=='\n') || (*string=='\r'))
		string++;	/* skip trailing newline */
	*word='\0';
	strcpy(s,string);
	return(TRUE);
}   /* end getnname() */

/***************************************************************************
*  dosescape
*  escape to dos for processing
*  put the connections to automated sleep while in DOS
************************************************************************/
int dosescape(void)
{
	int i;
	char *command_shell;

	nputs("Warning, some programs will interfere with network communication and can");
	nputs("cause lost connections.  Do not run any network programs from this DOS shell.");
	nputs("Type 'EXIT' to return to FTP");
/*
*  invoke a put-to-sleep routine which calls netsleep every 8/18ths of a sec
*/
    remove_break();
	tinst();
	command_shell=getenv("COMSPEC");
    if(command_shell!=NULL)
		i=system(command_shell);		/* call DOS */
	 else
		i= -1;
	tdeinst();
    install_break(&foundbreak);

	if(i<0) {
		nputs("\n\nError loading COMMAND.COM");
		nputs("Make sure COMMAND.COM is specified under COMSPEC.");
		nputs("It must also be in a directory which is in your PATH statement.");
      } /* end if */
	if(fromtty) 
    n_cur(n_row(),0);   /* in case screen is messed up rmg 940108 */

return 0;
}   /* end dosescape() */

static void nputs(char *line)
{
  if(fromtty) {
    if(!scrsetup) {
      scrsetup=1;
      if(!display_init) {
        Sgetconfig(&def);   /* get information provided in hosts file */
        display_init=TRUE;
        dos_color=n_color((int) def.color[0]);  /* set color to that set in config file */
      } /* end if */
      n_color((int) def.color[0]);  /* set color to that set in config file */
      n_clear();      /* clear screen */
  #ifdef NOT
      n_wrap(1);          /* cursor positioning */
  #endif
      n_cur(0,0);

      n_puts("National Center for Supercomputing Applications");
      sprintf(printline,"       %s %s",FTP_VERSION,FTP_REV_VERSION);
      n_puts(printline);
    } /* end if */

    if(*line) {
      n_puts(line);
    }
    else
      n_puts("");
  }
  else {
    if(!scrsetup) {
      scrsetup=1;
      if(!display_init) {
        Sgetconfig(&def);   /* get information provided in hosts file */
        display_init=TRUE;
        fputs("National Center for Supercomputing Applications",stderr);
        fputs("\n",stderr);
        sprintf(printline,"       %s %s",FTP_VERSION,FTP_REV_VERSION);
        fputs(printline,stderr);
        fputs("\n",stderr);
      } /* end if */
    } /* end if */

    puts(line);
  }

}   /* end nputs() */

static void nputchar(char ch)
{
#ifdef OLD_WAY
  if(fromtty && ttypass)
		putc(ch,stderr);
	else 
    n_putchar(ch);
#else
  if(!fromtty)
    putc(ch,stdout);
	else 
    n_putchar(ch);
    n_row();
    if(n_col() >= 79)
      n_puts("");
#endif
}   /* end nputchar() */

#ifdef	MSC
static char *stptok(char *p,char *toword,int len,char *delim)
{
    char *adv=toword;
 	int i;
 	int end=0;
 
 	do { 
        for(i=0; i<(int)strlen(delim); i++)
            if(*p==delim[i] || (!*p))
                end++;
        if(!end) {
 			if(adv>=(toword+len-1)) 
				end++;
 			*adv++=*p++;
          } /* end if */
      } while(!end);
 	*adv='\0';
    return(p);
}   /* end stptok() */
 
static char *stpblk(char *ch)
{
    while(*ch==' ' || *ch=='\t')
        ch++;
    return(ch);
}   /* end stpblk() */
#endif

int domore(void)
{
    int ch=0;

    if(usemore) {
        if(lineslft--<=1) {
			printf("-- more --");
			ch=0;
            while(!ch) {
                if(!kbhit())       /* if no char available */
					Stask();		/* keep the connection alive */
				else
                    ch=getch();   /* else get a character */
                if(foundbreak) {
					foundbreak=0;
					ch=32;
                  } /* end if */
              } /* end while */
			printf("\r          \r");
            if(ch==13)
				lineslft++;
            else
                lineslft=numlines;
          } /* end if */
      } /* end if */
	return(ch);
}   /* end domore() */

/************************************************************************/
/*                             check_file_name                          */
/*                                                                      */
/*  A function to check for and enforce MS-DOS file name compatibility  */
/*  and uniqueness.  All non-MS-DOS filename characters from 'in_fname' */
/*  are deleted.  If the filename exists in the current directory, a    */
/*  unique name is created to avoid file name collision.                */
/*  'out_fname' must be large enough for the full filename (13 bytes).  */
/*                                                                      */
/*       Returns:    pointer to 'out_fname'                             */
/*                                                                      */
/*       Allen W. Todd                        Dec 24, 1990              */
/*                                                                      */
/************************************************************************/
char *check_file_name(char *in_fname,char *out_fname)
{
    char buf[80],
        *s,
        *fname,
        *fextn,
        num_buf[20];
    long count=0L;

    strcpy(buf,in_fname);    /* make local copy */
    strlwr(buf);              /* convert to lower case */
    while((s=strpbrk(buf, "\"*+,/:;<=>?[\\]^|"))!=NULL)
        strcpy(s,s+1);        /* delete non-DOS chars */
    fname=&buf[0];           /* separate filename from file extension */
    if((fextn=strchr(buf,'.'))!=NULL)
        *fextn++='\0';
    if(strlen (fname)>8)
        *(fname+8)='\0';   /* fix filename @ 8 char */
    if(strlen(fextn)>3)
        *(fextn+3)='\0';   /* fix file extension @ 3 char */
    strcpy(out_fname,fname);     /* build complete filename */
    if(fextn) {
        strcat(out_fname,".");
        strcat(out_fname,fextn);
      } /* end if */
  /* RMG not_noclobber - not implemented yet */
//    if(check_overwrite) {
    while(access(out_fname,0)==0) {   /* if file already exists */
        ltoa(count++,num_buf,10);
        strcpy(out_fname,fname);     /* build complete filename */
        if((strlen(fextn)<3) && (strlen(num_buf)+strlen(fextn)<=3))
            strcat(fextn,num_buf);
        else {
            if(strlen(fname)>strlen(num_buf))
                strcpy(out_fname+(strlen(fname)-strlen(num_buf)),num_buf);
            else
                strcat(out_fname,num_buf);
          } /* end else */
        if(fextn) {
            strcat(out_fname,".");
            strcat(out_fname,fextn);
          } /* end if */
      } /* end while */
//  } /* end noclobber */
   return(out_fname);
}   /* end check_file_name() */


