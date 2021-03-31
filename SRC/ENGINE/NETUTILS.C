/*
	netutils.c
	Networking utilities for use with NCSA 2.3

	By James Nau, College of Engineering,
	University of Nebraska--Lincoln
*/

#include <stdio.h>
#include <stdlib.h>

#include "netevent.h"
#include "hostform.h"
#include "externs.h"

#include "netutils.h"
/*
extern struct machinfo *gethostinfo(char *hostinfo);
extern int connect_sock(struct *machine, int source_port, int dest_port);
*/

extern int debug;  /* debug variable */

/* gethostinfo() is designed to take either a hostname, or a
   hostip (dotted form), and return a machine structure */

struct machinfo *gethostinfo(char *hostinfo)
{
   struct machinfo *mach_info;   /* temp location for the machine info */
   int mach_number;     /* unique machine number in domain lookup */
   int theclass, dat;   /* for use with Sgetevent() */

    if(debug)
        printf("gethostinfo: gethostinfo(%s)\n", hostinfo);

/* first, try to look up the name in the predefined host cache */

   mach_info=Sgethost(hostinfo);

    if(debug) {
        if(mach_info)
            printf("gethostinfo: Sgethost(%s) was successful\n",hostinfo);
        else
            printf("gethostinfo: Sgethost(%s) was NOT sucessful\n",hostinfo);
      } /* end if */
    if(mach_info)
        return (mach_info); /* We found it in cache */

/* failing lookup up in local cache, do a domain query */
/* domain queries in NCSA have to be done with events, so, set it up
   and wait for the event to happen (Good or Bad) */

   if((mach_number=Sdomain(hostinfo)) < 0) {    /* no nameservers */
      printf("No NameServer(s) defined in network startup file\n");
      return((struct machinfo *)NULL);
   }

/* keep looping until the event occurs */ 
    while (!mach_info) {
        switch(Sgetevent(USERCLASS,&theclass,&dat)) {
            case DOMFAIL:
                return((struct machinfo *)NULL);   /* not found */
            case DOMOK:
                mach_info=Slooknum(mach_number);    /* got it */
          } /* end switch */
      } /* end while */

   if (debug) {
      printf("gethostinfo: domain lookup successful\n");
      printf("gethostinfo: theclass [%d], dat [%d]\n", theclass, dat);
   }

   return (mach_info);  /* send the structure back */
}


#ifndef NET14
/* connect_sock() is designed to open a connection (~socket) to
   a machine that has been looked up with gethostinfo.
*/
int connect_sock(struct machinfo *machine, int source_port, int dest_port)
{
    int connect_id;      /* connection id for netread, netwrite /(return value) */
    int theclass,dat;   /* for Sgetevent */
    int event;

   /* set source port as required by NCSA (so it's not Telnet port ?) */
    netfromport(source_port);
    if(debug)
        printf("connect_sock: from_port set to %d\n",source_port);

/* Try to connect */
    if((connect_id=Snetopen(machine,dest_port))<0) { /* assuming id's should be greater than 0, like U*x? */
        printf("connect_sock: Snetopen return value %d", connect_id);
        return(-1);
     }

    if(debug)
        printf("connect_sock: Snetopen(machine, %d) ok\n",dest_port);

/*   while (Sgetevent(CONCLASS, &theclass, &dat) != (CONOK)) */
    while(1) {
        if((event=Sgetevent(CONCLASS, &theclass, &dat))!=0) {
            if(connect_id!=dat) {
            /* put it back, it's not our problem */
/* do I need to do this */
/*            netputevent(theclass, event, dat); */
              }
            else {
                if(event==CONOPEN) {
                    if(debug)
                        printf("connect_sock: CONOPEN\n");
                    break;   /* how far does break break out? */
                  } /* end if */
                else {
                    printf("received event [%d]\n",event);
                    return(-1);
                  } /* end else */
              } /* end else */
          } /* end if */
      } /* end while */

    if(debug)
        printf("connect_sock: Connection opened [%d]\n",connect_id);
    return(connect_id);  /* send back connection id, that's why we're here */
}
#endif

