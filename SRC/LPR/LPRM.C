/*  -------------------------------------------------------------------
    lprm - remove job from line printer queue

    lprm removes a job or jobs from a print spooling queue managed by
    an LPD daemon running on a remote machine.
    Built on top of the NCSA TCP/IP package (version 2.2tn for MS-DOS).

    Paul Hilchey   May 1989

    Copyright (C) 1989  The University of British Columbia
    All rights reserved.

	 history
	 -------
	 1/6/89		Microsoft C port by Heeren Pathak (NCSA)
    -------------------------------------------------------------------
*/

#include <stdio.h>
#include <dos.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef MSC
#include <signal.h>
#include <time.h>
#endif

#define WINMASTER
#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif
#include "whatami.h"
#include "hostform.h"
#include "windat.h"
#include "lp.h"
#include "externs.h"

int debug = 0;          /* 1 = print debugging info; set with -D option */
int ftppassword,        /* not used; just to avoid unresolved external */
	bypass_passwd=0;	/* whether to bypass the password check */

unsigned char path_name[_MAX_DRIVE+_MAX_DIR],		/* character storage for the path name */
	temp_str[20],s[_MAX_DIR],temp_data[30];


#define JOB_AND_USER_BUFF_SIZE  150     /* size of buffer for job numbers and user name specified on the command line */

/* Function Protoypes */
void main(int argc,char *argv[]);
static void randomize(void );

/****************************************************************
 *  Main program.                                               *
 *     lprm [ -Pprinter ] [ -Sserver ] [ - ] [job# ...]         *
 ****************************************************************/
void main(int argc,char *argv[])
{
    int i;
    char *ptr;
    int  len;

    char username[9];       /* name of user (max 8 chars) */
    struct config *cp;      /* configuration information */

    static char buff[JOB_AND_USER_BUFF_SIZE] = ""; /* string of jobs to be removed */
    int something = 0;      /* 1 = we got something back from the server */
    struct machinfo *server_info_record;
    int server_connection_id;

    char *remote_name,      /* printer name on remote system */
         *remote_host;      /* address of remote host        */

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
        crash("network initialization failed.\n");
	  }	/* end if */

    /* select default printer and server */
    remote_name = getenv("PRINTER");
    if (remote_name == NULL) remote_name = DEFAULT_PRINTER;
    remote_host = getenv("SERVER");

    /* get info from the configuration file */
    cp = (struct config *)malloc(sizeof(struct config));
    Sgetconfig(cp);

    /* check that the machine name was set in the configuration file */
    if (0 == strlen(cp->me)) crash("`myname' not set in config file.");

    /* set user name.  use first part of machine name if nothing else */
    ptr = getenv("USER");
    if (NULL != ptr) {
        strncpy(username,ptr,8);
        username[8]='\0';
    }
    else {
        i = min(strcspn(cp->me,"."),sizeof(username)-1);
        strncpy(username,cp->me,i);
        username[i]='\0';
    }

    /* Loop through command line arguments */
    for (i=1; i<argc; ++i)

        if (0 == strncmp(argv[i],"-P",2))       /* select printer */
            if (argv[i][2])
                remote_name = &argv[i][2];
            else if (i+1 < argc)
                remote_name = argv[++i];
            else;

        else if (0 == strncmp(argv[i],"-S",2))  /* select server */
            if (argv[i][2])
                remote_host = &argv[i][2];
            else if (i+1 < argc)
                remote_host = argv[++i];
            else;

        else if (0 == strcmp(argv[i],"-")) {    /* delete all my jobs */
            strcat(buff," ");
            strcat(buff,username);
        }

        else if (0 == strncmp(argv[i],"-D",2))  /* turn on debug */
            debug = 1;

        else {                                  /* job number, add to list */
            strcat(buff," ");
            strcat(buff,argv[i]);
        };

    if (remote_host == NULL) crash("server not specified.");

    server_info_record = lookup(remote_host);
    if (server_info_record == 0)
        crash("domain lookup failed for %s.",remote_host);


    /* pick a source port at random from the set of privileged ports, */
    /* and open the connection                                        */
    randomize();
    server_connection_id = open_connection(server_info_record, rand() % MAX_PRIV_PORT, PRINTER_PORT);
    if (server_connection_id < 0) crash ("unable to open connection.");

    /* send the request */
    nprintf (server_connection_id, "%c%s %s%s\n", LPD_REMOVE_JOB,
             remote_name, username, buff);

    /* wait for replies from the server and print them */
    while(1) {
        len = nread(server_connection_id, buff, 132);
        if (len <= 0) break;
        if (buff[0] != LPD_ERROR) {
            printf("%.*s",len,buff);
            something = 1;
        }
    }

    netclose(server_connection_id);
    netshut();
    if (!something) puts("No applicable jobs.");
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
#endif
