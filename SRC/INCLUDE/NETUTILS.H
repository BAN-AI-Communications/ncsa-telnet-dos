/*
	netutils.h
	Networking utilities include file for use with NCSA 2.3

	By James Nau, College of Engineering,
	University of Nebraska--Lincoln
*/

#ifndef NETUTILS_H
#define NETUTILS_H

#define ALARM 128


extern struct machinfo *gethostinfo(char *hostinfo);
extern int connect_sock(struct machinfo *machine, int source_port,int dest_port);

#endif  /* NETUTILS_H */
