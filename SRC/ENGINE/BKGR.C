/*      BKGR.C
*
*  Background routines( FTP and RCP )
*
***************************************************************************
*                                                                          *
*      part of:                                                            *
*      TCP/IP kernel for NCSA Telnet                                       *
*      by Tim Krauskopf                                                    *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
***************************************************************************
*
*  Revision history:
*
*  11/86  Started by Timk
*  xx/88  Rewritten by everyone
*  5/89   clean up for 2.3 release, JKM
*  10/89  fixed ftp bugs            QAK
*  ...93  More ftp bugs, security   RMG
*/

/*
*  Includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#include "whatami.h"
#include "hostform.h"

/*
*  Defines
*/

#define HTELNET 23
#define HRSHD 514
#define HFTP 21
#define BUFFERS 8000
#define PATHLEN 256

#define RCPSEGSIZE 1024
#define EOLCHAR 10

#ifdef MSC
#define O_RAW  O_BINARY
#ifdef __TURBOC__
#include <dir.h>
#else
#include <direct.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#endif  /* msc */

#include "externs.h"


/*
*  Global Variables
*/

static int write_permission=0,wrtok=1;
int        twperm=0;

static int ftpenable=0,      /* is file transfer enabled? */
  ftpDir=0,
  rcpenable=0,               /* is rcp enabled? */
  ftpdata=-1,                /* port for ftp data connection */
  fnum=-1,                   /* port number for incoming ftp */
  rsnum=-1,                  /* port number for incoming rshell */
  rserr=-1,                  /* port number for rshd()stderr */
  lstype;                    /* what type of ftp list to do */
static unsigned char xs[BUFFERS+10],  /* buffer space for file transfer */
  pathname[PATHLEN],      /* space to keep path names */
  newfile[PATHLEN],      /* current file being received */
  myuser[17],           /* user name on my machine */
  mypass[30],            /* user pass on my machine (blank unless anonftp) */
  hisuser[17],         /* user name on his machine */
  waitchar;           /* character waiting for from net */

static int 
  curstate=-1,       /* state machine for background processes */
  retstate=200,      /* to emulate a subroutine call */
  ftpstate=0,        /* state of the ftp data transfer */
  isdir=0,           /* flag for rcp target pathname */
  waitpos=0,         /* marker for gathering strings from net */
  cnt=0,             /* number of characters from last netread()*/
  fh=0,              /* file handle when transfer file is open */
  ftpfh=0,           /* file handle for ftp data */
  xp=0,              /* general pointer */
  towrite=0,         /* file transfer pointer */
  len=0;             /* file transfer length */

static long int
  filelen=0L;             /* length of current file for transfer */

static char bkgr_path[PATHLEN];
static unsigned int curr_drive;
static char mungbuf[1024],crfound=0;
static char *nextfile;    /* pointer to the next filename returned by DOS */

static int Sfwrite(int ,char *,int );
static int Sfread(int ,char *,int );

extern char Sptypes[NPORTS];/* flags for port #'s */
#define PFTP 1
#define PRCP 2
#define PDATA 3

#ifdef PC

#define ga() while(!netwrite(rsnum,"",1))netsleep(0)

/************************************************************************/
/*  unsetrshd
*   remove the acceptance of rshd calls(rcp)
*/
void unsetrshd(void )
{
  netclose(rsnum);
  rsnum=-1;
  rcpenable=0;
}

/************************************************************************/
void setrshd(void )
{
  int i;
/*
*  set up to receive a rsh call connection 
*/
  if(rsnum>=0)
    return;
  curstate=199;          /* waiting for connection */
  i=netsegsize(RCPSEGSIZE);
  rsnum=netlisten(HRSHD);
  netsegsize(i);
  if(rsnum>=0)
    Sptypes[rsnum]=PRCP;
  rcpenable=1;
}

/************************************************************************/
/*  rshell
*   take an incoming rshell request and service it.  Designed to handle
*   rcp primarily.
*/
void rshd(int code)
{
  int i,j;

  if(!rcpenable)
    return;
  switch(curstate) {
    case 199:          /* wait to get started */
      if(code!=CONOPEN)
        break;
      curstate=0;
      netputuev(SCLASS,RCPACT,rsnum);    /* keep us alive */
      break;
/*
* in effect, this is a subroutine that captures network traffic while
* waiting for a specific character to be received
*/
    case 50:
      while(0<(cnt=netread(rsnum,&xs[waitpos],1))){
        if(xs[waitpos]==waitchar){
          curstate=retstate;
          netputuev(SCLASS,RCPACT,rsnum);    /* keep us alive */
          break;
        }
        else 
          waitpos+=cnt;
      }
      netpush(rsnum);
      break;

    case 51:        /* for recursion, passes straight through */
      break;

    case 0:          /* waiting for first string */
      retstate=1;
      curstate=50;
      waitchar=0;
      waitpos=0;
      netputuev(SCLASS,RCPACT,rsnum);    /* keep us alive */
      break;

    case 1:          /* we have received stderr port number */
      i=atoi(xs);      /* port number */
      curstate=51;
      if(i) {
        cnt=-1;        /* abort it all, we don't take rsh */
        break;
      }
      else
      rserr=-1;
      retstate=2; 
      curstate=50;
      waitpos=0; 
      waitchar=0;
      break;

    case 2:            /* get user name, my machine */
      strncpy(myuser,xs,16);
      retstate=3; 
      curstate=50;
      waitpos=0; 
      waitchar=0;
      break;

    case 3:             /* get user name, his machine */
      strncpy(hisuser,xs,16);
                        /*ftransinfo(hisuser); */
      retstate=4; 
      curstate=50;
      waitchar=0; 
      waitpos=0;
      break;

    case 4:
                  /*ftransinfo(xs);*/
/*
* ACK receipt of command line
*/
      if(rserr>=0)
        netwrite(rserr,(char *)&xp,1);    /* send null byte */
      else
        ga();            /* send NULL on main connection */
      if(!strncmp(xs,"rcp ",4)){
/*
*  rcp will be using wildcards, target must be a directory
*/
        if(!strncmp(&xs[4],"-d -t",5)){
          strncpy(pathname,&xs[10],PATHLEN);
          if(direxist(pathname)){    /* ftransinfo("no directory by that name "); */
            netwrite(rsnum,"\001 No dir found ",16);
            netpush(rsnum);
            cnt=-1;
            break;
            }
          isdir=1;
          retstate=20; curstate=50;
          waitchar='\012'; waitpos=0;
          ga();          /* ready for them to start */
          break;
          }
/*
* target could be a directory or a complete file spec
*/
        if(!strncmp(&xs[4],"-t",2)){
          strncpy(pathname,&xs[7],PATHLEN);
          if(!direxist(pathname))
            isdir=1;
          else
            isdir=0;
          retstate=20; 
          curstate=50;
          waitchar='\012'; 
          waitpos=0;
          ga();      /* ready for rcp to start */
          break;
          }
/*
*  rcp is requesting me to transfer file(s)(or giving directory name)
*/
        if(!strncmp(&xs[4],"-f",2)){
          strncpy(pathname,&xs[7],PATHLEN);
/*
*  direxist returns whether the path spec refers to a directory, and if
*  it does, prepares it as a prefix.  Therefore, if it is a dir, we append
*  a '*' to it to wildcard all members of the directory.
*  Firstname()takes a file spec(with wildcards)and returns a pointer
*  to a prepared ACTUAL file name.  nextname()returns successive ACTUAL
*  filenames based on firstname().
*/
          if(!direxist(pathname)){
            i=strlen(pathname);
            pathname[i]='*';    /* all members of directory*/
            pathname[++i]='\0';
            }
          nextfile=(char *)firstname(pathname,0);
          if(nextfile==NULL){
            /*ftransinfo(" file or directory not found ");*/
            netwrite(rsnum,"\001 File not found ",18);
            netpush(rsnum);
            cnt=-1;
            }
          else {
                        /* wait for other side to be ready */
            retstate=30;  
            curstate=50;
            waitchar=0; 
            waitpos=0;
            }
          break;
          }
        }
      break;

    case 20:
      xs[waitpos]='\0';    /* add terminator */

/*
*  get working values from command line just received
*  open file for receive
*/
      if(xs[0]!='C'||xs[5]!=' '){    /*ftransinfo(" Cannot parse filename line "); */
        netwrite(rsnum,"\001 Problem with file name ",26);
        cnt=-1;
        break;
        }

      filelen=atol(&xs[6]);
      for(i=6; xs[i]!=' '; i++)
        if(!xs[i]){
                    /*ftransinfo(" premature EOL ");*/
          netwrite(rsnum,"\001 Problem with file name ",26);
          cnt=-1;
          break;
          }
      strcpy(newfile,pathname);    /* path spec for file */
      if(isdir)            /* add file name for wildcards */
        strcat(newfile,&xs[++i]);
#ifdef MSC
      if(0>(fh=open(newfile,O_BINARY|O_CREAT|O_TRUNC|O_WRONLY,S_IREAD|S_IWRITE))){
#else          
      if(0>(fh=creat(newfile,O_RAW))){
#endif  
        netwrite(rsnum,"\001 Cannot open file for write ",29);
        cnt=-1;
        break;
       }
      netputevent(USERCLASS,RCPBEGIN,-1);
      ga();              /* start sending the file to me */
      xp=len=0;
      curstate=21;          /* receive file, fall through */
      break;

    case 21:
      do {        /* wait until xs is full before writing to disk */
        if(len<=0) {
          if(xp) {
            write(fh,xs,xp);
            xp=0;
            }
          if(filelen>(long)BUFFERS)
            len=BUFFERS;
          else
            len=(int)filelen;
          }
        cnt=netread(rsnum,&xs[xp],len);
        filelen-=(long)cnt;
        len-=cnt;
        xp+=cnt;
        /*printf(" %ld %d %d %d ",filelen,len,xp,cnt);n_row(); n_puts(""); */
        if(filelen<=0L || cnt<0) {
          write(fh,xs,xp);    /* write last block */
          close(fh);
          fh=0;          /* wait for NULL byte at end after closing file */
          curstate=50;  
          retstate=22;
          waitchar=0;   
          waitpos=0;
          break;
          }
        } while(cnt>0);
      break;

    case 22:
  /* cause next sequence of bytes to be saved as next filename to transfer     */
      ga();          /* tell other side, I am ready */
      waitchar='\012'; 
      waitpos=0;
      curstate=50; 
      retstate=20;
      break;
/*
*  transfer file(s)to the sun via rcp
*/
    case 30:
#ifdef MSC
      if(0>(fh=open(nextfile,O_BINARY|O_RDONLY))) {
#else
      if(0>(fh=open(nextfile,O_RAW))){
#endif
        netwrite(rsnum,"\001 File not found ",19);
        /*ftransinfo("Cannot open file to transfer: ");ftransinfo(nextfile); */
        cnt=-1;
        break;
      }
      netputevent(USERCLASS,RCPBEGIN,-1);
      filelen=lseek(fh,0L,2);    /* how long is file? */
      lseek(fh,0L,0);        /* back to beginning */
      for(i=0,j=-1; nextfile[i]; i++)
        if(nextfile[i]=='\\')
          j=i;
      sprintf(xs,"C0755 %lu %s\012",filelen,&nextfile[j+1]);
      netwrite(rsnum,xs,strlen(xs));  /* send info to other side */
                      /*ftransinfo(xs);check it */
      retstate=31; 
      curstate=50;
      waitchar=0;  
      waitpos=0;
      towrite=xp=0;
      break;

    case 31:
/*
*   we are in the process of sending the file 
*/
      netputuev(SCLASS,RCPACT,rsnum);    /* keep us alive */
      if(towrite<=xp){
        towrite=read(fh,xs,BUFFERS);
        xp=0;
        filelen -=(long)towrite;
        }
      i=netwrite(rsnum,&xs[xp],towrite-xp);
      if(i>0)
        xp+=i;
      /*printf(" %d %d %d %ld\012",i,xp,towrite,filelen);n_row();*/
/*
*  done if:  the file is all read from disk and all sent
*  or other side has ruined connection
*/
      if((filelen<=0L && xp>=towrite) || netest(rsnum)){
        close(fh);
        fh=0;
        nextfile=(char *)nextname(0);      /* case of wildcards */
        ga(); 
        netputuev(SCLASS,RCPACT,rsnum);
        if(nextfile==NULL)
          retstate=32;
        else
          retstate=30;
        curstate=50;
        waitchar=0;  waitpos=0;
        }
      break;

    case 32:
      cnt=-1;
      break;

    case 5:
      break;

    default:
      break;

    }
/*
*  after reading from connection, if the connection is closed,
*  reset up shop.
*/
  if(cnt<0){
    if(fh>0){
      close(fh);
      fh=0;
    }
    curstate=5;
    cnt=0;
    netclose(rsnum);
    rsnum=-1;
    netputevent(USERCLASS,RCPEND,-1);
    setrshd();          /* reset for next transfer */
    }
}
#endif

/************************************************************************/
/************************************************************************/
/*  ftp section
*   This should be extracted from rcp so that it compiles cleanly
*/

#define CRESP(A) netwrite(fnum, messs[(A)], strlen(messs[(A)]))
#define FCRESP3(A,B,C) {char msg[80];sprintf(msg,messs[(A)],B,C),netwrite(fnum,msg, strlen(msg));}

#ifdef MSC
#ifdef __TURBOC__
#define FASCII O_RAW
#else
#define FASCII O_TEXT
#endif
#else
#define FASCII  0
#endif
#define FIMAGE O_RAW
#define FAMODE 0
#define FIMODE 1
#define FMMODE 2        /* Mac Binary, when ready */

int bypass_passwd=0;       /* flag to indicate whether or not to bypass the password check (gives root access) */
int root=0;                /* once in ftp*/

static int rfstate,
  portnum[8],
  ftpfilemode=FASCII,      /* how to open the file */
  ftptmode=FAMODE;      /* how to transfer the file on net */

/*
*  set up to receive a telnet connection for ftp commands
*/
static uint16 fdport;

void setftp(void )
{
  rfstate=0;
  ftpstate=0;
  fnum=netlisten(HFTP);
  ftpenable=1;

  if(fnum>=0)          /* signal that events should be caught */
    Sptypes[fnum]=PFTP;
  strcpy(myuser,"unknown");  /* set unknown user name */
}

void unsetftp(void )
{
  rfstate=0;
  ftpstate=0;
  netclose(fnum);
  if(ftpdata >= 0) {
    netclose(ftpdata);
  }
  fnum=-1;
  ftpenable=0;
}

void setftpwrt(int mode)  /* rmg 931100 */
{
  wrtok=mode;
}

/***********************************************************************/
/*
*  resident ftp server -- enables initiation of ftp without a username
*  and password, as long as this telnet is active at the same time
*  Now checks for the need of passwords.
*/

static char *messs[]={
  /* RMG handy marker (FTP messages) */
  "220 PC Resident FTP server, ready \015\012",
  "451 Error in processing list command \015\012",
  "221 Goodbye \015\012",            /*2*/
  "200 This space intentionally left blank<>\015\012",
  "150 Opening %s mode connection for file %s\015\012",
  "226 Transfer complete \015\012",      /*5*/
  "200 Type set to A, ASCII transfer mode \015\012",
  "200 Type set to I, binary transfer mode \015\012",
  "500 Command not understood \015\012",    /*8*/
  "200 Okay \015\012",
  "230 User logged in \015\012",
  "550 File not found \015\012",        /*11*/
  "501 Directory not present or syntax error\015\012",
  "250 Chdir okay\015\012",
  "257 \"",
  "\" is the current directory \015\012",    /*15*/
  "501 File not found \015\012",
  "504 Parameter not accepted, not implemented\015\012",
  "200 Stru F, file structure\015\012",
  "200 Mode S, stream mode\015\012",    /*19*/
  "202 Allocate and Account not required for this server\015\012",
  "501 Cannot open file to write, check for valid name\015\012",
  "530 USER and PASS required to activate me\015\012",
  "331 Password required\015\012",      /*23 */
  "530 Login failed\015\012",
  "200 MacBinary Mode enabled\015\012",
  "200 MacBinary Mode disabled\015\012",  /*26 */
  "552 Disk write error, probably disk full\015\012",
  "214-NCSA Telnet FTP server, supported commands:\015\012",
  "    USER  PORT  RETR  ALLO  PASS  STOR  CWD  XCWD  XPWD  LIST NLST\015\012",
  "    HELP  QUIT  MODE  TYPE  STRU  ACCT  CDUP  DELE  MKD  RMD NOOP\015\012", /*30*/
  "    A Macintosh version of NCSA Telnet is also available.\015\012",
  "214 Direct comments and bugs to pctelnet@ncsa.uiuc.edu\015\012",
  "200 Type set to I, binary transfer mode [MACBINARY ENABLED]\015\012", /* 33 */
  "200 Type set to I, binary transfer mode [macbinary disabled]\015\012",
  "\" created\015\012",    /*35*/
  "553 File name not allowed\015\012",
  "502 Command not implemented\015\012",
  "550 Permission Denied\015\012",
  ""
};

void rftpd(int code)
{
  int i;

  if(!ftpenable)
    return;

  netpush(fnum);

  switch(rfstate) {
    case 0:
      if(code!=CONOPEN)
        break;
      ftpfilemode=FASCII;
      ftptmode=FAMODE;
      netputevent(USERCLASS,FTPCOPEN,-1);
      rfstate=1;          /* drop through */

    case 1:
      dopwd(bkgr_path,256);
      getdrive(&curr_drive);
      CRESP(0);
      netgetftp(portnum,fnum);  /* get default ftp information */
      for(i=0; i<4; i++)        /* copy IP number */
        hisuser[i]=(char)portnum[i];
      fdport=portnum[6]*256+portnum[7];
      waitpos=0; 
      waitchar='\012';
      rfstate=50;             /* note skips over */
      if(Sneedpass())
        retstate=3;        /* check pass */
      else
        retstate=5;        /* who needs one ? */
      break;

    case 3:     /* check for passwords */
    case 4:
      waitpos=0;  
      waitchar='\012';
      rfstate=50;  
      if(!strncmp("USER",xs,4)) {
        strncpy(myuser,&xs[5],16);    /* keep user name */
        netputevent(USERCLASS,FTPUSER,-1);
        if(Sneedpass()) {
          CRESP(23);
          retstate=4;        /* check pass */
        }
        else {
          CRESP(10);
          netputevent(USERCLASS,FTPPWSK1,-1);
          if(wrtok)
            netputevent(USERCLASS,FTPPWWT,-1);
          retstate=5;        /* skip pass */
        }
        break;
      } /* end if */
      if(!strncmp("PASS",xs,4)) {              /* more security  rmg 930617 */
        root=0; /* Umm, don't remove this */
        if((i=Scheckpass(myuser,&xs[5])) || (bypass_passwd)) {  /* check for correct passwd, or check whether we bypassed the passwd from the keyboard */
          if(bypass_passwd) {
            bypass_passwd=0;    /* reset the bypassing flag */
            CRESP(10);
            netputevent(USERCLASS,FTPPWSK2,-1);
            netputevent(USERCLASS,FTPPWRT,-1);
            root=1;
            retstate=5;
          }
          else {
            if(i == 2) { /* Scheckpass reports anonftp  rmg 931111 */
              strncpy(mypass,&xs[5],30);    /* keep user name */
              netputevent(USERCLASS,FTPANON,-1);
            }
            i=gosafedir(myuser);
            write_permission=twperm;
            if(i && (i != 3)) {
              netputevent(USERCLASS,FTPPWNO,-1);
              CRESP(24);        /* better not happen (error in PW file) */
              retstate=3;
            }
            else {
              if(wrtok && write_permission)
                netputevent(USERCLASS,FTPPWWT,-1);
              netputevent(USERCLASS,FTPPWOK,-1);
              CRESP(10);
              retstate=5;
              if(i==3) {
                netputevent(USERCLASS,FTPPWRT,-1);
                root=1;
              }
            }
          }
        } /* end if */
        else {
          netputevent(USERCLASS,FTPPWNO,-1);
          CRESP(24);
          retstate=3;
        } /* end else */
        break;
      } /* end if */
      if(!strncmp("QUIT",xs,4)) {
        CRESP(2);
        cnt=-1;
      } /* end if */
      else
        CRESP(22);
      retstate=3;      /* must have password first */
      break;        
        
/*
*  interpret commands that are received from the other side
*/
    case 5:
      for(i=4; (unsigned int)i < strlen(xs); i++)
        if(xs[i]=='/')    /* flip slashes */
          xs[i]='\\';
/*
*  set to a safe state to handle recursion
*  wait for another command line from client
*  
*/
      rfstate=50; 
      retstate=5;
      waitchar='\012';  
      waitpos=0;

#ifdef AUX
  fprintf(stdaux,xs);
#endif

      if(!strncmp(xs,"LIST",4) || !strncmp(xs,"NLST",4)) {
        if(Sneedpass() && !root) {
          i=safedir(fixdirnm(&xs[5]),myuser,1);
          if(i==2 || !(twperm & 4) ) {
            CRESP(38);
            break;
          }
        }

        if(!strncmp(xs,"LIST",4)) {
          if((strlen(xs)<6) || (xs[5]=='.'))
            strcpy(xs,"LIST *");
          lstype=1;
        }  /* end if */
        else {
          if((strlen(xs)<6) || (xs[5]=='.'))
            strcpy(xs,"NLST *");
          lstype=0;
#ifdef NOT_EVEN_
          else
            if(!strcmp(&xs[5],"-sF")) { /* for future ls-sF support RMG */
              strcpy(xs,"LIST *");
              lstype=1;
            }
#endif
        } /* end else */
        nextfile=(char *)firstname(&xs[5],lstype);  /* find first name */
        if(nextfile==NULL)
          CRESP(16);
        else {
          while(!strcmp(nextfile,".") || !strcmp(nextfile,".."))
            nextfile=nextname(lstype);  /* skip . and ..  rmg 931100 */
          ftpgo();              /* open the connection */
          fdport=portnum[6]*256+portnum[7];   /* reset to def */
          if(ftpdata>=0)
            Sptypes[ftpdata]=PDATA;
          ftpstate=40;            /* ready to transmit */
          FCRESP3(4,"ASCII","list");
          netputevent(USERCLASS,FTPLIST,-1);
        }  /* end else */
      }  /* end if */

      else if(!strncmp(xs,"CWD",3)) {
        if(Sneedpass() && !root) {
          i=safedir(fixdirnm(&xs[4]),myuser,0);
          write_permission=twperm;
          if(i || !(twperm & 1))
            if(i==1)
              CRESP(12);       /* not there */
            else /* i == 2 || !(twperm & 1) */
              CRESP(38);       /* Ree-jected! */
          else
            CRESP(13);         /* success */
        }
        else {
          if(chgdir(fixdirnm(&xs[4]))) /* no pw needed */
            CRESP(12);                 /* not there */
          else
            CRESP(13);                 /* success */
        }
      } /* end if */

      else if(!strncmp(xs,"CDUP",3)) {
        if(Sneedpass() && !root) {
          i=safedir("..",myuser,0);
          write_permission=twperm;
          if(i || !(twperm & 1))
            if(i==1)
              CRESP(12);       /* not there */
            else /* i == 2 || !(twperm & 1) */
              CRESP(38);       /* Ree-jected! */
          else
            CRESP(13);         /* success */
        }
        else {
          if(chgdir(".."))     /* no pw needed */
            CRESP(12);         /* not there */
          else
            CRESP(13);         /* success */
        }
      } /* end if */

      else if(!strncmp(xs,"MKD",3)) {
        if(!root && !wrtok) {
          CRESP(38);
          break;
        }
        if(Sneedpass() && !root) {
          i=safedir(fixdirnm(&xs[4]),myuser,1);
          if(i==2 || !(twperm & 2) ) {
            CRESP(38);
            break;
          }
        }
        if(mkdir(&xs[4]))
          CRESP(36);
        else {                /* success */
          CRESP(14);            /* start reply */
          netwrite(fnum,&xs[4],strlen(&xs[4]));  /* write dir name */
          CRESP(35);            /* finish reply */
        }  /* end else */
      }  /* end if */

      else if(!strncmp(xs,"RMD",3)) {
        if(!root && !wrtok) {
          CRESP(38);
          break;
        }
        if(Sneedpass() && !root) {
          i=safedir(fixdirnm(&xs[4]),myuser,1);
          if(i==2 || !(twperm & 2)) {
            CRESP(38);
            break;
          }
        }
        if(rmdir(&xs[4]))          /* failed */
          CRESP(11);
        else                /* success */
          CRESP(9);
          }  /* end if */

      else if(!strncmp(xs,"DELE",4)) {
        if(!root && !wrtok) {
          CRESP(38);
          break;
        }
        if(Sneedpass() && !root) {
          i=safedir(fixdirnm(&xs[5]),myuser,1);
          if(i==2 || !(twperm & 2)) {
            CRESP(38);
            break;
          }
        }
        if(remove(&xs[5]))          /* failed */
          CRESP(11);
        else                /* success */
          CRESP(9);
          }  /* end if */

      else if(!strncmp(xs,"STOR",4)) {
        if(!root && !wrtok) {
          CRESP(38);
          break;
        }
        if(Sneedpass() && !root) {
          i=safedir(fixdirnm(&xs[5]),myuser,1);
          if(i==2 || !(twperm & 2)) {
            CRESP(38);
            break;
          }
        }
        ftpDir=1;
#ifdef MSC
        if(0>(ftpfh=open(&xs[5],ftpfilemode|O_CREAT|O_TRUNC|O_WRONLY,S_IREAD|S_IWRITE))) {
#else          
        if(0>(ftpfh=creat(&xs[5],ftpfilemode))) {
#endif        
          CRESP(21);
          break;
        } /* end if */
        ftpstate=0;
        strncpy(newfile,&xs[5],PATHLEN-1);
        ftpgo();            /* open connection */
        fdport=portnum[6]*256+portnum[7]; /* reset to def */
        if(ftpdata>=0)
          Sptypes[ftpdata]=PDATA;
                FCRESP3(4,(ftpfilemode==FASCII)?"ASCII":"BINARY",&xs[5]);
        ftpstate=30;        /* ready for data */
        }  /* end if */

      else if(!strncmp(xs,"RETR",4)) {
        ftpDir=0;
        if(Sneedpass() && !root) {
          i=safedir(fixdirnm(&xs[5]),myuser,1);
          if(i==2) {
            CRESP(38);
            break;
          }
        }
#ifdef MSC
        if(0>(ftpfh=open(&xs[5], ftpfilemode|O_RDONLY))) {
#else          
        if(0>(ftpfh=creat(&xs[5],ftpfilemode))) {
#endif
          CRESP(11);
          break;
        } /* end if */
        strncpy(newfile,&xs[5],PATHLEN-1);
        ftpgo();        /* open connection */
        fdport=portnum[6]*256+portnum[7]; /* reset to def */
        ftpstate=20;    /* ready for data */
        if(ftpdata>=0)
          Sptypes[ftpdata]=PDATA;
                FCRESP3(4,(ftpfilemode==FASCII)?"ASCII":"BINARY",&xs[5]);
        }  /* end if */

      else if(!strncmp(xs,"TYPE",4)) {
        if(toupper(xs[5])=='I') {
          ftpfilemode=FIMAGE;
          ftptmode=FIMODE;
          CRESP(7);
                  } /* end if */
        else 
          if(toupper(xs[5])=='A') {
            ftpfilemode=FASCII;
            ftptmode=FAMODE;
            CRESP(6);
                      } /* end if */
          else
            CRESP(17);
        }  /* end if */
      else if(!strncmp(xs,"PORT",4)) {
/*
* get the requested port number from the command given
*/
       
sscanf(&xs[5],"%d,%d,%d,%d,%d,%d",&portnum[0],&portnum[1],&portnum[2],&portnum[3],&portnum[4],&portnum[5]);
        fdport=portnum[4]*256+portnum[5];
        CRESP(3);
        }  /* end if */

      else if(!strncmp(xs,"QUIT",4)) {
        chgdir(bkgr_path);
        setdrive(curr_drive);
        CRESP(2);
        rfstate=60;
        netputuev(CONCLASS,CONDATA,fnum);  /* post back to me */
        }  /* end if */

      else if(!strncmp(xs,"XPWD",4) || !strncmp(xs,"PWD",3)) {
        CRESP(14);            /* start reply */
        dopwd(xs,1000);          /* get directory */
        netwrite(fnum,xs,strlen(xs));  /* write dir name */
        CRESP(15);            /* finish reply */
      }  /* end if */

      else if(!strncmp(xs,"USER",4)) {
        strncpy(myuser,&xs[5],16);    /* keep user name */
        netputevent(USERCLASS,FTPUSER,-1);
        if(Sneedpass()) {
          CRESP(23);
          retstate=4;        /* check pass */
        }
        else {
          CRESP(10);
          netputevent(USERCLASS,FTPPWSK1,-1);
          netputevent(USERCLASS,FTPPWWT,-1);
          if(wrtok)
            netputevent(USERCLASS,FTPPWWT,-1);
          retstate=5;        /* skip pass */
        }
        break;
      }  /* end if */

      else if(!strncmp(xs,"STRU",4)) {  /* only one stru allowed */
        if(xs[5]=='F')
          CRESP(18);
        else
          CRESP(17);
      }  /* end if */

      else if(!strncmp(xs,"MODE",4)) {  /* only one mode allowed */
        if(xs[5]=='S')
          CRESP(19);
        else
          CRESP(17);
        }  /* end if */

      else if(!strncmp(xs,"ALLO",4) || !strncmp(xs,"ACCT",4))
        CRESP(20);

      else if(!strncmp(xs,"HELP",4)) {
        for(i=28; i<33; i++)
          CRESP(i);
        }  /* end if */

      else if(!strncmp(xs,"NOOP",4))
        CRESP(9);

      else if(!strncmp(xs,"REIN",4) || !strncmp(xs,"PASV",4) ||
            !strncmp(xs,"APPE",4) || !strncmp(xs,"REST",4) || 
            !strncmp(xs,"RNFR",4) || !strncmp(xs,"RNTO",4) || 
            !strncmp(xs,"ABOR",4) || !strncmp(xs,"SITE",4) || 
            !strncmp(xs,"STAT",4) || !strncmp(xs,"SMNT",4) || 
            !strncmp(xs,"SMNT",4) || !strncmp(xs,"SYST",4) || 
            !strncmp(xs,"STOU",4))    /* unimplemented commands */
        CRESP(37);
      else      /* command not understood */
        CRESP(8);
      break;
/*
*  subroutine to wait for a particular character
*/
    case 50:
      while(0<(cnt=netread(fnum,&xs[waitpos],1))) {
        if(xs[waitpos]==waitchar) {
          rfstate=retstate;
#ifdef OLD_WAY /* chopped off spaces */
          while(xs[waitpos]<33)    /* find end of string */
            waitpos--;
          xs[++waitpos]='\0';      /* put in terminator */
#else /* done to allow null arguments to passwd  rmg 931031 */
          xs[--waitpos]='\0';      /* put in terminator */
#endif
          for(i=0; i<4; i++)        /* want upper case */
            xs[i]=(unsigned char)toupper((int)xs[i]);
          break;
        }  /* end if */
        else
          waitpos+=cnt;
      }  /* end while */
      break;

        case 60:    /* wait for message to get through or connection is broken */
                    /* printf("%d,%d",netpush(fnum),netest(fnum));*/
      if(!netpush(fnum) || netest(fnum))
        cnt=-1;
      else
        netputuev(CONCLASS,CONDATA,fnum);  /* post back to me */
      break;
    default:
      break;
  } /* end switch */


  if(cnt<0) {
    if(ftpfh>0) {
      close(ftpfh);
      ftpfh=0;
          } /* end if */
    if(ftpdata>0) {
      netclose(ftpdata);
      netputevent(USERCLASS,FTPEND,-1);
          } /* end if */
    rfstate=100;
    ftpstate=0;
    cnt=0;
    netclose(fnum);
    netputevent(USERCLASS,FTPCLOSE,-1);
    fnum=-1;
    ftpdata=-1;
    setftp();        /* reset it */
  } /* end if */
}   /* end rftpd() */

/***********************************************************************/
/* ftpgo
*  open the FTP data connection to the remote host
*/
void ftpgo(void )
{
  int savest;
  struct machinfo *m;

  xs[0]=(unsigned char)portnum[0];
  xs[1]=(unsigned char)portnum[1];
  xs[2]=(unsigned char)portnum[2];
  xs[3]=(unsigned char)portnum[3];

  netfromport(20);   /* ftp data port */

  if(NULL==(m=Slookip(xs))){    /* use default entry */
    if(NULL==(m=Shostlook("default")))
      return;
    savest=m->mstat;
    m->mstat=HAVEIP;
    movebytes(m->hostip,xs,4);
    ftpdata=Snetopen(m,fdport);
    m->mstat=savest;
    movebytes(m->hostip,"\0\0\0\0",4);
    return;
    }
  ftpdata=Snetopen(m,fdport);
}

/*********************************************************************/
/*
*  FTP receive and send file functions
*/
static int fcnt=0;

void ftpd(int code,int curcon)
{
  int i;

  if(curcon!=ftpdata)    /* wrong event, was for someone else */
    return;
    switch(ftpstate) {
    default:
      break;

    case 40:        /* list file names in current dir */
      if(code==CONFAIL)  /* something went wrong */
        fcnt=-1;
      if(code!=CONOPEN)  /* waiting for connection to open */
        break;
      ftpstate=41;
/*
*  send the "nextfile" string and then see if there is another file
*  name to send
*/
    case 41:
      netputuev(SCLASS,FTPACT,ftpdata);
      netpush(ftpdata);
      if(NULL!=nextfile) { /* empty directory */
        i=strlen(nextfile);
        if(i!=netwrite(ftpdata,nextfile,i)) {
          CRESP(1);
          fcnt=-1;
          break;
        }
      netwrite(ftpdata,"\015\012",2);
      }
      if(NULL==(nextfile=nextname(lstype)))  /* normal end */
        ftpstate=22;               /* push data through */
      break;
      
    case 30:
      if(code==CONFAIL)  /* something went wrong */
        fcnt=-1;
      if(code!=CONOPEN)  /* waiting for connection to open */
        break;
      ftpstate=31;
      crfound=0;
      len=xp=0;
      filelen=0L;
      netputevent(USERCLASS,FTPBEGIN,-2);
      break;

    case 31:
/*
* file has already been opened, take everything from the connection
* and place into the open file: ftpfh
*/

/* cRMG hangs on disk errors Doh! */

      do {        /* wait until xs is full before writing to disk */
        if(len<=2000) {
          if(xp) {
            if(0>write(ftpfh,xs,xp)) {   /* disk full err */
              netclose(ftpdata);
              fcnt= -1;
              CRESP(27);
              break;
            } /* end if */
            xp=0;
          } /* end if */
          len=BUFFERS;    /* expected or desired len to go */
        } /* end if */
        if(ftptmode==FAMODE)
          fcnt=Sfread(ftpdata,&xs[xp],len);
        else
          fcnt=netread(ftpdata,&xs[xp],len);
        if(fcnt>=0) {
          len-=fcnt;
          xp+=fcnt;
          filelen+=fcnt;
        } /* end if */
/*      printf(" %d %d %d \012",len,xp,fcnt);
        n_row();
*/
        if(fcnt<0) {
          if(0>write(ftpfh,xs,xp)) {   /* disk full check */
            CRESP(27);
            break;
          } /* end if */
          close(ftpfh);
          ftpfh=0;
          CRESP(5);
        } /* end if */
      } while(fcnt>0);
      break;

    case 20:
      if(code==CONFAIL)          /* something went wrong */
        fcnt=-1;
      if(code!=CONOPEN)          /* waiting for connection to open */
        break;
      ftpstate=21;
      filelen=lseek(ftpfh,0L,2);      /* how long is file? */
      lseek(ftpfh,0L,0);          /* back to beginning */
      towrite=0;
      xp=0;
      netputevent(USERCLASS,FTPBEGIN,-1);

    case 21:
/*
*  transfer file(s)to the other host via ftp request
*  file is already open=ftpfh
*/
      netputuev(SCLASS,FTPACT,ftpdata);
      if(towrite<=xp) {
        i=BUFFERS;
        towrite=read(ftpfh,xs,i);
        xp=0;
              } /* end if */

      if(towrite<=0 || netest(ftpdata)) {
        ftpstate=22;
        break;
      } /* end if */


      if(ftptmode==FAMODE)
        i=Sfwrite(ftpdata,&xs[xp],towrite-xp);
      else
        i=netwrite(ftpdata,&xs[xp],towrite-xp);
      if(i>0) {
        xp+=i;
        filelen-=i;
        if(filelen<0L)
          filelen=0L;
              } /* end if */
      break;

    case 22:            /* wait for data to be accepted */
      netputuev(SCLASS,FTPACT,ftpdata);
      fcnt=netpush(ftpdata);    /* will go negative on err */
      if(!fcnt || netest(ftpdata))
        fcnt=-1;
      if(fcnt<0)
        CRESP(5);
      break;

    case 0:
      break;
    }  /* end of switch */

/*
*  after reading from connection, if the connection is closed,
*  reset up shop.
*/
    if(fcnt<0) {
      if(ftpfh>0) {
        close(ftpfh);
        ftpfh=0;
      } /* end if */
      ftpstate=0;
      fcnt=0;
      if(ftpdata>=0) {
        netclose(ftpdata);
        netputevent(USERCLASS,FTPEND,-1);
        ftpdata=-1;
      } /* end if */
    } /* end if */
}   /* end ftpd() */

/***************************************************************************/
/*  Sfwrite
*   Write an EOL translated buffer into netwrite.
*   Returns the number of bytes which were processed from the incoming
*   buffer.  Uses its own 1024 byte buffer for the translation(with Sfread).
*/
static int Sfwrite(int pnum,char *buf,int nsrc)
{
  int i,ndone,nout,lim;
  char *p,*q;

  ndone=0;

  while(ndone<nsrc){
    if(0>(i=netroom(pnum)))
      return(-1);
    if(i<1024)          /* not enough room to work with */
      return(ndone);
/*
*  process up to 512 source bytes for output(could produce 1K bytes out)
*/
    if(nsrc-ndone>512)
      lim=512;
    else
      lim=nsrc-ndone;
    p=buf+ndone;        /* where to start this block */
    q=mungbuf;          /* where munged stuff goes */
    for(i=0; i<lim; i++) {
      if(*p==EOLCHAR) {
        *q++=13;
        *q++=10;
        p++;
        }
      else
        *q++=*p++;
      }
    ndone+=lim;          /* # of chars processed */
    nout=q-mungbuf;        /* # of chars new */
    netwrite(pnum,mungbuf,nout);  /* send them on their way */
    }
  return(ndone);
}

/*
*  important note:  for Sfread, nwant must be 256 bytes LARGER than the amount
*  which will probably be read from the connection.
*  Sfread will stop anywhere from 0 to 256 bytes short of filling nwant
*  number of bytes.
*/
static int Sfread(int pnum,char *buf,int nwant)
{
  int i,ndone,lim;
  char *p,*q;

  if(nwant<1024)
    return(-1);
  ndone=0;
  while(ndone<nwant - 1024){    /* bugfix 6/88 - TK(added -1024)*/
    if(0>=(lim=netread(pnum,mungbuf,1024))) {
      if(ndone || !lim)      /* if this read is valid, but no data */
        return(ndone);
      else
        return(-1);        /* if connection is closed for good */
      }
    p=mungbuf;
    q=buf+ndone;
    for(i=0; i<lim; i++) {
      if(crfound) {
        if(*p==10)
          *q++=EOLCHAR;
        else 
          if(*p==0)
            *q++=13;      /* CR-NUL means CR */
        crfound=0;
        }
      else 
        if(*p==13)
          crfound=1;
        else 
          *q++=*p;        /* copy the char */
      p++;
      }
    ndone=q-buf;          /* count chars ready */
    }
  return(ndone);
}

/***********************************************************************/
/* Sftpname and Sftpuser and Sftphost
*  record the name of the file being transferred, to use in the status
*  line updates
*/
void Sftpname(char *s)
{
  strcpy(s,newfile);
}

void Sftpuser(char *user)
{
  strcpy(user,myuser);      /* user name entered to log in */
}

void Sftppass(char *user)   /* rmg 931111 */
{
  strcpy(user,mypass);      /* user name entered as passwd to log in as anon */
}

int SftpDirection(void)
{
    return ftpDir;
}

void Sftphost(char *host)
{
  movebytes(host,hisuser,4);    /* IP address of remote host */
}

void Sftpstat(long *byt)
{
  *byt=filelen;
}
