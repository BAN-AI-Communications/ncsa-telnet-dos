/*
*   IP.C
*   IP level routines, including ICMP
*   also includes a basic version of UDP, not generalized yet
*
****************************************************************************
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
*      This program is in the public domain.                               *
*                                                                          *
****************************************************************************
*
*   IP level routines ( including an ICMP handler )
*
****************************************************************************
*  Revision history:
*
*   10/87  Initial source release, Tim Krauskopf
*   2/88  typedefs of integer lengths, TK
*   5/88    clean up for 2.3 release, JKM   
*   9/91    Add input sanity checking, reorder tests, Nelson B. Bolyard
*
*/

/*
*   Includes
*/
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "protocol.h"
#include "data.h"
#include "windat.h"
#include "externs.h"
#include "debug.h"
#include "defines.h"

#ifdef OLD_WAY
static int waiting_for_ping = FALSE;
int (*pingfunc)(ICMPKT *p, int icmplen) = NULL;
#endif
extern struct config Scon;
extern struct config def;

extern int SQwait;
extern int OKpackets;

static int neticmpturn(ICMPKT *,int );
static int icmpinterpret(ICMPKT *,int );
static int ipfraghandle(IPKT *p, int iplen);  /* rmg 931100 */

#define NBBDEBUG

#ifdef NBBDEBUG
int ipchecklen(char *s,int len,int line)
{
    if(len<=0 || len>2048 ) {
#ifdef TELBIN
        tprintf(console->vs,"ipchecklen: invalid len %d (%04x) at LINE %d\r\n",len,len,line);
#endif
        return((int)0xDEAD);  /* bad checksum, trust me! */
      } /* end if */
    return ipcheck(s,len);
}   /* end ipchecklen() */

#define ipcheck(s,l) ipchecklen(s,l,__LINE__)
#endif

/*
*   ipinterpret ( p )
*
*   Called by the packet demuxer to interpret a new ip packet.  Checks the
* validity of the packet (checksum, flags) and then passes it on to the
* appropriate protocol handler.
*
IPKT *p;    ptr to packet from network
*/
static unsigned char junk[]={0,0,0,0};

int ipinterpret(IPKT *p)
{
    int iplen;
    int hlen;

/*
*  Extract total length of packet
*/
    iplen=intswap(p->i.tlen);
    hlen=(p->i.versionandhdrlen&0x0f)<<2;

    if(hlen<sizeof(p->i)   /* Header too small */
            || iplen<hlen             /* Header inconsistent */
            || iplen>2048 ) {         /* WAY too big */
        netposterr(300);    /* bad IP checksum */
        return(1);          /* drop packet */
      } /* end if */

/*
*  checksum verification of IP header
*/
#ifdef QAK
    if(p->i.check) {                       /* no IP checksumming if check=0 */
#endif
        if(ipcheck(&p->i.versionandhdrlen,(p->i.versionandhdrlen&0x0f)<<1)) {
            netposterr(300);        /* bad IP checksum */
            return(1);          /* drop packet */
          }
#ifdef QAK
      }  
#endif


    if(iplen<=hlen)     /* silently toss this legal-but-useless packet */
{
        return(1);
}

/*
*  See if there are any IP options to be handled.
*  We don't understand IP options, post a warning to the user and drop
*  the packet.
*/
    if(hlen>sizeof(p->i)) {      /* check for options in packet */
        netposterr(302);
        return(1);
      } /* end if */

    iplen-=hlen;

    /* check for fragment and handle. note that the &0x20 above was WRONG */
    if(p->i.frags&0xffbf ) {  /* NOW check for a fragmented packet - mtk add*/
                              /* don't forget to ignore the "dont fragment" bit - RMG 931230 */
#ifdef AUX /* RMG */
fprintf(stdaux," Frags: %x ",p->i.frags);
#endif
      ipfraghandle(p,iplen);  /* pass in computed iplen to save time */
      return(1);
    }

/*
*  check to make sure that the packet is for me.
*  Throws out all packets which are not directed to my IP address.
*
*  This code is incomplete.  It does not pass broadcast IP addresses up
*  to higher layers.  It used to report packets which were incorrectly
*  addressed, but no longer does.  Needs proper check for broadcast 
*  addresses.
*/
    if(!comparen(nnipnum,p->i.ipdest,4)) {     /* potential non-match */
        if(comparen(nnipnum,junk,4) && p->i.protocol==PROTUDP)
            return(udpinterpret((UDPKT *)p,iplen));
#ifdef AUX_NOT
fprintf(stdaux,"blip");
#endif
        return(1);              /* drop packet */
      } /* end if */

    switch (p->i.protocol) {        /* which protocol to handle this packet? */
        case PROTUDP:
            return(udpinterpret((UDPKT *)p,iplen));

        case PROTTCP:
            return(tcpinterpret((TCPKT *)p,iplen));   /* pass tcplen on to TCP */

        case PROTICMP:
            return(icmpinterpret((ICMPKT *)p,iplen));

        default:
            netposterr(303);
            return(1);
      }
}   

#ifdef NNDEBUG
 /*
 *  ipdump ( p )
 *
 *  Routine to dump an IP packet -- only compiled if the debug option is 
 * enabled.
 */
ipdump(IPKT *p)
{
    uint16 iplen,iid;

    iid=intswap(p->i.ident);
    iplen=intswap(p->i.tlen);

    puts("found IP packet:");
    printf("Version+hdr: %x     service %d      tlen %u   \n",
            p->i.versionandhdrlen,p->i.service,iplen);
    printf("Ident: %u    frags: %4x    ttl: %d    prot: %d  \n",
            iid,p->i.frags,p->i.ttl,p->i.protocol);
    printf("addresses: s: %d.%d.%d.%d    t: %d.%d.%d.%d \n",
        p->i.ipsource[0],p->i.ipsource[1],p->i.ipsource[2],p->i.ipsource[3],
        p->i.ipdest[0],p->i.ipdest[1],p->i.ipdest[2],p->i.ipdest[3]);
    puts("\n");
}

/***************************************************************************/
/*  ipsend   THIS ROUTINE HAS NOT BEEN TESTED, NEVER USED!
* 
*   generic send of an IP packet according to parameters.  Use of this
*   procedure is discouraged.  Terribly inefficient, but may be useful for
*   tricky or diagnostic situations.  Unused for TCP.
*
*   usage:  ipsend(data,ident,prot,options,hdrlen)
*       data is a pointer to the data to be sent
*       ident is the 16 bit identifier
*       prot is the protocol type, PROTUDP or PROTTCP or other
*       hlen is in bytes, total header length, 20 is minimum
*       dlen is the length of the data field, in bytes
*       who is ip address of recipient
*       options must be included in hlen and hidden in the data stream
*/
ipsend(uint8 *data,int dlen,int iid,uint8 iprot,uint8 *who,int hlen)
{
    int iplen;

    if(dlen>512)
        dlen=512;
    iplen=hlen+dlen;                        /* total length of packet */
    blankip.i.tlen=intswap(iplen);            /* byte swap */
    blankip.i.versionandhdrlen=0x40|(hlen>>2);
    blankip.i.ident=intswap(iid);           /* byte swap */
    blankip.i.protocol=iprot;
    blankip.i.check=0;                    /* set to 0 before calculating */
    movebytes(blankip.i.ipdest,who,4);
    movebytes(blankip.d.me,myaddr,DADDLEN);
    movenbytes(blankip.x.data,data,dlen);  /* might be header options data */
    blankip.i.check=ipcheck(&blankip.i.versionandhdrlen,hlen>>1);
                                    /* checks based on words */
                                    /* resolve knowledge of Ethernet hardware addresses */
/*
*  This is commented out because I know that this procedure is broken!
*  If you use it, debug it first.

    dlayersend(&blankip,iplen+14);
*/
    return(0);
}
#endif

/****************************************************************************/
/*
*   icmpinterpret ( p, icmplen )
*
* Interpret the icmp message that just came off the wire
*
*/
static int icmpinterpret(ICMPKT *p,int icmplen)
{
    uint i;
    IPLAYER *iptr;

    i=p->c.type;
    netposterr(600+i);      /* provide info for higher layer user */
    if(p->c.check) {        /* ignore if chksum=0 */
        if(ipcheck((char *)&p->c,icmplen>>1)) {
            netposterr(699);
            return(-1);
          } /* end if */
      } /* end if */
    switch(i) {
        case 8:                         /* ping request sent to me */
            p->c.type=0;                /* echo reply type */
            neticmpturn(p,icmplen);     /* send back */
            break;

        case 5:                         /* ICMP redirect */
            iptr=(IPLAYER *)p->data;
            netputuev(ICMPCLASS,IREDIR,0);      /* event to be picked up */
            movebytes(nnicmpsave,iptr->ipdest,4);       /* dest address */
            movebytes(nnicmpnew,&p->c.part1,4);         /* new gateway */
            break;

        case 4:                         /* ICMP source quench */
#ifdef TELBIN
            tprintf(console->vs,"ICMP: source quench received");
#endif
            OKpackets=0;
            SQwait+=100;
            break;

#ifdef OLD_WAY
        case 0:                         /* ping reply ? */
            if(waiting_for_ping) {
                if (!pingfunc)
                    waiting_for_ping = FALSE;
                else {
                    if ((*pingfunc)(p, icmplen)) {
                        waiting_for_ping = FALSE;
                        pingfunc = NULL;
                    }
                }
            }
            break;
#endif

        default:
#ifdef ASK_JEFF
			printf("ICMP\n");
#endif
            break;
      } /* end switch */
    return(0);
}   /* end icmpinterpret() */

/***************************************************************************/
/*  neticmpturn
*
*   send out an icmp packet, probably in response to a ping operation
*   interchanges the source and destination addresses of the packet,
*   puts in my addresses for the source and sends it
*
*   does not change any of the ICMP fields, just the IP and dlayers
*   returns 0 on okay send, nonzero on error
*/
static int neticmpturn(ICMPKT *p,int ilen)
{
#ifdef OLD_WAY
    unsigned char *pc;
#endif

/*
*  reverse the addresses, dlayer and IP layer
*/
    if(comparen(p->d.me,broadaddr,DADDLEN))
        return(0);
    movebytes(p->d.dest,p->d.me,DADDLEN);
#ifdef OLD_WAY
/*
*   look up address in the arp cache if we are using AppleTalk
*   encapsulation.
*/
    if(!nnemac) {
        pc=getdlayer(p->i.ipsource);
        if(pc!=NULL)
            movebytes(p->d.dest,pc,DADDLEN);
        else
            return(0);      /* no hope this time */
      }
#endif
    movebytes(p->i.ipdest,p->i.ipsource,4);
    movebytes(p->d.me,nnmyaddr,DADDLEN);
    movebytes(p->i.ipsource,nnipnum,4);
/*
*  prepare ICMP checksum
*/
    p->c.check=0;
    p->c.check=ipcheck((char *)&p->c,ilen>>1);
/*
*   iplayer for send
*/
    p->i.ident=intswap(nnipident++);
    p->i.check=0;
    p->i.check=ipcheck((char *)&p->i,10);
/*
*  send it
*/
    return((int)dlayersend((DLAYER *)p,sizeof(DLAYER)+sizeof(IPLAYER)+ilen));
}

/***************************************************************************/
/*  icmpunreach
*/
int icmpunreach(TCPKT *badp)  /* rmg 931100 */
{
  ICMPKT p;
  char icmp_type=3;  /* unreachable... */
  char icmp_code=3;  /* ...port */
  int  icmp_dlen;    /* will include badp's iplayer + 64 bits */
  int i;

#ifdef DODEDODE
  for(i=0;i<sizeof(ICMPKT);i++)
    *(((char *)&p)+i)=(char)0;
#endif

  movebytes(&p,&blankip,sizeof(DLAYER)+sizeof(IPLAYER));  /* copy from blankip */
  p.i.protocol=PROTICMP;

/*
*  reverse the addresses, dlayer and IP layer
*/
  movebytes(&p.d.dest,badp->d.me,DADDLEN);
  movebytes(&p.i.ipdest,badp->i.ipsource,4);
  movebytes(&p.d.me,nnmyaddr,DADDLEN);
  movebytes(&p.i.ipsource,nnipnum,4);

  p.c.type=icmp_type;
  p.c.code=icmp_code;
  p.c.part1=0;
  p.c.part2=0;
  i=(badp->i.versionandhdrlen && 0x0f);
  printf("i=%d (should be 5)\n",i);
  icmp_dlen=28;                  /* iplayer of badp + 64 bits of data in badp */

  movebytes(p.data,&badp->i,sizeof(ICMPLAYER)+icmp_dlen);

  p.i.tlen=intswap(sizeof(IPLAYER)+sizeof(ICMPLAYER)+icmp_dlen);

/*
*  prepare ICMP checksum
*/
  p.c.check=0;
  p.c.check=ipcheck((char *)&p.c,(sizeof(ICMPLAYER)+icmp_dlen)>>1);
/*
*   iplayer for send
*/
  p.i.ident=intswap(nnipident++);
  p.i.check=0;
  p.i.check=ipcheck((char *)&p.i,10);

#ifdef DODEDODE
  printf("this is the packet:\n");
  for(i=0;i<160;i++)
    printf("%x",*(((char *)&p)+i));
  printf("\n");
#endif

return((int)dlayersend((DLAYER *)&p,sizeof(DLAYER)+sizeof(IPLAYER)+sizeof(ICMPLAYER)+icmp_dlen));
}


/*
* IP Fragment Reassembly Hack
* by Matthew T Kaufman (matthew@echo.com)
* 1/1993, 8/1993
*/

typedef struct ipb {
        DLAYER d;
        IPLAYER i;
        uint8 data[4104];	/* "Big Enough" */
}FIPKT;

#define IPF_CHUNKS 513 /* 4104 / 8 */
#define IPF_BITWORDS 18  /* 513 / 32 round up + 1*/
#define IPF_BUFFERS 7  /* Max # of different fragmented pkts in transit */

typedef struct {
	FIPKT pkt;
	unsigned long bits[IPF_BITWORDS];
	int lastchunk;
	unsigned long lasttime;
	unsigned int iplen;
}FPBUF;

static FPBUF far Frag[IPF_BUFFERS];
                             /* the above mostly belongs in protocol.h  RMG */

int ipfraghandle(IPKT *p, int iplen)
{


	uint16 fraginfo;
	uint16 foffset;
	uint16 iden;
	FPBUF far *buf;
	int i;


	fraginfo = intswap(p->i.frags);
	foffset = fraginfo & (0x1fff);
#define morefrags (fraginfo & (0x2000))

	iden = intswap(p->i.ident);

/* we already KNOW that this IS fragmented */

/* see if we can find any friends who've already arrived... */

	buf = (FPBUF *) 0L;


	for(i=0; i<IPF_BUFFERS; i++)
	{
		if(p->i.ident == Frag[i].pkt.i.ident)
		{
			buf = &(Frag[i]);
			goto foundfriend;
		}
	}

	/* otherwise, we must be the first one here */
	{	
    unsigned long oldtime = 0x7fffffff;
		int oldest = 0;

		for(i=0; i<IPF_BUFFERS; i++)
		{
			if(Frag[i].lasttime == 0)	/* unused buffer? */
			{
				buf = &(Frag[i]);
				goto foundempty;
			}

			if(Frag[i].lasttime < oldtime)	/* track LRU */
			{
				oldtime = Frag[i].lasttime;
				oldest = i;
			}
		}

		/* if we're here, we need to reuse LRU */

		buf = &(Frag[oldest]);

foundempty:	;
		/* initialize new buffer */

		/* time will be filled in later */

		for(i=0; i<IPF_BITWORDS; i++) buf->bits[i] = 0L; /* reset */
		buf->lastchunk = 0;	/* reset */

		/* fill in the header with the current header */

    /* movemem and movebytes have reversed src/dest arguments */
    movebytes(&(buf->pkt), p, sizeof(DLAYER) + sizeof(IPLAYER) );
	}

		
foundfriend: ;

	/* now, deal with this specific fragment... */

	/* copy data */

  movebytes(&(buf->pkt.data[8 * foffset]),&(p->x.data),iplen);

	/* update rx chunks information */
  for(i=foffset; i<= (foffset+(iplen / 8)); i++)
	{
		buf->bits[i/32] |= (unsigned long) (1L<<(i % 32));
	}

	if(!morefrags)
	{
		/* now we can tell how long the total thing is */
		buf->iplen = (8*foffset)+iplen;
		buf->lastchunk = foffset;
			/* actually, lastchunk is more than this, but it */
			/* IS true that we only need to check through    */
			/* this foffset value to make sure everything has */
			/* arrived  -mtk */
	}

	/* now touch the time field, for buffer LRU */
  //buf->lasttime = clock(); /* rmg 931100 */
  buf->lasttime = n_clicks();

	/* check to see if there are fragments missing */


	if(buf->lastchunk == 0)
	{
		/* we haven't even gotten a fragment with a cleared MORE */
		/* FRAGMENTS flag, so we're missing THAT piece, at least */
		return 1;
	}

	for(i=0; i<= buf->lastchunk; i++)
	{
		/* scanning to see if we have everything */
		if(0 == ((buf->bits[i/32]) & (unsigned long)(1L<<(i % 32))) )
		{
			return 1;	/* still waiting for more */
		}
	}

	/*  otherwise, done waiting... use the packet we've gathered */


	/* first clear stuff from fragment buffer: */
	buf->lasttime = 0L;	/* mark as free to take */

	buf->lastchunk = 0;	/* need to do this, because we use it as flag */

	buf->pkt.i.ident = 0;	/* so we don't find this later */

	buf->pkt.i.frags = 0;	/* in case anybody above us checks */

	/* then send it on its way... */

    if(!comparen(nnipnum,p->i.ipdest,4)) {     /* potential non-match */
        if(comparen(nnipnum,junk,4) && p->i.protocol==PROTUDP)
            return(udpinterpret((UDPKT *)p,iplen));
        return(1);              /* drop packet */
      } /* end if */

   	switch (buf->pkt.i.protocol) {        /* which protocol */
        	case PROTUDP:
            	return(udpinterpret((UDPKT *)&(buf->pkt),buf->iplen));
		
        	case PROTTCP:
            	return(tcpinterpret((TCPKT *)&(buf->pkt),buf->iplen));  
		
        	case PROTICMP:
        	return(icmpinterpret((ICMPKT *)&(buf->pkt),buf->iplen));
		
       	default:
       		netposterr(303);
       		return(1);
  	}


}

