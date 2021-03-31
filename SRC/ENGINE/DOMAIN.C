/*
*	DOMAIN.C
*
*	Domain processing
*
*****************************************************************************
*																			*
*	 part of:																*
*	 TCP/IP kernel for NCSA Telnet											*
*	 by Tim Krauskopf														*
*																			*
*	 National Center for Supercomputing Applications						*
*	 152 Computing Applications Building									*
*	 605 E. Springfield Ave.												*
*	 Champaign, IL  61820													*
*																			*
*****************************************************************************
*
*	Revision history:
*
*	10/87  Initial source release, Tim Krauskopf
*	5/89	clean up for 2.3 release, JKM	
*
*/

#define DOMAINMASTER

/*
 *	Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "whatami.h"
#include "hostform.h"
#include "domain.h"
#include "defines.h"
#include "externs.h"

extern struct config Scon;		/* hardware configuration */
extern struct machinfo *Sns;
static int domwait=0;			/* is domain waiting for nameserver? */

/*
*  data for domain name lookup
*/
static struct useek {
	struct dhead h;
	uint8 x[DOMSIZE];
} question;


/* STATIC function declarations */
static int packdom(char *dst,char *src);
static int unpackdom(char *dst,char *src,char buf[]);
static int ddextract(struct useek *,unsigned char *);
static void sendom(char *,char *,int16 );
static void qinit(void );

static void qinit(void )
{
	question.h.flags=intswap(DRD);
	question.h.qdcount=intswap(1);
	question.h.ancount=0;
	question.h.nscount=0;
	question.h.arcount=0;
}

/*********************************************************************/
/*  packdom
*   pack a regular text string into a packed domain name, suitable
*   for the name server.
*/
static int packdom(char *dst,char *src)
{
	char *p,*q,*savedst;
	int i,dotflag,defflag;

	p=src;
	dotflag=defflag=0;
	savedst=dst;
	do {							/* copy whole string */
		*dst=0;
		q=dst+1;
/*
*  copy the next label along, char by char until it meets a period or
*  end of string.
*/
		while(*p && (*p!='.')) 
			*q++=*p++;
		i=p-src;
		if(i>0x3f)
			return(-1);
		*dst=(char)i;
		*q=0;
		if(*p) {					/* update pointers */
			dotflag=1;
			src=++p;
			dst=q;
		  }
		else
			if(!dotflag && !defflag && Scon.defdom) {
				p=Scon.defdom;		/* continue packing with default */
				defflag=1;
				src=p;
				dst=q;
				netposterr(801);		/* using default domain */
			  }
	  } while(*p);
	q++;
	return((int)(q-savedst));			/* length of packed string */
}

/*********************************************************************/
/*  unpackdom
*  Unpack a compressed domain name that we have received from another
*  host.  Handles pointers to continuation domain names -- buf is used
*  as the base for the offset of any pointer which is present.
*  returns the number of bytes at src which should be skipped over.
*  Includes the NULL terminator in its length count.
*/
static int unpackdom(char *dst,char *src,char buf[])
{
	int i,j,retval=0;
	char *savesrc;

	savesrc=src;
	while(*src) {
		j=*src;
		while((j & 0xC0)==0xC0) {
			if(!retval)
				retval=src-savesrc+2;
			src++;
			src=&buf[(j & 0x3f)*256+*src];		/* pointer dereference */
			j=*src;
		  }
		src++;
		for(i=0; i<(j & 0x3f) ; i++)
			*dst++=*src++;
		*dst++='.';
	  }
	*(--dst)=0;				/* add terminator */
	src++;					/* account for terminator on src */
	if(!retval)
		retval=src-savesrc;
	return(retval);
}

/*********************************************************************/
/*  sendom
*   put together a domain lookup packet and send it
*   uses port 53
*/
static void sendom(char *s,char *towho,int16 num)
{
	uint16 i,ulen;
	uint8 *psave,*p;

	psave=(uint8 *)question.x;
	i=packdom(question.x,s);
/*
*  load the fields of the question structure a character at a time so
*  that 68000 machines won't choke.
*/
	p=&question.x[i];
	*p++=0;					/* high byte of qtype */
	*p++=DTYPEA;			/* number is<256, so we know high byte=0 */
	*p++=0;					/* high byte of qclass */
	*p++=DIN;				/* qtype is<256 */
	question.h.ident=intswap(num);
	ulen=sizeof(struct dhead)+(p-psave);
  netusend(towho,53,997,(char *)&question,ulen,DNRINDEX);
}

/**************************************************************************/
/*  Sdomain
*   DOMAIN based name lookup
*   query a domain name server to get an IP number
*	Returns the machine number of the machine record for future reference.
*   Events generated will have this number tagged with them.
*   Returns various negative numbers on error conditions.
*	 Checks for different port and save the port number
*/
int Sdomain(char *mname)
{
	struct machinfo *m;
	int new,i,port,pflag=0;

	if(!Sns) 							/* no nameserver, give up now */
		return(-1);
	while(*mname && *mname<33)			/* kill leading spaces */
		mname++;
	if(!(*mname))
		return(-1);

/*
*	Find out what port to open to
*/
	for(i=0; (mname[i]!=' ') && (mname[i]!='#') && (mname[i]!='\0'); i++);

	if((mname[i]=='#') || (mname[i]==' ')) {
		mname[i++]='\0';
		new=i;
		pflag=1;
		for( ; (mname[i]!='\0') && isdigit(mname[i]) ; i++);

		if(mname[i]!='\0') 
			pflag=0;
		if(pflag) 
			port=(unsigned int)atoi(&mname[new]);
  	}

	if(!(m=Smadd(mname)))
		return(-1);						/* adds the number to the machlist */
	if(domwait<Scon.domto)
		domwait=Scon.domto;				/* set the minimum timeout */
	qinit();							/* initialize some flag fields */
  netulisten(997,DNRINDEX);          /* pick a return port */
	if(!m->hname)
		m->hname=m->sname;				/* copy pointer to sname */
	if(pflag)
		m->port=port;		/* Put save port number */
	sendom(m->hname,Sns->hostip,m->mno);	/* try UDP */
	Stimerset(SCLASS,UDPTO,m->mno,domwait);	/* time out quickly first time */
	m->mstat=UDPDOM;
	return(m->mno);
}

/*********************************************************************/
/*  getdomain
*   Look at the results to see if our DOMAIN request is ready.
*   It may be a timeout, which requires another query.
*/
int udpdom(void )
{
	struct machinfo *m;
	int i,uret,num;
	char *p;

  uret=neturead((char *)&question,DNRINDEX,sizeof(question));
	if(uret<0) {
/*		netputevent(USERCLASS,DOMFAIL,-1);  */
		return(-1);
	  }
	num=intswap(question.h.ident);		/* get machine number */
/*
*  check to see if the necessary information was in the UDP response
*/
	m=Slooknum(num);				/* get machine info record */
	if(!m) {
		netputevent(USERCLASS,DOMFAIL,num);
		return(-1);
	  }
/*
*  got a response, so reset timeout value to recommended minimum
*/
	domwait=Scon.domto;
	i=ddextract(&question,m->hostip);
	switch (i) {
		case 3:						/* name does not exist */
			netposterr(802);
			p=neterrstring(-1);
			strncpy(p,m->hname,78);		/* what name */
			netposterr(-1);
			netputevent(USERCLASS,DOMFAIL,num);
			Stimerunset(SCLASS,UDPTO,num);
			return(-1);

		case 0:						/* we found the IP number */
			Stimerunset(SCLASS,UDPTO,num);
			m->mstat=DOM;			/* mark that we have it from DOMAIN */
			netputevent(USERCLASS,DOMOK,num);
			return(0);

		case -1:					/* strange return code from ddextract */
			netposterr(803);
			break;

		default:
			netposterr(804);
			break;
	  }
	return(0);
}

/**************************************************************************/
/*  domto
*   Handle time out for DOMAIN name lookup
*   Retry as many times as recommended by config file
*/
int domto(int num)
{
	struct machinfo *m;

	m=Slooknum(num);
	if(!m)
		return(-1);
	if(m->mstat>UDPDOM+Scon.ndom) {	/* permanent timeout */
		netputevent(USERCLASS,DOMFAIL,num);
		return(-1);
	  }
	else
		m->mstat++;			/* one more timeout */
	if(domwait<20)		/* exponential backoff */
		domwait <<= 1;
	Snewns();				/* rotate to next nameserver */
	qinit();
  netulisten(997,DNRINDEX);          /* pick a return port */
	sendom(m->hname,Sns->hostip,num);		/* try UDP */
	Stimerset(SCLASS,UDPTO,num,domwait);	/* time out more slowly */
	return(num);
}

/*********************************************************************/
/*  ddextract
*   extract the ip number from a response message.
*   returns the appropriate status code and if the ip number is available,
*   copies it into mip
*/
static int ddextract(struct useek *qp,uint8 *mip)
{
	uint16 i,j,nans,rcode;
	struct rrpart *rrp;
	uint8 *p,space[260];

	nans=intswap(qp->h.ancount);				/* number of answers */
	rcode=DRCODE & intswap(qp->h.flags);		/* return code for this message*/
	if(rcode>0)
		return((int)rcode);
	if(nans>0 && (intswap(qp->h.flags) & DQR)) {			/* response flag is set  and at least one answer */
		p=(uint8 *)qp->x;					/* where question starts */
		i=unpackdom(space,p,(char *)qp);			/* unpack question name */
/*  spec defines name then  QTYPE+QCLASS=4 bytes */
		p+=i+4;
/*
*  at this point, there may be several answers.  We will take the first
*  one which has an IP number.  There may be other types of answers that
*  we want to support later.
*/
		while(nans-->0) {					/* look at each answer */
			i=unpackdom(space,p,(char *)qp);			/* answer name to unpack */
/*			n_puts(space);*/
			p+=i;						/* account for string */
			rrp=(struct rrpart *)p;			/* resource record here */
/*
*  check things which might not align on 68000 chip one byte at a time
*/
			if(!*p && *(p+1)==DTYPEA && !*(p+2) && *(p+3)==DIN) {		/* correct type and class */
				movebytes(mip,rrp->rdata,4);	/* save IP # 		*/
				return(0);						/* successful return */
			  }
			movebytes(&j,&rrp->rdlength,2);		/* 68000 alignment */
			p+=10+intswap(j);					/* length of rest of RR */
		  }
	  }
	return(-1);						/* generic failed to parse */
}
