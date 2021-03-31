/*
*	UTIL.C
*
*	Session interface routines ( use these in user interface )
*
*****************************************************************************
*																			*
*	  part of:																*
*	 TCP/IP kernel for NCSA Telnet											*
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
*	Revision history:
*
*	10/87  Initial source release, Tim Krauskopf
*	5/88	clean up for 2.3 release, JKM	
*
*/

/*
*	Includes
*/
/* #define DEBUG  */    /* define for debug printfs */
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(MSC)
#include <malloc.h>
#endif
#if defined(MSC) || defined(__TURBOC__)
#include <conio.h>
#include <io.h>
#include <direct.h> /* for getcwd */
#endif
#include "whatami.h"
#include "hostform.h"
#include "protocol.h"
#include "data.h"
#include "externs.h"
#include "confile.h"            /* include the configuration variables and definitions, but we are not CONFIG_MASTER */

/*
*#define	NETX25	0
*/

static unsigned char *Ssstemps[]={	/* standard output files */
	"capfile",
	"hp.out",
	"ps.out",
	"tek.out"
};

char Sptypes[NPORTS];        /* port types assigned for session use */
extern struct config Scon;   /* hardware configuration */
extern int ftpdata;          /* current ftp data port */
#if (defined(TELBIN) && defined(FTP)) || defined(FTPBIN)
extern int twperm;           /* FTP write permission. */
#endif
#define NTIMES 30            /* size of timer queue */

/*
*  timer queue of events which will be placed into the event queue
*  when the time is up.
*/
static struct {
	unsigned char eclass,				/* event queue data */
		event;
	int next,							/* next item in list */
		idata;
	int32 when;							/* when timer is to go off */
} Stq[NTIMES];

static int Stfirst,
		Stfree;							/* pointers for timer queue */

#define PFTP 1
#define PRCP 2
#define PDATA 3
#define PDOMAIN 4

/************************************************************************/
/*
*	Snetinit ()
*
*	Handle all the network initialization, including reading up the config
* file.  This also starts up the hardware and gets arps on their merry way.
*
*/
int Snetinit(void )
{
	int i;
BUG("Snetinit");
/*
*  set up the file names
*/
	Scon.capture=Ssstemps[0];
	Scon.hpfile=Ssstemps[1];
	Scon.psfile=Ssstemps[2];
	Scon.tekfile=Ssstemps[3];

    neteventinit();                 /* initializes for error messages to count */
BUG("After neteventinit");
	for(i=0; i<NPORTS; i++)
		Sptypes[i]=-1;				/* clear port type flags */

	for(i=0; i<NTIMES; i++)
		Stq[i].next=i+1;			/* load linked list */
	Stq[NTIMES-1].next=-1;			/* anchor end */
	Stfirst=-1;
	Stfree=0;
BUG("About to read config file");
    if(!Sreadhosts()) {             /* parses config file */
BUG("after Sreadhosts - successful");
		netparms(Scon.irqnum,Scon.address,Scon.ioaddr);
#ifdef DEBUG
		printf("irqnum = %X\n",Scon.irqnum);
		printf("address = %X\n",Scon.address);
		printf("ioaddr = %X\n",Scon.ioaddr);
#endif

        netconfig(Scon.hw);
		netsetbroad(Scon.broadip);		/* set up IP broadcast address */
BUG("Before netinit");
        if((i=netinit())==0) {          /* starts up hardware */
BUG("After netinit - successful");
/*
*  Check for the need to RARP and do it
*/
			netgetip(Scon.myipnum);	/* get stored ip num */
BUG("have ip");
            if(comparen(Scon.myipnum,"RARP",4)) {   /* need RARP */
				if(netgetrarp())	/* stores in nnipnum at lower layer */
					return(-2);			/* failure return */
				netgetip(Scon.myipnum);
				netsetip(Scon.myipnum);	
			}
/*
 *	Check for bootp
 */
            if(comparen(Scon.myipnum,"BOOT",4)) {   /* need BOOTP */
BUG("BOOTP");
                if(bootp())
					return(-3);
            }
BUG("After BOOTP");
/*
 *	Check for x25 boot
 */
#ifdef	NETX25
            if(comparen(Scon.myipnum,"X25",3))      /* need server and buds */
				if(X25Boot())
					return(-4);
#endif
/*
*  Give the lower layers a chance to check to see if anyone else
*  is using the same ip number.  Usually generates an ARP packet.
*/
BUG("about to ARP");
            netarpme(Scon.myipnum);
BUG("arped-about to crash");
            Ssetgates();            /* finishes IP inits */
BUG("about to Stask()");
            Stask();
BUG("returning");
            return(0);
		  }
BUG("After netinit - failed");
		return(-1);		/* netinit() failed */
	  }
BUG("after Sreadhosts - failed");
	return(-5);			/* Sreadhosts() failed */
}

/************************************************************************/
/*  Sgetconfig
*   copy the configuration information into the user's data structure
*   directly.  The user can do with it what he feels like.
*/
void Sgetconfig(struct config *cp)
{
	movebytes(cp,&Scon,sizeof(struct config));
}

/************************************************************************/
/*  Smadd
*   If machine is there, just returns pointer, else
*   Add a machine to the list. Increments machine number of machine.
*   Puts in parameters copied from the "default" entry.
*
*/
struct machinfo *Smadd(char *mname)
{
	int i;
	struct machinfo *m;
/*
*  First, do we have the name already?
*/
	m=Shostlook(mname);
	if(m)
		return(m);
/*
*   Don't have name, add another record
*/
	Smptr=(struct machinfo *)malloc(sizeof(struct machinfo));
	if(Smptr==NULL)
		return(NULL);
	for(i=0; i<NUMSPECS-99; i++)
		Sflags[i]=0;					/* we have no parms */
	Scopyfrom("default");
	Smptr->sname=NULL;
	Smptr->hname=malloc(strlen(mname)+1);
	if(Smptr->hname)
		strcpy(Smptr->hname,mname);		/* copy in name of machine */
	Smptr->mno=++mno;
	Smptr->mstat=NOIP;
	Smptr->next=Smachlist;			/* add to front of machlist */
	Smachlist=Smptr;
	return(Smptr);
}

/*********************************************************************/
/*  Snewns()
*   Rotate to the next nameserver
*   Chooses the next highest number from the nameserv field
*/
int Snewns(void)
{
	struct machinfo *m,*low;
	int i;

	if(!Sns)					/* safety, should never happen */
		Sns=Smachlist;
	low=Sns;
	i=Sns->nameserv;			/* what is value now? */
	m=Smachlist;
	while(m) {
		if((m->nameserv)==(unsigned char)(i+1)) {
			Sns=m;
			return(0);
		  }
		if((m->nameserv>0) && (m->nameserv<low->nameserv))
			low=m;
		m=m->next;
	  }
	if(Sns==low)
		return(1);				/* no alternate */
	else
		Sns=low;
	return(0);
}

#ifndef NET14
/**************************************************************************/
/*  Slookip
*   For FTP to look up the transfer options to use when running
*
*/
struct machinfo *Slookip(unsigned char *ipnum)
{
	struct machinfo *m;

	m=Smachlist;
	while(m) {
		if(comparen(m->hostip,ipnum,4))
			return(m);
		m=m->next;
	  }
	return(NULL);
}
#endif

/************************************************************************/
/*  Shostlook
*   The straightforward list searcher.  Looks for either the
*   session name matching or the host name matching.  NULL if neither.
*/
struct machinfo *Shostlook(char *hname)
{
	struct machinfo *m;

	m=Smachlist;
	while(m!=NULL) {
		if((m->sname && !ncstrcmp(hname,m->sname)) || (m->hname && !ncstrcmp(hname,m->hname)))
			return(m);
		m=m->next;
	  }
	return(NULL);
}

/************************************************************************/
/*  Slooknum
*   get the host record by machine number, used primarily in DOMAIN name
*   lookup.
*/
struct machinfo *Slooknum(int num)
{
	struct machinfo *m;

	m=Smachlist;
	while(m) {
		if(m->mno==num)
			return(m);
		m=m->next;
	  }
	return(NULL);
}

/**************************************************************************/
/*
*	Snetopen ( m, tport )
*
*	Takes a pointer to a machine record (already looked up with Sgethost) and
* sends a TCP open call.  Uses port *tport*.  
*
*/
int Snetopen(struct machinfo *m,int tport)
{
	int j;

	if(!m || m->mstat<HAVEIP)
		return(-1);
	j=netxopen(m->hostip,tport,m->retrans,m->mtu,m->maxseg,m->window);	/* do the open call */
	if(j>=0) {
		Sptypes[j]=-1;			/* is allocated to user */
		Stimerset(CONCLASS,CONFAIL,j,m->conto);
#ifdef OLDWAY
        Stimerset(SCLASS,RETRYCON,j,m->retrans/TICKSPERSEC+2);
#else
        Stimerset(SCLASS,RETRYCON,j,m->retrans);
#endif
      } /* end if */
	return(j);
}   /* end Snetopen() */

/***********************************************************************/
/*
*	Scwritemode ( mode )
*
*	Set write mode
*
*/
static int son=1;

void Scwritemode(int mode)
{
	son=mode;
}

#ifndef NET14
/***********************************************************************/
/*
*	Scmode ()
*
*	Return what the current mode is 
*
*/
int Scmode(void )
{
	return(son);
}

/***********************************************************************/
/*
*	Stekmode ( mode )
*
*	Set tek mode
*
*/
static int tekon=1;

void Stekmode(int mode)
{
	tekon=mode;
}

/***********************************************************************/
/*
*	Stmode ()
*
*	Return the value of the tekmode
*
*/
int Stmode(void )
{
	return(tekon);
}
#endif

/***********************************************************************/
/*	Srcpmode ( mode )
*
*	Set the RCP mode either on or off, install the proper watchers
*
*/
static int rcpon=1;

void Srcpmode(int mode)
{
	rcpon=mode;
#ifdef RCP
	if(rcpon)
		setrshd();								/* turn on the watcher */
	else
		unsetrshd();							/* turn off the watcher */
#endif
}

#ifdef NOT_USED
/***********************************************************************/
/*
*	Srmode ()
*
*	Return the value of the rcp flag
*
*/
int Srmode(void )
{
	return(rcpon);
}
#endif

/***********************************************************************/
/*
*	Sftpmode ( mode )
*
*	Turn on or off the ftp watcher
*
*/
static int ftpon=0;

int Sftpmode(int mode)
{
BUG("Sftpmode");
  if(ftpon && mode)
    return(-1);
  ftpon=mode;
#ifdef FTP
  if(ftpon)
    setftp();             /* set the ftp watcher */
  else
    unsetftp();           /* clear the ftp watcher */
#endif
return(0);
}

#ifndef NET14

#ifdef NOT_USED
/***********************************************************************/
/*
*	Sfmode ()
*
*	Return whether the ftp watcher is on or off
*
*/
int Sfmode(void )
{
	return(ftpon);
}
#endif

/***********************************************************************/
/*
*	Snewcap ( s )
*
*   Set a new capture file name
*
*/
int Snewcap(char *s)
{
	if(NULL==(Scon.capture=malloc(strlen(s)+1)))
		return(1);
	strcpy(Scon.capture,s);
	return(0);
}

/***********************************************************************/
/*
*	Snewps ( s )
*
*	Set a new postscript file name
*
*/
int Snewpsfile(char *s)
{
	if(NULL==(Scon.psfile=malloc(strlen(s)+1)))
		return(1);
	strcpy(Scon.psfile,s);
	return(0);
}

/***********************************************************************/
/*
*	Snewhpfile ( s )
*
*	Set a new HPGL file name
*
*/
int Snewhpfile(char *s)
{
	if(NULL==(Scon.hpfile=malloc(strlen(s)+1)))
		return(1);
	strcpy(Scon.hpfile,s);
	return(0);
}

/***********************************************************************/
/*
*	Snewtekfile ( s )
*
*	Set a new tek file name
*
*/
int Snewtekfile(char *s)
{
	if(NULL==(Scon.tekfile=malloc(strlen(s)+1)))
		return(1);
	strcpy(Scon.tekfile,s);
	return(0);
}

/***********************************************************************/
/*
*	Sopencap ()
*
*	Returns a file handle to an open capture file
*
*/
FILE *Sopencap(void )
{
	FILE *retfp;

	if(NULL==(retfp=fopen(Scon.capture,"ab"))) 
		return(NULL);
	fseek(retfp,0L,2);		/* seek to end */
	return(retfp);
}
#endif

/**************************************************************************/
/*
*	Stask ()
*	A higher level version of net sleep -- manages the timer queue.  Always
* call this in your main loop.
*
*/
static int32 recent=0L;

void Stask(void)
{
	long t;
	int i;

	netsleep(0);
/*
*  Check the timer queue to see if something should be posted
*  First check for timer wraparound
*/

  t=n_clicks();
	if(t<recent) {
		i=Stfirst;
		while(i>=0) {
			Stq[i].when-=WRAPTIME;
			i=Stq[i].next;
		  }
	  }
	recent=t;							/* save most recent time */
	while (Stfirst>=0 && t> Stq[Stfirst].when) {	/* Q is not empty and timer is going off */
		i=Stfirst;
		netputevent(Stq[i].eclass,Stq[i].event,Stq[i].idata);
		Stfirst=Stq[Stfirst].next;	/* remove from q */
		Stq[i].next=Stfree;
		Stfree=i;						/* add to free list */
	  }
}

/**************************************************************************/
/*
*	Stimerset ( class, event, dat, howlong )
*
*	Set an async timer which is checked in Stask -- when time elapses sticks
* an event in the network event queue
*
*	class, event, dat is what gets posted when howlong times out.
*
*/
int Stimerset(int class,int event,int dat,int howlong)
{
	int i,j,jlast,retval;
	int32 gooff;

	retval=0;
    gooff=n_clicks()+howlong;
	if(Stfree<0) {				/* queue is full, post first event */
		Stfree=Stfirst;
		Stfirst=Stq[Stfirst].next;
		Stq[Stfree].next=-1;
		netputevent(Stq[Stfree].eclass,Stq[Stfree].event,Stq[Stfree].idata);
		retval=-1;
	  }
	Stq[Stfree].idata=dat;				/* event to occur at that time */
	Stq[Stfree].event=(unsigned char)event;
	Stq[Stfree].eclass=(unsigned char)class;
	Stq[Stfree].when=gooff;
	i=Stfree;							/* remove from free list */
	Stfree=Stq[i].next;
	if(Stfirst<0) {					/* if no queue yet */
		Stfirst=i;
		Stq[i].next=-1;				/* anchor active q */
	  }
	else 
		if(gooff<Stq[Stfirst].when) {	/* goes first on list */
			Stq[i].next=Stfirst;				/* at beginning of list */
			Stfirst=i;
		  }
		else {									/* goes in middle */
			j=jlast=Stfirst;				/* search q from beginning */
			while (gooff>=Stq[j].when&&j>=0) {
				jlast=j;
				j=Stq[j].next;
			  }
			Stq[i].next=j;					/* insert in q */
			Stq[jlast].next=i;
		  }
	return(retval);
}

/****************************************************************************/
/*
*	Stimerunset ( class, event, dat )
*
*	Remove all timer events from the queue that match the class/event/dat.
*
*
*/
int Stimerunset(unsigned char class,unsigned char event,int dat)
{
	int i,ilast,retval;

	retval=ilast=-1;
	i=Stfirst;
	while (i>=0 ) {					/* search list */
		if(Stq[i].idata==dat&&Stq[i].eclass==class && Stq[i].event==event) {
			retval=0;					/* found at least one */
/*
* major bug fix -- if first element matched, old code could crash
*/
			if(i==Stfirst) {
				Stfirst=Stq[i].next;			/* first one matches */
				Stq[i].next=Stfree;			/* attach to free list */
				Stfree=i;
				i=Stfirst;
				continue;						/* start list over */
			  }
			else {
				Stq[ilast].next=Stq[i].next;	/* remove this entry */
				Stq[i].next=Stfree;			/* attach to free list */
				Stfree=i;
				i=ilast;
			  }
		  }
		ilast=i;
		i=Stq[i].next;
	  }
	return(retval);
}

#ifndef NET14
/****************************************************************************/
/*
*	Scheckpass ( us, ps )
*
*	Check the password file for the user/password combination from ftp.
*
*	Returns valid/invalid
*
*/
int Scheckpass(char *us,char *ps)
{
	char buf[81],*p;
	FILE *fp;

  if(NULL==(fp=fopen(Scon.pass,"r"))) 
		return(0);
  while (NULL != fgets(buf,80,fp)) {
		p=strchr(buf,'\n');
    *p='\0';                                  /* remove \n */
    p=strchr(buf,':');                        /* find delimiter */
		*p++='\0';
#ifdef OLD_WAY
    if(!strcmp(buf,us) && Scompass(ps,p)) {   /* does password check? */
			fclose(fp);
			return(1);
    }
#else
    if(!strcmp(buf,us) && (!strcmp("",p) || Scompass(ps,p))) {   /* does password check, or is password field blank? */
      if(!strcmp("",p)) {  /* anon FTP  rmg 931031 */
#ifdef AUX
        fprintf(stdaux," ANONFTP login by %s ",ps);
#endif
        fclose(fp);
        return(2);
      }
      fclose(fp);
      return(1);
    }
#endif
    fgets(buf,256,fp);                        /* skip dirs */
  }
	fclose(fp);
  return(0);
}

/****************************************************************************/
/*
*	Sneedpass ()
*
*	Check to see if the password file is used -- 0 if not, 1 if it is.
*
*/
int Sneedpass(void )
{
  if(Scon.pass==NULL)
		return(0);
	return(1);
}

/****************************************************************************/
/*
*	Scompass ( ps, en )
*
*	Compute and check the encrypted password
*
*/
int Scompass(char *ps,char *en)
{
	int ck;
	char *p,c;

	ck=0;
	p=ps;


  if(!strcmp("*",en))  /* "*" in passwd field is disabled login  rmg 931031 */
    return(1);

	while (*p)							/* checksum the string */
		ck+=(int)*p++;
  c=(char)ck;
	while (*en) {
		if((((*ps^c)|32)&127)!=*en)		/* XOR with checksum */
			return(0);
		if(*ps)
			ps++;
		else
			c++;						/* increment checksum to hide length */
		en++;
  }
	return(1);
}
#endif

/****************************************************************************/
/*
*	Sgetevent ( class, what, datp )
*
*	Gets events from the network and filters those for session related
*/
int Sgetevent(int class,int *what,int *datp)
{
  int retval;

	if(retval=netgetevent(SCLASS,what,datp)) {	/* session event */
		switch (retval) {
#ifdef FTP
      case FTPACT:
        ftpd(0,*datp);
        break;
#endif

#ifdef RCP
      case RCPACT:                /* give CPU to rsh for rcp */
        rshd(0);
        break;
#endif

			case UDPTO:					/* name server not responding */
				domto(*datp);
				break;

			case RETRYCON:
				if(0<netopen2(*datp)) 	/* connection open yet? */
          Stimerset(SCLASS,RETRYCON,*datp,20); /* was 4 */
                     /* 20 (ticks) for retrans is a kludge (rekludged, rmg 931100) */
				break;

			default:
				break;
		  }
	  }

	Stask();						/* allow net and timers to take place */

	if(!(retval=netgetevent((unsigned char)class,what,datp))) return(0);

	if(retval==CONOPEN) Stimerunset(CONCLASS,CONFAIL,*datp);   /* kill this timer */

  if((*datp==997) && (retval==UDPDATA))
    udpdom();
	else {
    if((*what==CONCLASS) && (Sptypes[*datp]>=0)) {    /* might be for session layer */
			switch (Sptypes[*datp]) {
#ifdef FTP
        case PFTP:
					rftpd(retval);
          break;

				case PDATA:
					ftpd(retval,*datp);
					break;
#endif

#ifdef RCP
        case PRCP:
					rshd(retval);
          break;
#endif

				default:
					break;
			  }	/* end switch */
		  }	/* end if */
        else
            return(retval);                /* let higher layer have it */
      } /* end else */
	return(0);
}

#if (defined(TELBIN) && defined(FTP)) || defined(FTPBIN)
char *fixdirnm(char *dirnm)
{
	int len;
 
	if (!dirnm)
		return (char *)NULL;			/* name is nil */
	len = strlen(dirnm);
	while (len > 1 &&					/* name contains multiple characters */
	  (dirnm[len - 1] == '/' || dirnm[len - 1] == '\\') && /* trailing slash */
	  dirnm[len - 2] != ':')					/* not "disk:/" or "disk:\"  */
		dirnm[--len] = '\0';	/* strip off trailing slash character */

	return(dirnm);
}

/* safedir  checks the password file for permission to change to this
   directory structure.  Mode 1 == strip ending filename.
   The most specific directory in the password file is used. */
int safedir(char *d, char *user, int mode)
{
  char buf[256],*p;
  char dirnm[_MAX_DIR],
       okdir[_MAX_DIR],
       olddir[_MAX_DIR];
   int best_num=0;
   int best_perm=0;
   int old_perm=0;
   int driveonly=0;
  FILE *fp;

#ifdef AUX
  fprintf(stdaux," sdir ");
#endif

  if(!strcmp("",d))
    return(0);             /* retain current settings if no change */

  old_perm=twperm;         /* save in case of failure */

  getcwd(olddir,_MAX_DIR);
  strcpy(dirnm,d);
  if(mode) {                   /* strip off filename for deletions */
    if(chgdir(dirnm)) {        /* if valid directory, do not strip */
      if(p=strrchr(dirnm,'\\')) {
        if( *(p-1) == ':' ) {
          p++;
        }
        *p++='\0';
      }
#ifdef NOT_USED /* flipped in bkgr.c */
      else if(p=strrchr(dirnm,'/')) {
        *p++='\0';
#endif
      else if(p=strchr(dirnm,':')) {
        *++p='\0';
      }
      else if(p=strchr(dirnm,'|')) { /* for WWW & Mosaic */
        *++p='\0';
      }
      else {                     /* filename only, no directories specified */
        chgdir(olddir);
#ifdef AUX
  fprintf(stdaux,"%s is file. ",dirnm);
#endif
        return(0);               /* must be OKdokee */
      }
    }
  chgdir(olddir);
  }

  if(NULL==(fp=fopen(Scon.pass,"r"))) {
#ifdef AUX
  fprintf(stdaux,"no pw file. ");
#endif
    return(0);
  }
  while (NULL != fgets(buf,80,fp)) {
    p=strchr(buf,'\n');
    *p='\0';                   /* remove \n */
    p=strchr(buf,':');         /* find delimiter */
    *p++='\0';
    if(!strcmp(buf,user)) {
      fgets(buf,256,fp);
      p=buf;
      if(chgdir(dirnm)) {
#ifdef AUX
  fprintf(stdaux,"cd fail. ");
#endif
        return(1);             /* chdir failed */
      }
      getcwd(dirnm,_MAX_DIR);
      while(0<sscanf(p,"%d %s",&twperm, okdir)) {
#ifdef OLD_WAY /* change to first matching dir */
        if(!strnicmp(okdir,dirnm,strlen(okdir))) {     /* is dir allowed ?*/
          fclose(fp);
          if(mode)
            chgdir(olddir);    /* operation expects curr dir */
          return(0);           /* vurked */
        }
        p=strstr(buf,okdir);
        p += strlen(okdir);    /* shorten search string */
      }
      chgdir(olddir);          /* they cant stay there */
#else /* change to best fitting dir */
        if(!strnicmp(okdir,dirnm,strlen(okdir))) {     /* is dir allowed ?*/
          if(strlen(okdir) >= (unsigned) best_num) {
            best_num=strlen(okdir);
            best_perm=twperm;
          }
        }
        p += strlen(okdir) + 3;    /* shorten search string */
      }
      fclose(fp);
      if(best_num && best_perm) {
        if(mode)
          chgdir(olddir);      /* operation expects curr dir */
        twperm=best_perm;
#ifdef AUX
  fprintf(stdaux,"%s %d. ",dirnm,twperm);
#endif
        return(0);             /* vurked */
      }
      else {
        twperm=old_perm;       /* rmg 940127 */
        chgdir(olddir);        /* operation expects curr dir */
#ifdef AUX
  fprintf(stdaux,"%s denied. ",dirnm);
#endif
        return(2);             /* dir not found or not allowed */
      }
#endif
    }
    fgets(buf,256,fp);         /* skip dirs */
  }
  fclose(fp);
#ifdef AUX
  fprintf(stdaux,"user %s not found. ",user);
#endif
  return(2);                   /* user not found (impossible) */
}

/* gosafedir places the FTP user in their home directory upon user validation.
   the home directory is defined as the first directory in the passwd list.
   the root case removes all access restrictions for the FTP session. */
int gosafedir(char *user)            /* returns 0=OK 1,2=error 3=root */
{
  char buf[256],*p;
  FILE *fp;

  if(NULL==(fp=fopen(Scon.pass,"r")))
    return(0);
  while (NULL != fgets(buf,256,fp)) {
    p=strchr(buf,'\n');
    *p='\0';                   /* remove \n */
    p=strchr(buf,':');         /* find delimiter */
    *p++='\0';
    if(!strcmp(buf,user)) {
      fscanf(fp,"%d %s",&twperm,buf);     /* move to the first directory listed */
                                          /* does not check permissions */
      if(!strcmp(buf,"root")) {
        fclose(fp);
        return(3);             /* special root case */
      }
      fclose(fp);
      if(chgdir(buf)) {
        return(1);             /* chdir failed */
      }
      else {
        return(0);             /* vurked */
      }
    }
    else
      fgets(buf,256,fp);       /* skip dirs */
  }
  fclose(fp);
  return(2);                   /* user not found */
}
#endif /* telbin or ftpbin */
