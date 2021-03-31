/*
 *	UDP.C
 *
 *	UDP Protocol Routines
 *
 ***************************************************************************
 *                                                                         *
 *     part of:                                                            *
 *     TCP/IP kernel for NCSA Telnet                                       *
 *     by Tim Krauskopf                                                    *
 *                                                                         *
 *     National Center for Supercomputing Applications                     *
 *     152 Computing Applications Building                                 *
 *     605 E. Springfield Ave.                                             *
 *     Champaign, IL  61820                                                *
 *                                                                         *
 ***************************************************************************
 *
 *	Revision history:
 *
 *	5/88	split out of ip.c for 2.3 release, JKM
 *
 */

/*
 *	Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <time.h>
#include <string.h>
#include <graph.h>
#include <conio.h>

#include "protocol.h"
#include "data.h"
#include "externs.h"

/* 
 *	udpinterpret ( p, ulen )
 *
 *	Take an incoming UDP packet and make the available to the user level 
 * routines.  Currently keeps the last packet coming in to a port.
 *
 *	Limitations :
 *
 * Can only listen to one UDP port at a time, only saves the last packet
 * received on that port.  Port numbers should be assigned like TCP ports.
 *
 * NB:
 *
 * The above limitation has been removed.  You are now required to pass not
 * the udp port number, but the index of the uport structure.  This is similar
 * to the tcp calls where one passes the index of the tcp structure.
 *
 * In the past, Telnet dealt with udp stuff by creating a global variable
 * known as ulist.  Ulist was used exclusively for domain name resolution.
 * At the time, this was adequete, but with the addition of sockets, the
 * code had to be changed to support multiple udp structures.
 *
 * To keep from creating new bugs, ulist gets the first entry in the udplist
 * which is position 0.  This is allocated back in protinit.c.  Its under
 * naturally enough the udp initialization procedure.
 *
 * I've discovered a most ugly "feature" in Telnet.  The port number 997 is
 * used for address resolution and is hard wired in.  Not pound defined.
 * Bad programmer...
 *              - eej 7/2/92
 *
 */

int udpinterpret(UDPKT *p,int ulen)
{
    uint    hischeck,mycheck;
    int     udpindex;

/*
 * Okay, we have been given a packet to interpret.  The first goal is to
 * see if any of the active ports in the udplist would like to have it.
 */
    for(udpindex=0;udpindex<NUDPPORTS+1;udpindex++)
        if(intswap(p->u.dest)==udplist[udpindex]->listen)
            break;

    if (udpindex>NUDPPORTS)    /* Does that data belong to no one? */ {
        return(-1); }

/*
 *  first compute the checksum to see if it is a valid packet
 */
	hischeck=p->u.check;
	p->u.check=0;
	if (hischeck) {
		movebytes(tcps.source,p->i.ipsource,8);
		tcps.z=0;
		tcps.proto=p->i.protocol;
		tcps.tcplen=intswap(ulen);
		mycheck=tcpcheck((char *)&tcps,(char *)&p->u,ulen);
		if(hischeck!=mycheck) {
			netposterr(700);
			return(2);
		  }
		p->u.check=hischeck;					/* put it back */
	  }
	ulen-=8;						/* account for header */
	if(ulen>UMAXLEN)				/* most data that we can accept */
        ulen=UMAXLEN;
    movebytes(udplist[udpindex]->who,p->i.ipsource,4);  /* eej */
    /**** Copy the new data from p->data to our udp structure's buffer ****/
    movebytes(&(udplist[udpindex]->data[udplist[udpindex]->end_index]),p->data,ulen); /* eej */
    /**** Update the length of the buffer, and the end index ****/
    udplist[udpindex]->length=(udplist[udpindex]->end_index - udplist[udpindex]->start_index) + ulen; /* eej */
    udplist[udpindex]->end_index = udplist[udpindex]->length + udplist[udpindex]->start_index; /* eej */
    udplist[udpindex]->stale=0;
    netputuev(USERCLASS,UDPDATA,udplist[udpindex]->listen);      /* post that it is here */
	return(0);
}

/*
 * neturead ( buffer, udpindex, nbytes )
 *
 *	Get the data from the UDP buffer and transfer it into your buffer.
 *
 *	Returns the number of bytes transferred or -1 of none available
 *
 *  Support for multiple ports added.  See "udpinterpet" up above - eej
 *
 */

int neturead(char *buffer,int udpindex,unsigned int nbytes)
{

    if (udplist[udpindex] == NULL)
        return(-1);

    if (udplist[udpindex]->stale)
        return(-1);

    if (nbytes > (udplist[udpindex]->length))
        {
        nbytes = udplist[udpindex]->length;
        }

    if((udplist[udpindex] == NULL) || (udplist[udpindex]->stale))
        return(-1);
    movebytes(buffer,&(udplist[udpindex]->data[udplist[udpindex]->start_index]),nbytes); /* eej */
    udplist[udpindex]->start_index = udplist[udpindex]->start_index + nbytes; /* eej */
    udplist[udpindex]->length = udplist[udpindex]->length - nbytes; /* eej */
    if (udplist[udpindex]->start_index >= udplist[udpindex]->end_index)
        {
        udplist[udpindex]->start_index = 0;
        udplist[udpindex]->end_index = 0;
        udplist[udpindex]->length = 0;
        udplist[udpindex]->stale = 1;
        }
    return((int) nbytes);
}

/*
 *  netulisten ( port, udpindex )
 *
 *	Specify which UDP port number to listen to -- can only listen to one port
 * at a time.
 *
 * The above restriction has been removed.  You must pass in the udpindex.
 * See udpinterpret for details -- eej 7/2/92
 *
 */

int netulisten(int port, int udpindex)
{
    if (udplist[udpindex] == NULL) {
        return(-1);
        }
    udplist[udpindex]->listen=port;
}

/*
 *  netusend ( machine, port, retport, buffer, n, udpindex )
 *
 *	Send some data out in a udp packet ( uses the preinitialized data in the
 *  port packet *udplist[udpindex]->udpout* )
 *
 *	Returns 0 on ok send, non-zero for an error
 *
 *  Multiple udp ports have been added.  See udpinterpret, way at the top,
 *  for details on the changes.  eej 7/2/92
 *
 */

int netusend(uint8 *machine,uint16 port,uint16 retport,uint8 *buffer,int n,int udpindex)
{
    unsigned char   *pc;

	if(n>UMAXLEN)
		n=UMAXLEN;

/*
 *  make sure that we have the right dlayer address
 */
    if (udplist[udpindex] == NULL) {
        return(-1);
        }

    if(!comparen(machine,udplist[udpindex]->udpout.i.ipdest,4)) {
		pc=netdlayer(machine);
		if(pc==NULL) 
			return(-2);
        movebytes(udplist[udpindex]->udpout.d.dest,pc,DADDLEN);
        movebytes(udplist[udpindex]->udpout.i.ipdest,machine,4);
        movebytes(udplist[udpindex]->tcps.dest,machine,4);
	  }
    udplist[udpindex]->udpout.u.dest=intswap(port);
    udplist[udpindex]->udpout.u.source=intswap(retport);
    udplist[udpindex]->tcps.tcplen=udplist[udpindex]->udpout.u.length=intswap(n+sizeof(UDPLAYER));
    movenbytes(udplist[udpindex]->udpout.data,buffer,n);
/*
 *  put in checksum
 */
    udplist[udpindex]->udpout.u.check=0;
    udplist[udpindex]->udpout.u.check=tcpcheck((char *)&udplist[udpindex]->tcps,(char *)&udplist[udpindex]->udpout.u,n+sizeof(UDPLAYER));
/*
 *   iplayer for send
 */
    udplist[udpindex]->udpout.i.tlen=intswap(n+sizeof(IPLAYER)+sizeof(UDPLAYER));
    udplist[udpindex]->udpout.i.ident=intswap(nnipident++);
    udplist[udpindex]->udpout.i.check=0;
    udplist[udpindex]->udpout.i.check=ipcheck((char *)&udplist[udpindex]->udpout.i,10);
/*
 *  send it
 */
    return(dlayersend((DLAYER *)&udplist[udpindex]->udpout,sizeof(DLAYER)+sizeof(IPLAYER)+sizeof(UDPLAYER)+n));
}

/**
 ** newudp()
 **
 **  * Creates a new udp structure
 **  * The max number of structures is NUDPPORTS, and can be different from TCP.
 **
 **  This function was added, because in the past, only one udp structure
 **  was created, and if UDP sockets are created, more udp structures are
 **  needed.  This function needs to be called when a new udp port is
 **  required.
 **
 **  Keep in mind that Telnet will create at the first UDP structure in
 **  order to do name resolving.
 **
 **/

int     newudp()
{
    int     i;

    /* First find an open pointer in the udplist */

    for(i=0;i<NUDPPORTS+1;i++)
        if (udplist[i]==NULL)
            break;

    if (i>NUDPPORTS)
        return(-1);     /* There were no open pointers in the udplist */

    /* Initialize it */

    udplist[i] = (struct uport *) malloc(sizeof(struct uport));
    udplist[i]->stale=0;
    udplist[i]->length=0;

    movebytes(&udplist[i]->udpout,&blankip,sizeof(DLAYER)+sizeof(IPLAYER));
    udplist[i]->udpout.i.protocol=PROTUDP;                /* UDP type */
    udplist[i]->tcps.z=0;
    udplist[i]->tcps.proto=PROTUDP;
    movebytes(udplist[i]->tcps.source,nnipnum,4);
    udplist[i]->start_index = 0;
    udplist[i]->end_index = 0;
    return(i); /* Return the index to the udp structure */
}

