/*
*    Whatami.h
*  Machine specific information for NCSA Telnet TCP/IP kernel
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
****************************************************************************
*   Defines for TCP/IP library, are you a Mac or a PC?
*/

#ifndef WHATAMI_H
#define WHATAMI_H

#define PC 1
#define ETHER 1
#define UB 1 

/*
*   Defines which have to do with Ethernet addressing versus Appletalk
*   addressing.  Ethernet has 6 bytes of hardware address, ATALK has 4
*/
#ifdef ETHER
#define DADDLEN 	6
#define WINDOWSIZE 	4096
#define TSENDSIZE 	512
#define DEFWINDOW 	1024
#define DEFSEG		1024
#define TMAXSIZE 	1024
#define UMAXLEN 	1024
#define ICMPMAX 	300 
#else
/*  define for AppleTalk */
#define DADDLEN 	4
#define WINDOWSIZE 	4096
#define TSENDSIZE 	512
#define DEFWINDOW 	512
#define DEFSEG		512
#define TMAXSIZE 	512
#define UMAXLEN 	512
#define ICMPMAX 	300 
#endif

/*
*  define length of an integer - can be 16 or 32, we need to know which
*/
typedef char int8;
typedef unsigned char uint8;
typedef unsigned char byte;
typedef int int16;
typedef unsigned int uint16;
typedef unsigned int uint;
typedef long int int32;
typedef unsigned long int uint32;

#define TICKSPERSEC 	18
#define SMINRTO 		5
#define WRAPTIME 		86400L			/* in seconds, only for PC */
#define NPORTS			30
#define NUDPPORTS               3
#define CONNWAITTIME	20				/* default contime in seconds */
#define KIP nnkip
#define NFDEF 2
#define NBDEF 0
#define BFDEF 0
#define BBDEF 2
#define UFDEF 1
#define UBDEF 0

/*
*  Events for event processing in NCSA Telnet.
*  Used for netgetevent().
*/
#define USERCLASS	1		/* the user program will accept these events */
#define ICMPCLASS	2		/* ICMP in netsleep will look for these */
#define ERRCLASS    4		/* the user may or may not read these error messages */
#define SCLASS		8		/* the background server will take these */
#define CONCLASS    0x10	/* the application manages connections with these */
#define ERR1		1		/* an error message is waiting, ERRCLASS */
#define IREDIR		1		/* ICMP redirect, ICMPCLASS */
#define CONOPEN 	1		/* connection has opened, CONCLASS */
#define CONDATA 	2       /* there is data available on this connection */
#define CONCLOSE 	3		/* the other side has closed its side of the connection */
#define CONFAIL 	4		/* connection open attempt has failed */
#define UDPDATA 	1		/* UDP data has arrived on listening port, USERCLASS */
#define DOMOK		2		/* domain name ready */
#define DOMFAIL 	3		/* domain name lookup failed */
#define FTPCOPEN 	20     	/* FTP command connection has opened */
#define FTPCLOSE 	21     	/* FTP command connection has closed */
#define FTPBEGIN 	22     	/* FTP transfer beginning, dat =1 for get, 0 for put */
#define FTPEND   	23     	/* FTP transfer ending */
#define FTPLIST  	24     	/* FTP file listing taking place */
#define FTPUSER  	25     	/* FTP user name has been entered */
#define FTPANON         36      /* FTP anon session begun */
#define FTPPWOK         26              /* FTP password verified */
#define FTPPWSK1        32              /* FTP password skipped */
#define FTPPWSK2        35              /* FTP password skipped */
#define FTPPWWT         34              /* FTP write access */
#define FTPPWRT         33              /* FTP root access gained */
#define FTPPWNO         27              /* FTP password failed */
#define RCPBEGIN 	30		/* RCP beginning */
#define RCPEND 		31		/* RCP ending */
#define UDPTO 		1		/* UDP request from DOMAIN timed out, SCLASS */
#define FTPACT 		2		/* FTP transfer is active, keep sending */
#define TCPTO  		3		/* TCP for DOMAIN timed out */
#define RCPACT 		4		/* rcp is active, needs CPU time */
#define RETRYCON 	5		/* retry connection packet, might be lost */
#define CLOSEDONE 	6		/* Close completion routine has been called (for Mac Drivers only) */

#endif
