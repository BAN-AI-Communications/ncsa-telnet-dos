/*
*  TCP.C
*
*  TCP level routines
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
*
*  Revision history:
*
*  10/86 started
*  2/88 mods for int16/int32
*  5/89  clean up for 2.3 release, JKM  
*
*/

/*
 *  Includes
 */
#include <stdio.h>
#include "protocol.h"
#include "data.h"
#include "externs.h"

static void  checkfin(struct port *,TCPKT *);
static int   estab1986(struct port *,TCPKT *,int ,int );
static int   ackcheck(struct port *,TCPKT *,int );
static void  checkmss(struct port *,TCPKT *,int );
static int   tcpreset(TCPKT *);
static int   tcpdo(struct port *,TCPKT *,int ,int);

/*
 *  Semi-Global Vars
 */
static int pnum;                  /* port number */

/************************************************************************
*
*  tcpinterpret ( p, tlen ) 
*
* Called when a packet comes in and passes the IP checksum and is ov
* TCP protocol type.  Check to see if we have an open connection on
* the appropriate port and stuff it in the right buffer
*
*/
int tcpinterpret(TCPKT *p,int tlen)
{
  uint i,myport,hlen,hisport;
  struct port *prt;

/*
*  checksum
*    First, fill the pseudo header with its fields, then run our
*  checksum to confirm it.
*
*/
  if(p->t.check) {
    movebytes(tcps.source,p->i.ipsource,8);  /* move both addresses */
    tcps.z=0;
    tcps.proto=p->i.protocol;
    tcps.tcplen=intswap(tlen);      /* byte-swapped length */
    if(tcpcheck((char *)&tcps,(char *)&p->t,tlen)) {  /* compute checksum */
      netposterr(400);
      return(2);
    } /* end if */
  } /* end if */
/*
*  find the port which is associated with the incoming packet
*  First try open connections, then try listeners
*/
  myport=intswap(p->t.dest);
  hisport=intswap(p->t.source);
  hlen=p->t.hlen>>2;                /* bytes offset to data */
  for(i=0; i<NPORTS; i++) {
    prt=portlist[i];
    if(prt!=NULL && prt->in.port==myport && prt->out.port==hisport) {
      pnum=i;
      return(tcpdo(prt,p,tlen,hlen));
    } /* end if */
  } /* end for */
/*
*  check to see if the incoming packet should go to a listener
*/
  for(i=0; i<NPORTS; i++) {
    prt=portlist[i];
    if(prt!=NULL && !prt->out.port && prt->in.port==myport && (p->t.flags&TSYN)) {
      pnum=i;
      return(tcpdo(prt,p,tlen,hlen));
    } /* end if */
  } /* end for */

/*
*  no matching port was found to handle this packet, reject it
*/
  tcpreset(p);                /* tell them they are crazy */
  if(!(p->t.flags&TSYN)) {    /* no error message if it is a SYN */
    netposterr(407);        /* invalid port for incoming packet */
    inv_port_err(1,myport,p->i.ipdest);
  } /* end if */
  return(1);                  /* no port matches */
}   /* end tcpinterpret() */

/**********************************************************************/
/*
*  tcpdo ( prt, p, tlen, hlen )
*
*  Deliver the incoming packet.
*
*/
static int tcpdo(struct port *prt,TCPKT *p,int tlen,int hlen)
{
  switch(prt->state) {
    case SLISTEN:          /* waiting for remote connection */
      if(p->t.flags&TRESET)   /* ignore Resets on a connection in LISTEN state */
        break;

      if(p->t.flags&TSYN) {  /* receive SYN */
/*
*   remember anything important from the incoming TCP header 
*/
        prt->out.size=intswap(p->t.window);  /* credit window */
        prt->out.port=intswap(p->t.source);
        prt->in.nxt=longswap(p->t.seq)+1;
/*
*  set the necessary fields in the outgoing TCP packet
*/
        prt->tcpout.t.dest=p->t.source;
        prt->tcpout.t.ack=longswap(prt->in.nxt);
        prt->tcpout.t.flags=TSYN|TACK;
        prt->tcpout.t.hlen=24<<2;
/*
*  note that the maxmimum segment size is installed by 'netlisten()'
*  hence the header length is 24, not 20
*/
/*
*  initialize all of the low-level transmission stuff(IP and lower)
*/
        movebytes(prt->tcps.dest,p->i.ipsource,4);
        movebytes(prt->tcpout.i.ipdest,p->i.ipsource,4);
        movebytes(prt->tcpout.d.dest,p->d.me,DADDLEN);
#ifdef OLD_WAY
/*
*   look up address in the arp cache if using Localtalk encapsulation
*/
        if(!nnemac) {
          unsigned char *pc;

          pc=getdlayer(p->i.ipsource);
                    if(pc!=NULL)
            movebytes(prt->tcpout.d.dest,pc,DADDLEN);
          else
            return(0);    /* no hope this time */
          }
#endif
        tcpsend(prt,4);
        prt->state=SSYNR;    /* syn received */
              } /* end if */
      break;

    case SSYNR:
      if(!(p->t.flags&TACK)) {
        tcpsend(prt,4);
        break;          /* not the right one */
      } /* end if */
      prt->tcpout.t.hlen=20<<2;
      prt->out.lasttime=n_clicks();           /* don't need response */
      prt->out.nxt++;              /* count SYN as sent */
      prt->out.ack=longswap(p->t.ack);     /* starting ACK value */
      prt->out.size=intswap(p->t.window);  /* allowed window */
      prt->tcpout.t.flags=TACK;    /* starting ACK flag */
      prt->state=SEST;        /* drop through to established */
      checkmss(prt,p,hlen);      /* see if MSS option is there */
      netputevent(CONCLASS,CONOPEN,pnum);
                      /* fall through */

    case SEST:      /* normal data transmission */  
/*
*  check and accept a possible piggybacked ack
*/
      ackcheck(prt,p,pnum);
      estab1986(prt,p,tlen,hlen);
      return(0);

    case SSYNS:        /* check to see if it ACKS correctly */
      /* remember that tcpout is pre-set-up */
      if(p->t.flags&TACK) {    /* It is ACKING us */
        if((uint32)longswap(p->t.ack)!=(prt->out.nxt)) {
          netposterr(401);
          return(1);
        } /* end if */
      } /* end if */
      if(p->t.flags&TRESET) {
        netposterr(507);
        prt->state=SCLOSED;
        netputuev(CONCLASS,CONCLOSE,pnum);
        return(1);
      } /* end if */
      if(p->t.flags&TSYN) {      /* need to send ACK */
        prt->tcpout.t.flags=TACK;
        prt->in.nxt=longswap(p->t.seq) + 1;
        prt->tcpout.t.ack=longswap(prt->in.nxt);
        prt->out.ack=longswap(p->t.ack);
        prt->out.size=intswap(p->t.window);  /* credit window */
        prt->out.lasttime=0L;
        if(p->t.flags&TACK) {
          prt->state=SEST;
          netputevent(CONCLASS,CONOPEN,pnum);
          checkmss(prt,p,hlen);
        } /* end if */
        else
          prt->state=SSYNR;    /* syn received */
      } /* end if */
      break;

    case SCWAIT:
      ackcheck(prt,p,pnum);
      if(!prt->in.contain) {
        prt->tcpout.t.flags=TFIN|TACK;
        prt->out.lasttime=0L;
        prt->state=SLAST;
      } /* end if */
      break;

    case SLAST:      /* check ack of FIN, or reset to see if we are done */
      if((p->t.flags&TRESET) || ((uint32)longswap(p->t.ack)==(prt->out.nxt+1)))
        prt->state=SCLOSED;
      break;

    case SFW1:  /* waiting for ACK of FIN */
      /* throw away data */
      prt->in.nxt=longswap(p->t.seq)+tlen-hlen;
      if(p->t.flags&TRESET)
        prt->state=SCLOSED;
      else 
        if((uint32)longswap(p->t.ack)!=(prt->out.nxt+1)) {
          if(p->t.flags&TFIN) {  /* got FIN, no ACK for mine */
            prt->in.nxt++;        /* account for FIN byte */
            prt->tcpout.t.ack=longswap(prt->in.nxt);
            prt->tcpout.t.flags=TACK;  /* final byte has no FIN flag */
            prt->out.lasttime=0L;    /* cause last ACK to be sent */
            prt->state=SCLOSING;
          } /* end if */
          else {
            prt->tcpout.t.ack=longswap(prt->in.nxt);
            prt->tcpout.t.flags=TACK|TFIN;
            prt->out.lasttime=0L;
          } /* end else */
        } /* end if */
        else 
          if(p->t.flags&TFIN) {  /* ACK and FIN */
            prt->in.nxt++;        /* account for his FIN flag */
            prt->out.nxt++;        /* account for my FIN */
            prt->tcpout.t.ack=longswap(prt->in.nxt);
            prt->tcpout.t.flags=TACK;  /* final byte has no FIN flag */
            prt->out.lasttime=0L;    /* cause last ACK to be sent */
            prt->state=STWAIT;    /* we are done */
          } /* end if */
          else {              /* got ACK, no FIN */
            prt->out.nxt++;        /* account for my FIN byte */
            prt->tcpout.t.flags=TACK;  /* final pkt has no FIN flag */
            prt->state=SFW2;
        } /* end else */
        break;

    case SFW2:                /* want FIN */
      prt->in.nxt=longswap(p->t.seq)+tlen-hlen;
      if(p->t.flags&TRESET)
        prt->state=SCLOSED;
      else 
        if(p->t.flags&TFIN) {    /* we got FIN */
          prt->in.nxt++;          /* count his FIN byte */
          prt->tcpout.t.ack=longswap(prt->in.nxt);
          prt->out.lasttime=0L;    /* cause last ACK to be sent */
          prt->state=STWAIT;
        } /* end if */
      break;

    case SCLOSING:            /* want ACK of FIN */
      if(p->t.flags&TRESET)
        prt->state=SCLOSED;
      else 
        if(!ackcheck(prt,p,pnum)) {
          prt->out.nxt++;        /* account for my FIN byte */
          prt->state=STWAIT;    /* time-wait state next */
        } /* end if */
      break;

    case STWAIT:            /* ack FIN again? */
      if(p->t.flags&TRESET)
        prt->state=SCLOSED;
      if(p->t.flags&TFIN)       /* only if he wants it */
        prt->out.lasttime=0L;
            if(prt->out.lasttime && (prt->out.lasttime+WAITTIME<n_clicks()))
        prt->state=SCLOSED;
      break;      

    case SCLOSED:
      prt->in.port=prt->out.port=0;
      break;

    default:
      netposterr(403);      /* unknown tcp state */
      break;
  } /* end switch */
  return(0);
}   /* end tcpdo() */

/**********************************************************************/
/*  checkmss
*  Look at incoming SYN,ACK packet and check for the options field
*  containing a TCP Maximum segment size option.  If it has one,
*  then set the port's internal value to make sure that it never
*  exceeds that segment size.
*/
static void checkmss(struct port *prt,TCPKT *p,int hlen)
{
  unsigned int i;
/*
*  check header for maximum segment size option
*/
  if(hlen>20 && p->x.options[0]==2 && p->x.options[1]==4) {
    movebytes((char *)&i,(char *)&p->x.options[2],2);  /* swapped value of maxseg */
    i=intswap(i);
    if(i<(unsigned int)(prt->sendsize))    /* we have our own limits too */
      prt->sendsize=i;
  } /* end if */
}   /* end checkmss() */

/**********************************************************************/
/* tcpreset
*  Send a reset packet back to sender
*  Use the packet which just came in as a template to return to
*  sender.  Fill in all of the fields necessary and dlayersend it back.
*/
static int tcpreset(TCPKT *t)
{
  uint tport;
  struct pseudotcp xxx;

  if(t->t.flags&TRESET)    /* don't reset a reset */
    return(1);

/*
*  swap TCP layer portions for sending back
*/
	if(t->t.flags&TACK) {
		t->t.seq=t->t.ack;		/* ack becomes next seq # */
		t->t.ack=0L;				/* ack # is 0 */
    t->t.flags=TRESET;
  } /* end if */
	else {
#ifdef OLD_WAY
        t->t.seq=0L;
        t->t.ack=longswap(longswap(t->t.seq)+t->i.tlen-sizeof(IPLAYER));
        t->t.flags=TRESET|TACK;
#else
        /* Thanks to Hsiao-yang Cheng  rmg 931100 */
        t->t.ack=t->t.seq;
        t->t.seq=0L;
        if (t->t.flags&TSYN) t->t.ack=longswap(longswap(t->t.ack)+1L);
        t->t.flags=TRESET|TACK;
#endif
  } /* end else */

  tport=t->t.source;              /* swap port #'s */
  t->t.source=t->t.dest;
  t->t.dest=tport;
    t->t.hlen=20<<2;                /* header len */
/*
*  create pseudo header for checksum
*/
  xxx.z=0;
  xxx.proto=t->i.protocol;
  xxx.tcplen=intswap(20);
  movebytes(xxx.source,t->i.ipsource,4);
  movebytes(xxx.dest,t->i.ipdest,4);
  t->t.check=0;  
  t->t.check=tcpcheck((char *)&xxx,(char *)&t->t,sizeof(struct tcph));
/*
*  IP and data link layers
*/  
  movebytes(t->i.ipdest,t->i.ipsource,4);  /* machine it came from */
  movebytes(t->i.ipsource,nnipnum,4); 
  t->i.tlen=intswap(sizeof(IPLAYER)+sizeof(TCPLAYER));
  t->i.ident=nnipident++;
  t->i.ttl=30;
  t->i.check=0;
  t->i.check=ipcheck((char *)&t->i,10);
  movebytes(t->d.dest,t->d.me,DADDLEN);  /* data link address */
  movebytes(t->d.me,blankd.me,DADDLEN);  /* my address */

  return(dlayersend((DLAYER *)t,sizeof(DLAYER)+sizeof(IPLAYER)+sizeof(TCPLAYER)));
}   /* end tcpreset() */

/***************************************************************************/
/*  tcpsend
*     transmits a TCP packet.  
*
*   For IP:
*      sets ident,check,totallen
*   For TCP:
*      sets seq and window from port information,
*    fills in the pseudo header and computes the checksum.
*      Assumes that all fields not filled in here are filled in by the
*      calling proc or were filled in by makeport(). 
*      (see all inits in protinit)
*
*/
int tcpsend(struct port *pport,int dlen)
{
  struct port *p;

  p=pport;

  if(p==NULL) {
    netposterr(404);
    return(-1);
  } /* end if */
/*
*  do IP header first
*/
  p->tcpout.i.ident=intswap(nnipident++);
  p->tcpout.i.tlen=intswap(sizeof(struct iph)+sizeof(struct tcph) + dlen);
  p->tcpout.i.check=0;        /* install checksum */
  p->tcpout.i.check=ipcheck((char *)&p->tcpout.i,10);
/*
*  do TCP header
*/
  p->tcpout.t.seq=longswap(p->out.nxt);      /* bytes swapped */
/*
*  if the port has some credit limit, use it instead of large
*  window buffer.  Generally demanded by hardware limitations.
*/
  if((uint)(p->credit) < (p->in.size))
    p->tcpout.t.window=intswap(p->credit);
  else
    p->tcpout.t.window=intswap(p->in.size);  /* window size */
/*
*  prepare pseudo-header for checksum
*/
  p->tcps.tcplen=intswap(dlen+sizeof(TCPLAYER));
  p->tcpout.t.check=0;
  p->tcpout.t.check=tcpcheck((char *)&p->tcps,(char *)&p->tcpout.t,dlen+sizeof(struct tcph));
  p->out.lasttime=n_clicks();
  return(dlayersend((DLAYER *)&p->tcpout,sizeof(DLAYER)+sizeof(IPLAYER)+sizeof(TCPLAYER)+dlen));
}   /* end tcpsend() */

/***************************************************************************/
/*  ackcheck
*   take an incoming packet and see if there is an ACK for the outgoing
*   side.  Use that ACK to dequeue outgoing data.
*/
static int ackcheck(struct port *p,TCPKT *t,int pnum)
{
  uint32 ak;
  int32 rttl;
  int i;

  if((t->t.flags&TRESET) && (t->t.seq==p->tcpout.t.ack)) {
    netposterr(405);
    p->state=SCLOSED;
    netputuev(CONCLASS,CONCLOSE,pnum);
    return(1);
  } /* end if */
  if(!(t->t.flags&TACK))        /* check ACK flag */
    return(1);               /* if no ACK, no go */
  p->out.size=intswap(t->t.window);  /* allowable transmission size */
/*
*  rmqueue any bytes which have been ACKed, update p->out.nxt to the
*  new next seq number for outgoing.  Update send window.
*
*/
  ak=longswap(t->t.ack);      /* other side's ACK */
/*
*  Need to add code to check for wrap-around of sequence space
*  for ak.  ak - p->out.ack may be affected by sequence wraparound.
*  If you have good, efficient code for this, please send it to me.
*
*  If ak is not increasing (above p->out.nxt) then we should assume
*  that it is a duplicate packet or one of those stupid keepalive
*  packets that 4.2 sends out.
*/
  if(ak>p->out.nxt) {
    rmqueue(&p->out,(int)(ak - p->out.ack));  /* take off of queue */
    p->out.nxt=ak;
    p->out.ack=ak;
/*
*  Check to see if this acked our most recent transmission.  If so, adjust
*  the RTO value to reflect the newly measured RTT.  This formula reduces
*  the RTO value so that it gradually approaches the most recent round
*  trip measurement.  When a packet is retransmitted, this value is
*  doubled (exponential backoff).
*/
    rttl=n_clicks()-p->out.lasttime;
    if(!p->out.contain && rttl<(long)(MAXRTO) && p->rto>=MINRTO) {  /* just now emptied queue */
      i=(int)(rttl);
      i=((p->rto-MINRTO)*3+i+1)>>2;  /* smoothing function */
      p->rto=i+MINRTO;
    }
    if(!p->out.contain && p->out.push)  /* if the queue emptied and push was set, clear push */
      p->out.push=0;
    if(p->out.contain>0)
      p->out.lasttime=0L;      /* forces xmit */
    return(0);
  }

/* the following line was added by QAK in an attempt to get ftpbin working again. */
#ifdef NOT
    if(p->out.size>0)
    p->out.lasttime=0L;      /* forces xmit */
#endif
    return(1);
}   /* end ackcheck() */

/***************************************************************************/
/*  estab1986
*   take a packet which has arrived for an established connection and
*   put it where it belongs.
*/
static int estab1986(struct port *prt,TCPKT *pkt,int tlen,int hlen)
{
  int dlen;
  uint32 sq,want;

  dlen=tlen-hlen;
/*
*  see if we want this packet, or is it a duplicate?
*/
  sq=longswap(pkt->t.seq);

  want=prt->in.nxt;
  if(sq!=want) {      /* we may want it, may not */
    if(sq<want && sq+dlen >= want) {    /* overlap */
      hlen+=(int)(want-sq);    /* offset desired */
      dlen-=(int)(want-sq);    /* skip this much */
    } /* end if */
    else {                  /* tough it */
      /* Insert real cool stream reassembly to fix FTP <here>  RMG */
      prt->out.lasttime=0L;        /* make the ACK time out */
      return(-1);
    } /* end else */
  } /* end if */
  else 
    if(dlen<=0) {                      /* only an ACK packet */
      checkfin(prt,pkt);               /* might still have FIN */
      return(0);
    } /* end if */
/*
*  If we have room in the window, update the ACK field values
*/
  if((prt->in.size)>=(uint)dlen) {
    prt->in.nxt+=dlen;                 /* new ack value */
    prt->in.size-=dlen;                /* new window size */
    prt->out.lasttime=0L;              /* force timeout for ACK */
    enqueue(&prt->in,pkt->x.data+hlen-20,dlen);
    netputuev(CONCLASS,CONDATA,pnum);  /* tell user about it */
    prt->tcpout.t.ack=longswap(prt->in.nxt);
    prt->in.lasttime=n_clicks();
  } /* end if */
  else {                  /* no room in input buffer */
    prt->out.lasttime=0L;        /* re-ack old sequence value */
  } /* end else */
/* 
*  Check the FIN bit to see if this connection is closing
*/
  checkfin(prt,pkt);
  return(0);
}   /* end estab1986() */

/***************************************************************************/
/* checkfin
*   Check the FIN bit of an incoming packet to see if the connection
*   should be closing, ACK it if we need to.
*   Half open connections immediately, automatically close.  We do
*   not support them.  As soon as the incoming data is delivered, the
*   connection will close.
*/
static void checkfin(struct port *prt,TCPKT *pkt)
{
  if(pkt->t.flags&TFIN) {    /* fin bit found */
    prt->in.nxt++;        /* count the FIN byte */
    prt->state=SCWAIT;    /* close-wait */
    prt->tcpout.t.ack=longswap(prt->in.nxt);  /* set ACK in packet */
    prt->credit=0;
    prt->out.lasttime=0L;    /* cause ACK to be sent */
    netputuev(CONCLASS,CONCLOSE,pnum);
/*
*   At this point, we know that we have received all data that the other
*   side is allowed to send.  Some of that data may still be in the 
*   incoming queue.  As soon as that queue empties, finish off the TCP
*   close sequence.  We are not allowing the user to utilize a half-open
*   connection, but we cannot close before the user has received all of
*   the data from the incoming queue.
*/
    if(!prt->in.contain) {          /* data remaining? */
      prt->tcpout.t.flags=TFIN|TACK;
      tcpsend(prt,0);
      prt->state=SLAST;
    } /* end if */
  } /* end if */
}   /* end checkfin() */


