/*
 * 	Part of NCSA telnet
 *
 *	These routines are needed by the utilitites other than telnet
 *	to avoid unresolved references.  They may be expanded for
 * 	more funcionatity at a later date.
 *
 *	New - 6/27/91	Jeff Wiedemeier
 */

#include <stdio.h>
#include "whatami.h"
#include "windat.h"
#include "externs.h"

/*
 * The following routines are to satisfy look.c and memdebug.c
 */

struct twin fool_them, *console=&fool_them;

void vprint(int w,char *s)
{
	w=w;	/* No warning this way */
#ifdef DEBUG	/* without debug - ignore messages */
	fprintf(stderr, s);
#else
	s = s;	/* no warning */
#endif /* DEBUG */
}

int tprintf(int w,char *s,...)
{
	w=w;	/* No warning this way */
#ifdef DEBUG	/* without debug - ignore messages */
	fprintf(stderr, s);
#else
	s = s;	/* no warning */
#endif /* DEBUG */
    return(0);
}

void inv_port_err(int service, int port, uint8 *ip)
{
	service=service;
	port=port;
	ip=ip;
}

