/*
*	  TOOLS.C
*
****************************************************************************
*																			*
*	  part of:																*
*	  TCP/IP kernel for NCSA Telnet									  		*
*	  by Tim Krauskopf														*
*																			*
*	  National Center for Supercomputing Applications						*
*	  152 Computing Applications Building									*
*	  605 E. Springfield Ave.												*
*	  Champaign, IL  61820													*
*																		 	*
*     This program is in the public domain.                                 *
*                                                                           *
****************************************************************************
*
*  Portions of the driver code that are not specific to a particular protocol
*
*/
#include <stdio.h>
#include <string.h>
#include "protocol.h"
#include "data.h"
#include "externs.h"

extern int ftpdata;						/* current ftp data port */

static char *get_name(int port);		/* returns name of a port */
static char *find_port(int port);


int pingtransq(struct port *prt);   /* CGW - 2/10/93 */
int transp(struct port *prt);       /* CGW - 2/11/93 */



/************************************************************************/
/*  netsleep
*	  sleep, while demuxing packets, so we don't miss anything
*
*/
int CDECL netsleep(int n)
{
  unsigned int u;
  int i,nmux,redir;
	int32 t,gt,start;
  struct port *p, **q;
	uint8 *pc;

	redir=0;
    start=n_clicks();

	if(n)
		t=start+n*TICKSPERSEC;
	else
		t=start;

	do {
    nmux=demux(1);        /* demux all packets */

/*
*  if there were packets in the incoming packet buffer, then more might
*  have arrived while we were processing them.  This gives absolute priority
*  to packets coming in from the network.
*/
		if(nmux)
			continue;
/*
*  Check for any ICMP redirect events.
*/
    if(IREDIR==netgetevent(ICMPCLASS,&i,&i))
			redir=1;
/*
*  Check each port to see if action is necessary.
*  This now sends all Ack packets, due to p->lasttime being set to 0L.
*  Waiting for nmux==0 for sending ACKs makes sure that the network
*  has a much higher priority and reduces the number of unnecessary ACKs.
*/
    gt=n_clicks();
    q=&portlist[0];
    for(u=0; u<NPORTS; u++,q++) {
      p=*q;
      if((p!=NULL) && (p->state>SLISTEN)) {
        if(!p->out.lasttime)
          transq(p);        /* takes care of all ACKs */
        else
          if((p->out.contain>0) || (p->state>SEST)) {
/*
*  if a retransmission timeout occurs, exponential back-off.
*  This number returns toward the correct value by the RTT measurement
*  code in ackcheck.
*
*  fix: 5/12/88, if timer was at MAXRTO, transq didn't get hit - TK
*/
            if((p->out.lasttime+p->rto)<gt) {
              if(p->rto<MAXRTO)
								p->rto<<=1;		/* double it */
#ifdef AUX /* RMG */
              fprintf(stdaux,"retrans ");
#endif

#ifdef OLD_WAY
              transq(p);
#else                   /* CGW - 2/10/93 */
              transp(p);
#endif
            }
          }
          if((p->out.lasttime+POKEINTERVAL<gt) && (p->state==SEST))
            transq(p);
/*
*  check to see if ICMP redirection occurred and needs servicing.
*  If it needs servicing, try to get the new hardware address for the new 
*  gateway.  If getdlayer fails, we assume an ARP was sent, another ICMP
*  redirect will occur, this routine will reactivate, and then the hardware
*  address will be available in the cache.
*  Check all ports to see if they match the redirected address.
*/
          if(redir && comparen(p->tcpout.i.ipdest,nnicmpsave,4)) {
            pc=getdlayer(nnicmpnew);
            if(pc!=NULL)
              movebytes(p->tcpout.d.dest,pc,DADDLEN);
          } /* end if */
      } /* end if */
    } /* end for */
		redir=0;				/* reset flag for next demux */
  } while((t>n_clicks()) && (n_clicks()>=start));  /* allow for wraparound of timer */

	return(nmux);				/* will demux once, even for sleep(0) */
}


/***************************************************************************/
/*  enqueue
*   add something to a TCP queue.  Used by both 'write()' and tcpinterpret
*   WINDOWSIZE is the size limitation of the advertised window.
*/
int enqueue(struct window *wind,char *buffer,int nbytes)
{
	int i;

	i=WINDOWSIZE-wind->contain;
	if(i<=0 || nbytes==0)
		return(0);						/* no room at the inn */
	if(nbytes>i)
		nbytes=i;
	i=wind->where-wind->endlim;		/* room at end */
	i+=WINDOWSIZE;
	if(i<nbytes) {
		movebytes(wind->endlim,buffer,i);
		movebytes(wind->where,(char *)(buffer+i),nbytes-i);
		wind->endlim=wind->where+nbytes-i;
  }
	else {
		movebytes(wind->endlim,buffer,nbytes);		/* fits in one chunk */
		wind->endlim+=nbytes;
  }
	wind->contain+=nbytes;			/* more stuff here */
	return(nbytes);
}

/*************************************************************************/
/* dequeue
*	 used by read, this copies data out of the queue and then
*  deallocates it from the queue.
*  cpqueue and rmqueue are very similar and are to be used by tcpsend
*  to store unacknowledged data.
*
*  returns number of bytes copied from the queue
*/
int dequeue(struct window *wind,char *buffer,int nbytes)
{
	int i;

	if(wind->contain==0)
		return(0);
	if((wind->contain)<(uint)nbytes)
		nbytes=wind->contain;
	i=wind->endbuf-wind->base;
	if(i<=nbytes) {
		movebytes(buffer,wind->base,i);
		movebytes((char *)(buffer+i),wind->where,nbytes-i);
		wind->base=wind->where+nbytes-i;
  }
	else {
		movebytes(buffer,wind->base,nbytes);
		if((wind->contain)==(uint)nbytes) 
			wind->base=wind->endlim=wind->where;
		else
			wind->base+=nbytes;
  }
	wind->contain-=nbytes;
	return(nbytes);
}

/**************************************************************************/
/*  rmqueue
*	 does the queue deallocation that is left out of cpqueue
*
*   rmqueue of WINDOWSIZE or greater bytes will empty the queue
*/
int rmqueue(struct window *wind,int nbytes)
{
	int i;

	if((wind->contain)< (uint)nbytes)
		nbytes=wind->contain;
	i=wind->endbuf-wind->base;
	if(i<=nbytes) 
		wind->base=wind->where+nbytes-i;
	else {
		if((wind->contain)==(uint)nbytes)
			wind->base=wind->endlim=wind->where;
		else
			wind->base+=nbytes;
  }
	wind->contain-=nbytes;
	return(nbytes);
}

/************************************************************************/
/*  transq
*
*   Needed for TCP, not as general as cpqueue, 
*   but is required for efficient transmit of the whole window.
*
*   Transmit the entire queue (window) to the other host without expecting
*   any sort of acknowledgement.
*
*/
int transq(struct port *prt)
{
	uint bites;
	unsigned int i,j,n;
	struct window *wind;
	uint32 saveseq;
	uint8 *endb,*whereb,*baseb;

	if(prt==NULL) {
		nnerror(406);		/* NULL port for trans */
		return(-1);
  }
	wind=&prt->out;
/*
*   find out how many bytes the other side will allow us to send (window)
*/
	bites=wind->size;
	if(wind->contain<bites)
		bites=wind->contain;
/*
*  set up the tcp packet for this, ACK field is same for all packets
*/
	prt->tcpout.t.ack=longswap(prt->in.nxt);
/*
*  any more flags should be set?
*/
	if(wind->push && (bites>0))					/* is push indicator on? */
		prt->tcpout.t.flags|=TPUSH;
	else
		prt->tcpout.t.flags&=~TPUSH;		/* else clear push */
  /* we never set push flag unless we are actually sending data */
	if((bites<=0) || prt->state!=SEST) {	/* if no data to send . . . */
		tcpsend(prt,0);				/* just a retransmission or ACK */
		return(0);
  }
/*
 *  we have data to send, get the correct sequence #'s
 *  To be really real, we should check wraparound sequence # in the loop.
 */
	saveseq=wind->nxt;
	whereb=wind->where;
	endb=wind->endbuf;
	baseb=wind->base;
/*
 *  in a loop, transmit the entire queue of data
 */
	for(i=0; i<bites; i+=prt->sendsize) {
		n=prt->sendsize;
		if(i+n>bites)
			n=bites-i;
		j=endb-baseb;
		if(j<n) {
			movebytes(prt->tcpout.x.data,baseb,j);
			movebytes((char *)(prt->tcpout.x.data+j),whereb,n-j);
			baseb=whereb+n-j;
    }
    else {
      movebytes(prt->tcpout.x.data,baseb,n);
			baseb+=n;
    }
		tcpsend(prt,n);						/* send it */
		wind->nxt+=n;
  } /* end for */
	wind->nxt=saveseq;					/* get back first seq # */
	return(0);
}

/************************************************************************/
/*  transp
*
*   Transmit one packet, instead of the entire queue (window), to the
*   other host without expecting any sort of acknowledgement.
*
*/
int transp(struct port *prt) {
	uint bites;
	unsigned int i,j,n;
	struct window *wind;
	uint32 saveseq;
	uint8 *endb,*whereb,*baseb;

	if(prt==NULL) {
		nnerror(406);		/* NULL port for trans */
		return(-1);
  }
	wind=&prt->out;
/*
*   find out how many bytes the other side will allow us to send (window)
*/
  bites=prt->sendsize;
	if(wind->contain<bites)
		bites=wind->contain;
/*
*  set up the tcp packet for this, ACK field is same for all packets
*/
	prt->tcpout.t.ack=longswap(prt->in.nxt);
/*
*  any more flags should be set?
*/
	if(wind->push && (bites>0))					/* is push indicator on? */
		prt->tcpout.t.flags|=TPUSH;
	else
		prt->tcpout.t.flags&=~TPUSH;		/* else clear push */
  /* we never set push flag unless we are actually sending data */
	if((bites<=0) || prt->state!=SEST) {	/* if no data to send . . . */
		tcpsend(prt,0);				/* just a retransmission or ACK */
		return(0);
    }
/*
 *  we have data to send, get the correct sequence #'s
 *  To be really real, we should check wraparound sequence # in the loop.
 */
	saveseq=wind->nxt;
	whereb=wind->where;
	endb=wind->endbuf;
	baseb=wind->base;
/*
 *  transmit the next packet of data
 */
    n=bites;
    j=endb-baseb;
    if(j<n) {
        movebytes(prt->tcpout.x.data,baseb,j);
        movebytes((char *)(prt->tcpout.x.data+j),whereb,n-j);
        baseb=whereb+n-j;
    } else {
        movebytes(prt->tcpout.x.data,baseb,n);
        baseb+=n;
    }
    tcpsend(prt,n);                     /* send it */
	wind->nxt=saveseq;					/* get back first seq # */
	return(0);
}

/************************************************************************/
/*  netposterr
*   place an error into the event q
*   Takes the error number and puts it into the error structure
*/
void netposterr(int num)
{
	if(netputevent(ERRCLASS,ERR1,num))
		netputuev(ERRCLASS,ERR1,501);			/* only if we lost an event */
}

/***********************************************************************/
/*  netgetevent
*   Retrieves the next event (and clears it) which matches bits in
*   the given mask.  Returns the event number or -1 on no event present.
*   Also returns the exact class and the associated integer in reference
*   parameters.
*
*   The way the queue works:
*	 There is always a dummy record pointed to by nnelast.
*	 When data is put into the queue, it goes into nnelast, then nnelast
*		looks around for another empty one to obtain.
*		It looks at nnefree first, then bumps one from nnefirst if necessary.
*	 When data is retrieved, it is searched from nnefirst to nnelast.
*		Any freed record is appended to nnefree.
*/
int netgetevent(uint8 mask,int *retclass,int *retint)
{
	int i,j=0;

	i=nnefirst;
	while(i!=nnelast) {
		if(mask&nnq[i].eclass) {
			if(i==nnefirst) 
				nnefirst=nnq[nnefirst].next;		/* step nnefirst */
			else 
				nnq[j].next=nnq[i].next;			/* bypass record i */
			nnq[i].next=nnefree;
			nnefree=i;							/* install in free list */
			*retint=nnq[i].idata;
			*retclass=nnq[i].eclass;
			return((int)nnq[i].event);
		  }
		j=i;
		i=nnq[i].next;
	  }
	return(0);
}

/***********************************************************************/
/*  netputevent
*   add an event to the queue.
*   Will probably get the memory for the entry from the free list.
*   Returns 0 if there was room, 1 if an event was lost.
*/
int netputevent(int class,int what,int dat)
{
	int i;

	i=nnelast;
	nnq[i].eclass=(uint8)class;					/* put data in */
	nnq[i].event=(uint8)what;
	nnq[i].idata=dat;
	if(nnefree>=0) {						/* there is a spot in free list */
		nnq[i].next=nnelast=nnefree;
		nnefree=nnq[nnefree].next;		/* remove from free list */
		return(0);
	  }
	else {
		nnq[i].next=nnelast=nnefirst;
		nnefirst=nnq[nnefirst].next;		/* lose oldest event */
		return(1);
	  }
}

/***************************************************************************/
/*  netputuev
*   put a unique event into the queue
*   First searches the queue for like events
*/
int netputuev(int class,int what,int dat)
{
	int i;

	i=nnefirst;
	while(i!=nnelast) {
		if(nnq[i].idata==dat && nnq[i].event==(uint8)what && nnq[i].eclass==(uint8)class)
			return(0);
		i=nnq[i].next;
	  }
	return(netputevent(class,what,dat));
}

/************************************************************************/
/*  neterrstring
*   returns the string associated with a particular error number
*
*   error number is formatted %4d at the beginning of the string
*/
#ifndef NET14
static char *errs[]={
	"   0 Error unknown",
	" 100 Network jammed, probable break in wire",
	" 101 Could not initialize hardware level network driver",
	" 102 ERROR: The conflicting machine is using the same IP number",
	" 103 RARP request failed, an IP number is required",
	" 300 Bad IP checksum",
	" 301 IP packet not for me",
	" 302 IP packet with options received",
	" 303 IP: unknown higher layer protocol",
	" 304 IP: fragmented packet received, frags not supported",
	" 400 TCP: bad checksum",
	" 401 ACK invalid for TCP syn sent",
	" 403 TCP in unknown state",
	" 404 Invalid port for TCPsend",
	" 405 TCP connection reset by other host",
	" 406 Null port specified for ackandtrans",
	" 407 Packet received for invalid port -- reset sent",
	" 500 No internal TCP ports available",
	" 501 Warning: Event queue filled, probably non-fatal",
	" 504 Local host or gateway not responding",
	" 505 Memory allocation error, cannot open port",
	" 506 Not allowed to connect to broadcast address",
	" 507 Reset received: syn sent, host is refusing connection",
	" 600 ICMP:	Echo reply",
	" 603 ICMP:	Destination unreachable",
	" 604 ICMP:	Source Quench",
	" 605 ICMP:	Redirect, another gateway is more efficient",
	" 608 ICMP:	Echo requested (ping requested)",
	" 611 ICMP:	Time Exceeded on Packet",
	" 612 ICMP:	Parameter problem in IP",
	" 613 ICMP:	Timestamp request",
	" 614 ICMP:	Timestamp reply",
	" 615 ICMP:	Information request",
	" 616 ICMP:	Information reply",
	" 699 ICMP: Checksum error",
	" 700 Bad UDP checksum",
	" 800 Domain: Name request to server failed",
	" 801 Domain: Using default domain",
	" 802 Domain: name does not exist",
	" 803 Domain: UDP name server did not resolve the name",
	" 804 Domain: name server failed, unknown reason",
	" 805 Host machine not in configuration file",
	" 806 Missing IP number, requires domain lookup",
	" 900 Session: Cannot find or open configuration file",
	" 901 Session: Cannot allocate memory for processing",
	" 902 Session: Invalid keyword in configuration file",
	" 903 Session: Element too long (>200), maybe missing quote",
	" 904 Session: Probable missing quote marks, a field must be on one line",
	" 905 Session: 'name' field required before other machine entries",
	" 906 Session: Syntax error, invalid IP number",
	" 907 Session: Syntax error, Subnet mask invalid",
	" 908 Session: Syntax error, IP address for this PC is invalid",
	""};
#endif

static char errspace[80];		/* room for user-defined errors */

char *neterrstring(int errno)
{
	int i;
	char s[10];

    if(errno<0)
		return(errspace);
#ifndef NET14
	sprintf(s,"%4d",errno);
	i=0;
	do {
		if(!strncmp(errs[i],s,4))
			return(errs[i]+5);			/* pointer to error message  */
		i++;
      } while(*errs[i] || i>100);       /* until NULL found */
	return(errs[0]+5);					/* error unknown */
#endif
}
