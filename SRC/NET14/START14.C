
#define NET14MASTER

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
#include "externs.h"

/* #define DEBUG */

/*
*	Global variables
*/
static unsigned char path_name[_MAX_DRIVE+_MAX_DIR],   /* character storage for the path name */
	temp_str[100],temp_data[100];		/* temporary character storage */
#ifdef CHECKNULL
unsigned char nullbuf[1024];        /* buffer to hold the lowest 1024 bytes */
#endif
unsigned char buf[256];
static char *config;
int debug = 0;              /* enable with -D option */

#define SERIAL  0x14
#define NUM_COMM_PORTS  4   /* the number of comm. ports supported, remember to change this variable in int14.asm also */

extern volatile unsigned char *data_begin[NUM_COMM_PORTS];   /* pointers to the beginning of the data buffer for the comm.ports */
extern unsigned char *data_max[NUM_COMM_PORTS];     /* pointers to the maximum value for the data_end pointers */
extern volatile unsigned char *data_end[NUM_COMM_PORTS];     /* pointers to the end of the data buffer for the comm. ports */
extern unsigned char *data_start[NUM_COMM_PORTS];   /* pointers to the start of the data buffer for the comm. ports */
#define PORT_DATA_SIZE      2048        /* this is sort of a guess, might need larger */
unsigned char port_data[NUM_COMM_PORTS][PORT_DATA_SIZE]; /* data buffer for data read in from the comm. port */
int comm_port_index[NPORTS];                /* lookup table from a network port number to a comm port number */
unsigned char myipnum[4];           /* the ip number for this machine */
uint speedup=2;

#ifdef OLD_WAY
int	bypass_passwd=0;		/* whether to bypass the password check, not used */
struct machinfo *mp;

extern struct config def;   /* Default settings obtained from host file */
extern unsigned char initialized_flags;     /* flags indicating whether a port has been initialized */
extern unsigned char connected_flags;       /* flags indicating whether a port is connected yet */
extern unsigned char opening_flags;         /* flags indicating whether a port is negotiating an open connection */
extern unsigned char port_buffer[NUM_COMM_PORTS][64];    /* four buffers to store the machine name to connect to */
extern unsigned char buffer_offset[NUM_COMM_PORTS];      /* the offset into the buffer currently */
extern int pnum[NUM_COMM_PORTS];                         /* the port number we are connected to */



unsigned char parsedat[32];                 /* character buffer to store network writes */
int telstate[NUM_COMM_PORTS]={STNORM,STNORM,STNORM,STNORM};  /* the telnet state for each comm. port */
int substate[NUM_COMM_PORTS]={0,0,0,0};                  /* the telnet state for each comm. port */
int echo[NUM_COMM_PORTS]={1,1,1,1};
int igoahead[NUM_COMM_PORTS]={0,0,0,0};
int ugoahead[NUM_COMM_PORTS]={0,0,0,0};
int timing[NUM_COMM_PORTS]={0,0,0,0};
char c;
#ifdef QAK
int buffer_stat=0,max_buffer_stat=0;
#endif
#endif

/*
*	main ( argc, argv )
*
*	Entry : 
*
*		parameter 1 : machine name
*
*/
 
/* #define TSR
#define PORT 1 */

int main(argc,argv)
int argc;
char *argv[];
{
#ifdef TSR
    union REGS inregs;      /* register set going into the interrupt */
    union REGS outregs;     /* register set going out of the interrupt */
    unsigned int str_length;
#endif
    int i,
#ifdef TSR
        cnt,ev,what,dat,
#endif
        exec_num;           /* the argument number which is the name of the file to execute */
    char file_name[_MAX_PATH],
        *str,
        spawn_str[_MAX_PATH];    /* file name oft he executable to spawn */

#ifdef QAK
printf("stackavail=%u\n",stackavail());
#endif

#ifdef __TURBOC__
	fnsplit(argv[0],path_name,buf,temp_str,temp_data);
#else
    _splitpath(argv[0],path_name,buf,temp_str,temp_data);   /* split the full path name of net14.exe into it's components */
#endif
	strcat(path_name,buf);	/* append the real path name to the drive specifier */


    config = (getenv("CONFIG.TEL"));    /* check for a config.tel in the environment */
    if(config)                  /* set a different config.tel file */
        Shostfile(config);

#ifdef QAK
    puts("National Center for Supercomputing Applications");    /* put the banner on the screen */
    puts("Interrupt 14h driver");
    puts("January 1991\n");
#endif

#ifndef TSR
    if(argc<2) {
        puts("Usage: net14 [-h hostfile] childname [child_param]\n");
		exit(1);
      } /* end if */
#endif
/*
*  work on parms
*/
	for(i=1; i<argc; i++) {			/* look at each parm */
        if(*argv[i]=='-') {     /* command line parameter */
			switch(*(argv[i]+1)) {
				case 'h':
					Shostfile(argv[++i]);	/* set new name for host file */
					break;

                case 'D':   /* turn on debugging */
                    debug=1;
                    break;

                case '#':   /* set the clock speedup */
                    speedup=atoi(argv[i]+2);    /* get the speedup factor */
                    if(speedup!=1 && speedup!=2 && speedup!=4 && speedup!=8 && speedup!=16 && speedup!=32)  /* only accept multiples of two between 1 and 32 for the speedup factor */
                        speedup=1;
                    break;

				default:
                    puts("Usage: net14 [-h hostfile] [-#<speedup>] filename [param]\n");
					exit(1);
			  }
		  }
        else {  /* must be the name of the file to execute */
            exec_num=i;     /* save the argument number of the name to execute */
            break;          /* break out of the loop (any further arguments must pertain to the program to execute) */
          } /* end else */
	  }

    if(!int14init())    /* install the network stuff */
        exit(1);

    for(i=0; i<NUM_COMM_PORTS; i++) {    /* initialize the port buffers */
        data_begin[i]=port_data[i];     /* start at the beginning of the buffer */
        data_end[i]=port_data[i];       /* set the end of the buffer at the beginning currently */
        data_start[i]=port_data[i];     /* set the start of each buffer */
        data_max[i]=port_data[i]+PORT_DATA_SIZE;    /* set the maximum value the buffer can use */
/* Special: grab at least one port to communicate TCP over */
        makeport();
#ifdef DEBUG
printf("%d: data_begin=%p, data_max=%p, data_end=%p\n",i,data_begin[i],data_max[i],data_end[i]);
#endif
      } /* end for */

    for(i=0; i<NPORTS; i++)     /* initialize the comm port indices */
        comm_port_index[i]=-1;

#ifdef DEBUG
printf("data_begin[2]=[%x][%x]\n",(unsigned)(*((unsigned *)&data_begin[2])),(unsigned)(*(((unsigned *)&data_begin[2])+1)));
#endif

    int14inst();        /* install the int 14h handler */
#ifdef QAK
printf("int14 =%p\n",_dos_getvect(0x14));
#endif

/* #ifndef TSR */
    timeinst();         /* install the timer interrupt handler */
/* #endif */
#ifdef QAK
printf("int1C =%p\n",_dos_getvect(0x1C));
#endif

#ifdef TSR
    inregs.h.ah=0;          /* set to initialize the comm. port */
    inregs.x.dx=PORT;          /* use port two */
    int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */

    inregs.h.ah=1;          /* set to send a character */
    inregs.h.al=2;          /* send initialization character */
    inregs.x.dx=PORT;          /* use port two */
    int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */

    str=argv[1];
    str_length=strlen(str);
   for(i=0; i<(int)str_length; i++,str++) {
        inregs.h.ah=1;          /* set to send a character */
        inregs.h.al=*str;       /* send name character */
        inregs.x.dx=PORT;          /* use port two */
        int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
      } /* end for */

    inregs.h.ah=1;          /* set to send a character */
    inregs.h.al=3;          /* send initialization character */
    inregs.x.dx=PORT;          /* use port two */
    int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */

#ifdef QAK
    if(!int14open(argv[1]),2)    /* open session to the specified machine */
        exit(1);
#endif

	c = 0;
    do {
        if(kbhit()) {               /* has key been pressed */
			c=(char)getch();
if(c==16)
    break;
if(c==14)
#ifdef QAK
    printf("max_buffer_stat=%d\n",max_buffer_stat);
#else
    printf("stackavail()=%u\n",stackavail());
#endif
            inregs.h.ah=1;          /* set to send a character */
            inregs.h.al=c;          /* send character */
            inregs.x.dx=PORT;          /* use port two */
            int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
		  }
#ifdef QAK
int14netsleep();            /* call this to maintain network */
#endif
#ifdef QAK
        inregs.h.ah=3;          /* set to check status */
        inregs.x.dx=PORT;          /* use port two */
        int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
        if(outregs.h.ah&0x01) { /* check for data being ready */
            inregs.h.ah=2;          /* set to receive a character */
            inregs.x.dx=PORT;          /* use port two */
            int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
            c=(int)outregs.h.al;         /* get the return value */
            putchar(c);
          } /* end if */
#else
        dat=int14check();
        if(dat&0x0100) {        /* chek for data begin ready */
            c=int14receive();
            putchar(c);
          } /* end if */
#endif
      } while(c!=16);            /* Ctrl-P, arbitrary escape */
#ifdef QAK
printf("max_buffer_stat=%d\n",max_buffer_stat);
#endif
    timedeinst();   /* remove the timer interrupt handler */
    int14deinst();  /* remove the int 14h handler */
    netclose(pnum[PORT]);
	netshut();
#else
#ifdef QAK
    go_tsr();
#else
#ifdef QAK
printf("sizeof(struct port)=%d\n",sizeof(struct port));
#endif
#ifdef CHECKNULL
    memcpy(nullbuf,MK_FP(0x0,0x0),1024);
#endif
#ifdef OLD_WAY
    str=malloc(5000);      /* malloc a large chunk of memory */
    free(str);      /* just to get it into the memory pool */
#endif
    strupr(argv[exec_num]);
    _searchenv(argv[exec_num],"PATH",file_name);  /* get the pathname of the file to execute */
    if(file_name[0]=='\0') {  /* couldn't find the executable in the path, is must be in the current directory */
        strcpy(spawn_str,argv[exec_num]);
        strcat(spawn_str,".COM");
        _searchenv(spawn_str,"PATH",file_name);  /* get the pathname of the file to execute */
        if(file_name[0]=='\0') {  /* couldn't find the executable in the path, is must be in the current directory */
            strcpy(spawn_str,argv[exec_num]);
            strcat(spawn_str,".EXE");
            _searchenv(spawn_str,"PATH",file_name);  /* get the pathname of the file to execute */
            if(file_name[0]=='\0')  /* couldn't find the executable in the path, is must be in the current directory */
                getcwd(file_name,_MAX_PATH);
          } /* end if */
      } /* end if */
#ifdef CHECKNULL
/* If we are running in NULL debugging mode, check that */
    if(memcmp(nullbuf,MK_FP(0x0,0x0),1024)) {
        unsigned u;
        unsigned char *s;

        puts("NULL overwritten!");
        s=MK_FP(0x0,0x0);
        for(u=0; u<1024; u++,s++)
            if(*s!=nullbuf[u])
                printf("s=%p, u=%u, val=%u\n",s,u,(unsigned int)*s);
        getch();
      } /* end if */
    else {
        puts("NULL looks fine right before spawning a program");
        getch();
      } /* end else */
#endif

    i=spawnvp(P_WAIT,file_name,&argv[exec_num]);   /* call the program which needs the re-direction */
puts("After spawn returns");
    if(i==(-1)) {       /* check for error spawning the process */
        switch(errno) { /* find out what the problem was */
            case E2BIG:     /* argument list too long to child process */
                puts("Error: bad argument list to child process\n");
                break;

            case EINVAL:    /* the execution mode was invalid */
                puts("Error: invalid execution mode for child process\n");
                break;

            case ENOENT:    /* file not found to spawn */
                puts("Error: child process not found\n");
                break;

            case ENOEXEC:   /* not an executable file */
                puts("Error: child process is not an executable file\n");
                break;

            case ENOMEM:    /* not enough memory to spawn process */
                puts("Error: not enough memory for child process\n");
                break;
          } /* end switch() */
      } /* end if */
    timedeinst();   /* remove the timer interrupt handler */
    int14deinst();  /* remove the int 14h handler */
puts("After removing the interrupt handlers");
    netshut();
puts("After shutting down the network");
#endif
#endif
    return(0);
}

#ifdef QAK
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
#endif

