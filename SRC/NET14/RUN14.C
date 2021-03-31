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
#include <process.h>
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
#include "netutils.h"
#include "externs.h"

/* #define DEBUG */

/*
*	Global variables
*/
struct machinfo *mp;
unsigned char parsedat[32];      /* character buffer to store network writes */
extern unsigned char connected_flags;   /* flags indicating whether a port is connected yet */

extern unsigned char buf[256];
#ifdef CHECKNULL
extern unsigned char nullbuf[1024];     /* buffer for checking the NULL area */
#endif

#define SERIAL  0x14
#define NUM_COMM_PORTS  4   /* the number of comm. ports supported, remember to change this variable in int14.asm also */

/* Definitions for telnet protocol */

#define STNORM      0

#define SE			240
#define NOP			241
#define DM			242
#define BREAK		243
#define IP			244
#define AO			245
#define AYT			246
#define EC			247
#define EL			248
#define GOAHEAD 	249
#define SB			250
#define WILLTEL 	251
#define WONTTEL 	252
#define DOTEL	 	253
#define DONTTEL 	254
#define IAC		 	255


/* Assigned Telnet Options */
#define BINARY	 			0
#define ECHO				1
#define RECONNECT			2
#define SGA 				3
#define AMSN				4
#define STATUS				5
#define TIMING				6
#define RCTAN				7
#define OLW					8
#define OPS					9
#define OCRD				10
#define OHTS				11
#define OHTD				12
#define OFFD				13
#define OVTS				14
#define OVTD				15
#define OLFD				16
#define XASCII				17
#define LOGOUT				18
#define BYTEM				19
#define DET					20
#define SUPDUP				21
#define SUPDUPOUT			22
#define SENDLOC				23
#define TERMTYPE 			24
#define EOR					25
#define TACACSUID			26
#define OUTPUTMARK			27
#define TERMLOCNUM			28
#define REGIME3270			29
#define X3PAD				30
#define NAWS				31
#define TERMSPEED			32
#define TFLOWCNTRL			33
#define LINEMODE 			34
	#define MODE 1
		#define EDIT     1
		#define TRAPSIG  2
		#define MODE_ACK 4

	#define FORWARDMASK 2

	#define SLC 3 
		#define NO_SUPPORT		0
		#define CANTCHANGE		1
		#define SLC_VALUE		2
		#define SLC_DEFAULT		3
		#define SLC_LEVELBITS	3
		#define SLC_AWK			128

		#define SLC_SYNCH		1
		#define SLC_BRK			2
		#define SLC_IP			3
		#define SLC_AO			4
		#define SLC_AYT			5
		#define SLC_EOR			6
		#define SLC_ABORT		7
		#define SLC_EOF			8
		#define SLC_SUSP		9
		#define SLC_EC			10
		#define SLC_EL   		11
		#define SLC_EW   		12
		#define SLC_RP			13
		#define SLC_LNEXT		14
		#define SLC_XON			15
		#define SLC_XOFF		16
		#define SLC_FORW1		17
		#define SLC_FORW2		18
#define XDISPLOC			35
#define XOPTIONS			255

	
#define ESCFOUND 5
#define IACFOUND 6
#define NEGOTIATE 1

#ifdef DEBUG
char *telstates[]={
    "Subnegotiation End",
    "NOP",
    "Data Mark",
    "Break",
    "Interrupt Process",
    "Abort Output",
    "Are You There",
    "Erase Character",
    "Erase Line",
    "Go Ahead",
    "Subnegotiate",
	"Will",
	"Won't",
	"Do",
	"Don't"
};

char *teloptions[256]={
	"Binary",				/* 0 */
	"Echo",
	"Reconnection",
	"Supress Go Ahead",
	"Message Size Negotiation",
	"Status",				/* 5 */
	"Timing Mark",
	"Remote Controlled Trans and Echo",
	"Output Line Width",
	"Output Page Size",
	"Output Carriage-Return Disposition",	/* 10 */
	"Output Horizontal Tab Stops",
	"Output Horizontal Tab Disposition",
	"Output Formfeed Disposition",
	"Output Vertical Tabstops",
	"Output Vertical Tab Disposition",		/* 15 */
	"Output Linefeed Disposition",
	"Extended ASCII",
	"Logout",
	"Byte Macro",
	"Data Entry Terminal",					/* 20 */
	"SUPDUP",
	"SUPDUP Output",
	"Send Location",
	"Terminal Type",
	"End of Record",						/* 25 */
	"TACACS User Identification",
	"Output Marking",
	"Terminal Location Number",
	"3270 Regime",
	"X.3 PAD",								/* 30 */
	"Negotiate About Window Size",
	"Terminal Speed",
	"Toggle Flow Control",
	"Linemode",
	"X Display Location",					/* 35 */
	"36","37","38","39",
	"40","41","42","43","44","45","46","47","48","49",
	"50","51","52","53","54","55","56","57","58","59",
	"60","61","62","63","64","65","66","67","68","69",
	"70","71","72","73","74","75","76","77","78","79",
	"80","81","82","83","84","85","86","87","88","89",
	"90","91","92","93","94","95","96","97","98","99",
	"100","101","102","103","104","105","106","107","108","109",
	"110","111","112","113","114","115","116","117","118","119",
	"120","121","122","123","124","125","126","127","128","129",
	"130","131","132","133","134","135","136","137","138","139",
	"140","141","142","143","144","145","146","147","148","149",
	"150","151","152","153","154","155","156","157","158","159",
	"160","161","162","163","164","165","166","167","168","169",
	"170","171","172","173","174","175","176","177","178","179",
	"180","181","182","183","184","185","186","187","188","189",
	"190","191","192","193","194","195","196","197","198","199",
	"200","201","202","203","204","205","206","207","208","209",
	"210","211","212","213","214","215","216","217","218","219",
	"220","221","222","223","224","225","226","227","228","229",
	"230","231","232","233","234","235","236","237","238","239",
	"240","241","242","243","244","245","246","247","248","249",
	"250","251","252","253","254",
	"Extended Options List"		/* 255 */
};

char *LMoptions[]={
	"None",
	"SYNCH",
	"BREAK",
	"IP",
	"ABORT OUTPUT",
	"AYT",
	"EOR",
	"ABORT",
	"EOF",
	"SUSP",
	"EC",
	"EL",
	"EW",
	"RP",
	"LNEXT",
	"XON",
	"XOFF",
	"FORW1",
	"FORW2"
};

char *LMflags[]={
	"NOSUPPORT",
	"CANTCHANGE",
	"VALUE",
	"DEFAULT"
};
#endif


static void int14parse(int comm_port,unsigned char *st,int cnt);
static void int14parsewrite(int comm_port,char *dat,int len);

extern unsigned char port_buffer[NUM_COMM_PORTS][64];    /* four buffers to store the machine name to connect to */
extern unsigned char buffer_offset[NUM_COMM_PORTS];      /* the offset into the buffer currently */
extern int pnum[NUM_COMM_PORTS];                         /* the port numbers we are connected to */

extern volatile unsigned char *data_begin[NUM_COMM_PORTS];   /* pointers to the beginning of the data buffer for the comm.ports */
extern unsigned char *data_max[NUM_COMM_PORTS];     /* pointers to the maximum value for the data_end pointers */
extern volatile unsigned char *data_end[NUM_COMM_PORTS];     /* pointers to the end of the data buffer for the comm. ports */
extern unsigned char *data_start[NUM_COMM_PORTS];   /* pointers to the start of the data buffer for the comm. ports */

extern int comm_port_index[NPORTS];     /* lookup table from a network port number to a comm port number */
int telstate[NUM_COMM_PORTS];    /* the telnet state for each comm. port */
int substate[NUM_COMM_PORTS];    /* the telnet substate for each comm. port */
int igoahead[NUM_COMM_PORTS];
int ugoahead[NUM_COMM_PORTS];
int echo[NUM_COMM_PORTS];
int timing[NUM_COMM_PORTS];
static char c;

#ifndef DEBUG
#ifdef __WATCOMC__
void cdecl print_int(int num)
#else
void print_int(int num)
#endif
{
#ifdef QAK
    printf("number: %d:%x\n",num,num);
#else
    printf("number: %d:%x\t",num,num);
#endif
}   /* end print_int() */

#ifdef __WATCOMC__
void cdecl print_int2(int num)
#else
void print_int2(int num)
#endif
{
    printf("number2: %d:%x\n",num,num);
}   /* end print_int2() */
#endif

/*
*   int14open
*
*   Entry :
*       int comm_port - which comm port this connection is attached to
*
*/
#ifdef __WATCOMC__
int cdecl int14open(int comm_port)
#else
int int14open(int comm_port)
#endif
{
    int ev,what,dat;
	char *errmsg;

#ifdef QAK
printf("opening connection to: %s:%d\n",port_buffer[comm_port],comm_port);
#endif
    mp=gethostinfo(port_buffer[comm_port]);    /* look up in hosts cache, also checks for domain lookup */
#ifdef QAK
puts("after opening connection to the machine\n");
#endif
    if(!mp) {
        puts("need to domain lookup!?!?\n");
        return(0);          /* error return */
      } /* end if */
	else {
        while((ev=Sgetevent(ERRCLASS,&what,&dat))!=0) { /* sift through the error events potentially generated by a domain lookup */
#ifdef OLD_WAY
            if(dat!=801 && dat!=805) {  /* filter out reports of using domain lookup */
                errmsg=neterrstring(dat);
                puts(errmsg);
              } /* end if */
#endif
          } /* end while */
        if(0>(pnum[comm_port]=Snetopen(mp,mp->port))) { /* use requested ports RMG 931230 */
#ifdef QAK
			errhandle();
#else
            puts("Network open failed");
#endif
			netshut();
            return(0);
          } /* end if */
        comm_port_index[pnum[comm_port]]=comm_port;     /* assign the comm port to the TCP port number */
	  }
    return(1);      /* indicate sucessful session opening */
}   /* end int14open() */


/*********************************************************************/
/* int14process
*  take incoming data and process it.  Close the connection if it
*  is the end of the connection.
*/
int int14process(int comm_port)
{
	int cnt;

  cnt=netread(pnum[comm_port],buf,64); /* get some from incoming queue */
	if(cnt<0) {					/* close this session, if over */
    netclose(pnum[comm_port]);
    return(0);
  }

	if(cnt) 
    int14parse(comm_port,buf,cnt);            /* display on screen, etc.*/
#ifdef QAK
puts("leaving int14process()");
#endif
	return(0);
}

/*********************************************************************/
/*  int14parse
*   Do the telnet negotiation parsing.
*
*   look at the string which has just come in from outside and
*   check for special sequences that we are interested in.
*
*   Tries to pass through routine strings immediately, waiting for special
*   characters ESC and IAC to change modes.
*/
void int14parse(int comm_port,unsigned char *st,int cnt)
{
  int i;
	unsigned char *mark,*orig;

	orig=st;				/* remember beginning point */
	mark=st+cnt;			/* set to end of input string */
  netpush(pnum[comm_port]);

/*
*  traverse string, looking for any special characters which indicate that
*  we need to change modes.
*/
	while(st<mark) {
      switch(telstate[comm_port]) {
			case ESCFOUND:
        int14parsewrite(comm_port,"\033",1);        /* send the missing ESC */
        telstate[comm_port]=STNORM;
				break;

			case IACFOUND: 				/* telnet option negotiation */
				if(*st==IAC) {			/* real data=255 */
					st++;				/* real 255 will get sent */
                    telstate[comm_port]=STNORM;
					break;
				  }

                if(*st>239) {
                    telstate[comm_port]=*st++; /* by what the option is */
					break;
				  }

#ifdef DEBUG
                printf("strange telnet option %s\n",itoa(*st,parsedat,10));
#endif
				orig=st;
                telstate[comm_port]=STNORM;
				break;

            case EL:        /* received a telnet erase line command */
            case EC:        /* received a telnet erase character command */
            case AYT:       /* received a telnet Are-You-There command */
            case AO:        /* received a telnet Abort Output command */
            case IP:        /* received a telnet Interrupt Process command */
            case BREAK:     /* received a telnet Break command */
            case DM:        /* received a telnet Data Mark command */
            case NOP:       /* received a telnet No Operation command */
            case SE:        /* received a telnet Subnegotiation End command */
#ifdef DEBUG
                printf("RECV: %s\n",telstates[telstate[comm_port]-SE]);
#endif
                telstate[comm_port]=STNORM;
                orig=st;
                break;

            case GOAHEAD:       /* telnet go ahead option*/
                telstate[comm_port]=STNORM;
				orig=st;
				break;

			case DOTEL:		/* received a telnet DO negotiation */
#ifdef DEBUG
                printf("RECV: %s %s\n",telstates[telstate[comm_port]-SE],teloptions[*st]);
#endif
				switch(*st) {
					case SGA:		/* Suppress go-ahead */
                        if(igoahead[comm_port]) { /* suppress go-ahead */
#ifdef DEBUG
                            printf("SEND: %s %s\n",telstates[WILLTEL-SE],teloptions[*st]);
#endif
                            sprintf(parsedat,"%c%c%c",IAC,WILLTEL,*st);
                            netwrite(pnum[comm_port],parsedat,3);  /* take it */
                            igoahead[comm_port]=1;
						  }
#ifdef DEBUG
						else
                            printf("NO REPLY NEEDED: %s %s\n",telstates[WILLTEL-SE],teloptions[SGA]);
#endif
                        telstate[comm_port]=STNORM;
                        orig=++st;
						break;

					default:
#ifdef DEBUG
                        printf("SEND: %s %s\n",telstates[WONTTEL-SE],teloptions[*st]);
#endif
						sprintf(parsedat,"%c%c%c",IAC,WONTTEL,*st++);
                        netwrite(pnum[comm_port],parsedat,3);  /* refuse it */
                        telstate[comm_port]=STNORM;
						orig=st;
					  	break;

				}
				break;

			case DONTTEL:		/* Received a telnet DONT option */
#ifdef DEBUG
                printf("RECV: %s %s\n",telstates[telstate[comm_port]-SE],teloptions[*st]);
#endif
                telstate[comm_port]=STNORM;
				orig=++st;
				break;

			case WILLTEL:		/* received a telnet WILL option */
#ifdef DEBUG
                printf("RECV: %s %s\n",telstates[telstate[comm_port]-SE],teloptions[*st]);
#endif
                telstate[comm_port]=STNORM;
				switch(*st++) {
					case SGA:					/* suppress go-ahead */
                        if(ugoahead[comm_port])
							break;

                        ugoahead[comm_port]=1;
#ifdef DEBUG
                        printf("SEND: %s %s\n",telstates[DOTEL-SE],teloptions[*st]);
#endif
						sprintf(parsedat,"%c%c%c",IAC,DOTEL,3);	/* ack */
                        netwrite(pnum[comm_port],parsedat,3);
						break;

					case ECHO:						/* echo */
                        if(echo[comm_port])
							break;

                        echo[comm_port]=1;
#ifdef DEBUG
                        printf("SEND: %s %s\n",telstates[DOTEL-SE],teloptions[*st]);
#endif
						sprintf(parsedat,"%c%c%c",IAC,DOTEL,1);	/* ack */
                        netwrite(pnum[comm_port],parsedat,3);
						break;

					case TIMING:		/* Timing mark */
                        timing[comm_port]=0;
						break;

					default:
#ifdef DEBUG
                        printf("SEND: %s %s\n",telstates[DONTTEL-SE],teloptions[*st]);
#endif
						sprintf(parsedat,"%c%c%c",IAC,DONTTEL,*(st-1));
                        netwrite(pnum[comm_port],parsedat,3);  /* refuse it */
						break;
                  } /* end switch */
				orig=st;
				break;
							
			case WONTTEL:		/* Received a telnet WONT option */
#ifdef DEBUG
                printf("RECV: %s %s\n",telstates[telstate[comm_port]-SE],teloptions[*st]);
#endif
                telstate[comm_port]=STNORM;
				switch(*st++) {			/* which option? */
					case ECHO:				/* echo */
                        if(echo[comm_port])
							break;

                        echo[comm_port]=0;
						sprintf(parsedat,"%c%c%c",IAC,DONTTEL,ECHO);
                        netwrite(pnum[comm_port],parsedat,3);  /* OK with us */
						break;

					case TIMING:	/* Telnet timing mark option */
                        timing[comm_port]=0;
						break;

					default:
						break;
				  }
				orig=st;
				break;

            case SB:        /* telnet sub-options negotiation */
                telstate[comm_port]=NEGOTIATE;
				orig=st;
                i=substate[comm_port]=0;               /* Defined for each */
				break;

			case NEGOTIATE:
                if(substate[comm_port]<200) {
					switch(*st) {
						case IAC:
							if(*(st+1)==IAC) {	/* skip over double IAC's */
								parsedat[i++]=*st++;
                                parsedat[i++]=*st++;
							  } /* end if */
							else {
								parsedat[i]='\0';
                                substate[comm_port]=*st++;
							  } /* end else */
							break;

						default:
							parsedat[i++]=*st++;
							break;
					  }	/* end switch */
				  }	/* end if */
				else {
                    switch(substate[comm_port]) {
						case IAC:
                            substate[comm_port]=*st++;
							orig=st;
                            telstate[comm_port]=STNORM;
							break;

						default:
							orig=st;
                            telstate[comm_port]=STNORM;
							break;
					  }	/* end switch */
				  }	/* end else */
				break;

			default:
                telstate[comm_port]=STNORM;
				break;
		}

/*
* quick scan of the remaining string, skip chars while they are
* uninteresting
*/
        if(telstate[comm_port]==STNORM) {
/*
*  skip along as fast as possible until an interesting character is found
*/
			while(st<mark && *st!=27 && *st<IAC) {
/* #ifdef EIGHT_BIT_CLEAN */
/*                if(!tw->binary) */
  /* RMG 8bit */
//          *st &= 127;         /* mask off high bit */
/* #endif */
				st++;
			  }
            if(!timing[comm_port] && st>orig)
                int14parsewrite(comm_port,orig,st-orig);
			orig=st;				/* forget what we have sent already */
			if(st<mark)
				switch (*st) {
					case IAC:			/* telnet IAC */
                        telstate[comm_port]=IACFOUND;
						st++;
						break;

					case 27:			/* ESCape code */
						if(st==mark-1 || *(st+1)==12 || *(st+1)=='^')
                            telstate[comm_port]=ESCFOUND;
						st++;			/* strip or accept ESC char */
						break;

#ifdef DEBUG
					default:
#ifdef QAK
						vprint(cv," strange char>128\r\n");
#else
            puts("strange char>128\n");
#endif
						st++;
						break;
#endif
				  }	/* end switch */
		  }	/* end if */
    }  /* end while */
#ifdef QAK
puts("leaving int14parse()");
#endif
}

/*********************************************************************/
/*  int14parsewrite
*   write out some chars from parse
*   Has a choice of where to send the stuff
*/
void int14parsewrite(int comm_port,char *dat,int len)
{
    int i;      /* local counting variable */

    for(i=0; i<len; i++) {
#ifdef QAK
buffer_stat++;
if(buffer_stat>max_buffer_stat)
    max_buffer_stat=buffer_stat;
printf("buffer_stat=%d, max_buffer_stat=%d\n",buffer_stat,max_buffer_stat);
#endif
#ifdef DEBUG
printf("data_end[%d]=%p, data_begin[%d]=%p\n",comm_port,data_end[comm_port],comm_port,data_begin[comm_port]);
#endif
        /* small fix here  rmg 931100 */
//      *data_end[comm_port]++= dat[i];

        *data_end[comm_port] = dat[i];    /* store the character in the appropriate character buffer */
        data_end[comm_port]++;

        if(data_end[comm_port]>=data_max[comm_port])    /* check if we went off the end of the buffer */
            data_end[comm_port]=data_start[comm_port];  /* wrap back to the beginning */
        if(data_end[comm_port]==data_begin[comm_port]) {    /* check if we are going to wrap around */
#ifdef DEBUG
printf("data_end[%d] pushed data_begin[%d]\n",comm_port,comm_port);
#endif
            data_begin[comm_port]++;    /* lose data at the beginning of the buffer */
            if(data_begin[comm_port]>=data_max[comm_port])    /* check if we went off the end of the buffer */
                data_begin[comm_port]=data_start[comm_port];   /* wrap back to the beginning */
          } /* end if */
      } /* end for */
#ifdef DEBUG
printf("b:data_end[%d]=%p, data_begin[%d]=%p\n",comm_port,data_end[comm_port],comm_port,data_begin[comm_port]);
#endif
}

/*********************************************************************/
/*  get_comm_char
*   used by the interrupt service routine to get characters from the
*       comm. buffers
*/
#ifdef __WATCOMC__
int cdecl get_comm_char(int comm_port)
#else
int get_comm_char(int comm_port)
#endif
{
    int i;      /* local counting variable */

    while(data_end[comm_port]==data_begin[comm_port]);  /* wait for a character */
#ifdef QAK
buffer_stat--;
#endif
    i=*data_begin[comm_port]++;
    if(data_begin[comm_port]>=data_max[comm_port])
        data_begin[comm_port]=data_start[comm_port];
    return(i);
}

/*********************************************************************/
/*  int14netsleep
*   manage the network in the background, grabs packets from the network
*   Has a choice of where to send the stuff
*/
#ifdef __WATCOMC__
void cdecl int14netsleep(void)
#else
void int14netsleep(void)
#endif
{
    int ev,what,dat;
    char *errmsg;

#ifdef DEBUG
printf("int14netsleep().a: stackavail=%u\n",stackavail());
#endif
#ifdef CHECKNULL
/* If we are running in NULL debugging mode, check that */
    if(memcmp(nullbuf,MK_FP(0x0,0x0),1024)) {
        unsigned u;
        unsigned char *s;

        s=MK_FP(0x0,136);
        if(*s!=nullbuf[136]) {
            nullbuf[136]=*s++;
            nullbuf[137]=*s++;
            nullbuf[138]=*s++;
            nullbuf[139]=*s;
          } /* end if */
        else {
            puts("NULL overwritten!");
            s=MK_FP(0x0,0x0);
            for(u=0; u<1024; u++,s++)
                if(*s!=nullbuf[u])
                    printf("s=%p, u=%u, val=%u\n",s,u,(unsigned int)*s);
            getch();
          } /* end else */
      } /* end if */
#endif

/* check to make certain we are actually connected to the network before
*   dealing with network events
*/
    if(!connected_flags)    /* if any ports are connected, we have to parse data */
        return;
               
/*
*  get event from network, these two classes return all of the events
*  of user interest.
*/
#ifdef QAK
    ev=Sgetevent(USERCLASS | CONCLASS | ERRCLASS,&what,&dat);
#else
    ev=Sgetevent(CONCLASS | ERRCLASS,&what,&dat);
#endif
    if(!ev) {
#ifdef DEBUG
printf("int14netsleep().b: stackavail=%u\n",stackavail());
#endif
        return;
      } /* end if */
    if(what==ERRCLASS) {                /* error event */
#ifdef OLD_WAY
        errmsg=neterrstring(dat);
        puts(errmsg);       /* ifdef'ed out because of messing up DOS */
#endif
      }
    else
        if(what==CONCLASS) {        /* event of user interest */
            switch (ev) {
                case CONOPEN:               /* connection opened or closed */
#ifdef DEBUG
puts("CONOPEN received");
#endif
                    netpush(dat);           /* negotiation */
                    netwrite(dat,"\377\375\001\377\375\003\377\374\030",9);
                    break;

                default:
                    break;

                case CONDATA:               /* data arrived for me */
#ifdef OLD_WAY
                    cnt=netread(dat,buf,80);
                    for(i=0; i<cnt; i++)
                        if(buf[i]<128)
                            putchar(buf[i]);
#else
#ifdef QAK
puts("int14netsleep: data received\n");
#endif
                    if(int14process(comm_port_index[dat]))
                        c=16;       /* break out of the loop */
#endif
                    break;

                case CONFAIL:
#ifdef DEBUG
puts("CONFAIL received");
#endif
#ifdef DEBUG
                    puts("Connection attempt failed");
#endif
                                        /* drop through to exit */

                case CONCLOSE:
#ifdef DEBUG
puts("CONCLOSE received");
#endif
#ifdef QAK
                    netshut();
                    exit(1);
#else
                    c=16;
#endif
                }
            }
#ifdef QAK
        else
            if(what==USERCLASS) {
                switch (ev) {
                    case DOMOK:                         /* domain worked */
#ifdef DEBUG
puts("DOMOK received");
#endif
                        mp=Slooknum(dat);               /* get machine info */
                        pnum[PORT]=Snetopen(mp,mp->port);           /* open to host name */ /* use requested ports RMG 931230 */
                        break;

                    case DOMFAIL:   /* domain failed */
#ifdef DEBUG
                        n_puts("domain failed");
#endif
#ifdef QAK
                        netshut();
                        exit(1);
#else
                        c=16;
#endif
                    default:
                        break;
                  }
              }
#endif
#ifdef DEBUG
printf("int14netsleep().c: stackavail=%u\n",stackavail());
#endif
}

