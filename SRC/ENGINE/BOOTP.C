/*
 *	 BOOTP
 *	 Bootp Routines
 *
 ****************************************************************************
 *																			*
 *	  part of:																*
 *	  TCP/IP kernel for NCSA Telnet									   		*
 *	  by Tim Krauskopf														*
 *																		  	*
 *	  National Center for Supercomputing Applications					 	*
 *	  152 Computing Applications Building								 	*
 *	  605 E. Springfield Ave.											 	*
 *	  Champaign, IL  61820													*
 *																		  	*
 ****************************************************************************
 */

#define REALTIME
/*#define DEBUG */       /* for debug prints */
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif

#include "protocol.h"
#include "data.h"
#include "bootp.h"
#include "windat.h"
#include "hostform.h"
#include "data.h"
#include "defines.h"
#include "externs.h"

/*
 *	Bootp routines : from the Clarkson 2.2 version of NCSA Telnet.
 *	Thanks to Brad Clements for implementing this!
 *
 * bootp routines - These routines are based on the stanford/clarkson
 * bootp code. Originally developed at Stanford University.
 *
 * Bootp is a UDP based protocol that determines the clients IP address and
 * gateway information etc.
 */

static struct bootp	bootpacket;
static u_long	bootp_xid;
extern int foundbreak;
static int bootp_udpidx;

/* Local functions */
static int sendbootp(void );
static void bootp_init(void );
static int parse_bootpacket(struct bootp *bp);

/* sends a bootp broadcast packet */
/* this routine does not do the initial setup of the bootp packet */

static int sendbootp()
{
    return(netusend(broadip,IPPORT_BOOTPS,IPPORT_BOOTPC, (unsigned char *)&bootpacket, (int)sizeof(struct bootp), bootp_udpidx));
}

/* initialize the bootp packet */
static void bootp_init()
{
  bootp_udpidx=newudp();
  bootp_xid=time(NULL);   /* get a unique transaction ID */
	memset((char *) &bootpacket,0,sizeof(bootpacket));
	bootpacket.bp_op=BOOTREQUEST;
	bootpacket.bp_htype=1;	 /* hardware type 1 is ethernet. This should be made more robust. */
	bootpacket.bp_hlen=sizeof(nnmyaddr);
	bootpacket.bp_xid=bootp_xid;
	bootpacket.bp_secs=1;
	memcpy(bootpacket.bp_chaddr, nnmyaddr, sizeof(nnmyaddr));
}

/* parse an incoming bootp packet */
static int parse_bootpacket(struct bootp *bp)
{
	int	 x,items,len;
	unsigned char *c;
	char message[80],*cp;
	int gateway=0,nameserver=0;
	struct machinfo *sp;
	extern struct config Scon;
	extern struct machinfo *Sns;

	netsetip(&bp->bp_yiaddr.addr[0]);			 /* set my ip address */
	movebytes(Scon.myipnum,&bp->bp_yiaddr,4);
#ifndef GO
printf("\nmyIP: %d.%d.%d.%d\n",bp->bp_yiaddr.addr[0],bp->bp_yiaddr.addr[1],
                               bp->bp_yiaddr.addr[2],bp->bp_yiaddr.addr[3]);
#endif
	if(comparen(bp->bp_vend,VM_RFC1048,4)) {
#ifndef GO
printf("VM_RFC1048\n");
#endif
		c=bp->bp_vend+4;
		while((*c!=255)&&((c-bp->bp_vend)<64)) {
			switch(*c) {
				case 0:		/* nop pad */
					c++;
					break;

				case 1:		/* subnet mask */
					len=*(c+1);
					c+=2;
					memcpy(Scon.netmask,c, 4);
					netsetmask(Scon.netmask);
#ifndef GO
printf("mask: %d.%d.%d.%d\n",c[0],c[1],c[2],c[3]);
#endif
          c+=len;
					Scon.havemask=1;
					break;

				case 2:		/* time offset */
					c+=*(c+1)+2;
					break;

				case 3:		/* gateways	 */
					len=*(c+1);
					items=len/4;
					c+=2;
					for(x=0; x<items; x++) {
						sprintf(message,"%d.%d.%d.%d",*c, *(c+1), *(c+2), *(c+3));
#ifndef GO
printf("GW: %s\n",message);
#endif
            if(!(sp=Smadd(message))) {
							printf("Out of Memory Adding Gateway-Smadd()\n\r");
							return(-1);
						  }
						gateway++;
						sprintf(message,"BootP: Adding Gateway number %d IP %s\n\r",gateway,sp->hname);
						sp->gateway=(unsigned char)gateway;
						memcpy(sp->hostip,c,4);
						sp->mstat=HFILE;
						c+=4;
					  }
					break;

				case 4:		/* time servers */
				case 5:		/* IEN=116 name server */
					c+=*(c+1)+2;
					break;

				case 6:		/* domain name server */
					len=*(c+1);
					items=len/4;
					c+=2;
					for(x=0; x<items; x++) {
						sprintf(message,"%d.%d.%d.%d",*c, *(c+1), *(c+2), *(c+3));
#ifndef GO
printf("DNS: %s\n",message);
#endif
            if(!(sp=Smadd(message))) {
							printf("Out of Memory Adding Nameserver-Smadd()\n\r");
							return(-1);
						  }
						nameserver++;
						sp->nameserv=(unsigned char)nameserver;
						memcpy(sp->hostip,c,4);
						sp->mstat=HFILE;
						if(!Sns)
							Sns=sp;
						c+=4;
					  }
					Scon.nstype=1;
					break;
				
				case 7:		/* log server */
				case 8:		/* cookie server */
				case 9:		/* lpr server */
				case 10:		/* impress server */
				case 11:		/* rlp server */
					c+=*(c+1)+2;
					break;
	
				case 12:		/* client host name	*/
					len=*(c+1);
					strncpy(message,c+2, len);
					message[len]=0;
#ifndef GO
printf("myName: %s\n",message);
#endif
          if(!(sp=Smadd(message))) {
						printf("Out of Memory Adding client name-Smadd()\n\r");
						return(-1);
					  }
					if(!strlen(Scon.me)) 
						strncpy(Scon.me,sp->hname,31);
					Scon.me[31]=0;
					if(cp=strchr(sp->hname,'.')) {	 /* assume fully qualified name if a . is in hostname */
#ifdef LATER
						if(!Scon.domainpath) { /* if no domain set yet */
							message[0]=0;
							while(cp) {
								strcat(message,",");
								strcat(message,cp+1);
								cp=strchr(cp+1,'.');
							  }
							Scon.domainpath=malloc(strlen(message)+1);
							strcpy(Scon.domainpath, message);
							removejunk(Scon.domainpath);
						  }
#endif
					  }
					c+=len+2;
					break;

				case 255:
					break;

				default:
					c+=*(c+1)+2;
					break;						
			  }				/* end switch */
		  }					/* end while	*/
	  }						/* end if comparen */
	if(!gateway) {			/* if none were in the rfc1048 vend packet, add the default gateway as an entry */
#ifndef GO
printf("!RFC1048\n",Scon.myipnum);
#endif
    c=bp->bp_giaddr.addr;
		sprintf(message,"%d.%d.%d.%d",*c, *(c+1), *(c+2), *(c+3));
		if(!(sp=Smadd(message))) {
			printf("Out of Memory Adding Gateway-Smadd()\n\r");
			return(-1);
		  }
		gateway++;
		sp->gateway=(unsigned char)gateway;
		memcpy(sp->hostip,c,4);
		sp->mstat=HFILE;
	  }
	return(0);
}

/*
*   main processing of bootp lookup request calls 
*	    sendbootp to send a bootp request,
*  sets up the udp listen port etc, handles retries
*/
int bootp(void )
{
	int x,y,delay;
	time_t start_time;
	union {
		char junk_buff[1506];
		struct bootp bootpacket;
	} udp_data;
	unsigned char *ea;
	struct bootp *bp;
	static unsigned char myip[]={0,0,0,0};

BUG("bootp()");
	time(&start_time);
	netsetip(myip);
	bootp_init();
BUG("after bootp_init");
	bp=&udp_data.bootpacket;
	ea=(unsigned char *) nnmyaddr;
  while(neturead(udp_data.junk_buff,bootp_udpidx,1506)!=-1);  /* should only go around once */
	for(x=0; x<BOOTP_RETRIES; x++) {
    netulisten(IPPORT_BOOTPC,bootp_udpidx);
BUG("sending bootp");
		if(y=sendbootp()) {				/* do some error processing */
      printf("\n\rError %d from sendbootp\n\r",y);
      return(-1);
    }
BUG("bootp sent");
		start_time=time(NULL);
		delay=((rand() % 10)+1);
		while((time(NULL)-start_time)<(long) delay) {
			if(!demux(1))
				continue;		 /* process all packets */
      if(neturead(udp_data.junk_buff,bootp_udpidx,1506) ==-1)
				continue;
			delay=0;
			break;
        }
BUG("done waiting");
		if(delay)
			continue;		/* time ran out and got nothing */
		if(((bp->bp_xid)==bootp_xid) && ((bp->bp_op)==BOOTREPLY) &&
		  comparen(bp->bp_chaddr,nnmyaddr,sizeof(nnmyaddr)))
			break;			 /* got a valid reply */
    }
	if(x==BOOTP_RETRIES) {		/* do some error processing */
		printf("\nBOOTP Timeout. No Response from BOOTP server\n\r");
		return(-1);
    }
BUG("About to parser bootp packet");
	if(parse_bootpacket(bp))
		return(-1);
	return(0);
}

