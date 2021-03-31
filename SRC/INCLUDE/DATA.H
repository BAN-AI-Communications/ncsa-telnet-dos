/*
*    data.h
*   Declarations of global variables for TCP/IP libraries
*
****************************************************************************
*                                                                          *
*                                                                          *
*      NCSA Telnet                                                         *
*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
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
*/

#ifndef DATA_H
#define DATA_H

/*
*  Start with declarations that tell the difference between PC and other
*  computers
*/

#include "pcdefs.h"

#ifdef MASTERDEF
unsigned char
#ifdef NOT_USED
	copyr[]=
{"Copyright (c) 1987 by the Board of Trustees of the University of Illinois"},
	us[]= 
{"National Center for Supercomputing Applications -- TCP/IP by Tim Krauskopf"},
#endif
    nnmyaddr[DADDLEN],      /*  my ethernet hardware address */
	broadaddr[DADDLEN],		/*  the broadcast address */	
	nnipnum[4],				/*  my ip number */
#ifdef NOT_USED
	nnredir=0,				/*  flag indicating need for redirect */
#endif
	nnicmpsave[4],			/*  address for icmp redirect */
	nnicmpnew[4],			/*  new gateway from icmp redirect */
	nnmask[4]=	{0,0,0,0},	/*  the default subnet mask */
	nnamask[4]=	{255,0,0,0},		/* class A mask */
	nnbmask[4]=	{255,255,0,0}, 		/* class B mask */
	nncmask[4]= {255,255,255,0},	/* class C mask */
	broadip[4]= {0xff,0xff,0xff,0xff};
int
#ifndef NET14
    numline=23,                     /* number of line on the screen-1 */
#endif
#ifndef QAK
	nnkip=0,						/*	use of KIP arping necessary (only for appletalk networks) */
#else
 	nnkip=1,						/*  use of KIP arping necessary */
#endif
	nnipident=1,					/*  ident field of outgoing ip packets*/
	nnefirst=0,						/* first entry in event q */
	nnelast=0,						/* last entry in event q */
	nnefree=0,						/* free list for event q */
#ifdef OLD_WAY
	nnemac=0,						/* Macintosh is using direct EtherTalk */
#endif
	nndto=DLAYTIMEOUT,				/* dlayertimeout */
	nnfromport=0,					/* can force a port number selection */
    nncredit=CREDIT                 /* limited window in some cases */
#if !defined NET14 || defined NETSPACE || defined WIN
    ,nnsegsize=MAXSEG               /* maximum segment size  (intswapped) */
#endif
    ;               

struct port *portlist[NPORTS];		/* allocate like iobuffers in UNIX */
struct uport *udplist[NPORTS];                                     /* buffer for UDP */
struct pseudotcp tcps;				/* for checksums */
struct acache arpc[CACHELEN];		/* cache for hardware addresses */
struct eq nnq[NEVENTS];				/* event queue */

ARPKT arp;
DLAYER blankd;
IPKT blankip;
/*ICMPKT blankicmp;*/
#else
extern unsigned char
	nnmyaddr[DADDLEN],		/*  my ethernet hardware address */
	broadaddr[DADDLEN],		/*  the broadcast address */
	broadip[4],
	nnipnum[4],
#ifdef NOT_USED
	nnredir,				/*  flag indicating need for redirect */
#endif
	nnicmpsave[4],			/*  address for icmp redirect */
	nnicmpnew[4],			/*  new gateway from icmp redirect */
	nnmask[4],
	nnamask[4],
	nnbmask[4],
	nncmask[4];
extern int
#ifndef NET14
    numline,                /* number of screen lines */
#endif
 	nnkip,					/* using KIP? */
	nnipident,				/*  ident field of ip */
	nnefirst,				/* first entry in event q */
	nnelast,				/* last entry in event q */
	nndto,					/* dlayertimeout */
	nnefree,
	nnemac,					/* Macintosh is using direct EtherTalk */
	nnfromport,				/* can force a port number selection */
    nncredit
#ifndef NET14
    ,nnsegsize              /* maximum segment size */
#endif
    ;

extern struct port *portlist[NPORTS];		/* allocate like iobuffers in UNIX */
extern struct uport *udplist[NUDPPORTS];           /* buffer for UDP */
extern struct pseudotcp tcps;				/* for checksums */
extern struct acache arpc[CACHELEN];		/* cache for hardware addresses */
extern struct eq nnq[NEVENTS];				/* event queue */
extern ARPKT arp;
extern DLAYER blankd;
extern IPKT blankip;
/*extern ICMPKT blankicmp;*/

#endif
/*
*   defines of constants and macros that everyone needs to know
*/

#define nnerror(A)  netposterr(A)

#endif

