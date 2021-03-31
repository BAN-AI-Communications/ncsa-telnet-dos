/*
  Whois.c
  Provides a whois client services for NCSA 2.3.
        Reference: RFC 812

  Contributed By: Stan Barber
  internet: sob@bcm.tmc.edu
  Baylor College of Medicine
*/

#define WHOIS_PORT 43
#define DEFAULT_HOST "rs.internic.net"
#define OLD_DEFAULT_HOST "nic.ddn.mil"
#define BCM_HOST "whois.bcm.tmc.edu"

#define ALARM 128

#ifdef __TURBOC__
#include "turboc.h"
#endif
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


#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif
#include "netevent.h"
#include "hostform.h"
#include "whatami.h"
#include "netutils.h"
#include "externs.h"


int debug = 0;          /* enable with -D option */
int timeout=300;        /* timeout period in clicks */
int bypass_passwd=0;    /* whether to bypass the password check, not used */


unsigned char path_name[_MAX_DRIVE+_MAX_DIR],     /* character storage for the path name */
  temp_str[20],buf[_MAX_DIR],temp_data[30];

/* Function Prototypes */
int main(int argc, char *argv[]);
#ifndef __TURBOC__
static void randomize(void );
#endif
static void usage(void);
#ifdef OLD_WAY
static int index(char *string, char ch);
#endif
static void do_whois(char *host, char *line);
static void filter(char *buffer, int buffer_len);

/*
   whois [-c filename] [-h hostname] keyword
   -c        filename is alternative CONFIG.TEL file
   -h        hostname is an alternative to rs.internic.net
   keyword   query string
*/
int main(int argc, char *argv[])
{
    int i;
    char query[128];
    char *ptr=NULL;           /* pointer to CONFIG.TEL file */
    char *remote_host=NULL;      /* name of remote host */

#ifdef __TURBOC__
    fnsplit(argv[0],path_name,buf,temp_str,temp_data); /* split path up */
#else
    _splitpath(argv[0],path_name,buf,temp_str,temp_data);   /* split path up */
#endif
    strcat(path_name,buf);         /* append the path name */
    strcat(path_name,temp_str);    /* append filename */

    query[0] = '\0';
/* get the command line arguments */
    i = 1;
    while((argv[i][0] == '-' || argv[i][0] == '/') && i < argc) {
        if(debug)
            printf("arguement is %c%c\n", argv[i][0],argv[i][1]);
        switch(argv[i][1]) {
            case 'D':
                debug = 1;
                if(debug)
                    printf("Debugging now ON\n");
                break;

            case 'c':
                if(++i < argc)
                    ptr = argv[i];
                else
                    usage();
                if(debug)
                    printf("Using config file [%s]\n",ptr);
                break;

            case 'h':
                if(++i < argc)
                    remote_host = argv[i];
                else
                    usage();
                if(debug)
                    printf("Using host %s\n",remote_host);
                break;

            case 't':
                if (++i < argc)
                    timeout=atoi(argv[i]);
                else
                    usage();
                printf("Timeout set to %d clicks.\n",timeout);
                break;

            default:
                usage();   /* Tell user how to use it */
          } /* end switch */
        i++;
      } /* end while */
    if(argc < 2 || argc == i)
        usage();
    strcpy(query,argv[i]);
#ifdef BCM
    if(remote_host == NULL)
        remote_host = BCM_HOST;
#else
    if(remote_host == NULL)
        remote_host = DEFAULT_HOST;
#endif
   /* OK, we should have all the info we need to do the query */
   /* So, now let's go do it */
    if(debug)
        printf("remote_host [%s]\n", remote_host);
    if(debug)
        printf("query    [%s]\n", query);

/* exit (0); */  /* For debugging if you have no network! */

    signal(SIGINT,SIG_IGN);   /* Microsoft intercept of break */

/* Do session initialization.  Snetinit reads config file. */
    /* go find a valid CONFIG.TEL file */
    if(ptr == (char *)NULL)
        ptr = getenv("CONFIG.TEL");
    if(debug)
        printf("ptr after getenv [%s]\n",ptr);
    if(ptr != (char *)NULL)
        Shostfile(ptr);

    if(Snetinit()) { /* Should return 0 if network is OK */
        printf("network init failed\n");
        exit (1);
      } /* end if */

    strcat(query, "\x0d\x0a");  /* end with <CR><LF> as per RFC 812*/

    if(debug)
        printf("sending [%s] to [%s]\n",query, remote_host);

/* Just Do It */
    do_whois(remote_host, query);

    netshut();      /* shut down all network stuff */
    exit(0);
    return(0);
}


void do_whois(char *remote_host, char *sendline)
{
   int from_port;
   int conn_id;
   struct machinfo *whois_host;
   char buff[132]; /* text returned from the server */
   int bufflen=132;
   int len;        /* length of text read from server */

   int theclass, dat;
   int event;

    /* pick a source port at random from the set of privileged ports */
    randomize();
    from_port = rand() % 1023;

    /* do name lookup for server */
    whois_host = gethostinfo(remote_host);
    if(whois_host == (struct machinfo *)NULL) {     /* couldn't do it, message in gethostinfo */
        printf("Couldn't lookup host [%s]\n", remote_host);
        return;
    }

         /* open the connection */
    conn_id = connect_sock(whois_host, from_port, WHOIS_PORT);
    if(conn_id<0) {
        netshut();
        printf("connect_sock returned %d, exiting\n",conn_id);
      } /* end if */
    if(debug)
        printf("connection ident [%d]\n",conn_id);

         /* send the request */
    netwrite(conn_id, sendline, strlen(sendline));
    if(debug)
        printf("netwrite succeeded--flushing buffer\n");

    netpush(conn_id);
    if(debug)
        printf("buffer flushing\n");

    if(debug)
        if(!netest(conn_id))
            printf("Connection OK\n");
        else
            printf("nettest: %d\n", netest(conn_id));

 /* OK, now, we'll attempt to set an alarm to time out in timeout seconds
    if we get a connection, but no response */

    Stimerset(USERCLASS, ALARM, 0, timeout);
    if(debug)
        printf("timer set to go off in %d seconds\n", timeout);
    theclass = 0;
    event = 0;
    while((theclass != USERCLASS || event != ALARM) && !netest(conn_id)) { /* no alarm and good connection */
        Stask();
        event = Sgetevent(CONCLASS | USERCLASS, &theclass, &dat);
        if(debug)
            printf("event[%d] theclass[%d] dat[%d]\n",event, theclass, dat);
        if(!event)
            continue;
        if(conn_id != dat)
            continue;

        if(event == CONDATA) {
            len=netread(conn_id, buff, bufflen);
            if(len == 0)
                continue;
            filter(buff, len);
            printf("%.*s", len, buff);
          } /* end if */
      } /* end while */
    if(theclass == USERCLASS && event == ALARM) {
        printf("Connection timed out in %d clicks\n", timeout);
        netshut();
        exit (2);
      } /* end if */
    if(debug)
        printf("Closing Connection\n");
    netclose(conn_id);
}   /* end do_whois() */

#ifdef MSC
#ifndef __TURBOC__
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
#endif


static void filter(char *buffer, int buffer_len)
 /* filter out control characters, escape sequences, etc
    keeps from reprogramming keys, ...
 */
{
    int i;
    char ch;

    for(i=0; i<buffer_len; i++) {
        ch=*(buffer+i);
        if(!(isprint(ch) || isspace(ch)))
            *(buffer+i)='?';
      } /* end for */
}   /* end filter() */


#ifdef OLD_WAY
static int index(char *string, char ch)
{
 /* shouldn't there be one of these somewhere? */
    int i;

    for (i=0; i<(int)strlen(string); i++)
       if ( *(string+i) == ch) return (i+1);
    return (0);     /* if not found */
}
#endif

static void usage(void)
{
    printf("Usage: %s [-c filename] [-t ##] [-h hostname] query\n\n", path_name);

    printf("   -c        filename is alternative CONFIG.TEL file\n");
    printf("   -t ##     set timeout period to ## clicks (default 300)\n");
 #ifdef BCM
    printf("   -h        hostname to query other than %s\n",BCM_HOST);
 #else
    printf("   -h        hostname to query other than %s\n",DEFAULT_HOST);
 #endif
    printf("   query     information to ask about\n");
    exit (-1);
}

