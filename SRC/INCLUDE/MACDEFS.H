#ifndef MACDEFS_H
#define MACDEFS_H

/*
*	Raw is the buffer pointer
*
*/

#ifdef MASTERDEF
char *raw;
#else
extern char *raw;
#endif
/*
*   defined in assembly language file for interrupt driven Ether buffering
*
*/
/*MAC: this is in my structure */
struct BUFREC {
	char *bufpt, 
	*bufread,
	*buforg,
	*bufend;
	short bufbig,
	buflim;
};

#ifdef MASTERDEF
struct BUFREC bufinfo;
#else
extern struct BUFREC bufinfo;
#endif
/*
*  hardware address for Ethernet broadcast address (used for ARP)
*/
#ifdef MASTERDEF
unsigned char bseed[]={0xff,0xff,0xff,0xff,0xff,0xff};
#else
extern unsigned char bseed[];
#endif
/*
*  timeout for response to ARP packet for Ethernet
*/
#define DLAYTIMEOUT 15*60
/*
*  timeout for opening of a TCP connection
*    1. number of times to send a copy of the first packet
*    2. number of seconds to wait between each send of packet
*/
/*
*  how often to poke a TCP connection to keep it alive and make
*  sure other side hasn't crashed.
*/
/*#define RETRANSINTERVAL 3		 this is the only retrans cause , keep it small */
/*#define POKEINTERVAL 200		 this really doesn't matter because everyone else*/
								/* is doing funny things with the spec! */
								/* I vote we reset the suckers! */
#define POKEINTERVAL 200*60
#define MAXRTO  	 600
#define MINRTO 		 15
#define	CACHETO		 21000			/* Wow long to cache the ARPS */
#define	ARPTO		 90				/* How long to wait  for arps */
#define WAITTIME 	 35
#define LASTTIME 	 12000
#define CACHELEN 	 10
/* #ifdef ETHER					CHANGE - how to handle this ? */
#define MAXSEG 0x0400
/*								#else
								#define MAXSEG 0x0200
								#endif
*/
#define CREDIT 4096
#ifdef MPW
pascal long TickCount()
	extern 0xA975;
#define time(a)	TickCount()
#else
pascal long	TickCt()=0xa975;
#define time(a)	TickCt()
#endif 
extern int KIP;					/* are we kipping ??? */

#endif
