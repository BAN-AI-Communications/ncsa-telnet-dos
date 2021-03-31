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
#ifdef MSC
#include <signal.h>
#include <time.h>
#endif


/* #define DEBUG */

/*
*	Global variables
*/

#define SERIAL  0x14
#define PORT    1

/*
*	main ( argc, argv )
*
*	Entry : 
*
*		parameter 1 : machine name
*
*/
 
int main(argc,argv)
int argc;
char *argv[];
{
    union REGS inregs;      /* register set going into the interrupt */
    union REGS outregs;     /* register set going out of the interrupt */
    int i,cnt,ev,what,dat;
    unsigned int str_length;
    char *errmsg,
        *str;
    char c;

    if(argc<2)
		exit(1);

printf("int14h vector=%p\n",_dos_getvect(0x14));
printf("int1Ch vector=%p\n",_dos_getvect(0x1C));

#ifdef QAK
    inregs.h.ah=3;          /* set to initialize the comm. port */
    inregs.x.dx=PORT;          /* use port two */
    int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
printf("ax=%X\n",outregs.x.ax);     /* print the return value */
#endif

    inregs.h.ah=0;          /* set to initialize the comm. port */
    inregs.x.dx=PORT;          /* use port two */
    int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
printf("ax=%X\n",outregs.x.ax);     /* print the return value */

    inregs.h.ah=1;          /* set to send a character */
    inregs.h.al=2;          /* send initialization character */
    inregs.x.dx=PORT;          /* use port two */
    int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
printf("ax=%X\n",outregs.x.ax);     /* print the return value */

    str=argv[1];
    str_length=strlen(str);
    for(i=0; i<(int)str_length; i++,str++) {
        inregs.h.ah=1;          /* set to send a character */
        inregs.h.al=*str;       /* send name character */
        inregs.x.dx=PORT;          /* use port two */
        int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
printf("ax=%X\n",outregs.x.ax);     /* print the return value */
      } /* end for */

puts("right before opening a connection");
    inregs.h.ah=1;          /* set to send a character */
    inregs.h.al=3;          /* send initialization character */
    inregs.x.dx=PORT;          /* use port two */
    int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
printf("ax=%X\n",outregs.x.ax);     /* print the return value */

puts("after opening a connection");
	c = 0;
puts("before dropping into the do loop");
    do {
        if(kbhit()) {               /* has key been pressed */
			c=(char)getch();
if(c==16)
    break;
            inregs.h.ah=1;          /* set to send a character */
            inregs.h.al=c;          /* send character */
            inregs.x.dx=PORT;          /* use port two */
            int86(SERIAL,&inregs,&outregs);     /* call to initialize the interrupt */
		  }
#ifdef QAK
puts("before int14check() call");
#endif
        dat=int14check();
#ifdef QAK
printf("after int14check() call, dat=%X\n",dat);
#endif
        if(dat&0x0100) {        /* check for data being ready */
#ifdef QAK
printf("data ready, dat=%x",dat);
#endif
            c=int14receive();
#ifdef QAK
printf("c=%x:%d:%c\n",(unsigned)c,(unsigned)c,(char)c);
#endif
            putchar(c);
          } /* end if */
      } while(c!=16);            /* Ctrl-P, arbitrary escape */
#ifdef QAK
    timedeinst();   /* remove the timer interrupt handler */
    int14deinst();  /* remove the int 14h handler */
    netclose(pnum[2]);
	netshut();
#endif
    exit(0);
}

