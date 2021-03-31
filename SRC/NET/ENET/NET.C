/*
*  net.c
*****************************************************************************
*																		  	*
*	  part of:																*
*	  TCP/UDP/ICMP/IP Network kernel for NCSA Telnet						*
*	  by Tim Krauskopf														*
*																		  	*
*	  National Center for Supercomputing Applications					 	*
*	  152 Computing Applications Building								 	*
*	  605 E. Springfield Ave.											 	*
*	  Champaign, IL  61820													*
*																		  	*
*****************************************************************************
*
*   Revision History:
*   9/91    Add SLIP support    Nelson B. Bolyard
*
*  those generic tool-type things that only work on PCs.
*  includes all hardware-level calls to Ethernet that are unique to the PC
*
*  Function pointers for Ether calls
*/

/*#define DEBUG*/

#ifdef NETD
# include <conio.h>
#else
#ifdef DEBUG
# include <conio.h>
#endif
#endif
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include "protocol.h"
#include "data.h"
#include "hostform.h"
#include "externs.h"

#ifdef PKT_ONLY
#define NO_APPLETALK
#endif

int SQwait=0;
int OKpackets=0;

#define ETOPEN_ARGS   unsigned char*,unsigned int,unsigned int,unsigned int
#define GA_ARGS       unsigned char*,unsigned int,unsigned int
#define SA_ARGS       unsigned char*,unsigned int,unsigned int
#define XM_ARGS       DLAYER*,unsigned int

/*
*   defined in assembly language file for interrupt driven Ether buffering
*
*/

extern unsigned char rstat;		/* last status from read */
extern char *bufpt,				/* current buffer pointer */
	*bufend,					/* pointer to end of buffer */
	*bufread,					/* pointer to where program is reading */
	*buforg;					/* pointer to beginning of buffer */
 
extern int bufbig,				/* number of bytes currently in buffer */
	buflim;						/* max number of bytes in buffer */

struct config def;

/*
*  Declare each and every Ethernet driver.
*  To add a driver, pick a unique 2 char prefix and declare your own
*  routines.  I want to keep the same parameters for EVERY driver.
*  If your driver needs additional parameters, then see netconfig() below 
*  for an indication of where to put custom board code.
*/
#ifndef PKT_ONLY /* save 20K by compiling packet driver only  rmg 931100 */
extern int  CDECL E1etopen(ETOPEN_ARGS),
            CDECL E1getaddr(GA_ARGS),
            CDECL E1setaddr(SA_ARGS),
            CDECL E1xmit(XM_ARGS),
            CDECL E1etclose(void);
extern void CDECL E1recv(void),
            CDECL E1etupdate(void);
extern int  CDECL E3etopen(ETOPEN_ARGS),
            CDECL E3getaddr(GA_ARGS),
            CDECL E3setaddr(SA_ARGS),
            CDECL E3xmit(XM_ARGS),
            CDECL E3etclose(void);
extern void CDECL E3recv(void),
            CDECL E3etupdate(void);
extern int  CDECL E5etopen(ETOPEN_ARGS),
            CDECL E5getaddr(GA_ARGS),
            CDECL E5setaddr(SA_ARGS),
            CDECL E5xmit(XM_ARGS),
            CDECL E5etclose(void),
            CDECL E5etdma();
extern void CDECL E5recv(void),
            CDECL E5etupdate(void);
extern int  CDECL ATetopen(ETOPEN_ARGS),
            CDECL ATgetaddr(GA_ARGS),
            CDECL ATxmit(XM_ARGS),
            CDECL ATetclose(void);
extern void CDECL ATrecv(void),
            CDECL ATetupdate(void);
extern int  CDECL M5etopen(ETOPEN_ARGS),
            CDECL M5getaddr(GA_ARGS),
            CDECL M5xmit(XM_ARGS),
            CDECL M5etclose(void);
extern void CDECL M5recv(void),
            CDECL M5etupdate(void);
extern int  CDECL M9etopen(ETOPEN_ARGS),
            CDECL M9getaddr(GA_ARGS),
            CDECL M9xmit(XM_ARGS),
            CDECL M9etclose(void);
extern void CDECL M9recv(void),
            CDECL M9etupdate(void);
extern int  CDECL U1etopen(ETOPEN_ARGS),
            CDECL U1getaddr(GA_ARGS),
            CDECL U1xmit(XM_ARGS),
            CDECL U1etclose(void);
extern void CDECL U1recv(void),
            CDECL U1etupdate(void);
extern int  CDECL U2etopen(ETOPEN_ARGS),
            CDECL U2getaddr(GA_ARGS),
            CDECL U2xmit(XM_ARGS),
            CDECL U2etclose(void);
extern void CDECL U2recv(void),
            CDECL U2etupdate(void);
extern int  CDECL WDetopen(ETOPEN_ARGS),
            CDECL WDgetaddr(GA_ARGS),
            CDECL WDxmit(XM_ARGS),
            CDECL WDetclose(void);
extern void CDECL WDrecv(void),
            CDECL WDetupdate(void);
extern int  CDECL WAetopen(ETOPEN_ARGS),
            CDECL WAgetaddr(GA_ARGS),
            CDECL WAxmit(XM_ARGS),
            CDECL WAetclose(void);
extern void CDECL WArecv(void),
            CDECL WAetupdate(void);
extern int  CDECL E2etopen(ETOPEN_ARGS),
            CDECL E2getaddr(GA_ARGS),
            CDECL E2xmit(XM_ARGS),
            CDECL E2etclose(void);
extern void CDECL E2recv(void),
            CDECL E2etupdate(void);
extern int  CDECL E4etopen(ETOPEN_ARGS),
            CDECL E4getaddr(GA_ARGS),
            CDECL E4xmit(XM_ARGS),
            CDECL E4etclose(void);
extern void CDECL E4recv(void),
            CDECL E4etupdate(void);
#endif /* PKT_ONLY */

#ifndef EXTERNS_H
#ifndef NO_APPLETALK
#ifdef AUX
extern int  CDECL LTopen(AddrBlk *s,unsigned int irq,unsigned int addr,unsigned int ioaddr);
extern int  CDECL LTgetaddr(AddrBlk *s,unsigned int addr,unsigned int ioaddr);
extern int  CDECL LTxmit(DLAYER *ptr,unsigned int size);
extern int
#else
extern int  CDECL LTopen(ETOPEN_ARGS),
            CDECL LTgetaddr(GA_ARGS),
            CDECL LTxmit(XM_ARGS),
#endif
            CDECL LTclose(void);
extern void CDECL LTrecv(void),
            CDECL LTupdate(void);
#endif
extern int  CDECL pketopen(ETOPEN_ARGS),
            CDECL pkgetaddr(GA_ARGS),
            CDECL pkxmit(XM_ARGS),
            CDECL pketclose(void);
extern void CDECL pkrecv(void),
            CDECL pketupdate(void);
extern int  CDECL DNetopen(ETOPEN_ARGS),
            CDECL DNgetaddr(GA_ARGS),
            CDECL DNxmit(XM_ARGS),
            CDECL DNetclose(void);
extern void CDECL DNrecv(void),
            CDECL DNetupdate(void);
extern int  CDECL ILetopen(ETOPEN_ARGS),
            CDECL ILgetaddr(GA_ARGS),
            CDECL ILxmit(XM_ARGS),
            CDECL ILetclose(void);
extern void CDECL ILrecv(void),
            CDECL ILetupdate(void);
#endif

static int  CDECL (*etopen)(unsigned char *s,unsigned int irq,unsigned int addr,unsigned int ioaddr)=NULL,       /* open the device */
            CDECL (*getaddr)(unsigned char *s,unsigned int addr,unsigned int ioaddr)=NULL,                    /* get the Ether address */
            CDECL (*setaddr)(char *s,unsigned int addr,int ioaddr)=NULL,     /* set the Ether address */
            CDECL (*etclose)(void )=NULL,                           /* shut down network */
            CDECL (*xmit)(DLAYER *packet,unsigned int count)=NULL;    /* transmit a packet */
static void CDECL (*recv)(void )=NULL,                      /* load a packet from queue  */
            CDECL (*etupdate)(void )=NULL;                          /* update pointers in buffer */

#ifndef NET14
#ifdef NOT_USED
/**********************************************************************/
/*  statcheck
*   look at the connection status of the memory buffers to see if the
*   allocation schemes are working.  Only used as a debug tool.
*/

void statcheck(void )
{
	int i;
	struct port *p;

	for(i=0; i<20; i++) {
		printf("\n%d > ",i);
		p=portlist[i];
		if(p!=NULL) 
			printf("state: %d  %5u  %5u  %10ld  %5d  %5d",
					p->state,intswap(p->tcpout.t.source),
					intswap(p->tcpout.t.dest),p->out.lasttime,p->rto,
					p->out.contain);
	  }	/* end for */
}
#endif
#endif

/*************************************************************************/
/*  config network parameters
*   Set IRQ and DMA parameters for initialization of the 3com adaptor
*/
static uint nnirq=3,nnaddr=0xd000,nnioaddr=0x300;

int netparms(uint irq,uint address,uint ioaddr)
{
	BUG("netparms");

	nnirq=irq;
	nnaddr=address;
	nnioaddr=ioaddr;
#ifdef DEBUG
	printf("nnmyaddr=%X;nnaddr=%X;nnioaddr=%X\n",nnmyaddr,nnaddr,nnioaddr);
	printf("press a key\n");
	getch();
#endif
	return(0);
}

/**********************************************************************/
/* netconfig
*  load the function pointers for network access
*  Currently setaddr() is not used, so it isn't loaded.
*
*  Note that netparms is called BEFORE netconfig.  So if you have any
*  really special variables to set for your board that involve
*  irq,address and ioaddr, you can add calls to your special routines
*  in this section.
*
*  Some drivers will do the interrupt driver and board initialization
*  in etopen() and some will do it in getaddr().
*/
void netconfig(char *s)
{

	Sgetconfig(&def);				/* get information provided in hosts file */

#ifndef PKT_ONLY
	if(!strncmp(s,"iso",3) || !strncmp(s,"bicc",4)) {
		etopen=ILetopen;
		xmit=ILxmit;
		recv=ILrecv;
		getaddr=ILgetaddr;
		etupdate=ILetupdate;
		etclose=ILetclose;
	  } /* end if */
	else if(!strncmp(s,"3c505",5) || !strncmp(s,"505",3)) {
		etopen=E5etopen;
		xmit=E5xmit;
		recv=E5recv;
		getaddr=E5getaddr;
		etupdate=E5etupdate;
		etclose=E5etclose;
	  } /* end if */
	else if(!strncmp(s,"decnet",6) || !strncmp(s,"dn",2)) {
		etopen=DNetopen;
		xmit=DNxmit;
		recv=DNrecv;
		getaddr=DNgetaddr;
		etupdate=DNetupdate;
		etclose=DNetclose;
	  }	/* end if */
	else if(!strncmp(s,"star10",6) || !strncmp(s,"starlan",7)) {
		etopen=ATetopen;
		xmit=ATxmit;
		recv=ATrecv;
		getaddr=ATgetaddr;
		etupdate=ATetupdate;
		etclose=ATetclose;
	  }	/* end if */
  else if(!strncmp(s,"packet",6)) {
#else
  if(!strncmp(s,"packet",6)) {
#endif /* PKT_ONLY */
		etopen=pketopen;
		xmit=pkxmit;
		recv=pkrecv;
		getaddr=pkgetaddr;
		etupdate=pketupdate;
		etclose=pketclose;
	  }
#ifndef PKT_ONLY
	else if(!strncmp(s,"ni9",3) || !strncmp(s,"92",2)) {
		etopen=M9etopen;
		xmit=M9xmit;
		recv=M9recv;
		getaddr=M9getaddr;
		etupdate=M9etupdate;
		etclose=M9etclose;
	}
	else if(!strncmp(s,"ni5",3) || !strncmp(s,"mi",2)) {
		etopen=M5etopen;
		xmit=M5xmit;
		recv=M5recv;
		getaddr=M5getaddr;
		etupdate=M5etupdate;
		etclose=M5etclose;
/*
*   special initialization call would go here
*/
	}
	else if(!strncmp(s,"nicps",5)) {
		etopen=U2etopen;
		xmit=U2xmit;
		recv=U2recv;
		getaddr=U2getaddr;
		etupdate=U2etupdate;
		etclose=U2etclose;
	}
	else if(!strncmp(s,"nicpc",5) || !strncmp(s,"pcnic",5)) {
		etopen=U1etopen;
		xmit=U1xmit;
		recv=U1recv;
		getaddr=U1getaddr;
		etupdate=U1etupdate;
		etclose=U1etclose;
	}
	else if(!strncmp(s,"wd8003a",7) || !strncmp(s,"8003a",5)) {
		etopen=WAetopen;
		xmit=WAxmit;
		recv=WArecv;
		getaddr=WAgetaddr;
		etupdate=WAetupdate;
		etclose=WAetclose;
	}
	else if(!strncmp(s,"wd",2) || !strncmp(s,"800",3)) {
		etopen=WDetopen;
		xmit=WDxmit;
		recv=WDrecv;
		getaddr=WDgetaddr;
		etupdate=WDetupdate;
		etclose=WDetclose;
	}
	else if(!strncmp(s,"3c523",5) || !strncmp(s,"523",3)) {
		etopen=E2etopen;
		xmit=E2xmit;
		recv=E2recv;
		getaddr=E2getaddr;
		etupdate=E2etupdate;
		etclose=E2etclose;
	}
	else if( !strncmp(s,"3c503",5) || !strncmp(s,"503",3)) {
		etopen=E4etopen;
		xmit=E4xmit;
		recv=E4recv;
		getaddr=E4getaddr;
		etupdate=E4etupdate;
		etclose=E4etclose;
		E4setwire(def.wire*2);
	}
#ifndef NO_APPLETALK
	else if(!strncmp(s,"ltalk",5) || !strncmp(s,"atalk",5) || !strncmp(s,"apple",5)) {
    extern unsigned int LTalk_vector;

		nnkip=1;					/* turn kip arping on */
		LTalk_vector=nnirq;
#ifdef __WATCOMC__
        etopen=(void *)LTopen;
#else
        etopen=LTopen;
#endif
		xmit=LTxmit;
		recv=LTrecv;
#ifdef __WATCOMC__
        getaddr=(void *)LTgetaddr;
#else
        getaddr=LTgetaddr;
#endif
		etupdate=LTupdate;
		etclose=LTclose;
	}
#endif
	else if(!strncmp(s,"r501",4)) {		/* special reserve driver */
		etopen=E3etopen;
		xmit=E3xmit;
		recv=E3recv;
		getaddr=E3getaddr;
		etupdate=E3etupdate;
		etclose=E3etclose;
	}
	else {		/* default choice */
		etopen=E1etopen;
		xmit=E1xmit;
		recv=E1recv;
		getaddr=E1getaddr;
		etupdate=E1etupdate;
		etclose=E1etclose;
	}
#else
  else {    /* default choice */
    fprintf(stdout,"You are using the PACKET DRIVER ONLY version of Telnet\n");
#ifdef AUX
    fprintf(stdaux," packet driver version ");
#endif
	}
#endif
}   /* end netconfig() */

/**********************************************************************/
/*  netarpme
*   send an arp to my address.  arpinterpret will notice any response.
*   Checks for adapters which receive their own broadcast packets.
*/
int netarpme(char *s)
{
#ifndef PKT_ONLY
	if(etopen==U2etopen)
		return(0);
	if(etopen==U1etopen)
		return(0);
#ifndef NO_APPLETALK
#ifdef __WATCOMC__
  if(etopen==(void *)LTopen)      /* not needed for Local Talk */
		return(0);
#else
  if(etopen==LTopen)      /* not needed for Local Talk */
		return(0);
#endif
#endif
#endif
	reqarp(s);		/* send it */
    return(0);
}   /* end netarpme() */

/**********************************************************************/
int initbuffer(void)
{
	bufpt=bufread=buforg=raw;	/*  start at the beginning */
	BUG("initbuffer");

	bufbig=0;
#if defined(NET14)
    bufend=&raw[4500];          /* leave 2K breathing room, required */
    buflim=2000;                /* another 2K breathing room */
#elif defined(__TURBOC__) || defined(__WATCOMC__)
	bufend=&raw[7500];			/* leave 2K breathing room, required */
	buflim=5000;				/* another 2K breathing room */
#else
	bufend=&raw[14500];			/* leave 2K breathing room, required */
	buflim=12000;				/* another 2K breathing room */
#endif

	BUG("before getaddr");
#ifdef DEBUG
	printf("nnmyaddr=%X;nnaddr=%X;nnioaddr=%X\n",nnmyaddr,nnaddr,nnioaddr);
	printf("getaddr = %p\n",getaddr);
	printf("press a key\n");
	getch();
#endif

BUG("about to getaddr");
/* QAK was if-def'ed out */
	(*getaddr)(nnmyaddr,nnaddr,nnioaddr);
BUG("after getaddr");

#ifdef NETD
	printf("Addr=%x.%x.%x.%x.%x.%x\n",nnmyaddr[0],nnmyaddr[1],nnmyaddr[2],
		nnmyaddr[3],nnmyaddr[4],nnmyaddr[5]);
#endif
	return(0);
}

/**********************************************************************/
/*   demux
*	  find the packets in the buffer, determine their lowest level
*  packet type and call the correct interpretation routines
*
*  the 'all' parameter tells demux whether it should attempt to empty
*  the input packet buffer or return after the first packet is dealt with.
*
*  returns the number of packets demuxed
*/
int demux(int all)
{
	uint16 getcode;
	int nmuxed;
	DLAYER *firstlook;

	nmuxed=0;
	if(!etupdate)					/* check that network is hooked up */
		return(0);
	do {							/* while all flag is on */
#ifndef PKT_ONLY /* RMG whatever */
		(*recv)();					/* NULL operation for 3COM */
#endif
#ifdef NETD
printf("After Masking!\n");
printf("bufpt->%p, bufread->%p, bufend->%p\n",bufpt,bufread,bufend);
printf("bufbig = %d\n", bufbig);
getch();
#endif
		if(bufbig>0) {
			nmuxed++;
			firstlook=(DLAYER *)(bufread+2); 	/* where packet is */
#ifdef CHECKRARP
fprintf(stderr,"dest=%x.%x.%x.%x.%x.%x, me=%x.%x.%x.%x.%x.%x, type=0x%X\r\n",
        (unsigned int)firstlook->dest[0],(unsigned int)firstlook->dest[1],(unsigned int)firstlook->dest[2],
        (unsigned int)firstlook->dest[3],(unsigned int)firstlook->dest[4],(unsigned int)firstlook->dest[5],
        (unsigned int)firstlook->me[0],(unsigned int)firstlook->me[1],(unsigned int)firstlook->me[2],
        (unsigned int)firstlook->me[3],(unsigned int)firstlook->me[4],(unsigned int)firstlook->me[5],
        firstlook->type);
fprintf(stderr,"big=%d, size=%d\r\n",bufbig,(int)(bufpt-bufread));
#endif

			getcode=firstlook->type;		/* where does it belong? */
			switch(getcode) {				/* what to do with it? */
				case EARP:
				case ERARP:
#ifdef CHECKRARP
fprintf(stderr,"arp\r\n");
#endif
					arpinterpret((ARPKT *)firstlook);	/* handle ARP packet */
					break;

        case EIP:
#ifdef CHECKRARP
fprintf(stderr,"ip\r\n");
#endif
					ipinterpret((IPKT *)firstlook);
					break;

				default:
#ifdef CHECKRARP
fprintf(stderr,"garbage\r\n");
#endif
					break;
			  }	/* end switch */
			(*etupdate)();		/* update read pointers in buffer, free packet */
		  }	/* end if */
		else 
			all=0;
	  }	while(all);		/* should we look for more to deal with? */
	return(nmuxed);		/* no packets anymore */
}   /* end demux() */

/************************************************************************/
/*  dlayersend
*
*  usage:   err=dlayersend(ptr,size)
*	  err=0 for successful, non-zero error code otherwise
*	  ptr is to a dlayer packet header
*	  size is the number of bytes total
*
*  This particular dlayer routine is for Ethernet.  It will have to be
*  replaced for any other dlayer.
*
*  Ethernet addresses are resolved at higher levels because they will only
*  need to be resolved once per logical connection, instead of once per
*  packet.  Not too layer-like, but hopefully modular.
*
*/
int dlayersend(DLAYER *ptr,unsigned int size)
{
  int ret,i;

#ifndef OLD_WAY
  if(size<60)
    size=60;
	if(size&0x01)
		size+=1;
#else
#ifdef NEW_WAY
	unsigned char *c;

	c=(unsigned char *)ptr;
	*(c+size++)=0;		/* NULL pad last char */
	*(c+size++)=0;		/* NULL pad last char */
#else
  unsigned char *c;
  unsigned int u;

  /* This code is sort of a synthesis of the previous two pieces of code, let's see if it works */
	c=(unsigned char *)ptr;
  if(size<60) {
#ifdef AUX /* RMG */
    fprintf(stdaux," padding in dlayer ");
#endif
    for(u=size,c+=size; u<60; u++)
      *c++=0;     /* NULL pad added chars */
    size=60;
  } /* end if */
#ifdef QAK
  if(size&0x01)
    *(c+size++)=0;  /* NULL pad last char */
#endif
#endif
#endif

  for(i=0; i<SQwait; i++);

  if((++OKpackets)>10) {
    SQwait-=10;
    OKpackets=0;
  } /* end if */

  if(SQwait<10)
    SQwait=10;

	ret=(*xmit)((DLAYER *)ptr,size);	/* send it out, pass back return code */
                                    /* xmit checks for size < 60 */
/*
*   automatic, immediate retry once
*/
	if(ret) {
    if(ret==(*xmit)((DLAYER *)ptr,size))
    nnerror(100);   /* post user error message */
  } /* end if */

  return(ret);
}   /* end dlayersend() */

/***************************************************************************/
/* dlayerinit
*  Do machine dependent initializations of whatever hardware we have
*  (happens to be ethernet board here ) 
*/
int dlayerinit(void )
{
	int my_var;

	BUG("dlayerinit");
#ifdef DEBUG    
	printf("etopen = %p\n",etopen);
#endif
	if(initbuffer() || !etopen)
		return(-10);

#ifdef QAK
printf("dlayerinit(): after initbuffer()\n");
printf("etopen=%p\n",etopen);
printf("nnmyaddr=%p\n",nnmyaddr);
printf("nnirq=%x, nnaddr=%x, nnioaddr=%x\n",(uint16)nnirq,(uint16)nnaddr,(uint16)nnioaddr);
getch();
#endif
/*
 * Call (*etopen) first to be sure any board/driver initializations are taken care of
 */
    my_var = ((*etopen)(nnmyaddr,nnirq,nnaddr,nnioaddr));
#ifdef QAK
printf("my_var is %d\n", my_var);
getch();
#endif
    return(my_var);
}

void dlayershut(void )
{
	if(etclose)
		(*etclose)();
}

/***************************************************************************/
/*  pcgetaddr
*   return results from indirect getaddr call.
*   This is a pc-specific request for the 48-bit address which was added
*   so that the user program could print the value.
*/
void pcgetaddr(char *s,int x,int y)
{
	if(getaddr)
		(*getaddr)(s,x,y);
}
