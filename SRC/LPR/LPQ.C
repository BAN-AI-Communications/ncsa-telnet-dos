/*  -------------------------------------------------------------------
    lpq - line printer queue

    Used to display the queue of printer jobs managed by an LPD daemon
    running on a remote machine.
    Built on top of the NCSA TCP/IP package (version 2.2tn for MS-DOS).

    Paul Hilchey   May 1989

    Copyright (C) 1989  The University of British Columbia
    All rights reserved.

    history:
    8/15/89    allow spaces after -P and -S options
    1/6/89     port to Microsoft C 5.1 by Heeren Pathak (NCSA)
    -------------------------------------------------------------------
*/

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <ctype.h>
#ifdef MSC
#include <signal.h>
#include <time.h>
#endif

#define WINMASTER

#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif
#include "netevent.h"
#include "hostform.h"
#include "whatami.h"
#include "windat.h"
#include "lp.h"
#include "externs.h"

int debug = 0;			/* enable with -D option */
int ftppassword,		/* not used; just to avoid unresolved external */
	bypass_passwd=0;	/* whether to bypass the password check */

unsigned char path_name[_MAX_DRIVE+_MAX_DIR],		/* character storage for the path name */
	temp_str[20],s[_MAX_DIR],temp_data[30];

#define DEFAULT_INTERVAL        10      /* 10 second interval for update */
#define REQUEST_BUFF_SIZE       200     /* size of buffer used to build request sent to the server */
#define JOB_AND_USER_BUFF_SIZE  150     /* size of buffer for job numbers and user names specified on the command line */

/* Function Prototypes */
void main(int argc, char *argv[]);
static void query_server(char *remote_host, char *request, int interval);
static void clrscr(void );
static void randomize(void );

/****************************************************************
 *  Main program.                                               *
 *     lpq  [option ...] [job# ...] [username ...]              *
 ****************************************************************/
void main(int argc, char *argv[])
{
    int i;  /* index for command line arguments */
    static char request[REQUEST_BUFF_SIZE] = "";

    /* buffer of the job numbers and user names specified on the command line */
    static char job_and_user_buff[JOB_AND_USER_BUFF_SIZE] = "";
    int job_and_user_buff_len = 0;

    char request_code = LPD_DISPLAY_SHORT_QUEUE;  /* short list is default */
    int  interval = 0;         /* delay interval for periodic update */
    char *ptr;

    char *remote_name,       /* printer name on remote system */
         *remote_host;       /* address of remote host        */

	_splitpath(argv[0],path_name,s,temp_str,temp_data);	/* split the full path name of telbin.exe into it's components */
	strcat(path_name,s);	/* append the real path name to the drive specifier */

#ifdef MSC
	signal(SIGINT,breakstop);		/* Microsoft intercept of break */
#else
    ctrlbrk(breakstop);     /* set up ctrl-c handler */
#endif


    /* Do session initialization.  Snetinit reads config file. */
    ptr = getenv("CONFIG.TEL");
    if (ptr != NULL) Shostfile(ptr);
    if(i=Snetinit()) {
		if(i==-2)		/* check for BOOTP server not responding */
			netshut();	/* release network */
		crash("network initialization failed.");
	  }	/* end if */

    /* get printer and server from environment variables */
    remote_name = getenv("PRINTER");
    if (remote_name == NULL) remote_name = DEFAULT_PRINTER;
    remote_host = getenv("SERVER");

    /*****************************************
       Loop through command line arguments
     *****************************************/
    for (i=1; i<argc; ++i)

        if (0 == strncmp(argv[i],"-P",2))          /* select printer */
            if (argv[i][2])
                remote_name = &argv[i][2];
            else if (i+1 < argc)
                remote_name = argv[++i];
            else ;

        else if (0 == strncmp(argv[i],"-S",2))      /* select server */
            if (argv[i][2])
                remote_host = &argv[i][2];
            else if (i+1 < argc)
                remote_host = argv[++i];
            else ;

        else if (0 == strncmp(argv[i],"-l",2))      /* long form */
            request_code = LPD_DISPLAY_LONG_QUEUE;

        else if (0 == strncmp(argv[i],"-D",2))      /* debug mode */
            debug = 1;

        else if (argv[i][0] == '+')  {              /* repeat interval */
            if (0 >= sscanf(argv[i]+1,"%d",&interval))
                interval=DEFAULT_INTERVAL;
            else
                interval = max(interval,0);
        }

        else {                                          /* job number or user name */
            job_and_user_buff_len += strlen(argv[i])+1; /* include space to separate items */
            if (job_and_user_buff_len >= JOB_AND_USER_BUFF_SIZE)
                crash("too many command line arguments.");
            strcat(job_and_user_buff," ");
            strcat(job_and_user_buff,argv[i]);     /* append to buffer */
        };

    if (remote_host == NULL) crash("server not specified.");

    /**************************************
       build request to send to the daemon
     **************************************/
    if ((strlen(remote_name)+job_and_user_buff_len+2) >= REQUEST_BUFF_SIZE)
        crash("your command line arguments are too long.");
    sprintf(request,"%c%s%s\n",request_code,remote_name,job_and_user_buff);
    if (debug) printf("%c\"%s\"\"%s\"\n",request_code,remote_name,job_and_user_buff);
    if (debug) printf(request);

    /* do it */
    query_server(remote_host,request,interval);

    netshut();   /* shut down all network stuff */
}

/****************************************************************
 * query_server                                                 *
 * Open the connection, send the request, and display the       *
 * text the server sends us.  If interval is non-zero, the      *
 * request is repeated with a screen clear between updates.     *
 * Periodic updating is terminated by a keypress or a           *
 * "No entries" message from the server.                        *
 *                                                              *
 * parameters: null terminated name or ip address of the server *
 *             null terminated string to send to the server to  *
 *                request the queue list                        *
 *             the delay interval in seconds between updates    *
 ****************************************************************/
static void query_server(char *remote_host, char *request, int interval)
{
    int clear_screen_pending;
    int source_port;
    int server_connection_id;
    struct machinfo *server_info_record;
    char buff[132];         /* text returned from the server */
    int len;                /* length of text read from server */
    int i;                  /* loop counter */

    /* pick a source port at random from the set of privileged ports */
    randomize();
    source_port = rand() % MAX_PRIV_PORT;

    /* do name lookup for server */
    server_info_record = lookup(remote_host);
    if (server_info_record == 0)
        crash("domain lookup failed for %s.",remote_host);

    do {
        /* increment source port number for each connection we open */
        source_port = (source_port + 1) % MAX_PRIV_PORT;

        /* open the connection */
        server_connection_id = open_connection(server_info_record, source_port, PRINTER_PORT);
        if (server_connection_id < 0) crash ("unable to open connection.");

        /* send the request */
        netwrite(server_connection_id, request, strlen(request));

        /* we wait until we get something from the server before clearing the screen */
        clear_screen_pending =((interval > 0) && (!debug));

        while(1) {
            len = nread(server_connection_id, buff, 132);
            if (len <=0 ) break;
            if (clear_screen_pending) {
                clrscr();
                clear_screen_pending = 0;
            }
            printf("%.*s",len,buff);
            if (strnicmp(buff,"No entries",10) == 0) interval = 0;
        }

        netclose(server_connection_id);

        /* delay before repeating query */
        for (i=1; i<=interval; i++) {
            netsleep(1);   /* one second */
            checkerr();    /* check for any error events */
            if (kbhit()) interval = 0;
        }
    } while (interval != 0);
}

#ifdef MSC

/******************************************************************
*
* randomize()
*
* replicates the randomize function of Turbo C
* MSC 5.1 does not contain it so we have to write it ourselves.
*
*/

static void randomize(void )
{
	srand((unsigned)time(NULL));
}

/*******************************************************************
*
* clrscr()
*
* just clear the screen.  Use interrupt 0x10(hex) subfunction 6
*
*/

static void clrscr(void )
{
	union REGS regs;

	regs.h.al=0;
	regs.h.ah=6;
	regs.h.cl=0;
	regs.h.ch=0;
	regs.h.dl=79;
	regs.h.dh=24;
	regs.h.bh=0;
	int86(0x10,&regs,&regs);
}

#endif


