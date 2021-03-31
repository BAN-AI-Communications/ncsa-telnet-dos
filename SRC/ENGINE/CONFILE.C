/*
*   Confile.c
*   Split from util.c 5/88
*   Reads and stores the appropriate information for the config file
*
*   version 2, full session layer, TK started 6/17/87
*
*****************************************************************************
*																			*
*	  part of:																*
*	  Network kernel for NCSA Telnet										*
*	  by Tim Krauskopf														*
*																			*
*	  National Center for Supercomputing Applications						*
*	  152 Computing Applications Building									*
*	  605 E. Springfield Ave.												*
*	  Champaign, IL  61820													*
*																			*
*     This program is in the public domain.                                 *
*                                                                           *
*****************************************************************************
*
*	Revision History:
*
*	Initial release - 11/87 TK
*	Cleanup for 2.3 release - 6/89 QK
*
*/

/*
*	Includes
*/
/*#define DEBUG */         /* define this to enable debugging print statements */
#include "debug.h"

#ifdef DEBUG
#include <conio.h>
#endif
#ifdef __TURBOC__
#include "turboc.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#ifdef __WATCOMC__
#include <process.h>
#endif
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#include "whatami.h"
#include "hostform.h"
#include "externs.h"
#define CONFIG_MASTER
#include "confile.h"

/* STATIC function declarations */
static FILE *pfopen(char *name,char *mode);	/* open file with required mode */

/************************************************************************/
/*  Sreadhosts
*   read in the hosts file into our in-memory data structure.
*   Handle everything by keyword, see docs for specifications about file.
*/
int Sreadhosts(void)
{
	FILE *fp;
    int c,retval,i,j;
    char *envstr, envnam[8];

	Smachlist=Smptr=NULL;
	mno=0;
	Sspace=malloc(256);				/* get room for gathering stuff */
	if(Sspace==NULL) {
		Serrline(901);
		return(1);
      } /* end if */

	position=constate=inquote=lineno=0;   /* state vars */	
	if(NULL==(fp=pfopen(Smachfile,"r"))) {	/* open the configuration file */
		Serrline(900);
		return(1);
      } /* end if */
	retval=0;
	while(!retval) {
		c=fgetc(fp);
		if(c=='#' && !inquote)
			while(c!=EOF && c!='\n' && c!='\r')		/* skip to EOL */
				c=fgetc(fp);
        if(c=='\n' || c=='\r')
			lineno++;
 		retval=Scontoken(c);		/* add character to token */
      } /* end while */
	fclose(fp);

    position=constate=inquote=0;
    for(i=0; i<100; i++) {
        sprintf(envnam,"NCSA%2.2d",i);
        if(envstr=getenv(envnam)) {
            if(*envstr=='\"')
                *(envstr+strlen(envstr++)-2)=(char)NULL;
            for(j=0,retval=0; *(envstr+j) && !retval; j++) {
                if((*(envstr+j)=='~') || (*(envstr+j)==' '))
                    *(envstr+j)='=';
                retval=Scontoken(*(envstr+j));
              } /* end for */
            Scontoken(';');     /* make sure last token is taken */
          } /* end if */
      } /* end for */

    if(commandLineOveride) {
        position=constate=inquote=0;
        for(i=0,retval=0; *(commandLineOveride+i) && !retval; i++) {
            if((*(commandLineOveride+i)=='~') || (*(commandLineOveride+i)==' '))
                *(commandLineOveride+i)='=';
            retval=Scontoken(*(commandLineOveride+i));
          } /* end for */
        free(commandLineOveride);
      } /* end if */

    free(Sspace);
    Smadd("default");               /* make sure name is in list */
	if(retval==EOF)				/* EOF is normal end */
		return(0);
	else
		return(retval);
}

/************************************************************************/
/*  ncstrcmp
*   No case string compare.
*   Only returns 0=match, 1=no match, does not compare greater or less
*   There is a tiny bit of overlap with the | 32 trick, but shouldn't be
*   a problem.  It causes some different symbols to match.
*/
int ncstrcmp(char *sa,char *sb)
{
	while(*sa && *sa<33)		/* don't compare leading spaces */
		sa++;
	while(*sb && *sb<33)
		sb++;
	while(*sa && *sb) {
		if((*sa!=*sb) && ((*sa | 32)!=(*sb | 32)))
			return(1);
		sa++;
		sb++;
	  }	/* end while */
	if(!*sa && !*sb)		/* if both at end of string */
		return(0);
	else
		return(1);
}	/* end ncstrcmp() */

/************************************************************************/
/*  Serrline
*   prints the line number of the host file error and posts the event
*   for the line number error and posts the hosts file error.
*/
void Serrline(int n)
{
	char *p;

	p=neterrstring(-1);
	sprintf(p,"Config file: error at line %4d",lineno+1);
	netposterr(-1);
	netposterr(n);
}

/************************************************************************/
/* Scontoken
*  tokenize the strings which get passed to Sconfile.
*  Handles quotes and uses separators:  <33, ;:=
*/
int Scontoken(int c)
{
	int retval;

	if(c==EOF) {
		Sspace[position++]='\0';
		Sconfile(Sspace);
		if(!Sflags[0]) {			/* make sure last entry gets copied */
			if(ncstrcmp("default",Smptr->sname))
				Scopyfrom("default");
			else
				Scopyfrom("==");
    }
    return(-1);
  }
	if(!position && Sissep(c))		/* skip over junk before token */
    return(0);
	if(inquote || !Sissep(c)) {
		if(position>200) {
			Serrline(903);
			return(1);
    }
/*
*   check for quotes, a little mixed up here, could be reorganized
*/
		if(c=='"' ) {
			if(!inquote) {			/* beginning of quotes */
				inquote=1;
				return(0);
            }
			else
				inquote=0;		/* turn off flag and drop through */
        }
		else {
			if(c=='\n') {			/* check for EOL inside quotes */
				Serrline(904);
				return(1);
            }
			Sspace[position++]=(char)c;	/* include in current string */
			return(0);
        }
    }
	Sspace[position++]='\0';
	retval=Sconfile(Sspace);			/* pass the token along */
	position=0;
	inquote=0;
	Sspace[0]='\0';
	return(retval);
}

/************************************************************************/
/*  Sconfile
*   take the characters read from the file and parse them for keywords
*   which require configuration action.
*/
int Sconfile(char *s)
{
	int i,a,b,c,d;

#ifdef QAK
printf("Sconfile: %s\n",s);
#endif

	switch(constate) {
		case 0:								/* lookup keyword */
			if(!(*s))						/* empty token */
				return(0);

			for(i=1; *Skeyw[i] && ncstrcmp(Skeyw[i],s); i++);	/* search the keyboard list for this keyword */

			if(!(*Skeyw[i])) {			/* not in list */
				Serrline(902);
				return(0);				/* don't die - helps backward compatibility */
      } /* end if */
			constate=100+i;	/* change to state for keyword */
/*
*  check if this is a machine specific parm without a machine to
*  give it to.  "name" being the only machine specific parm allowed, of course
*/
			if(Smptr==NULL && constate>CONNAME && constate<=NUMSPECS) {
				Serrline(905);
				return(1);
      }
			break;

		case CONNAME:		/* session name */
/*
*  allocate space for upcoming parameters
*/
      constate=0;             /* back to new keyword */
      if(Smachlist==NULL) {
        Smachlist=(struct machinfo *)malloc(sizeof(struct machinfo));
        if(Smachlist==NULL) {   /* check for bad malloc */
          Serrline(901);
          return(0);              /* don't die - helps backward compatibility */
        } /* end if */
				Smptr=Smachlist;
      }
			else {
				if(!Sflags[0]) {
					if(ncstrcmp("default",Smptr->sname))
            Scopyfrom("default");
					else
						Scopyfrom("==");	/* to make sure 'default' gets set */
        }
				Smptr->next=(struct machinfo *)malloc(sizeof(struct machinfo));
        if(Smptr->next==NULL) {     /* check for bad malloc */
          Serrline(901);
          return(0);              /* don't die - helps backward compatibility */
        } /* end if */
        Smptr=Smptr->next;
      }
			Smptr->next=NULL;
			Smptr->hname=NULL;				/* guarantee to be null */
			Smptr->sname=malloc(position);	/* size of name string */
      if(Smptr->sname!=NULL)          /* try to avoid copying into a bad allocation */
        strcpy(Smptr->sname,s);         /* keep name field */
      Smptr->ftpoptions=NULL;         /* no options */
      for(i=0; i<NUMSPECS-99; i++)
        Sflags[i]=0;        /* we have no parms */
			Smptr->mno=++mno;				/* new machine number */
			break;

		case CONHOST:	/* also a host name */
            constate=0;
            Smptr->hname=malloc(position);
            if(Smptr->hname==NULL) {    /* check for bad allocate */
                Serrline(901);
                return(0);              /* don't die - helps backward compatibility */
              } /* end if */
			strcpy(Smptr->hname,s);
			Sflags[CONHOST-100]=1;	/* set the flag to indicate hostname is found */
			break;

		case CONIP:		/* IP number for host */
			if(4!=sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)) {
				Serrline(906);
				return(3);
			  }
			Smptr->hostip[0]=(char)a; 	/* keep number */
			Smptr->hostip[1]=(char)b;
			Smptr->hostip[2]=(char)c;
			Smptr->hostip[3]=(char)d;
			Smptr->mstat=HFILE;
			constate=0;
			Sflags[CONIP-100]=1;		/* set the flag to indicate ip number found */
			break;

		case CONGATE:	/* this machine is a gateway */
			Smptr->gateway=(unsigned char)atoi(s);		/* gateway level */
			constate=0;
			Sflags[CONGATE-100]=1;		/* set the flag for this name being a gateway */
			break;

		case CONCOLOR:		/* support old color format */
			Smptr->nfcolor=s[1]-48;
			Smptr->nbcolor=s[0]-48;
			Smptr->ufcolor=s[3]-48;
			Smptr->ubcolor=s[2]-48;
			Smptr->bfcolor=s[5]-48;
			Smptr->bbcolor=s[4]-48;
			constate=0;
			Sflags[CONNF-100]=1;		/* sets them all at one shot */
			Sflags[CONNB-100]=1;
			Sflags[CONBF-100]=1;
			Sflags[CONBB-100]=1;
			Sflags[CONUF-100]=1;
			Sflags[CONUB-100]=1;
			break;

        case CONBKSP:   /* backspace parameter */
			if(!ncstrcmp(s,"backspace"))
				Smptr->bksp=8;
			else
				Smptr->bksp=127;
			constate=0;
			Sflags[CONBKSP-100]=1;
			break;

		case CONBKSC:		/* the number of line for scrollback */
			Smptr->bkscroll=atoi(s);
			constate=0;
			Sflags[CONBKSC-100]=1;
			break;

		case CONRETR:		/* how long before retransmitting */
      Smptr->retrans=atoi(s)*18;  /* (roughly) takes seconds now.  rmg 931100 */
			constate=0;
			Sflags[CONRETR-100]=1;
			break;

		case CONWIND:		/* transmission window for this host */
			Smptr->window=atoi(s);
			constate=0;
			Sflags[CONWIND-100]=1;
			break;

		case CONSEG:		/* segment size */
			Smptr->maxseg=atoi(s);
			constate=0;
			Sflags[CONSEG-100]=1;
			break;

		case CONMTU:	/* maximum transmission unit */
			Smptr->mtu=atoi(s);
			constate=0;
			Sflags[CONMTU-100]=1;
			break;

		case CONNS:		/* nameserver level */
			Smptr->nameserv=(unsigned char)atoi(s);
			if(!Sns || (Sns->nameserv>Smptr->nameserv))	/* keep NS */
				Sns=Smptr;
			constate=0;
			Sflags[CONNS-100]=1;
			break;

		case CONTO:		/* time until time out */
      i=atoi(s)*18;  /* takes seconds now.  rmg 931100 */
			if(i>2) {
				Smptr->conto=i;
				Sflags[CONTO-100]=1;
			  }
			constate=0;
			break;

    case CONCRMAP:    /* carriage return mapping  rmg 931100 */
      if(!ncstrcmp(s,"crlf"))
        Smptr->crmap=0;
      else if(!ncstrcmp(s,"crnul"))
        Smptr->crmap=3;
      else if(!ncstrcmp(s,"4.3BSDCRNUL"))
        Smptr->crmap=3;
      else if(!ncstrcmp(s,"cr"))
        Smptr->crmap=1;
      else if(!ncstrcmp(s,"lf"))
        Smptr->crmap=2;
      else
        Smptr->crmap=0;
			Sflags[CONCRMAP-100]=1;
			constate=0;
			break;

		case CONDUP:		/* duplex */
			if(!ncstrcmp(s,"half")) {
				Smptr->halfdup=1;
				Sflags[CONDUP-100]=1;
			  }
			constate=0;
			break;

		case CONWRAP:		/* whether lines wrap */
			if('Y'==toupper(s[0])) 
				Smptr->vtwrap=1;
			else
				Smptr->vtwrap=0;
			Sflags[CONWRAP-100]=1;
			constate=0;
			break;

		case CONWIDE:		/* how wide the screen is */
			if(132==atoi(s)) 
				Smptr->vtwidth=132;
			else
				Smptr->vtwidth=80;
			Sflags[CONWIDE-100]=1;
			constate=0;
			break;

		case CONFONT:		/* the font type */
            constate=0;
            Smptr->font=malloc(position);
            if(Smptr->font==NULL) {    /* check for bad allocate */
                Serrline(901);
                return(0);              /* don't die - helps backward compatibility */
              } /* end if */
            strcpy(Smptr->font,s);
			Sflags[CONFONT-100]=1;
			break;

		case CONFSIZE:		/* the font point size */
            Smptr->fsize=(unsigned char)atoi(s);
			Sflags[CONFSIZE-100]=1;
			constate=0;
			break;

		case CONNF:		/* foreground normal color */
			Scolorset(&Smptr->nfcolor,s);
			Sflags[CONNF-100]=1;
			constate=0;
			break;

		case CONNB:		/* background normal color */
			Scolorset(&Smptr->nbcolor,s);
			Sflags[CONNB-100]=1;
			constate=0;
			break;

		case CONRF:
		case CONBF:		/* blink foreg color */
			Scolorset(&Smptr->bfcolor,s);
			Sflags[CONBF-100]=1;	/* in copyfrom, r's are really b's */
			constate=0;
			break;

		case CONRB:
		case CONBB:		/* blink bg color */
			Scolorset(&Smptr->bbcolor,s);
			Sflags[CONBB-100]=1;
			constate=0;
			break;

		case CONUF:		/* foreground underline color */
			Scolorset(&Smptr->ufcolor,s);
			Sflags[CONUF-100]=1;
			constate=0;
			break;

		case CONUB:		/* bg underline color */
			Scolorset(&Smptr->ubcolor,s);
			Sflags[CONUB-100]=1;
			constate=0;
			break;

		case CONCLMODE:		/* save cleared lines */
			if('N'==toupper(s[0])) 
				Smptr->clearsave=0;
			else
				Smptr->clearsave=1;
			Sflags[CONCLMODE-100]=1;
			constate=0;
			break;

		case CONPORT:		/* the port number */
			i=atoi(s);
			if(i<1)
				i=23;
			Smptr->port=i;
			Sflags[CONPORT-100]=1;
			constate=0;
			break;

		case CONFTPBAK:		/* the options for the ftp command line */
      constate=0;
      Smptr->ftpoptions=malloc(position);
      if(Smptr->ftpoptions==NULL) {    /* check for bad allocate */
        Serrline(901);
        return(0);              /* don't die - helps backward compatibility */
      } /* end if */
      strcpy(Smptr->ftpoptions,s);
			Sflags[CONFTPBAK-100]=1;
			break;

        case CONDEBUGCONSOLE:     /* the debugging level for console output */
            constate=0;
            i=atoi(s);
            if(i<0)
                i=0;
            else if(i>3)
                i=3;
            Smptr->consoledebug=i;
            Sflags[CONDEBUGCONSOLE-100]=1;
			break;

        case CONMAPOUTPUT:     /* the output mapping flag for each machine */
            if('Y'==toupper(s[0]))
                Smptr->mapoutflag=1;
			else
                Smptr->mapoutflag=0;
            Sflags[CONMAPOUTPUT-100]=1;
			constate=0;
            break;

/*
*  now the one-time entries
*  Generally this information goes into the "Scon" structure for later
*  retrieval by other routines.
*
*/
		case CONNDOM:				/* DOMAIN number of retries */
			i=atoi(s);
			if(i>1)
				Scon.ndom=i;
			constate=0;
			break;

		case CONMASK:		/* the subnet mask */
			if(4!=sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)) {
				Serrline(907);
				return(3);
			  }
			Scon.netmask[0]=(unsigned char)a;
			Scon.netmask[1]=(unsigned char)b;
			Scon.netmask[2]=(unsigned char)c;
			Scon.netmask[3]=(char)d;
			Scon.havemask=1;
			constate=0;
			break;

		case CONMYIP:		/* what my ip number is */
			constate=0;
			if(!ncstrcmp(s,"rarp")) {
				movebytes(Scon.myipnum,s,4);
				netsetip("RARP");
				break;
			  }
			if(!ncstrcmp(s,"bootp")) {
				movebytes(Scon.myipnum,s,4);
				netsetip("BOOT");
				break;
			  }
#ifdef	NETX25
			if(!ncstrcmp(s,"x25")) {
				movebytes(Scon.myipnum,s,3);
				netsetip("X25");
				break;
			  }
#endif
			if(4!=sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)) {
				Serrline(908);
				return(3);
			  }
			Scon.myipnum[0]=(char)a; 	/* put number back in s */
			Scon.myipnum[1]=(char)b;
			Scon.myipnum[2]=(char)c;
			Scon.myipnum[3]=(char)d;
			netsetip(Scon.myipnum);		/* make permanent set */
			break;

		case CONHPF:				/* File name for HP dump */
			Snewhpfile(s);
			constate=0;
			break;

		case CONPSF:				/* File name for PS dump */
			Snewpsfile(s);
			constate=0;
			break;

		case CONTEKF:				/* File name for Tek dump */
			Snewtekfile(s);
			constate=0;
			break;

		case CONME:				/* what my name is */
			strncpy(Scon.me,s,30);
			Scon.me[30]='\0';
			constate=0;
			break;

		case CONCCOL:		/* set all the colors */
			for(i=0; i<3; i++)
        Scon.color[i]=(unsigned char) (((s[i*2]-48)<<4)+(s[i*2+1]-48));
			constate=0;
			break;

		case CONHW:			/* what hardware are we using? */
			i=strlen(s);
			if(i>9) i=9;
			s[i]='\0';
			i--;
			while(i--)
				s[i]=(char)tolower((int)s[i]);
			strcpy(Scon.hw,s);
			constate=0;
			break;

		case CONADDR:		/* segment address for board */
			sscanf(s,"%x",&i);
			Scon.address=i;
			constate=0;
			break;

		case CONIOA:		/* io address for board */
			sscanf(s,"%x",&i);
			Scon.ioaddr=i;
			constate=0;
			break;

		case CONDEF:						/* default domain */
            constate=0;
            Scon.defdom=malloc(position);   /* space for name */
            if(Scon.defdom==NULL) {    /* check for bad allocate */
                Serrline(901);
                return(0);              /* don't die - helps backward compatibility */
              } /* end if */
            strcpy(Scon.defdom,s);          /* copy it in */
			break;

		case CONINT:			/* what IRQ to use */
			sscanf(s,"%x",&i);
			Scon.irqnum=(char)i;
			constate=0;
			break;

		case CONBIOS:		/* use BIOS screen writes */
			if(toupper(*s)=='Y') {
				Scwritemode(0);
				Scon.bios=1;
			  }
			constate=0;
			break;

		case CONTEK:	/* whether to support tek graphics */
			if(toupper(*s)=='N') {
				Stekmode(0);
				Scon.tek=0;
      }
			constate=0;
			break;

#ifdef MAYBE /* RMG */
    case CONGIN6:   /* optional sixth input char for GIN */
      sscanf("%c",&Scon.gin6); /* space for name */
      constate=0;
      break;
#endif

		case CONVIDEO:		/* what type of screen we have */
			i=strlen(s);
			if(i>9)
				i=9;
			s[i]='\0';
			if(!strcmp(s,"ega43") || !strcmp(s,"vga50") ) {		/* check for special video modes */
				if(!strcmp(s,"ega43")) {
					Scon.ega43=1;
					strcpy(s,"ega");
				 }	/* end if */
				else{
					Scon.ega43=2;
					strcpy(s,"vga");
				}
            } else {
                Scon.ega43=0;
                strcpy(s,"ega");
            }
			strcpy(Scon.video,s);
			i--;
			while(i--)
				s[i]=(char)tolower((int)s[i]);
			constate=0;
			break;

		case CONFTP:		/* is ftp enabled? */
      if(toupper(*s)=='N')
         Scon.ftp=0;
      else
         Scon.ftp=1;
      constate=0;
			break;

    case CONRCP:	/* is rcp enabled? */
      if(toupper(*s)=='N')
        Scon.rcp=0;
      else
        Scon.rcp=1;
      constate=0;
      break;

    case CONFTPWRT: /* do we allow incoming ftp writes?  rmg 931100 */
      if(toupper(*s)=='N')
        Scon.ftpw=0;
      else
        Scon.ftpw=1;
      constate=0;
      break;

		case CONPASS:		/* do we need a password for ftp? */
      constate=0;
      Scon.pass=malloc(position); /* space for name */
      if(Scon.pass==NULL) {    /* check for bad allocate */
        Serrline(901);
        return(0);              /* don't die - helps backward compatibility */
      } /* end if */
      strcpy(Scon.pass,s);            /* copy it in */
      break;

		case CONCAP:	/* capture file name */
			Snewcap(s);
			constate=0;
			break;

		case CONTTYPE:		/* what terminal type to emulate */
            constate=0;
            Scon.termtype=malloc(position);
            if(Scon.termtype==NULL) {    /* check for bad allocate */
                Serrline(901);
                return(0);              /* don't die - helps backward compatibility */
              } /* end if */
            strcpy(Scon.termtype,s);
			break;

		case CONFROM:	/* copy the rest from another entry in the table */
			Scopyfrom(s);
			Sflags[0]=1;	/* indicate did copy from */
			constate=0;
			break;

		case CONARPTO:	/* need to lengthen arp time-out (secs) */
			i=atoi(s);
			if(i>0)
				netarptime(i);
			constate=0;
			break;

		case CONZONE:		/* the appletalk zone we are in */
#ifdef KIPCARD
			KIPsetzone(s);
#endif
			constate=0;
			break;

		case CONDOMTO:				/* DOMAIN timeout value */
      i=atoi(s)*18; /* converted to seconds.  rmg 931100 */
			if(i>1)
				Scon.domto=i;
			constate=0;
			break;

    case CONKBFILE:       /* File name for alternate keyboard map */
      Scon.keyfile=malloc(position);  /* store keyfile name for reloading  rmg 940216 */
                                      /* other filenames are set in user.c */
      if(Scon.keyfile==NULL) {    /* check for bad allocate */
        Serrline(901);
        return(0);              /* don't die - helps backward compatibility */
      } /* end if */
      strcpy(Scon.keyfile,s);
      if(read_keyboard_file(s)<0) { /* Now replaces look.c lines  rmg 931100 */
        puts("Error reading settings from keymap file.");
        getch();
      } /* end if */
      constate=0;
			break;

		case CONWIRE:			/* set the wire type for 3C503 board */
			if(!strcmp(s,"thick"))
				Scon.wire=0;
			if(!strcmp(s,"thin"))
				Scon.wire=1;
			constate=0;
			break;

		case CONCURSORTOP:		/* what scan line the cursor starts on */
			Scon.cursortop=atoi(s);
			constate=0;
			break;

		case CONCURSORBOTTOM:	/* what scan line the cursor ends on */
			Scon.cursorbottom=atoi(s);
			constate=0;
			break;

		case CONWINDOWGOAWAY:	/* whether windows go away when they close, or whether they wait for a keypress */
			if(toupper(*s)=='Y')
				Scon.wingoaway=1;
			if(toupper(*s)=='N')
				Scon.wingoaway=0;
			constate=0;
			break;

		case CONAUTOSCROLL:		/* do we automatically scroll in scrollback */
			if(!strcmp(s,"no"))
				Scon.autoscroll=0;
			else
				Scon.autoscroll=1;
			constate=0;
			break;

		case CONCLOCK:			/* display the clock? */
			if(!strcmp(s,"no") || !strcmp(s,"off"))
				Scon.clock=0;
			else
				Scon.clock=1;
			constate=0;
			break;

		case CONBROADCAST:		/* broadcast IP number for network */
			if(4!=sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)) {
				Serrline(908);
				return(3);
			  }
			Scon.broadip[0]=(char)a; 	/* keep number */
			Scon.broadip[1]=(char)b;
			Scon.broadip[2]=(char)c;
			Scon.broadip[3]=(char)d;
			constate=0;
			break;

    case CONOUTPUTMAP:  /* File name for alternate output mapping */
        read_output_file(s);    /* read in the output mapping file */
        constate=0;
        default_mapoutput=1;
        break;

    case CONBEEP:   /* turn musical note on */
        if('Y'==toupper(s[0]))
            beep_notify=1;
        constate=0;
        break;

		case CONSERVICES:		/* path to services file */
      service_file(s);
			constate=0;
			break;

        default:
			constate=0;
			break;
	  }
	return(0);
}

/************************************************************************/
/*  Scopyfrom
*   Look at the Sflags array to determine which elements to copy from
*   a previous machine's entries.  If a machine name as been given as
*   "default", the state machine will fill in the fields from that
*   machine's entries.
*
*   If the machine name to copyfrom is not present in the list, set the
*   program default values for each field.
*/
void Scopyfrom(char *s)
{
	struct machinfo *m;
	int i;

	m=Shostlook(s);			/* search list */

	for(i=3; i<=NUMSPECS-100; i++) 		/* through list of parms */
		if(!Sflags[i]) {
			if(m) 							/* copy old value */
				switch(100+i) {
					case CONHOST:
						Smptr->hname=m->hname;
						break;

					case CONIP:
						movebytes(Smptr->hostip,m->hostip,4);
						Smptr->mstat=m->mstat;
						break;

					case CONGATE:			/* gateways cannot be copied from */
						Smptr->gateway=0;
						break;

					case CONNS:					/* can't copy nameservers either */
						Smptr->nameserv=0;
						break;

					case CONBKSP:
						Smptr->bksp=m->bksp;
						break;

					case CONBKSC:
						Smptr->bkscroll=m->bkscroll;
						break;

					case CONCLMODE:
						Smptr->clearsave=m->clearsave;
						break;

					case CONRETR:
						Smptr->retrans=m->retrans;
						break;

					case CONWIND:
						Smptr->window=m->window;
						break;

					case CONSEG:
						Smptr->maxseg=m->maxseg;
						break;

					case CONMTU:
						Smptr->mtu=m->mtu;
						break;

					case CONTO:
						Smptr->conto=m->conto;
						break;

					case CONCRMAP:
						Smptr->crmap=m->crmap;
						break;

					case CONDUP:
						Smptr->halfdup=m->halfdup;
						break;

					case CONWRAP:
						Smptr->vtwrap=m->vtwrap;
						break;

					case CONWIDE:
						Smptr->vtwidth=m->vtwidth;
						break;

					case CONNF:
						Smptr->nfcolor=m->nfcolor;
						break;

					case CONNB:
						Smptr->nbcolor=m->nbcolor;
						break;

					case CONBF:
						Smptr->bfcolor=m->bfcolor;
						break;

					case CONBB:
						Smptr->bbcolor=m->bbcolor;
						break;

					case CONUF:
						Smptr->ufcolor=m->ufcolor;
						break;

					case CONUB:
						Smptr->ubcolor=m->ubcolor;
						break;

					case CONFONT:
						Smptr->font=m->font;
						break;

					case CONFSIZE:
						Smptr->fsize=m->fsize;
						break;

					case CONPORT:
						Smptr->port=m->port;
						break;

					case CONFTPBAK:
						Smptr->ftpoptions=m->ftpoptions;
						break;

                    case CONDEBUGCONSOLE:
                        Smptr->consoledebug=m->consoledebug;
						break;

                    case CONMAPOUTPUT:
                        Smptr->mapoutflag=m->mapoutflag;
						break;

                    default:
						break;
				  }	/* end switch */
			else
				switch(100+i) {		/* m=NULL, install default values */
					case CONHOST:
						Smptr->hname=NULL;
						break;

					case CONIP:
						Smptr->mstat=NOIP;
						break;

					case CONGATE:			/* gateways cannot be copied from */
						Smptr->gateway=0;
						break;

					case CONBKSP:
						Smptr->bksp=127;
						break;

					case CONBKSC:
						Smptr->bkscroll=0;
						break;

					case CONCLMODE:
						Smptr->clearsave=1;
						break;

					case CONRETR:
						Smptr->retrans=SMINRTO;
						break;

					case CONWIND:
						Smptr->window=DEFWINDOW;
						break;

					case CONSEG:
						Smptr->maxseg=DEFSEG;
						break;

					case CONMTU:
						Smptr->mtu=TSENDSIZE;
						break;

					case CONNS:					/* can't copy nameservers either */
						Smptr->nameserv=0;
						break;

					case CONTO:
						Smptr->conto=CONNWAITTIME;
						break;

					case CONCRMAP:
            Smptr->crmap=0;
						break;

					case CONDUP:
						Smptr->halfdup=0;
						break;

					case CONWRAP:
						Smptr->vtwrap=0;
						break;

					case CONWIDE:
						Smptr->vtwidth=80;
						break;

					case CONNF:
						Smptr->nfcolor=NFDEF;
						break;

					case CONNB:
						Smptr->nbcolor=NBDEF;
						break;

					case CONBF:
						Smptr->bfcolor=BFDEF;
						break;

					case CONBB:
						Smptr->bbcolor=BBDEF;
						break;

					case CONUF:
						Smptr->ufcolor=UFDEF;
						break;

					case CONUB:
						Smptr->ubcolor=UBDEF;
						break;

					case CONFONT:
						Smptr->font="Monaco";
						break;

					case CONFSIZE:
						Smptr->fsize=9;
						break;

					case CONPORT:
						Smptr->port=23;			/* the telnet port */
						break;

          case CONFTPBAK:     /* ftp back to workstation option string */
            Smptr->ftpoptions=NULL;
						break;

          case CONDEBUGCONSOLE:   /* console debugging setting */
            Smptr->consoledebug=0;
            break;

          case CONMAPOUTPUT:      /* output mapping flag */
            Smptr->mapoutflag=0;
						break;

          default:
						break;
				  }	/* end switch */
		  }	/* end if */
	Sflags[0]=1;					/* set that this machine was copied */
}

/************************************************************************/
/* Shostfile
*   if the user wants to change the host file name from 'config.tel' to
*   something else.
*/
void Shostfile(char *ptr)
{
	Smachfile=ptr;	
/*
*  note that the area with the file name must stay allocated for
*  later reference, typically it is in some argv[] parm.
*/
}

#ifdef OLD_WAY
/************************************************************************/
/* Shostpath
*   if the user wants to change the host path name
*/
void Shostpath(char *ptr)
{
	Smachpath=ptr;
/*
*  note that the area with the file name must stay allocated for
*  later reference, typically it is in some argv[] parm.
*/
}
#endif


/************************************************************************/
/*  get host by name
*   Given the name of a host machine, search our database to see if we
*   have that host ip number.  Search first the name field, and then the
*   hostname field.  If the IP # is given, returns a ptr to the
*   default machine record with that IP # installed.
*   Returns the pointer to a valid record, or NULL if the IP # cannot
*   be deciphered.  
*/
struct machinfo *Sgethost(char *machine)
{
	int i,j,k,l;
	unsigned char ipto[4],myipnum[4],xmask[4];
	unsigned long hnum;
	struct machinfo *m=NULL;

/*
*  First, check for the pound sign character which means we should use
*  the current netmask to build an IP number for the local network.
*  Take the host number, install it in the ipto[] array.  Then mask
*  in my IP number's network portion to build the final IP address.
*/
	if('#'==machine[0]) {		/* on my local network */
		netgetip(myipnum);
    netgetmask(xmask);      /* mask of network portion of IP # */
    sscanf(&machine[1],"%ld",&hnum);/* host number for local network */
    for(i=3; i>=0; i--)
			ipto[i]=(unsigned char)((hnum>>(i*8)) & 255L);	/* take off a byte */
		for(i=0; i<4; i++) 
			ipto[i] |= (myipnum[i] & xmask[i]);		/* mask new one in */
	  }
/*
*  next, is it an IP number?  We take it if the number is in four
*  parts, separated by periods.
*/
	else 
		if(4==sscanf(machine,"%d.%d.%d.%d",&i,&j,&k,&l)) {	/* given ip num */
			ipto[0]=(unsigned char)i;
			ipto[1]=(unsigned char)j;
			ipto[2]=(unsigned char)k;
			ipto[3]=(unsigned char)l;
		  }
/*
*  lastly, it must be a name, first check the local host table
*  A first number of 127 means that it doesn't have an IP number, but
*  is in the table(strange occurrence)
*/
		else {									/* look it up */
			m=Shostlook(machine);
			if(m==NULL) {
				netposterr(805);			/* informative */
				return(NULL);
			  } 
			if(m->mstat<HAVEIP) {
				netposterr(806);			/* informative */
				return(NULL);
			  }
		  }
	if(!m) {
		m=Shostlook("default");
		movebytes(m->hostip,ipto,4);		/* copy in newest host # */
		m->mstat=HAVEIP;					/* we have the IP # */
	  }
  return(m);
}

/**************************************************************************/
/*  Sissep
*   is the character a valid separator for the hosts file?
*   separators are white space, special chars and :;=
*
*/
int Sissep(int c)
{
	if(c<33 || c==':' || c==';' || c=='=')
		return(1);
	return(0);
}

/************************************************************************/
/*  Ssetgates
*   set up the gateway machines and the subnet mask after netinit()
*   and start up ftp and rcp for them.
*/
void Ssetgates(void)
{
	struct machinfo *m;
	int level,again;

	BUG("Ssetgates");
	if(Scon.havemask) {					/* leave default unless specified */
		netsetmask(Scon.netmask);
	  }	/* end if */
/*
*  Search the list of machines for gateway flags.
*  Invoke netsetgate in increasing order of gateway level #s.
*  Terminates when it gets through list without finding next higher number.
*/
	level=0;
	BUG("looking for gateway");
	do {
		level++;
		again=0;
		m=Smachlist;
		while(m!=NULL) {
			BUG("checking a machine");
			if(m->gateway==(unsigned char)level && m->mstat>=HAVEIP) {
				BUG("have gateway");
				netsetgate(m->hostip);
			}	/* end if */
			BUG("after first if");
			if((m->gateway) == (unsigned char)(level+1))
				again=1;
			BUG("after second if");
			m=m->next;
		}
		BUG("need to go through machlist again?");
#ifdef DEBUG
		getch();
#endif
	} while(again);
#ifdef FTP
	BUG("about to set ftp mode");
	Sftpmode(Scon.ftp);
  setftpwrt(Scon.ftpw); /* rmg 931100 */
#endif
#ifdef RCP
  BUG("need to go through machlist again?");
	BUG("about to set rcp mode");
	Srcpmode(Scon.rcp);
#endif

	BUG("returning from Ssetgates");
}

/**********************************************************************
*  Function	:	pfopen
*  Purpose	:	open the file in the current directory, and if it is not
*				there, then search the DOS PATH variable for it
*  Parameters	:
*			name - the filename to open
*			mode - the mode to open the file in (see fopen())
*  Returns	:	a pointer to a FILE, or NULL for file not found
*  Calls	:	getenv(), fopen()
*  Called by	:	Sreadhosts()
**********************************************************************/
static FILE *pfopen(char *name,char *mode)
{
	char *path,				/* pointer to the PATH variable in the environment */
		filename[80],		/* storage for the pathes in the PATH variable */
		*fn;				/* temporary pointer to filename[] */
	FILE *fp;				/* pointer to the FILE opened */

	if((fp=fopen(name,mode))!=NULL)	/* check for the file being in the current directory */
		return(fp);
	path=getenv("PATH");		/* set the pointer to the PATH environment variable */
	if(path==NULL)	/* no PATH */
		return(NULL);
	while(*path) {	/* search for the file along the path */
		fn=filename;	/* reset the filename pointer */
		while(*path!=';' && *path)	/* copy the path into the array */
			*fn++=*path++;
    if ((*(fn-1) != '\\') && (*(fn-1) != ':') && (*(fn-1) != '/') && (*name != '\\') && (*name != '/'))
      *fn++ = '/';  /* NJT */
    strcpy(fn,name);  /* append the name to the path */
		if((fp=fopen(filename,mode))!=NULL)	/* check for the file in that path */
			return(fp);
		if(*path)	/* skip the ';' */
            path++;
	  }	/* end while */
	return(NULL);	/* file not on the path either */
}	/* end pfopen() */
