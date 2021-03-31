/*
*    kip.h
*	 by Gaige B. Paulsen
****************************************************************************
*                                                                          *
*      Uses	:															   *
*      TCP/IP kernel for NCSA Telnet                                       *
*      by Tim Krauskopf                                                    *
*	   with Macintosh code by Gaige B. Paulsen						  	   *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
*                                                                          *
****************************************************************************
*
*	KIP/Croft gateway structures.	
*
*/

#ifndef KIP_H
#define KIP_H

struct IPGP {
	long
		opcode,
		ipaddress,
		ipname,
		ipbroad,
		ipfile,
		ipother[4];
	char
		string[128];
	};

typedef struct IPGP IPGP;

#define ipgpAssign	1
#define ipgpName	2
#define ipgpServer	3
#define ipgpError	-1

#endif
