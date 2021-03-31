/*  
*	 ARP
*	 Hardware level routines, data link layer
*
*****************************************************************************
*																			*
*	  part of:																*
*	  TCP/IP kernel for NCSA Telnet											*
*	  by Tim Krauskopf														*
*																			*
*	  National Center for Supercomputing Applications						*
*	  152 Computing Applications Building									*
*	  605 E. Springfield Ave.												*
*	  Champaign, IL  61820													*
*																			*
*****************************************************************************
*/

/*
* Includes
*/
#include <stdio.h>
#include "protocol.h"
#include "data.h"
#include "externs.h"

static int rarp(void);
static int cacheupdate(uint8 *,uint8 *);
static int replyarp(uint8 *,uint8 *);

/************************************************************************/
/*
*   Address Resolution Protocol handling.  This can be looked at as
*   Ethernet-dependent, but the data structure can handle any ARP
*   hardware, with minor changes here.
*
*/
static int replyarp(uint8 *thardware,uint8 *tipnum)
{
	uint8 *pc;

	movebytes(arp.tha,thardware,DADDLEN);   /* who this goes to */
	movebytes(arp.tpa,tipnum,4);			/* requester's IP address */
	arp.op=intswap(ARPREP);					/* byte swapped reply opcode */
	movebytes(arp.d.dest,thardware,DADDLEN);	/* hardware place to send to */
    dlayersend((DLAYER *)&arp,sizeof(arp));
/*
*  check for conflicting IP number with your own
*/
	if(comparen(tipnum,nnipnum,4))	{	 /* we are in trouble */
		pc=neterrstring(-1);
		sprintf(pc,"Conflict with Ethernet hardware address: %2x:%2x:%2x:%2x:%2x:%2x",
		thardware[0],thardware[1],thardware[2],thardware[3],thardware[4],thardware[5]);
		netposterr(-1);
		netposterr(102);
		return(-3);
	  }
	return(0);		/* ARP ok */
}

/************************************************************************/
/*  reqarp
*	put out an ARP request packet, doesn't wait for response
*/
int reqarp(uint8 *tipnum)
{
#ifdef PKT_ONLY
#define NO_APPLETALK
#endif
#ifndef NO_APPLETALK
	if(nnkip) {
    if(0<KIParp(tipnum,(AddrBlk *)&arp.tha[0]))
			cacheupdate(tipnum, &arp.tha[0]);
		return(0);
	  }	/* end if */
#endif

	movebytes(arp.tha,broadaddr,DADDLEN); 
	movebytes(arp.tpa,tipnum,4);		  		/* put in IP address we want */
	arp.op=intswap(ARPREQ);						/* request packet */
	movebytes(arp.d.dest,broadaddr,DADDLEN);	/* send to everyone */
	if(dlayersend((DLAYER *)&arp,sizeof(arp)))
		return(1);		  						/* error return */
	return(0);
}

/************************************************************************/
/*  interpret ARP packets
*   Look at incoming ARP packet and make required assessment of usefulness,
*   check to see if we requested this packet, clear all appropriate flags.
*/
int arpinterpret(ARPKT *p)
{
/*
*  check packet's desired IP address translation to see if it wants
*  me to answer.
*/
#ifdef CHECKRARP
fprintf(stderr,"ARP received, arp->op=%u\r\n",p->op);
#endif
    if(p->op==intswap(ARPREQ) && (comparen(p->tpa,nnipnum,4))) {
#ifdef CHECKRARP
fprintf(stderr,"Bogus response!\r\n");
#endif
      cacheupdate(p->spa,p->sha);   /* keep her address for me */
		replyarp(p->sha,p->spa);		/* proper reply */
		return(0);
  }
/*
*  Check for a RARP reply.  If present, call netsetip()
*/
	else
    if(p->op==intswap(RARPR) && (comparen(p->tha,nnmyaddr,DADDLEN))) {
#ifdef CHECKRARP
fprintf(stderr,"RARP response!\r\n");
#endif
			movebytes(nnipnum,p->tpa,4);
			return(0);
    }
/* 
*  Check for a reply that I probably asked for.
*/
    if(comparen(p->tpa,nnipnum,4) && p->op==intswap(ARPREP) && p->hrd==intswap(HTYPE) && p->hln==DADDLEN && p->pln==4) {
#ifdef CHECKRARP
fprintf(stderr,"Reply I asked for\r\n");
#endif
        cacheupdate(p->spa,p->sha);
        return(0);
	  }
#ifdef CHECKRARP
fprintf(stderr,"Dropping out of arpinterpret()\r\n");
#endif
	return(1);
}

/*************************************************************************/
/* rarp
*  Send a rarp request to look up my IP number
*/
static int rarp(void)
{
/*
*  our other fields should already be loaded
*/
#ifdef CHECKRARP
fprintf(stderr,"sending RARP\r\n");
#endif
#ifdef CHECKNULL
check_null_area();
#endif
	movebytes(arp.tha,nnmyaddr,DADDLEN);	/* address to look up (me) */
	movebytes(arp.sha,nnmyaddr,DADDLEN);	/* address to look up (me) */
	arp.op=intswap(RARPQ);					/* request packet */
	movebytes(arp.d.dest,broadaddr,DADDLEN);		/* send to everyone */
	arp.d.type=ERARP;
	if(dlayersend((DLAYER *)&arp,sizeof(arp)))
		return(1);		  				/* error return */
	arp.d.type=EARP;						/* set back for ARP to use */
#ifdef CHECKNULL
check_null_area();
#endif
#ifdef CHECKRARP
fprintf(stderr,"done sending RARP\r\n");
#endif
    return(0);
}

/*************************************************************************/
/* cacheupdate
*  We just received an ARP, or reply to ARP and need to add the information
*  to the cache.
*
*  Reset arptime so that another machine may be ARPed.  This timer keeps
*  ARPs from going out more than one a second unless we receive a reply.
*/
static int32 arptime=0L;

int cacheupdate(uint8 *ipn,uint8 *hrdn)
{
	int i,found;
	int32 timer;

	found=-1;
/*
* linear search to see if we already have this entry
*/
	for(i=0; found<0 && i<CACHELEN; i++) 
		if(comparen(ipn,arpc[i].ip,4))
			found=i;
/*
*  if that IP number is not already here, take the oldest entry.
*  If it is already here, update the info and reset the timer.
*  These were pre-initialized to 0, so if any are blank, they will be
*  taken first because they are faked to be oldest.
*/
	if(found<0) {
		timer=arpc[0].tm;
		found=0;
		for(i=1; i<CACHELEN; i++) 
			if(arpc[i].tm<timer && !arpc[i].gate) {	/* exclude gateways */
				found=i;
				timer=arpc[i].tm;
			  }
	  }
/*
*   do the update to the cache
*/
	movebytes(arpc[found].hrd,hrdn,DADDLEN);
	movebytes(arpc[found].ip,ipn,4);
    arpc[found].tm=n_clicks();
	arptime=0L;					/* reset, allow more arps */
	return(found);
}

/*************************************************************************/
/*  cachelook
*   look up information in the cache
*   returns the cache entry number for the IP number given.
*   Returns -1 on no valid entry, also if the entry present is too old.
*
*   doarp is a flag for non-gateway requests which determines whether an
*   arp will be sent or not.
*/
int cachelook(uint8 *ipn,int gate,int doarp)
{
	int i,haveg;
/*
*  First option, we are not looking for a gateway, but a host on our
*  local network.
*/
	if(!gate) {
		for(i=0; i<CACHELEN; i++)
            if(comparen(ipn,arpc[i].ip,4) && arpc[i].tm+CACHETO>n_clicks())
				return(i);
/*
*  no valid entry, send an ARP
*/
        if(n_clicks()>=arptime && doarp) {  /* check time limit */
			reqarp(ipn);				/* put out a broadcast request */
            arptime=n_clicks()+ARPTO;
		  }
	  }
	else {
/*
*  Second option, we need a gateway.
*  if there is a gateway with a current ARP, use it.
*  if not, arp all of the gateways and return an error.  Next call will
*  probably catch the result of the ARP.
*/
		haveg=0;
		for(i=CACHELEN-1; i>=0; i--)
            if(arpc[i].gate && arpc[i].tm+CACHETO>n_clicks())
				return(i);
        if(n_clicks()>=arptime) {
			for(i=CACHELEN-1; i>=0; i--)
				if(arpc[i].gate) {
					haveg=1;
					reqarp(arpc[i].ip);	/* put out a broadcast request */
				  }
			if(!haveg) 			/* blind luck, try ARPing even for */
				reqarp(ipn);		/* a node not on our net. (proxy ARP)*/
            arptime=n_clicks()+ARPTO;
		  }
	  }
    return(-1);
}

/***************************************************************************/
/*  netdlayer
*	   get data layer address for insertion into outgoing packets.
*   searches based on ip number.  If it finds the address, ok, else . . .
*
*   Checks to see if the address is on the same network.  If it is,
*   then ARPs the machine to get address.  Forces pause between sending
*   arps to guarantee not saturating network.
*
*   If not on the same network, it needs the ether address of a 
*   gateway.  Searches the list of machines for a gateway flag.
*   Returns the first gateway found with an Ethernet address. 
*
*   Returns NULL if not here, or pointer to ether address if here.
*   If we don't have it, this also sends an ARP request so that the
*   next time we are called, the ARP reply may be here by then.
*
*/
uint8 *netdlayer(uint8 *tipnum)
{
	int32 t;
	uint8 *pc;

    t=n_clicks()+nndto*TICKSPERSEC;         /* some seconds time out */
	pc=NULL;
	do {
        if(t<=n_clicks())                   /* timed out */
			return(NULL);
		pc=getdlayer(tipnum);
		netsleep(0);						/* can't have deadlock */
	  } while(pc==NULL);
	return(pc);
}

/***************************************************************************/
/*  netgetrarp
*   Look for a RARP response to arrive
*   wait for nndto seconds before returning failure.
*   If response arrives, return success.
*/
int netgetrarp(void )
{
	int32 t,tr;

#ifdef CHECKRARP
fprintf(stderr,"before RARPing\r\n");
#endif
    t=n_clicks()+nndto*TICKSPERSEC*3;       /* some seconds time out */
	tr=0L;									/* one second retry */
#ifdef CHECKRARP
fprintf(stderr,"Timeout=%ld\r\n",t);
#endif
	do {
        if(tr<=n_clicks()) {                /* need retry? */
            rarp();
            tr=n_clicks()+TICKSPERSEC;
          } /* end if */
        if(t<=n_clicks()) {                 /* timed out */
			netposterr(103);
			return(-1);
          } /* end if */
		netsleep(0);							/* can't have deadlock */
	} while(comparen(nnipnum,"RARP",4));		/* until RARP is served */
	return(0);
}   /* end netgetrarp() */

/***************************************************************************/
/*  getdlayer
*   check for the hardware address one time
*/
uint8 *getdlayer(uint8 *tipnum)
{
	int needgate,i;

	needgate=0;
/*
*  Check to see if we need to go through a gateway.
*  If the machine is on our network, then assume that we can send an ARP
*  to that machine, otherwise, send the ARP to the gateway.
*
*  Uses internet standard subnet mask method, RFC950
*  if subnets are not in use, netmask has been pre-set to the appropriate 
*  network addressing mask.
*/ 
	for(i=3; i>=0; i--)
		if((nnmask[i] & tipnum[i])!=(nnmask[i] & nnipnum[i]))
			needgate=1;
	if(needgate && (0<=(i=cachelook(tipnum,1,1)))) 
		return(arpc[i].hrd);
	if(!needgate && (0<=(i=cachelook(tipnum,0,1))))
		return(arpc[i].hrd);
	return(NULL);
}

/***************************************************************************/
/*  netsetgate
*   Establish an IP number to use as a gateway.
*   They are added in the order that they arrive and there is a limit on
*   the number of gateways equal to CACHELEN/2.
*   ARPs them as they are added so that the Cache will get pre-filled
*   with gateways.
*
*   returns 0 if ok, -1 on error(full)
*/
int netsetgate(uint8 *ipn)
{
	int i;

	for(i=CACHELEN-1 ; i>=CACHELEN/2 ; i--) {
		if(!arpc[i].gate) {
			arpc[i].gate=1;
			movebytes(arpc[i].ip,ipn,4);
			reqarp(ipn);
			return(0);
		  }
	  }	/* end for */
	return(-1);
}
