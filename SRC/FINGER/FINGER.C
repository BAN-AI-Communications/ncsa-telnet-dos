/*
	finger.c
	Provides a finger client services for NCSA 2.3.
        Reference: RFC 742

	By James Nau, College of Engineering,
	University of Nebraska--Lincoln
*/

#define FINGER_PORT 79

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
#include "externs.h"

#include "netutils.h"

int debug = 0;          /* enable with -D option */
int timeout=300;        /* timeout period in clicks */
//int bypass_passwd=0;    /* whether to bypass the password check, not used */

unsigned char path_name[_MAX_DRIVE+_MAX_DIR],		/* character storage for the path name */
	temp_str[20],buf[_MAX_DIR],temp_data[30];

/* Function Prototypes */
int main(int argc, char *argv[]);
#ifndef __TURBOC__
static void randomize(void );
#endif
void usage(void);
int index (char *string, char ch);
void do_finger(char *host, char *line);
void filter(char *buffer, int buffer_len);

/*
   finger [-h filename] [-l | -w] [user]@hostname
   -h        filename is alternative CONFIG.TEL file
   -l -w     implements longer finger (whois)
   user      username to query
   hostname  the NAME (or Number?) of the host to finger
*/

int main(int argc, char *argv[])
{
   int i;
   char userinfo[128];  /* Ought not to be > 128 char usernames */
   int longform=0;      /* if 1, then try a long finger (whois) */
   int gotuser=0;       /* if 1, doing lookup on a particular user */
   int gothost=0;       /* if 1, we've found the host */
   int atsign;          /* location of @ */
   char *ptr=0;         /* pointer to CONFIG.TEL file */

   char remote_host[256];    /* address of remote host */

#ifdef __TURBOC__
	fnsplit(argv[0],path_name,buf,temp_str,temp_data);	 /* split path up */
#else
   _splitpath(argv[0],path_name,buf,temp_str,temp_data);   /* split path up */
#endif
   strcat(path_name,buf);         /* append the path name */
   strcat(path_name,temp_str);    /* append filename */

   userinfo[0] = '\0';  /* init to a null string */

/* get the command line arguments */
   for (i=1; i<argc; i++)
      if (argv[i][0] == '-')
      {
         switch (argv[i][1])
         {
            case 'D':
               debug = 1;
               if (debug) printf("Debugging now ON\n");
               break;
            case 'h':
               if (i+1 < argc)
                  ptr = argv[i+1];
               else
                  usage();
               if (debug) printf("Using config file [%s]\n",ptr);
               break;
            case 'l':
            case 'w':
               longform = 1;
               if (debug) printf("using long finger form\n");
               break;
			case 't':
			   i++;
			   timeout=atoi(argv[i]);
			   printf("Timeout set to %d clicks.\n",timeout);
			   break;
            default:
               usage ();   /* Tell user how to use it */
         }
         if (debug) printf("argv[%d][1]=-%c\n", i, argv[i][1]);
      }

   if (longform) strcpy(userinfo, "/W ");   /* long finger form, from RFC742 */

   if ((atsign = index(argv[argc-1], '@'))) /* find the host */
   {
      if (debug) printf("atsign [%d]\n", atsign);
      strcpy(remote_host,(argv[argc-1]+atsign));
      gothost++;
      if (atsign != 1)  /* Then, we have a username too */
      {
         strncat(userinfo, argv[argc-1], atsign-1); /* username */
         gotuser++;     /* Add 1 to users */
      }
   }
   else
      usage();     /* Tell them how to use it */

/* OK, we should have all the info we need to do the finger
   So, now let's go do it */

   if (debug) printf("remote_host [%s]\n", remote_host);
   if (debug) printf("userinfo    [%s]\n", userinfo);
   if (debug) printf("gothost [%d]\n", gothost);
   if (debug) printf("gotuser [%d]\n", gotuser);

/* exit (0); */  /* For debugging if you have no network! */

   signal(SIGINT,SIG_IGN);   /* Microsoft intercept of break */

/* Do session initialization.  Snetinit reads config file. */
   /* go find a valid CONFIG.TEL file */
   if (ptr == (char *)NULL) ptr = getenv("CONFIG.TEL");
   if (debug) printf("ptr after getenv [%s]\n",ptr);
   if (ptr != (char *)NULL) Shostfile(ptr);

   if (Snetinit()) /* Should return 0 if network is OK */
   {
      printf("network init failed\n");
      exit (1);
   }

   strcat(userinfo, "\x0d\x0a");  /* end with <CR><LF> as per RFC 742*/

   if (debug) printf("sending [%s] to [%s]\n",userinfo, remote_host);

/* Just Do It */
   do_finger(remote_host, userinfo);

   netshut();      /* shut down all network stuff */
   exit(0);
   return(0);
}


void do_finger(char *remote_host, char *sendline)
{
   int from_port;
   int conn_id;
   struct machinfo *finger_host;
   char buff[132]; /* text returned from the server */
   int bufflen=132;
   int len;        /* length of text read from server */
   int i;

   int theclass, dat;
   int event;

    /* pick a source port at random from the set of privileged ports */
    randomize();
    from_port = rand() % 1023;

    /* do name lookup for server */
    finger_host = gethostinfo(remote_host);
   if (finger_host == (struct machinfo *)NULL)     /* couldn't do it, message in gethostinfo */
   {
      printf("Couldn't lookup host [%s]\n", remote_host);
      return;
   }

/* print out the IP number of the destination host */
   	printf("[");
   	for (i=0; i<4; i++) {
      	printf("%d",finger_host->hostip[i]);
		if (i < 3)
			printf(".");
	}
   	printf("]\n");

        /* open the connection */
        conn_id = connect_sock(finger_host, from_port, FINGER_PORT);
        if (conn_id < 0)
        {
           netshut();
           printf("connect_sock returned %d, exiting\n",conn_id);
        }
        if (debug) printf("connection ident [%d]\n",conn_id);

        /* send the request */
        netwrite(conn_id, sendline, strlen(sendline));
        if (debug) printf("netwrite succeeded--flushing buffer\n");

        netpush(conn_id);
        if (debug) printf("buffer flushing\n");

        if (debug)
        if (!netest(conn_id))
           printf("Connection OK\n");
        else
           printf("nettest: %d\n", netest(conn_id));

/* OK, now, we'll attempt to set an alarm to time out in timeout seconds
   if we get a connection, but no response */

        Stimerset(USERCLASS, ALARM, 0, timeout);
        if (debug) printf("timer set to go off in %d seconds\n", timeout);
        theclass = 0;
        event = 0;
        while ((theclass != USERCLASS || event != ALARM) && !netest(conn_id))
                /* no alarm and good connection */
        {
           Stask();     /* I think I have to call this to post my alarm? */
           event = Sgetevent(CONCLASS | USERCLASS, &theclass, &dat);
           if (debug)
              printf("event[%d] theclass[%d] dat[%d]\n",event, theclass, dat);
           if (!event) continue;
           if (conn_id != dat) continue;

           if (event == CONDATA)
           {
              len = netread(conn_id, buff, bufflen);
              if (len == 0) continue;
              filter(buff, len);
              printf("%.*s", len, buff);
           }
                      
        }
        if (theclass == USERCLASS && event == ALARM)
        {
           printf("Connection timed out in %d clicks\n", timeout);
           netshut();
           exit (2);
        }

#ifdef OLD_WAY
  if (debug) printf("returning eof\n");
 return (EOF);
		while ((event = Sgetevent(CONCLASS,&theclass,&dat)) != CONDATA)
		{
		   printf("event [%d] [%d] [%d]\n",event, theclass, dat);
		   if (conn_id == dat)
		   {
		}
		if (debug) printf("got event CONDATA [%d]\n",CONDATA);
		printf("Event [%d] [%d] [%d]\n",event, theclass, dat);

		   len = netread(conn_id, buff, bufflen);
		   if (debug) printf("len is [%d]\n", len);
		   if (len <=0 ) break;
		   printf("%.*s",len,buff);
   }
   }
#endif
        if (debug) printf("Closing Connection\n");
        netclose(conn_id);
}


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


void filter (char *buffer, int buffer_len)
/* filter out control characters, escape sequences, etc
   keeps from reprogramming keys, ...
*/
{
   int i;
   char ch;

   for (i=0; i<buffer_len; i++)
   {
      ch = *(buffer+i);
      if (!(isprint(ch) || isspace(ch))) *(buffer+i) = '?';
   }
}


int index(char *string, char ch)
{
/* shouldn't there be one of these somewhere? */
   int i;

   for (i=0; i<(int)strlen(string); i++)
      if ( *(string+i) == ch) return (i+1);
   return (0);     /* if not found */
}

void usage()
{
   printf("Usage: %s [-h filename] [-t ##] [-l | -w] [user]@hostname\n\n", path_name);

   printf("   -h        filename is alternative CONFIG.TEL file\n");
   printf("   -t ##     set timeout period to ## clicks (default 300)\n");
   printf("   -l -w     implements longer finger (whois)\n");
   printf("   user      username to query\n");
   printf("   hostname  the NAME of the host to finger\n");
   exit (-1);
}

