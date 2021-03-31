/*
*	DOMAIN.H
*
*	Domain processing header file
*
*****************************************************************************
*																			*
*	 part of:																*
*	 TCP/IP kernel for NCSA Telnet											*
*	 by Tim Krauskopf														*
*																			*
*	 National Center for Supercomputing Applications						*
*	 152 Computing Applications Building									*
*	 605 E. Springfield Ave.												*
*	 Champaign, IL  61820													*
*																			*
*****************************************************************************
*
*	Revision history:
*
*	5/90	created for greater portability		QAK
*
*/

#ifndef DOMAIN_H
#define DOMAIN_H

/*
*  special domain data structures
*/

#define DOMSIZE 512			/* maximum domain message size to mess with */

/*
*  Header for the DOMAIN queries
*  ALL OF THESE ARE BYTE SWAPPED QUANTITIES!
*
*/
struct dhead {
	uint16	ident,			/* unique identifier */
		flags,
		qdcount,			/* question section, # of entries */
		ancount,			/* answers, how many */
		nscount,			/* count of name server RRs */
		arcount;			/* number of "additional" records */
};

/*
 *  flag masks for the flags field of the DOMAIN header
 */
#define DQR			0x8000			/* query=0, response=1 */
#define DOPCODE		0x7100			/* opcode, see below */
#define DAA			0x0400			/* Authoritative answer */
#define DTC			0x0200			/* Truncation, response was cut off at 512 */
#define DRD			0x0100			/* Recursion desired */
#define DRA			0x0080			/* Recursion available */
#define DRCODE		0x000F			/* response code, see below */

/* opcode possible values: */
#define DOPQUERY	0			/* a standard query */
#define DOPIQ		1			/* an inverse query */
#define DOPCQM		2			/* a completion query, multiple reply */
#define DOPCQU		3	 		/* a completion query, single reply */

/* the rest reserved for future */
/* legal response codes: */
#define DROK		0				/* okay response */
#define DRFORM		1				/* format error */
#define DRFAIL		2				/* their problem, server failed */
#define DRNAME		3				/* name error, we know name doesn't exist */
#define DRNOPE		4				/* no can do request */
#define DRNOWAY		5				/* name server refusing to do request */
#define DTYPEA		1				/* host address resource record (RR) */
#define DTYPEPTR	12				/* a domain name ptr */
#define DIN			1				/* ARPA internet class */
#define DWILD		255				/* wildcard for several of the classifications */

/*
 *  a resource record is made up of a compressed domain name followed by
 *  this structure.  All of these ints need to be byteswapped before use.
 */
struct rrpart {
	uint16	rtype,					/* resource record type=DTYPEA */
			rclass;					/* RR class=DIN */
	uint32	rttl;					/* time-to-live, changed to 32 bits */
	uint16	rdlength;				/* length of next field */
	uint8	rdata[DOMSIZE];			/* data field */
};

/*
*  data for domain name lookup
*/
#ifdef OLD_WAY
#ifndef DOMAINMASTER
extern
#endif
static
struct useek {
	struct dhead h;
	uint8 x[DOMSIZE];
} question;
#endif

#endif
