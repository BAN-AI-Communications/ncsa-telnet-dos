/*
*    Protocol structures for network communication
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
*  This file contains the structure definitions for each type of 
*  protocol that this program wishes to handle.  A companion file,
*  'protinit.c' initializes sample versions of each type of header, 
*  improving the efficiency of sending packets with constants in most
*  of the fields.
* 
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "whatami.h"
/************************************************************************/
/*  Ethernet frames
*      All Ethernet transmissions should use this Ethernet header which
*   denotes what type of packet is being sent.
*
*   The header is 14 bytes.  The first 6 bytes are the target's hardware
*   Ethernet address, the second 6 are the sender's hardware address and
*   the last two bytes are the packet type.  Some packet type definitions
*   are included here.
*
*   the two-byte packet type is byte-swapped, PC is lo-hi, Ether is hi-lo
*/

#define  EXNS    0x0006		   /* probably need swapping */
#define  EIP     0x0008
#define  EARP    0x0608
#define  ERARP	 0x3580			/* I guess this is RARP */
#define  ECHAOS  0x0408

typedef struct ether {
	uint8 dest[DADDLEN],				/* where the packet is going */
		me[DADDLEN];					/* who am i to send this packet */
	uint16 type;						/* Ethernet packet type  */
}DLAYER;

/*************************************************************************/
/*  Dave Plummer's  Address Resolution Protocol (ARP) (RFC-826) and 
*   Finlayson, Mann, Mogul and Theimer's Reverse ARP packets.
*
*   Note that the 2 byte ints are byte-swapped.  The protocols calls for
*   in-order bytes, and the PC is lo-hi ordered.
*   
*/
#define RARPR	0x0004		  	/*  RARP reply, from host, needs swap */
#define RARPQ	0x0003			/*  RARP request, needs swapping */
#define ARPREP  0x0002		  	/*  reply, byte swapped when used */
#define ARPREQ  0x0001		  	/*  request, byte-swapped when used */
#define ARPPRO	0x0800			/*  IP protocol, needs swapping */

#define HTYPE 	0x0001			/*  Ethernet hardware type, needs swapping */

typedef struct plummer {
	DLAYER d; 				/* data link layer packet header */

	uint16 hrd,			/* hardware type, Ethernet = 1 */
		pro;			/* protocol type to resolve for */
	uint8 hln,			/* byte length of hardware addr = 6 for ETNET */
		pln;			/* byte length of protocol = 4 for IP */
	uint16 op;			/* opcode, request = 1, reply = 2, RARP = 3,4 */
	uint8 sha[DADDLEN],
		spa[4],
		tha[DADDLEN],
		tpa[4];
/*
*   the final four fields (contained in 'rest') are:
*	  sender hardware address:   sha	   hln bytes
*	  sender protocol address:   spa	   pln bytes
*	  target hardware address:   tha	   hln bytes
*	  target protocol address:   tpa	   pln bytes
*/
}ARPKT;

/***********************************************************************/
/*  ARP cache
*   Data structure for saving low-level information until needed
*/
struct acache {
	uint8 hrd[DADDLEN],			/* hardware address for this IP address */
		ip[4],					/* the IP # in question */
		gate;					/* is this a gateway? */
	int32 tm;					/* time information */
};

/***********************************************************************/
/*   Internet protocol
*
*/
typedef struct iph {
        uint8 versionandhdrlen;
							/* I prefer to OR them myself */
							/* each half is four bits */
	uint8 service;			/* type of service for IP */
	uint16 tlen,				/* total length of IP packet */
		ident,				/* these are all BYTE-SWAPPED! */
		frags;				/* combination of flags and value */
	uint8 ttl,				/* time to live */
		protocol;			/* higher level protocol type */
	uint16 check;				/* header checksum, byte-swapped */
	uint8 ipsource[4],		/* IP addresses */
		ipdest[4];
}IPLAYER;

/*  
*  full IP packet, with data and ip header
*/
typedef struct ip {
	DLAYER d;
	IPLAYER i;
	union {
        uint8 data[556];            /* largest recommended, may include options */
		uint8 options[40];
	} x;
}IPKT;

#define PROTUDP		17
#define PROTTCP		6		/* standard protocol types for IP */
#define PROTICMP	1

/************************************************************************/
/* ICMP packet
*  all of them are of a similar form, some generic fields are spec'd here.
*/
typedef struct icmph {
	uint8 type,				/* ICMP type field */
		code;				/* ICMP code field */
	uint16 check,			  /* checksum */
		part1,part2;		/* depends on type and code */
}ICMPLAYER;

typedef struct icmp {
	DLAYER d;
	IPLAYER i;
	ICMPLAYER c;
	uint8 data[ICMPMAX];
}ICMPKT;

/**************************************************************************/
/*  TCP protocol
*	  define both headers required and create a data type for a typical
*	  outgoing TCP packet (with IP header)
*   
*  Note:  So far, there is no way to handle IP options fields
*	which are associated with a TCP packet.  They are mutually exclusive
*	for both receiving and sending.  Support may be added later.
*
*   The tcph and iph structures can be included in many different types of
*   arbitrary data structures and will be the basis for generic send and
*   receive subroutines later.  For now, the packet structures are optimized 
*   for packets with no options fields.  (seems to be almost all of them from
*   what I've observed.
*/

typedef struct tcph {
	uint16 source,dest;			/* TCP port numbers, all byte-swapped */
	uint32 seq,ack;				/* sequence, ACK numbers */
    uint8 hlen,                 /* length of TCP header in 4 byte words */
		flags;					/* flag fields */
    uint16 window,              /* advertised window, byte-swapped */
		check,					/* TCP checksum of whole packet */
		urgent;					/* urgent pointer, when flag is set */
}TCPLAYER;

/*
*  used for computing checksums in TCP
*/
struct pseudotcp {
    uint8 source[4],dest[4],    /* IP #'s for source,dest */
		z,proto;				/* zero and protocol number */
    uint16 tcplen;              /* byte-swapped length field */
};

typedef struct tcp {
	DLAYER d;
	IPLAYER i;
	TCPLAYER t;

	union {
		uint8 options[40];		/* not very likely, except on SYN */
		uint8 data[TMAXSIZE];	/* largest TCP data we will use */
	} x;
}TCPKT;

/* 
*  flag field definitions, first two bits undefined
*/

#define TURG	0x20
#define TACK	0x10
#define TPUSH	0x08
#define TRESET	0x04
#define TSYN	0x02
#define TFIN	0x01
/*************************************************************************/
/*   TCP queuing
*   data types for all TCP queuing operations
*   Each open port will have one of these structures assigned to it.
*/

struct window {
	uint32 nxt,				/* sequence number, not byte-swapped */
		ack;				/* what the other machine acked */
	int32 lasttime;			/* (signed) used for timeout checking */
	uint8 where[WINDOWSIZE],	/* storage for queue */
		*endbuf,			/* set to end of queue */
		*base,				/* where useful data is in queue */
		*endlim,			/* first spot in queue to add more data */
		push;				/* flag for TCP push */
	uint size,				/* size of window advertised */
		port,				/* port numbers from one host or another */
		contain;			/* how many bytes in queue? */
};

struct port {
	struct window in,out;
	TCPKT tcpout;				/* pre-initialized as much as possible */
	uint8 state;				/* connection state */
	struct pseudotcp tcps;		/* pseudo-tcp for checksumming */
	int credit,					/* choked-down window for fast hosts */
		sendsize,				/* MTU value for this connection */
		rto;					/* retrans timeout */
};

/*************************************************************************/
/*  TCP states
*	 each connection has an associated state in the connection flow.
*	 the order of the states now matters, those less than a certain
*	 number are the "inactive" states.
*/
#define SCLOSED 	1
#define SLISTEN 	2
#define STWAIT		3
#define SSYNR   	4
#define SSYNS		5
#define SEST		6
#define SFW1		7
#define SFW2		8
#define SCLOSING 	9
#define SCWAIT      10
#define SLAST       11

/*
*	 services which we will want to use
*/
#define HFTP		21
#define HTELNET		23
#define HNAME		42
#define HSUNRPC		111
#define HPRINTER 	515

/*************************************************************************/
/*  UDP
*   User Datagram Protocol
*   Each packet is an independent datagram, no sequencing information
*
*   UDP uses the identical checksum to TCP
*/

typedef struct udph {
	uint16 source,
		dest;					/* port numbers, all byte-swapped */
	uint16 length,				/* length of packet, including hdr */
		check;					/* TCP checksum of whole packet */
}UDPLAYER;

typedef struct udp {
	DLAYER d;
	IPLAYER i;
	UDPLAYER u;
	uint8 data[UMAXLEN];	  /* largest UDP data we will use */
}UDPKT;

struct uport {
	UDPKT udpout;
	struct pseudotcp tcps;		/* pseudo-tcp for checksumming */
	uint16 listen,				/* what port should this one listen to? */
		length;					/* how much data arrived in last packet? */
	uint8 data[UMAXLEN],		/* incoming, last datagram of that type */
		who[4],					/* who sent it to me ? */
                stale,                                  /* have we read this packet yet? */
                start_index,
                end_index;

};

/*************************************************************************/
/*  event queue
*   records what happens, especially errors, and keeps them for any
*   routines that poll, looking for what happened.
*   Eight event classes are masked in the event class byte.
*	There can be 256 event types per class.
*   The data field is handled differently by each event type.
*/
struct eq {
	uint8 eclass,		/* class, defined in netevent.h */
		event;			/* which event */
	int next,			/* ordering for events in queue  */
		idata;			/* integer data, if you feel like it */
};

/*  
*  events which can occur and be placed into the event queue
*/
#define NEVENTS 50
/*  classes defined in netevent.h   */

#endif

