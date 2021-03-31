/*
	rsh.c
	Provides rsh client services for NCSA 2.3.

	By James Nau, College of Engineering,
	University of Nebraska--Lincoln
*/

#define RSH_PORT 512

#define ALARM 128

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <ctype.h>
#ifdef __TURBOC__
#include "turboc.h"
#endif
#ifdef MSC
#include <signal.h>
#include <time.h>
#endif


#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif
#include "netevent.h"
#include "hostform.h"
#include "whatami.h"
#include "externs.h"

#include "netutils.h"

int debug = 0;          /* enable with -D option */
int bypass_passwd=0;    /* whether to bypass the password check, not used */

unsigned char path_name[_MAX_DRIVE+_MAX_DIR],		/* character storage for the path name */
	temp_str[20],buf[_MAX_DIR],temp_data[30];

/* Function Prototypes */
int main(int argc, char *argv[]);
#ifndef __TURBOC__
static void randomize(void );
#endif

void usage(void);
void do_rsh(char *host, char *line);
int rsh(char *host, int port, char *user, char *passwd, char *cmd,int *errchan);

/*
   rsh [-h filename] [-D] host command
   -h        filename is alternative CONFIG.TEL file
   -D        Debug Flag
   host      the NAME (or Number?) of a machine to execute command
   command   a command to pass to the remote host
*/

int main(int argc, char *argv[])
{
    int i;
    int switchcount=1,      /* How many switches and switch args are present */
        switchlimit;        /* How far into argc am I allowed to find switches */
    char command[80],       /* Text to send to remote host */
        *ptr=0,             /* pointer to CONFIG.TEL file */
        remote_host[256];   /* address of remote host */

#ifdef __TURBOC__
    fnsplit(argv[0],path_name,buf,temp_str,temp_data);   /* split path up */
#else
    _splitpath(argv[0],path_name,buf,temp_str,temp_data);   /* split path up */
#endif
    strcat(path_name,buf);         /* append the path name */
    strcat(path_name,temp_str);    /* append filename */

    if(argc<3)
        usage();    /* Oops, they haven't got it right */
/* rsh host cmd [-args] */
/* rsh -h file host cmd [-args] */
/* rrsh -h file -D host cmd [-args] */
/* rrsh -D host cmd [-args] */

//   switchlimit = (argc<5 ? argc-2 : 4);
    switchlimit = 3;     /* Assume -D has to come first :) */
    if(debug)
        printf("Switchlimit [%d]\n", switchlimit);

/* get the command line arguments */
/* Can't parse entire command line-- what if command is ps -eaf
   so, this appears to do the job for the number of switches allowed */
    for(i=1; i<switchlimit; i++)     /* Switches have to be first 3 */
        if(argv[i][0]=='-') {
            switch(argv[i][1]) {
                case 'D':        /* Debug switch */
                    debug=1;
                    if(debug)
                        printf("Debugging now ON\n");
                    switchcount++;
                    break;

                case 'h':        /* Alternate CONFIG.TEL file switch */
                    if(i+1<switchlimit)
                        ptr=argv[i+1];
                    else
                        usage();
                    if(debug)
                        printf("Using config file [%s]\n",ptr);
                    switchcount+=2;
                    break;

                default:
                    usage();   /* Tell user how to use it */
              } /* end switch */
            if(debug)
                printf("argv[%d][1]=-%c\n", i, argv[i][1]);
          } /* end if */

    if((switchcount+2)>argc)
        usage();    /* do we have the rest ?? */

    strcpy(remote_host,argv[switchcount++]);     /* the Host to execute */
    strcpy(command,argv[switchcount++]);         /* The Command */
    for(; switchcount<argc; ) {    /* How much command */
        strcat(command, " ");
        strcat(command, argv[switchcount++]);
     }  /* end for */

    if(debug)
        printf("sending [%s] to [%s]\n",command, remote_host);

// exit (0);  /* For debugging if you have no network! (No E-Net at home :( */

    signal(SIGINT,SIG_IGN);   /* Microsoft intercept of break */

/* Do session initialization.  Snetinit reads config file. */
   /* go find a valid CONFIG.TEL file */
    if(ptr==(char *)NULL)
        ptr=getenv("CONFIG.TEL");
    if(debug)
        printf("ptr after getenv [%s]\n",ptr);
    if(ptr!=(char *)NULL)
        Shostfile(ptr);

    if(Snetinit()) {     /* Should return 0 if network is OK */
        printf("network init failed\n");
        exit(1);
     }  /* end if */

/* Just Do It */
    do_rsh(remote_host, command);

    exit(0);
    return(0);
}


void do_rsh(char *remote_host, char *cmd)
{
    char user[33],      /* Username */
        passwd[17],     /* Password for said user */
        netbuf[512],    /* buffer for netread */
        ch;             /* Byte to send from kbhit()/getch */
    int errchan,        /* Dummy error channel, for now at least */
        conn_id,        /* Connection ID from rsh */
        netbuflen=512,  /* Length of netbuf */
        len,            /* length of data read from net */
        event,          /* event from Sgetevent */
        theclass,       /* class that is returned */
        dat;            /* for Sgetevent info too */

    user[0]='\0';           /* Init to NULL */
    passwd[0]='\0';         /* Init to NULL */

/* Establish connection and all that stuff... */
    conn_id=rsh(remote_host,RSH_PORT,user,passwd,cmd,&errchan);
    if(conn_id<0) {
        printf("Couldn't rsh\n");
        printf("rsh returned %d\n",conn_id);
        exit(1);
      } /* end if */

    theclass=0;
    event=0;

    if(debug)
        printf("we're goin in...\n");
//   while ((theclass == CONCLASS && event != CONCLOSE)  && !netest(conn_id))
    while(!netest(conn_id)) {   /* No CONnection info and good connection */
        if(kbhit()) {      /* Keyboard hit, let's send it */
            ch=(char)getch();           /* gotta get it */
            if(ch=='\x0d')
                ch='\x0a';    /* Translate cr to lf for Unix */
            netwrite(conn_id,&ch,1);   /* out it goes */
            netpush(conn_id);
          } /* end if */

        Stask();     /* I think I have to call this to post my alarm? */
        event=Sgetevent(CONCLASS|USERCLASS,&theclass,&dat);
        if(debug)
            printf("event[%d] theclass[%d] dat[%d] [%d]\n",event, theclass, dat, conn_id);
        if(!event || conn_id!=dat)
            continue;

        if(event==CONDATA) {
            len=netread(conn_id,netbuf,netbuflen);
            if(!len)
                continue;
            printf("%.*s", len, netbuf);
          } /* end if */
        else if(event==CONCLOSE) {      /* I guess we're done */
            if(debug)
                printf("Closing Connection!\n");
            netclose(conn_id);  /* close the connection */
            netshut();     /* Shut down the network */
            break;
          } /* end if */
        else {
            printf("Unexpected event: [%d]\n", event);
          } /* end else */
      } /* end while */
//   printf("out of while loop theclass[%d] event[%d] ntest[%d]\n",theclass, event, netest(conn_id));
    netclose(conn_id);
    netshut();
}


#if defined(MSC) && !defined(__TURBOC__)
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


void usage()
{
/*
   rsh [-h filename] [-D] host command
   -h        filename is alternative CONFIG.TEL file
   -D        Debug Flag
   host      the NAME (or Number?) of a machine to execute command
   command   a command to pass to the remote host
*/

    printf("Usage: %s [-h filename] host command\n\n", path_name);

    printf("   -h        filename is alternative CONFIG.TEL file\n");
    printf("   host      the host to execute command\n");
    printf("   command   command for remote system to execute\n");
    printf("\nNOTE:  This program (rsh) is flakey right now\n");
    printf("       it seems to work fine for OUTPUT only commands\n");
    printf("       (such as who, ps).  But, commands requiring input\n");
    printf("       do NOT work correctly  (it seems to end properly, but\n");
    printf("       it never kills the process).\n\n");
    exit (-1);
}


/* implement a Un*x like rsh function.
      host is the host we want to rsh to
      port is the destination port (usually 512 for rsh)
      user is the username on host
      passwd is the password on host
      cmd is command to pass to host
      errchan is where to route stderr to (see below)

   if username and passwd are NULL, then we prompt for them
   from stdin.  Maybe should have a USER environ var for a default
   user, but NEVER have a passwd scanned from anywhere since we're on
   a PC with No File Security...

   Also, currently, errchan is ignored.  It's supposed to be:  If errchan
   is non-zero, then open another socket back to the system to process
   stderr on.

   Return value is the connection id that we established.
                or negative if an error somewhere.

*/
int rsh(char *host, int port, char *user, char *passwd, char *cmd,int *errchan)
{
   int i;          /* scratch variable */
   int from_port;  /* where to come from */
   struct machinfo *host_info;    /* structure to return machine info in */
   int conn_id;         /* Connection id from connect_sock */
   char buff[256];      /* Command to send to remote host */
   int ubase;           /* how far into buff we are */
   int timeout=100;     /* timeout period */

   int event;           /* event from Sgetevent */
   int theclass;        /* class that is returned */
   int dat;             /* for Sgetevent info too */

   char nullbyte;       /* NULL byte we are supposed to get back */
   int len;             /* length of data back from netread */

    if(!*errchan)
        printf("Unsupported errchan right now--Continuing\n");

/* Hmm, may get into trouble if we run out of space, or are passed
   **pointers, instead of having space available???? */

//   if (user == (char *)NULL)      /* no user specified, go get it */
    if(!strlen(user)) {        /* no user specified, go get it */
        printf("Enter Username: ");
        gets(user);
      } /* end if */
//   if (passwd == (char *)NULL)    /* no passwd specified, go get one */
    if(!strlen(passwd)) {      /* no passwd specified, go get one */
        printf("Enter password: ");
        i=0;
        while((passwd[i]=(char)getch())!='\xd')
            i++;    /* get w/o echo */
        passwd[i]='\0';
        putchar('\n');
      } /* end if */

    buff[0]='\0';  /* Null, since we want to leave stderr alone */

    ubase=1;
    for(i=0; i< (int)strlen(user); i++)      /* Add username */
        buff[i+ubase]=user[i];
    ubase+=strlen(user);

    buff[ubase]='\0';       /* Terminate with NULL */
    ubase++;

    for(i=0; i<(int)strlen(passwd); i++)  /* add the password */
        buff[i+ubase]=passwd[i];
    ubase+=strlen(passwd);

    buff[ubase]='\0';       /* Terminate with NULL */
    ubase++;

    for(i=0; i<(int)strlen(cmd); i++)       /* add the command to execute */
        buff[i+ubase]=cmd[i];
    ubase+=strlen(cmd);

    buff[ubase]='\0';       /* Terminate with NULL */
    ubase++;

    if(debug) {
        for(i=0; i<ubase; i++)
            printf("%d ",buff[i]);
        printf("\n");

        for(i=0; i<ubase; i++)
            printf("%c",buff[i]);
        printf("\n");
      } /* end if */

   /* pick a source port at random from the set of privileged ports */
    randomize();
    from_port=rand() % 1023;

   /* do name lookup for server */
    host_info=gethostinfo(host);
    if(host_info == (struct machinfo *)NULL) {     /* couldn't do it, message in gethostinfo */
        printf("Couldn't lookup host [%s]\n", host);
        return(-1);
      } /* end if */

/* print out the IP number of the destination host */
/* this should come out in production.  Leave for debugging though */
    if(debug) {
        printf("[");
        for(i=0; i<4; i++)
            printf("%d.",host_info->hostip[i]);
        printf("]\n");
      } /* end if */

   /* open the connection */
    conn_id=connect_sock(host_info, port, RSH_PORT);
    if(conn_id<0) {
        netshut();        /* Shut down the network */
        printf("connect_sock returned %d, exiting\n",conn_id);
        return (-2);
      } /* end if */
    if(debug)
        printf("connection ident [%d]\n",conn_id);

   /* send the request */
   netwrite(conn_id, buff, ubase);
   if (debug) printf("netwrite succeeded--flushing buffer\n");

   netpush(conn_id);

   if (debug)
      if (!netest(conn_id))
         printf("Connection OK\n");
      else
         printf("nettest: %d\n", netest(conn_id));

/* OK, now, we'll attempt to set an alarm to time out in timeout seconds
   if we get a connection, but no response */

    Stimerset(USERCLASS, ALARM, 0, timeout);
    if(debug)
        printf("timer set to go off in %d seconds\n", timeout);

    theclass=0;
    event=0;
    while((theclass!=USERCLASS || event!=ALARM) && !netest(conn_id)) {  /* no alarm and good connection */
        Stask();     /* I think I have to call this to post my alarm? */
        event=Sgetevent(CONCLASS | USERCLASS, &theclass, &dat);
        if(debug)
            printf("event[%d] theclass[%d] dat[%d] [%d]\n",event, theclass, dat, conn_id);
        if(!event || conn_id!=dat)
            continue;

        if(event==CONDATA) {
            len=netread(conn_id, &nullbyte, 1);
            if(len==0)
                continue;
            if(nullbyte != 0)
                printf( "Byte != 0 [%d]\n", nullbyte);
            Stimerunset(USERCLASS,ALARM,0);
            return(conn_id);
          } /* end if */
        else if(event==CONCLOSE) {
            printf("Unexpected closing of connection!\n");
            netshut();          /* Shut down the network */
            return (-3);
          } /* end if */
        else {
            printf("Unexpected event: [%d]\n", event);
          } /* end else */
      } /* end while */
    if(theclass == USERCLASS && event == ALARM) {
        printf("Connection timed out in %d seconds\n", timeout);
        netshut();        /* Shut down the network */
        return(-4);
      } /* end if */
    netshut();      /* Shut down the network */
    return(-5);
}

