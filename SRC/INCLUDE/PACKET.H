/*
*    packet.h
*  Used for packet driver code
****************************************************************************
*                                                                          *
*                                                                          *
*      NCSA Telnet                                                         *
*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
*			Additions by Kurt Mahan, Heeren Pathak, & Quincey Koziol	   *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
****************************************************************************
*	Quincey Koziol
*   Defines for packet driver variables
*/

#ifndef PACKET_H
#define PACKET_H

#ifndef PACKET_MASTER
extern
#endif
int packet_vector           /* packet vector found */
#ifdef PACKET_MASTER
=0     /* non-zero if we found it */
#endif
,
    ip_handle           /* handle to use for IP packets */
#ifdef PACKET_MASTER
    =(-1)
#endif
,
    arp_handle            /* handle to use for ARP packets */
#ifdef PACKET_MASTER
    =(-1)
#endif
    ,
    rarp_handle           /* handle to use for RARP packets */
#ifdef PACKET_MASTER
    =(-1)
#endif
    ;

/* These variables are defined in packet2.asm */
extern int driver_version,  /* version of the driver */
    driver_type;            /* type of the driver */
extern char driver_class,   /* class of the driver */
    driver_number;          /* number of the driver */

/* Packet driver classes */
#define IC_ANY          0
#define IC_ETHERNET     1
#define IC_PRONET10     2
#define IC_PRONET4      3
#define IC_OMNINET      4
#define IC_APPLETALK    5
#define IC_SLIP         6
#define IC_STARLAN      7
#define IC_ARCNET       8
#define IC_X25          9
#define IC_KISS         10
#define IC_IEEE802      11
#define IC_FDDI         12
#define IC_INTERNETX25  13
#define IC_LANSTAR      14

/* Packet driver types */
#define IT_ANY      0xFFFF

#ifndef PACKET_MASTER
extern
#endif
char iptype[]={0x08,0x00},
    arptype[]={0x08,0x06},
    rarptype[]={0x80,0x35};
#define IPLEN       2
#define ARPLEN      2
#define RARPLEN     2

#endif  /* packet.h */

