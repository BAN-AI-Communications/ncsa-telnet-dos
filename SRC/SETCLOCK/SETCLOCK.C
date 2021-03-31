/*
	setclock.c
	Provides a clock setting for  NCSA 2.3.
        Reference: RFC 868

	By James Nau, College of Engineering,
	University of Nebraska--Lincoln
*/

#define TIME_PORT 37
#define REALTIME

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <dos.h>
#include <time.h>
#include <ctype.h>
#ifdef __TURBOC__
#include "turboc.h"
#endif


#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif

#include "netevent.h"
#include "hostform.h"
#include "whatami.h"
#include "externs.h"
#include "netutils.h"
#include "version.h"

int debug = 0;            /* enable with -D option */
int ftpok=0,twperm=0;     /* Unresolved useless (here anyway) externals */

unsigned char path_name[_MAX_DRIVE+_MAX_DIR],		/* character storage for the path name */
	temp_str[20],buf[_MAX_DIR],temp_data[30];

/* Function Prototypes */
int main(int argc, char *argv[]);
#ifndef __TURBOC__
static void randomize(void );
#endif
void usage(void);
void setclock(char *host);


/*
   setclock [-h filename] hostname
   -h        filename is alternative CONFIG.TEL file
   hostname  the NAME (or Number?) of the host to use for time
*/

int main(int argc, char *argv[])
{
   int i;
   int switchcount=1;   /* how many args are present */
   int gothost=0;       /* if 1, we've found the host */
   char *ptr=0;         /* pointer to CONFIG.TEL file */

   char remote_host[256];    /* address of remote host */

#ifdef __TURBOC__
	fnsplit(argv[0],path_name,buf,temp_str,temp_data);	 /* split path up */
#else
	_splitpath(argv[0],path_name,buf,temp_str,temp_data);	/* split path up */
#endif
	strcat(path_name,buf);		   /* append the path name */
	strcat(path_name,temp_str);    /* append filename */

/* get the command line arguments */
   for (i=1; i<argc; i++)
      if (argv[i][0] == '-')
      {
         switch (argv[i][1])
         {
            case 'D':
               debug = 1;
               if (debug) printf("Debugging now ON\n");
               switchcount++;
               break;
            case 'h':
               if (i+1 < argc)
                  ptr = argv[i+1];
               else
                  usage();
               if (debug) printf("Using config file [%s]\n",ptr);
               switchcount += 2;
               break;
            default:
               usage ();   /* Tell user how to use it */
         }
         if (debug) printf("argv[%d][1]=-%c\n", i, argv[i][1]);
      }

      if (argc != switchcount+1) usage();
      strcpy(remote_host,argv[argc-1]);
      gothost++;

   if (debug) printf("remote_host [%s]\n", remote_host);

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

/* Just Do It */
   setclock(remote_host);

   netshut();      /* shut down all network stuff */
   exit(0);
   return(0);
}


void setclock(char *remote_host)
{
   int from_port;
   int conn_id;
   struct machinfo *host_info;
   char buff[5]; /* text returned from the server */
   int bufflen=5;
   int len;             /* length of text read from server */
   int i;
   int timeout=60;      /* Timeout period (I hope) */
   time_t diff;         /* time now, and the difference between times */
   unsigned long nettime=0L;      /* binary value of returned time */
   unsigned long netoffset=0x83aa7e80; /* Jan 1, 1970 in secs from 1900 */
   /* netoffset is January 1, 1970 at 00:00 in offset from 1900
      where nettime starts */
   char *tz;            /* pointer to TZ env var */
   char yn[80];         /* Yes or no */

   struct tm *ltime;              /* for localtime */
   struct dosdate_t *dosdate;     /* date structure for _dos_setdate() */
   struct dostime_t *dostime;     /* time structure for _dos_settime() */

   int theclass, dat;
   int event;

	dosdate = malloc(sizeof(struct dosdate_t));
	dostime = malloc(sizeof(struct dostime_t));

    /* pick a source port at random from the set of privileged ports */
    randomize();
    from_port = rand() % 1023;

    /* do name lookup for server */
    host_info = gethostinfo(remote_host);
   if (host_info == (struct machinfo *)NULL)     /* couldn't do it, message in gethostinfo */
   {
      printf("Couldn't lookup host [%s]\n", remote_host);
      return;
   }

        /* open the connection */
        conn_id = connect_sock(host_info, from_port, TIME_PORT);
        if (conn_id < 0)
        {
           netshut();
           printf("connect_sock returned %d, exiting\n",conn_id);
           exit (2);
        }
        if (debug) printf("connection ident [%d]\n",conn_id);

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
              break;
           }
                      
        }
        if (theclass == USERCLASS && event == ALARM)
        {
           printf("Connection timed out in %d seconds\n", timeout);
           netshut();
           exit (2);
        }

        if (debug)
        {
           for (i=0; i<4; i++) printf("%2x ", 0xff&buff[i]);
           printf("\n");
        }

        for (i=0; i<4; i++) nettime = (nettime<<8) + (0xff&buff[i]);  /* convert to binary */

#ifdef PRE_MCS70 /* rmg 931100 */
        diff =  nettime - netoffset;   /* make the time (since 1-jan-1970) */
#else
        diff =  nettime + 0x15180;   /* make the time (since 12/31/1899) */
#endif

        tz = getenv("TZ");
        yn[0] = '\0';
        if (tz == NULL)
        {
           printf("TZ environment variable not set\n");
           printf("This will assume PST8PDT for a timezone\n");
           printf("Is that what you want? ");
           gets(yn);
        }
        tzset();        /* setup everything to use TZ */
        ltime = localtime(&diff);

                             /* Well, this all changed too */
        (ltime->tm_mday);
        dosdate->day = (unsigned char)(ltime->tm_mday);
        (ltime->tm_mon)++;  /* localtime range 0-11, but _dos_settime range 1-12 */
        dosdate->month = (unsigned char)(ltime->tm_mon);
        dosdate->year = (unsigned int)ltime->tm_year + 1900;
        (ltime->tm_wday);
        dosdate->dayofweek = (unsigned char)(ltime->tm_wday);

        dostime->hour = (unsigned char)ltime->tm_hour;
        dostime->minute = (unsigned char)ltime->tm_min;
        dostime->second = (unsigned char)ltime->tm_sec;
        dostime->hsecond = 0;

        if (tz != NULL || yn[0] == 'y' || yn[0] =='Y')
        {
           if (_dos_setdate(dosdate))
           {
              printf("Error setting date\n");
              printf("day  : %d\n", dosdate->day);
              printf("month: %d\n", dosdate->month);
              printf("year : %d\n", dosdate->year);
              printf("dayow: %d\n", dosdate->dayofweek);
           }
           if (_dos_settime(dostime))
           {
              printf("Error setting time\n");
              printf("hour  : %d\n",dostime->hour);
              printf("minute: %d\n",dostime->minute);
              printf("second: %d\n",dostime->second);
           }

#ifdef OLD_WAY /* rmg 931100 */
           printf("Time set to %s", asctime(ltime));
#else
           time(&diff);
#endif
           printf("Time set to %s", ctime(&diff));
        }
        else {  /* answered 'n' to time zone question */
		   printf("\nTime was not set. You must enter the environment variable TZ\n");
		   printf("in your environment.  This should be a a three-letter time zone\n");
		   printf("code (such as CMT for Central/Mountain), followed by an optionally\n");
		   printf("signed number giving the difference (in hours) between local time\n");
		   printf("and Greenwich Mean Time. (6 for Central/Mountain)  This is followed\n");
		   printf("by a three-letter Daylight Savings Time Zone, such as CDT.\n");
		};

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


void usage()
{
   printf("%s\n",SCLK_VERSION);
   printf("Usage: %s [-h filename] hostname\n\n", path_name);

   printf("   -h        filename is alternative CONFIG.TEL file\n");
   printf("   hostname  the NAME of the host to sync clock to\n");

   printf("             Depends on environmental variable TZ.\n");
   printf("             Example: set TZ=CST6CDT\n");
   exit (-1);
}

#ifdef __TURBOC__

int _dos_setdate(struct dosdate_t *ddate)
{
	struct date datep;

	datep.da_year = ddate->year;
	datep.da_mon = ddate->month;
	datep.da_day = ddate->day;
	setdate(&datep);
	return(0);
}

int _dos_settime(struct dostime_t *dtime)
{
	struct time timep;

	timep.ti_hour = dtime->hour;
	timep.ti_min = dtime->minute;
	timep.ti_sec = dtime->second;
	timep.ti_hund = dtime->hsecond;
	settime(&timep);
	return(0);
}

#endif

