/*
 * National Center for Supercomputing Applications (NCSA) Telnet source code
 *
 * February 10, 1989
 * (C) Copyright 1989 Planning Research Corporation
 *
 * Permission is granted to any individual or institution to use, copy,
 * modify, or redistribute this software and its documentation provided this
 * notice and the copyright notices are retained.  This software may not be
 * distributed for profit, either in original form or in derivative works.
 * Planning Research Corporation makes no representations about the
 * suitability of this software for any purpose.
 *
 * PLANNING RESEARCH CORPORATION GIVES NO WARRANTY, EITHER EXPRESS OR IMPLIED,
 * FOR THE PROGRAM AND/OR DOCUMENTATION PROVIDED, INCLUDING, WITHOUT
 * LIMITATION, WARRANTY OF MERCHANTABILITY AND WARRANTY OF FITNESS FOR A
 * PARTICULAR PURPOSE.
 */

/*
 * Revision History:
 *
 * Date			Initials		Comment
 * ----			--------		-------
 * 02-10-89		JGN@PRC			Initial version.
 * 03-04-89 	TKK@NCSA		KIP code added.
 * 10-20-90		QAK@NCSA		Twiddled for version 2.3
 *
 * Initials		Name			Organization
 * --------		----			------------
 * JGN@PRC		Jim Noble		Planning Research Corporation
 * TKK@NCSA 	Tim Krauskopf	National Center for Supercomputing Applications
 * QAK@NCSA		Quincey Koziol	National Center for Supercomputing Applications
 *
*/

#define KIPPER
#define LTALK_MASTER

#if defined(MSC) && !defined(__TURBOC__)
#define movmem(from,to,len)	memmove(to,from,len)
#endif

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MSC
#ifdef __TURBOC__
#include <mem.h>
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#include "protocol.h"
#include "data.h"
#include "ltalk.h"
#ifdef KIPPER
#include "kip.h"
#endif
#include "externs.h"

struct at_vector {
	char name[9];
	unsigned char	vers_minor,
		vers_major,
		driver_type,
		reserved[4];
};

extern unsigned char rstat;		/* last status from read */
extern char *buforg;	/* pointer to beginning of buffer space */
extern char *bufend;	/* pointer to end of buffer space (like EOT mark) */
extern char *bufread;	/* pointer to packet program is working on */
extern char *bufpt;		/* pointer to space for next packet */
extern int   buflim;	/* number of useable bytes in buffer space */
extern int   bufbig;	/* number of bytes in use in buffer space */

extern listen1();		/* ASM part of first-half DDP socket listener */

unsigned listen_ds;		/* Allows the ASM part of the socket listeners to */
						/* determine the data segment of the C part of the */
						/* socket listeners */

unsigned int LTalk_vector;	/* Interrupt vector to use to make AppleTalk driver calls */

static char ltinit=0,		/* has this driver been initialized yet? */
		KIPzone[32]={"*"},	/* the zone to send KIP queries to */
		*entity;			/* storage for entities to pass to driver */
#ifdef KIPPER
static IPGP	*gatedata;		/* configuration record to/from KIP server */
#endif
static BDSElement *bds;		/* buffer list for ATP request */
static ATPParams *atp_pb;	/* ATP request parameter block */
static NBPTabEntry mynbp;	/* storage for my own MBP registration */
static NBPParams *nbp_pb;	/* NBP parameter block */
extern int nnkip;			/* are we running KIP? */

static AddrBlk myaddr;		/* my network, node, socket */
static ATARPKT arpbuffer;	/* alternate format for arps req'd for LocalTalk */

#define SHOW_MESSAGE		1		/* Display LTalk_call error messages */
#define HIDE_MESSAGE		0		/* Don't display LTalk_call error messages */
#define LTIP				22		/* DDP protocol type for IP */
#define LTARP				23		/* DDP protocol type for ARP */
#define IPSock				72		/* DDP socket for LocalTalk */

#define MAX_ADDR_TRIES		100		/* Maximum tries to get the addres */

static void	LTalk_error (char *caller,int status);		/* Displays AppleTalk driver error messages */
static int LTalk_call(InfoParams *pb,char *caller,int display);	/* Makes AppleTalk driver calls */

/*
 * LTalk_error
 *
 * Display an AppleTalk driver error message.
 */
static void LTalk_error(char *caller,int status)
{
	fputs(caller,stderr);
	fputs(":  ", stderr);

	switch(status) {	/* Provide text for the errors that we might get */
		case ATNOTINITIALIZED:
			fputs("AppleTalk driver not initialized.",stderr);
			break;

		case BAD_PARAMETER:
			fputs("Illegal parameter or parameters.",stderr);
			break;

		case BAD_SYNC_CALL:
			fputs("Synchronous command was called at a time when the driver cannot handle it.",stderr);
			break;

		case CALLNOTSUPPORTED:
			fputs("Requested call not supported.",stderr);
			break;

		case DDP_CANCELLED:
			fputs("DDP transaction cancelled.",stderr);
			break;

		case DDP_LENERR:
			fputs("DDP data too long.",stderr);
			break;

		case DDP_SKTERR:
			fputs("Used unopen socket.",stderr);
			break;

		case HARDWARE_ERROR:
			fputs("Unrecoverable hardware error.",stderr);
			break;

		case MAXCOLISERR:
			fputs("Too many collisions detected.",stderr);
			break;

		case MAXDEFERERR:
			fputs("32 deferrals encountered (ALAP).",stderr);
			break;

		case MEMORY_CORRUPTED:
			fputs("Internal memory pool corrupted.  Reload driver.",stderr);
			break;

		case NOBRDGERR:
			fputs("No bridge found.",stderr);
			break;

		case NO_MEM_ERROR:
			fputs("Not enough memory to perform requested operation.",stderr);
			break;

		case SOFTWARE_ERROR:
			fputs("Internal software error.  Reinitialize driver.",stderr);
			break;

		case STACKERROR:
			fputs("Too many levels of nesting to complete call.",stderr);
			break;

		default:	/* Handle anything else that might come up */
			fprintf(stderr,"Unknown AppleTalk driver error %d.",status);
	  }	/* end switch */
	fputs("\n",stderr);
}	/* end LTalk_error() */

/*
 * LTalk_call
 *
 * Make an AppleTalk driver call.  Return the driver error code.
 */
static int LTalk_call(InfoParams *pb,char *caller,int display)
{
	union REGS	rgisters;
	struct SREGS	segments;
	int		status;

	rgisters.x.bx=FP_OFF(pb);
	segments.ds=FP_SEG(pb);
	int86x(LTalk_vector,&rgisters,&rgisters,&segments);
	status=pb->atd_status;
	if(status!=NOERR && display)
		LTalk_error(caller,status);
	return status;
}	/* end LTalk_call() */

/*
 * LTopen
 *
 * Check if AppleTalk driver is installed, initialize driver, open DDP socket
 * for IP packets, fill in network address parameter ('s') and return.  The
 * return value is 0 if successful and a non-zero driver error code if
 * something failed.  On entry the 'irq' parameter contains the 'interrupt=xx'
 * value from the configuration file and specifies the interrupt to use when
 * making driver calls.  The 'addr' and 'ioaddr' parameters are unused by this
 * routine.
 */
int CDECL LTopen(AddrBlk *s,unsigned int irq,unsigned int addr,unsigned int ioaddr)
{
	char		buffr[100];
	char		buffr2[100];
	DDPParams	ddp_pb;
	InfoParams	info_pb;
	struct SREGS segregs;
	int			status;
	int 		count;		/* counter for the number of tries to get the address */

	if(ltinit)								/* we are initialized already */
		return(0);

	/* Interrupt vector to use to make AppleTalk driver calls */
	LTalk_vector=irq;

    if(strncmp("AppleTalk",(*((char **)((unsigned long)(LTalk_vector*4))))-16,9))
	{
		status=ATNOTINITIALIZED;
		LTalk_error("LTopen",status);
		fputs("...AppleTalk driver not installed\n",stderr);
		return(status);
	}	/* end if */

	/* Initialize AppleTalk driver */
	info_pb.atd_command=ATInit;
	info_pb.atd_compfun=(CPTR) 0;
	info_pb.inf_nodeid=0;

	status=LTalk_call(&info_pb,"LTopen",SHOW_MESSAGE);
	if(status!=NOERR)
		return(status);

	ltinit=1;								/* flag we are open */

	/*
	 * Store current DS register in listen_ds so that listen1 and listen2
	 * (asm) can set DS register before calling listen1_c and listen2_c (C).
	 */
	segread(&segregs);
	listen_ds=segregs.ds;

	/* Open a DDP socket */
	ddp_pb.atd_command=DDPOpenSocket;
	ddp_pb.atd_compfun=(CPTR) 0;
	ddp_pb.ddp_socket=IPSock;		/* we use 72 like everyone else */
	ddp_pb.ddp_bptr=(DPTR)listen1;	/* interrupt listener */

    status=LTalk_call((InfoParams *)&ddp_pb,"LTopen",SHOW_MESSAGE);
	if(status!=NOERR)
		return(status);

	myaddr.socket=ddp_pb.ddp_socket;	/* keep a copy, always=72 */

/*
* Get the AppleTalk network and node numbers
* Store a copy of the values in our structure for use by other routines.
*/
	info_pb.atd_command=ATGetNetInfo;
	info_pb.atd_status=-1;
	info_pb.atd_compfun=NULL;
	info_pb.inf_bptr=NULL;
	info_pb.inf_buffsize=0;

	/* repeat this a few times if the network returns 0 */
	for(count=0; count<MAX_ADDR_TRIES; count++) {
		status=LTalk_call(&info_pb,"LTgetaddr",SHOW_MESSAGE);
		if(status!=NOERR) {
			myaddr.network=0;
			myaddr.nodeid=0;
			status=ATNOTINITIALIZED;
			return(status);
		  }	/* end if */
		else {
			myaddr.network=info_pb.inf_network;
			myaddr.nodeid=info_pb.inf_nodeid;
		  }	/* end else */
		if(myaddr.network!=0)
			break;
	  }	/* end for */
	if(myaddr.network==0) {
		sprintf(buffr2,"Error getting network number");
        printf(buffr2);
/*        n_puts(buffr2); */
		return(-1);
	  }	/* end if */

	sprintf(buffr2,"Network: %u,  Node: %u",intswap(myaddr.network),myaddr.nodeid);
    printf(buffr2);
/*    n_puts(buffr2);*/

	broadaddr[0]=0;
	broadaddr[1]=0;
	broadaddr[2]=0xff;
	broadaddr[3]=0x48;
	bseed[0]=0;
	bseed[1]=0;
	bseed[2]=0xff;
	bseed[3]=0x48;


#ifdef KIPPER
	if(nndto<1)			/* don't bother with KIP */
		return(0);
	if(nndto<2)			/* minimum value */
		nndto=2;

	if(NULL==(entity=malloc(100)))
		return(-1);
	if(NULL==(nbp_pb=malloc(sizeof(NBPParams))))
		return(-1);
	if(NULL==(gatedata=malloc(sizeof(IPGP))))
		return(-1);
	if(NULL==(bds=malloc(sizeof(BDSElement))))
		return(-1);
	if(NULL==(atp_pb=malloc(sizeof(ATPParams))))
		return(-1);

	nnkip=1;

/*
*  Look for a KIP gateway with NBP
*/
	sprintf(entity,"\001=\011IPGATEWAY%c%s",strlen(KIPzone),KIPzone);
	sprintf(buffr,"%c%c%c%c",0,0,0,0);  /* set the buffer to 0 */

	nbp_pb->atd_command=NBPLookup;
	nbp_pb->atd_compfun=NULL;
	nbp_pb->nbp_toget=1;
	nbp_pb->nbp_bptr=buffr;
	nbp_pb->nbp_buffsize=100;
	nbp_pb->nbp_interval=(unsigned char)(nndto/2);
	nbp_pb->nbp_retry=3;
	nbp_pb->nbp_entptr=entity;

	status=LTalk_call((InfoParams *)nbp_pb,"NBPLook",HIDE_MESSAGE);
	if(status!=NOERR)
		return(0);		/* cannot find KIP is not an error */
	sprintf(buffr2,"IPGATEWAY at Net %u.%u Node %u Skt %u\n",
		(int)(((unsigned char *)buffr)[0]),
		(int)(((unsigned char *)buffr)[1]),
		(int)(((unsigned char *)buffr)[2]),
		(int)(((unsigned char *)buffr)[3]));
    printf(buffr2);
/*    n_puts(buffr2); */

/*
*  get the parameters from the KIP server.
*  This obtains a dynamically assigned IP address if it can get one.
*/
	atp_pb->atd_command=ATPSndRequest;
	atp_pb->atd_compfun=NULL;

#ifdef OLD_WAY
	gatedata->opcode=longswap(1L);	/* Motorola byte order */
#else
	gatedata->opcode=0x1000000L;  /* Motorola byte order */
#endif
	gatedata->ipaddress=0;

	movmem(buffr,&atp_pb->atp_addr,sizeof(AddrBlk));
	bds->bds_bptr=(DPTR)gatedata;
	bds->bds_buffsize=sizeof(IPGP);
	atp_pb->atp_bdsptr=bds;
	atp_pb->atp_bptr=(DPTR)gatedata;
	atp_pb->atp_buffsize=sizeof(IPGP);
	atp_pb->atp_flags=32;
	atp_pb->atp_interval=5;
	atp_pb->atp_retry=5;
	atp_pb->atp_bdsbuffs=1;

	status=LTalk_call((InfoParams *)atp_pb,"ATPget",SHOW_MESSAGE);
	if(status!=NOERR)
		return(status);

/*	if(longswap(gatedata->opcode)>0) */    /* if we didn't get an error */
		netsetip((unsigned char *)&gatedata->ipaddress);	/* set the ipaddress */
/*
*  NBP register our IP number.
*  We aren't allowed to register outside of our zone, so we don't
*  use KIPzone in the entity.
*/
	nbp_pb->atd_command=NBPRegister;
	nbp_pb->atd_compfun=NULL;
	nbp_pb->nbp_bptr=(DPTR)&mynbp;
	nbp_pb->nbp_interval=1;
	nbp_pb->nbp_retry=3;

	mynbp.tab_nxtentry=NULL;
	mynbp.tab_tuple.tup_address.socket=72;
	sprintf(buffr,"%d.%d.%d.%d",
		(unsigned char)*((char *)&gatedata->ipaddress+0),
		(unsigned char)*((char *)&gatedata->ipaddress+1),
		(unsigned char)*((char *)&gatedata->ipaddress+2),
		(unsigned char)*((char *)&gatedata->ipaddress+3));

	sprintf(mynbp.tab_tuple.tup_entname,"%c%s\011IPADDRESS\001*",strlen(buffr),buffr);

	status=LTalk_call((InfoParams *)nbp_pb,"NBPRegister",SHOW_MESSAGE);
	if(status!=NOERR)
		return(status);
#endif
	return (NOERR);
}	/* end LTopen() */

/*
 * LTgetaddr
 *
 * Return the AppleTalk network address in parameter 's'.  Parameters 'addr'
 * and 'ioaddr' are unused by this routine.
 */
int CDECL LTgetaddr(AddrBlk *s,unsigned int addr,unsigned int ioaddr)
{
    int
#ifdef NEW_WAY
        count=10,
#endif
		open_status;

	s->network=0;
	s->nodeid=0;
	s->socket=0;

#ifdef NEW_WAY
	while(count--) {
		open_status=LTopen(s,LTalk_vector,addr,ioaddr);	/*  Make sure we are initialized */
		if(open_status<0)
			LTclose();
		else
			break;
	  }	/* end while */
#else
    open_status=LTopen(s,LTalk_vector,addr,ioaddr);
#endif

	if(open_status<0)
        return(1);

	s->network=myaddr.network;		/* copy from stored values */
	s->nodeid=myaddr.nodeid;
	s->socket=myaddr.socket;
    return(0);
}	/* end LTgetaddr() */

/*
 * LTxmit
 *
 * Transmit IP packet at *ptr on DDP socket and return AppleTalk driver error
 * code.  The return value is 0 for success and non-zero for failure.
 * Ethernet packet types must be converted to DDP types.  The DLAYER header is
 * not transmitted.
 */
int CDECL LTxmit(DLAYER *ptr,unsigned int size)
{
	DDPParams		ddp_pb;
	int			status;
/*
*  First set up fields for xmit, if any of them have to change
*  from the initial values, we'll change them.
*/
	/* Transmit data (skipping DLAYER header) */
	ddp_pb.atd_command=DDPWrite;
	ddp_pb.atd_compfun=(CPTR) 0;
	ddp_pb.ddp_addr=*((AddrBlk *)(ptr->dest));
	ddp_pb.ddp_socket=IPSock;
	ddp_pb.ddp_bptr=(char *)ptr+sizeof(DLAYER);
	ddp_pb.ddp_buffsize=size-sizeof(DLAYER);
	ddp_pb.ddp_chksum=0;


	/* Convert packet type from Ethernet types to DDP types */
	if(ptr->type==EIP)
		ddp_pb.ddp_type=LTIP;
	else if(ptr->type==EARP) {
		ARPKT *aptr;

		ddp_pb.ddp_type=LTARP;
		aptr=(ARPKT *)ptr;
#ifdef OLD_WAY
		arpbuffer.hrd=intswap(3);		/* appletalk == 3 */
#else
		arpbuffer.hrd=0x0300;			/* appletalk == 3 */
#endif
		arpbuffer.hln=4;			/* Ltalk length */
		arpbuffer.pln=4;			/* IP length */
		arpbuffer.pro=aptr->pro;		/* copy in command fields */
		arpbuffer.op=aptr->op;

		movmem(aptr->sha,arpbuffer.sha,4);	/* re-align ARP fields */
		movmem(aptr->spa,arpbuffer.spa,4);
		movmem(aptr->tha,arpbuffer.tha,4);
		movmem(aptr->tpa,arpbuffer.tpa,4);

		/* install new ARP format */
	  	ddp_pb.ddp_bptr=((char *)&arpbuffer)+sizeof(ATdlayer);
		ddp_pb.ddp_buffsize=sizeof(ATARPKT)-sizeof(ATdlayer);
	  }	/* end if */
	status=LTalk_call((InfoParams *)&ddp_pb,"LTxmit",SHOW_MESSAGE);
	return(status);
}	/* end LTxmit() */

#ifdef KIPPER
/******************************************************************/
/* KIParp
*  send the special KIParp request and return result.
*  1=OK, less than zero=error.
*/
int KIParp(unsigned char ipnum[4],AddrBlk *addrloc)
{
	char buffr[100];
	int status;

	nbp_pb->atd_command=NBPLookup;
	nbp_pb->atd_compfun=NULL;
	nbp_pb->nbp_toget=1;
	nbp_pb->nbp_bptr=buffr;
	nbp_pb->nbp_buffsize=100;
	nbp_pb->nbp_interval=1;
	nbp_pb->nbp_retry=2;
	nbp_pb->nbp_entptr=entity;

	sprintf(buffr,"%u.%u.%u.%u",ipnum[0],ipnum[1],ipnum[2],ipnum[3]);
	sprintf(entity,"%c%s\011IPADDRESS%c%s",strlen(buffr),buffr,strlen(KIPzone),KIPzone);

	status=LTalk_call((InfoParams *)nbp_pb,"NBPFind",SHOW_MESSAGE);
	if(status!=NOERR)
		return(status);

	movmem(buffr,addrloc,4);		/* give back address */
	return(1);
}	/* end KIParp() */

#ifdef OLD_WAY
/******************************************************************/
/* KIPsetzone
*  set the zone string for KIP lookups.
*/
void KIPsetzone(char *s)
{
	strncpy(KIPzone,s,32);
	KIPzone[31]='\0';
}	/* end KIPsetzone() */
#endif
#endif

/*
 * LTrecv
 *
 * Change DDP packet type of next packet in the queue to Ethernet packet type.
 */
void CDECL LTrecv(void )
{
	DLAYER		*dlptr;

	if(bufbig>0) {		/* Make DDP packet types look like Ethernet types */
		dlptr=(DLAYER *) (bufread+sizeof(int));
		if(dlptr->type==LTIP)
			dlptr->type=EIP;
		else if(dlptr->type==LTARP) {
/*
*  yes, I realize that this is an in-place modification of
*  the packet buffer which lengthens it.  It is tricky, but it
*  is planned that way.
*/
			ARPKT *eptr;
			ATARPKT *atptr;

			dlptr->type=EARP;
			eptr=(ARPKT *)dlptr;
			atptr=(ATARPKT *)dlptr;
#ifdef OLD_WAY
			eptr->hrd=intswap(1);	/* We are faking Ethernet */
#else
			eptr->hrd=0x0100;		/* We are faking Ethernet */
#endif
			eptr->hln=6;		/* Ethernet length */
			eptr->pln=4;		/* IP length */
			eptr->pro=atptr->pro;	/* copy in command fields */
			eptr->op=atptr->op;
			movmem(atptr->tpa,eptr->tpa,4);
			movmem(atptr->tha,eptr->tha,4);
			movmem(atptr->spa,eptr->spa,4);
			movmem(atptr->sha,eptr->sha,4);	/* re-align ARP fields */
			eptr->tha[4]=0;
			eptr->tha[5]=0;
			eptr->sha[4]=0;
			eptr->sha[5]=0;
		  }	/* end if */
	  }	/* end if */
}	/* end LTrecv() */

/*
 * LTupdate
 *
 * Release next packet in the queue.
 */
void CDECL LTupdate(void )
{
	int	pkt_size;

	/* Adjust pointers to deallocate packet in buffer */
	pkt_size=*((int *)bufread)+sizeof(int);

/* printf("s: %d, r: %ld, q: %d\n",pkt_size,(long)bufread,bufbig); */

	bufread+=pkt_size;
	if(bufread>=bufend)
		bufread=buforg;
	bufbig -= pkt_size;
}	/* end LTupdate() */

/*
 * LTclose
 *
 * Close the DDP socket for IP packets.
 */
int CDECL LTclose(void)
{
	DDPParams		ddp_pb;

	if(nnkip) {
		nbp_pb->atd_command=NBPRemove;
		nbp_pb->atd_compfun=NULL;
		nbp_pb->nbp_entptr=mynbp.tab_tuple.tup_entname;
		nbp_pb->nbp_interval=1;
		nbp_pb->nbp_retry=3;

		LTalk_call((InfoParams *)nbp_pb,"NBPRemove",SHOW_MESSAGE);
	  }	/* end if */

	/* Close the DDP socket */
	ddp_pb.atd_command=DDPCloseSocket;
	ddp_pb.atd_compfun=(CPTR) 0;
	ddp_pb.ddp_socket=IPSock;

	LTalk_call((InfoParams *)&ddp_pb,"LTclose",SHOW_MESSAGE);
    return(0);
}	/* end LTclose() */

/*
 * First-half DDP socket listener (C part)
 *
 * The first-half of the listener is called with the following inputs:
 *
 *		 socket		Socket
 *		 datalen		Length of data
 *		 header		Pointer to (LAP & DDP) header
 *
 * The first-half of the listener must decide whether to keep or discard the
 * data.  If the packet is to be discarded, the first-half listener must return
 * with *bufflen equal to zero; in this case, the second-half of the listener
 * will not be called.
 *
 * If the data is to be retained, the first-half listener must return with
 * the following values:
 *
 *		 *bptr Pointer to buffer for packet data
 *		 *bufflen Size of the buffer at *bptr
 */

/* #pragma check_stack (off) */

void CDECL listen1_c(unsigned char socket,unsigned int datalen,unsigned char header[],unsigned int *bufflen,unsigned char *bptr[])
{
	static unsigned char	DDP_type,nethi,netlo,sock,nodeid;
	static DLAYER			*dlptr;
	static int				*intptr;

	*bufflen=0;					/* Reject packet initially */
	if(socket==IPSock) {		/* Is it for our socket? */
						/* Yes... */
		if(header[2]==1) {			/* Get the DDP Protocol Type field */
			DDP_type=header[7];		/* Short format DDP header */
			nethi=(unsigned char)*(((char *)(unsigned long)myaddr.network)+1);
			netlo=(unsigned char)*((char *)(unsigned long)myaddr.network);
			nodeid=myaddr.nodeid;
			sock=header[6];
		  }	/* end if */
		else if(header[2]==2) {
			DDP_type=header[15];	   	/* Long format DDP header */
			nethi=header[9];
			netlo=header[10];
			nodeid=header[12];
			sock=header[14];
		  }	/* end if */
		else
			DDP_type=0;			/* Not DDP packet; reject it */
		if(DDP_type==LTIP || DDP_type==LTARP) { /* Is it an IP or ARP packet? */
						/* Yes... */
			unsigned char *p;

			datalen+=4;			/* leave room for play */
			if(bufbig<=buflim) {		/* Is there room in the buffer? */
					/* Yes... */
				*bufflen=datalen;	/* Accept whole packet */
				if(bufend<=bufpt)
					bufpt=buforg;
				intptr=(int *)bufpt;			  /* Put packet at bufpt */
				*intptr=datalen+sizeof(DLAYER); /* Size of packet plus
														dummy DLAYER header */
				dlptr=(DLAYER *)++intptr;
				p=(char *)dlptr;
				dlptr->type=DDP_type;		  	/* Remember packet type */
				*bptr=(char *)++dlptr;		/* Put IP packet after dummy
												   DLAYER header */
				/*  fill in addresses */
				*p++=(unsigned char)(myaddr.network&0xff);
				*p++=(unsigned char)(myaddr.network>>8);
				*p++=myaddr.nodeid;
				*p++=myaddr.socket;
				*p++=0;
				*p++=0;
				*p++=netlo;		/* fake src address */
				*p++=nethi;
				*p++=nodeid;
				*p++=sock;
				*p++=0;
				*p++=0;
			  }	/* end if */
		  }	/* end if */
	  }	/* end if */
}	/* end listen1_c() */

/*
 * Second-half DDP socket listener (C part)
 *
 * The second-half of the listener is called (through a FAR CALL) with
 * the following inputs:
 *
 *		 bptr		Pointer to buffer containing data
 *		 bufflen	Number of bytes of data in buffer at bptr
 *
 * The second-half listener may enable interrupts, and should exit with a FAR
 * RETURN.
 */
void CDECL listen2_c(unsigned int bufflen,DLAYER *bptr)
{
	/* Calculate total size of new packet, leave 4 for good measure */

	bufflen+=sizeof(DLAYER)+sizeof(int)+4;
	bufpt=&bufpt[bufflen];	/* Add total packet size to bufpt and bufbig */
	bufbig+=bufflen;
}	/* end listen2_c() */
