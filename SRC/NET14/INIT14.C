#ifdef __TURBOC__
#include "turboc.h"
#endif
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <dos.h>
#include <ctype.h>
#include <errno.h>
#ifdef MSC
#include <direct.h>
#include <signal.h>
#include <time.h>
#endif


#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif
#include "netevent.h"
#include "hostform.h"
#include "whatami.h"
#include "version.h"
#include "externs.h"

/* #define DEBUG */

/*
*	Global variables
*/
extern unsigned char buf[256];
extern unsigned char myipnum[4];           /* the ip number for this machine */

#define SERIAL  0x14
#define NUM_COMM_PORTS  4   /* the number of comm. ports supported, remember to change this variable in int14.asm also */

extern struct config def;   /* Default settings obtained from host file */
#ifdef QAK

extern unsigned char initialized_flags;     /* flags indicating whether a port has been initialized */
extern unsigned char connected_flags;       /* flags indicating whether a port is connected yet */
extern unsigned char opening_flags;         /* flags indicating whether a port is negotiating an open connection */
extern unsigned char port_buffer[NUM_COMM_PORTS][64];    /* four buffers to store the machine name to connect to */
extern unsigned char buffer_offset[NUM_COMM_PORTS];      /* the offset into the buffer currently */
extern int pnum[NUM_COMM_PORTS];                         /* the port number we are connected to */

#define PORT_DATA_SIZE      2048        /* this is sort of a guess, might need larger */
#endif

/*
*   int14init
*
*   Entry :  none
*
*/
int int14init(void)
{
#ifdef QAK
    config = (getenv("CONFIG.TEL"));    /* check for a config.tel in the environment */
    if(config)                  /* set a different config.tel file */
        Shostfile(config);
#endif

    puts("National Center for Supercomputing Applications");    /* put the banner on the screen */
    puts("Interrupt 14h driver");
    puts(N14_VERSION);

	if(Snetinit()) {			/* call session initialization */
#ifdef QAK
		errhandle();			/* Snetinit() reads config.tel file */
#else
        puts("Snetinit() failed");
#endif
        return(0);
	  }
    netgetip(myipnum);          /* get my IP number (in case of BOOTP or RARP) */
    Sgetconfig(&def);       /* get information provided in hosts file */

/*
*  Display my Ethernet (or Appletelk) and IP address for the curious people
*/
    pcgetaddr(&buf[200],def.address,def.ioaddr);
    printf("My Ethernet address: %x:%x:%x:%x:%x:%x\r\n",buf[200],buf[201],buf[202],buf[203],buf[204],buf[205]);
    printf("My IP address: %d.%d.%d.%d\r\n\n",myipnum[0],myipnum[1],myipnum[2],myipnum[3]);

    Stask();                    /* any packets for me? (return ARPs) */
    return(1);      /* inidicate sucessful initialization */
}   /* end int14init() */

