/*
*	PROTINIT.C
*
*	Packet template initialization routines
*
****************************************************************************
*                                                                          *
*      part of:                                                            *
*      TCP/UDP/ICMP/IP Network kernel for NCSA Telnet                      *
*      by Tim Krauskopf                                                    *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
*      This program is in the public domain.                               *
*                                                                          *
****************************************************************************
*   'protinit' initializes packets to make them ready for transmission.
*   For many purposes, pre-initialized packets are created for use by the
*   protocol routines, especially to save time creating packets for
*   transmit.
*
*	Important note :  Assumes that the hardware has been initialized and has
* set all the useful addresses such as the hardware addresses.
*
*   As this is a convenient place for it, this file contains many of the
*   data declarations for packets which are mostly static (pre-allocated).
*	Revision history:
****************************************************************************
*
*	10/87  Initial source release, Tim Krauskopf
*	5/88	clean up for 2.3 release, JKM	
*
*/

/*
 *	Includes
 */

#include <stdio.h>
#include <stdlib.h>
#if defined(MSC)
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#include "protocol.h"
#include "data.h"
#include "externs.h"
#include "defines.h"

static void setupwindow(struct window *,unsigned int );

/************************************************************************/
/*
 *	protinit () 
 *
 *	Calls all the other packet initialization keep this order as some packet
 * inits require lower layers already be initialized.
 *
*/
void protinit(void)
{
	etherinit();									/* dlayer packets */
	arpinit();  									/* ARP packets */
  ipinit();                     /* ip packets */
	tcpinit();										/* tcp packets */
	udpinit();										/* udp packets */
}

/*************************************************************************/
/*
 *	neteventinit ()
 *
 *	Setup all the pointers for the event queue -- makes a circular list which
 * is required for error messages. ( called from Snetinit () )
 *
*/

void neteventinit (void)
{
	int i;

	for(i=0; i<NEVENTS; i++)
		nnq[i].next=i+1;
	nnq[NEVENTS-1].next=-1;
	nnefirst=0;
	nnelast=0;
	nnefree=1;
}

/*
 *	etherinit ()
 *
 *	Setup the ethernet headers ( dlayer ) -- this needs to be done first as it
 * is copied for the other headers 
 *
 */
void etherinit(void)
{
	movebytes(broadaddr,bseed,DADDLEN);
	movebytes(blankd.dest,broadaddr,DADDLEN);	/* some are broadcast */
	movebytes(blankd.me,nnmyaddr,DADDLEN);		/* always from me */
	blankd.type=EIP;				/* mostly IP packets */
}

/*************************************************************************/
/*
 *	arpinit ()
 *	Setup an arp packet -- also sets up an arp cache
*
*/
void arpinit(void)
{
	int i;

	movebytes(&arp.d,&blankd,sizeof(DLAYER));
	arp.d.type=EARP;				/* 0x0806 is ARP type */
	arp.hrd=intswap(HTYPE);			/*  Ether=1 */
	arp.pro=intswap(ARPPRO);			/* IP protocol=0x0800 */
	arp.hln=DADDLEN;					/* Ethernet hardware length */
	arp.pln=4;						/* IP length=4 */
	movebytes(arp.sha,nnmyaddr,DADDLEN);	/* sender's hardware addr */
	movebytes(arp.tha,broadaddr,DADDLEN);	/* target hardware addr */
	movebytes(arp.spa,nnipnum,4);		/* sender's IP addr */
/*
*  initialize the ARP cache to 0 time, none are gateways to start
*/
	for(i=0; i<CACHELEN; i++) {
		arpc[i].tm=0L;
		arpc[i].gate=0;
	  }
}

/*************************************************************************/
/*
 *	ipinit ()
 *
 *	initialize on packet to use for internet transmission -- most packets will
 * be tcp/udp, so they will be initialized at a different layer, but some
 * require a generic ip packet.
 *
 *	Also takes a guess at setting a netmask if it hasn't happened by now.
 *
*/
void ipinit(void)
{
	movebytes(&blankip.d,&blankd,sizeof(DLAYER));
	blankip.i.versionandhdrlen=0x45;		/* smallest header, version 4 */
	blankip.i.service=0;					/* normal service */
	blankip.i.tlen=576;						/* no data yet, maximum size */
	blankip.i.ident=0;
	blankip.i.frags=0;						/* not a fragment of a packet */
	blankip.i.ttl=100;						/* 100 seconds should be enough */
  blankip.i.protocol=PROTUDP;   /* default to UDP */
	blankip.i.check=0;						/* disable checksums for now */
	movebytes(blankip.i.ipsource,nnipnum,4);	/* my return address */
	movebytes(blankip.i.ipdest,broadip,4);		/* to ? */

/*
*  create a mask which can determine whether a machine is on the same wire
*  or not.  RFC950
*  Only set the mask if not previously set.
*  This mask may be replaced by a higher level request to set the subnet mask.
*/
	if(comparen(nnmask,"\0\0\0\0",4)) {			/* now blank */
		if(!(nnipnum[0]&0x80))					/* class A */
			netsetmask(nnamask);
		else 
			if((nnipnum[0]&0xC0)==0x80)		/* class B */
				netsetmask(nnbmask);
			else 
				if((nnipnum[0]&0xC0)==0xC0)		/* class C */
					netsetmask(nncmask);
	  }
}

/**************************************************************************/
/*
 *	udpinit ()
 *
 *  Setup udplist[0] for receive of udp packets
 *
 *  Now supports multiple UDP ports...  EEJ   6/26/92
 *
 *  Keep in mind that back in the good ole days, telnet only had supported
 *  listening to one UDP port, and only had one global structure to read the
 *  the damn thing in.  Now, there is an array of udp ports.  But in order
 *  to keep some semblence of compatibility between versions of code,
 *  udplist[0] is technically the same as ulist, the global variable from long ago.
 *
 */

void udpinit(void)
{
    int         i;

    for(i=0;i<NPORTS;i++)
        udplist[i]=NULL;

    newudp();

    /**
     ** The code below is commented out, because these initilization
     ** functions are performed by the newudp() call above.  Keep in mind
     ** that the DNR assumes we got the first structure in the UDP list
     **
     **/


#ifdef NOT
    udplist[DNRINDEX] = (struct uport *) malloc(sizeof(struct uport));
    udplist[DNRINDEX]->stale=1;
    udplist[DNRINDEX]->length=0;

    movebytes(&udplist[DNRINDEX]->udpout,&blankip,sizeof(DLAYER)+sizeof(IPLAYER));
    udplist[DNRINDEX]->udpout.i.protocol=PROTUDP;                 UDP type
    udplist[DNRINDEX]->tcps.z=0;
    udplist[DNRINDEX]->tcps.proto=PROTUDP;
    movebytes(udplist[DNRINDEX]->tcps.source,nnipnum,4);
#endif

}

/**************************************************************************/
/*
 *	tcpinit ()
 *
 *	setup for makeport ()
 *
*/
void tcpinit(void)
{
	int i;

    for(i=0; i<NPORTS; i++)
		portlist[i]=NULL;			/* no ports open yet */
}

/**************************************************************************/
/*
 *	makeport ()
 *
 *   This is the intialization for TCP based communication.  When a port
 *   needs to be created, this routine is called to do as much pre-initialization
 *   as possible to save overhead during operation.
 *
 *   This structure is created upon open of a port, either listening or
 *   wanting to send.
 *
 *   A TCP port, in this implementation, includes all of the state data for the
 *   port connection, a complete packet for the TCP transmission, and two
 *   queues, one each for sending and receiving.  The data associated with
 *   the queues is in struct window.
*/
int makeport(void)
{
	int i,j,retval;

	struct port *p,*q;
/*
*  Check to see if any other connection is done with its port buffer space.
*  Indicated by the connection state of SCLOSED
*/
	p=NULL;
	i=0;
    do {    /* search for a pre-initialized port to re-use */
		q=portlist[i];
        if(q!=NULL && (q->state==SCLOSED || (q->state==STWAIT && q->out.lasttime+WAITTIME<n_clicks())))
			p=q;
		retval=i++;					/* port # to return */
    } while(p==NULL && i<NPORTS);
#ifdef DEBUG
#ifdef TELBIN
    if(p)
        tprintf(console->vs,"Recycled old port.\r\n");
    else
        tprintf(console->vs,"Allocated new port.\r\n");
#endif
#endif /* DEBUG */
/*  
* None available pre-allocated, get a new one, about 8.5 K with a 4K windowsize
*/
	if(p==NULL) {
		if((p=(struct port *)malloc(sizeof(struct port)))==NULL) {
#ifdef TELBIN
            tprintf(console->vs,"memory allocation error.\r\n");
#endif
			nnerror(500);
			return(-1);				/* out of room for ports */
		  }	/* end if */
        for(i=0; portlist[i]!=NULL; i++) {
			if(i>=NPORTS) {
				nnerror(500);
				return(-1);				/* out of room for ports */
              } /* end if */
          } /* end for */
		portlist[i]=p;
		retval=i;
      } /* end if */
	if(p==NULL) {
		nnerror(505);
		return(-1);
	  }
    movebytes(&p->tcpout,&blankip,sizeof(DLAYER)+sizeof(IPLAYER));  /* static initialization */
	p->tcpout.i.tlen=0;
    p->tcpout.t.urgent=0;               /* no urgent data */
	p->tcpout.t.hlen=20<<2;				/* header length << 2 */
	p->tcps.z=0;
	p->tcps.proto=PROTTCP;
	movebytes(p->tcps.source,nnipnum,4);
	setupwindow(&p->in,WINDOWSIZE);		/* queuing parameters */
	setupwindow(&p->out,WINDOWSIZE);
    do {    /* search for an un-used port */
        i=(int)n_clicks();
        i|=2048;            /* make sure it is at least this large */
		i&=0x3fff;			/* at least this small */
#ifdef OLD_WAY
		for(j=0; j<NPORTS && (uint)i!=portlist[j]->in.port; j++);
#else
        for(j=0; j<NPORTS; j++)     /* don't check NULL ports */
            if(portlist[j]!=NULL && i==portlist[j]->in.port)
                break;
#endif
	  } while(j<NPORTS);
	if(nnfromport) {			/* allow the from port to be forced */
		i=nnfromport;
		nnfromport=0;			/* reset it so the next one will be random */
	  }
	p->in.port=i;
	p->tcpout.t.source=intswap(i);
	p->tcpout.t.seq=longswap(p->out.nxt);
	p->state=SCLOSED;
	p->credit=nncredit;
	p->sendsize=TSENDSIZE;
	p->rto=MINRTO;
	return(retval);
}

/*
 *	setupwindow ( w, wsize )
 *
 *	Configure information about a window *w*
 *
 */

static void setupwindow(struct window *w,unsigned int wsize)
{
	w->endbuf=w->where+wsize;
	w->base=w->endlim=w->where;
	w->contain=0;						/* nothing here yet */
    w->lasttime=n_clicks();
	w->size=wsize;
	w->push=0;
/*
*  base this on time of day clock, for uniqueness
*/
	w->ack=w->nxt=((w->lasttime<<12)&0x0fffffff);
}

