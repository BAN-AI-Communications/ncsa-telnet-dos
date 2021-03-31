/*
*	MINITEL.C
*
*   Example TCP/IP program for the NCSA TCP/IP kernel
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
*	10/87  Initial source release, Tim Krauskopf
*	2/88  typedefs of integer lengths, TK
*	5/88	clean up for 2.3 release, JKM	
*
*/

#define MINITEL
/*
*	Includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif
#ifdef __TURBOC__
#include "turboc.h"
#endif
#include "whatami.h"
#include "hostform.h"
#include "externs.h"

/*
*	Global variables
*/
int twperm=0;    /* whether to bypass the password check, not used */
unsigned char path_name[_MAX_DRIVE+_MAX_DIR],	/* character storage for the path name */
	temp_str[100],temp_data[100];		/* temporary character storage */
struct machinfo *mp;
char buf[256];
char *config;

/*
*	main ( argc, argv )
*
*	Entry : 
*
*		parameter 1 : machine name
*
*/
 
void main(argc,argv)
int argc;
char *argv[];
{
	int i,cnt,ev,pnum,what,dat;
	char *errmsg;
	char c;

#ifdef __TURBOC__
	fnsplit(argv[0],path_name,buf,temp_str,temp_data);
#else
    _splitpath(argv[0],path_name,buf,temp_str,temp_data);   /* split the full path name of telbin.exe into it's components */
#endif
	strcat(path_name,buf);	/* append the real path name to the drive specifier */

	config = (getenv("CONFIG.TEL"));
	if (config) Shostfile(config);

	puts("National Center for Supercomputing Applications");
	puts("Mini-telnet example program");
	puts("January 1989\n");
	if(argc<2)
		exit(1);
	if(Snetinit()) {			/* call session initialization */
		errhandle();			/* Snetinit() reads config.tel file */
		exit(1);
	  }
	mp=Sgethost(argv[1]);		/* look up in hosts cache */
	if(!mp)
		Sdomain(argv[1]);		/* not in hosts, try domain */
	else {
		if(0>(pnum=Snetopen(mp,23))) {
			errhandle();
			netshut();
			exit(1);
		  }
	  }
	c = 0;
	do {
		if(kbhit()){				/* has key been pressed */
			c=(char)getch();
			netwrite(pnum,&c,1);	/* user input sent on connection */
		  }
/*
*  get event from network, these two classes return all of the events
*  of user interest.
*/
		ev=Sgetevent(USERCLASS | CONCLASS | ERRCLASS,&what,&dat);
		if(!ev)
			continue;
		if(what==ERRCLASS) {				/* error event */
			errmsg=neterrstring(dat);
			puts(errmsg);
		  }
		else 
			if(what==CONCLASS) {		/* event of user interest */
				switch (ev) {
					case CONOPEN:				/* connection opened or closed */
						netpush(dat);			/* negotiation */
						netwrite(dat,"\377\375\001\377\375\003\377\374\030",9);
						break;

					default:
						break;

					case CONDATA:				/* data arrived for me */
						cnt=netread(dat,buf,80);
						for(i=0; i<cnt; i++) 
						if(buf[i]<128)
							putchar(buf[i]);
						break;

					case CONFAIL:
						puts("Connection attempt failed");
										/* drop through to exit */

					case CONCLOSE:
						netshut();
						exit(1);
				  }
			  }
			else 
				if(what==USERCLASS) {
					switch (ev) {
						case DOMOK:							/* domain worked */
							mp=Slooknum(dat);				/* get machine info */
							pnum=Snetopen(mp,23);			/* open to host name */
							break;

						case DOMFAIL:	/* domain failed */
							n_puts("domain failed");
							netshut();
							exit(1);
							default:

						break;
					  }
				  }
	  }while(c!=16);			/* Ctrl-P, arbitrary escape */
	netclose(pnum);
	netshut();
	exit(0);
}

/*
*	errhandle ()
*
*	Write error messages to the console window
*
*/
void errhandle(void )
{
	char *errmsg;
	int i,j;

	while(ERR1==Sgetevent(ERRCLASS,&i,&j)) {
		errmsg=neterrstring(j);
		puts(errmsg);
	  }
}
