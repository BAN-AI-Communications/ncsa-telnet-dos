/*
*  defines that are only applicable to the IBM PC/ PC-DOS environment
*  other files will take this one's place for other machines
*/

#ifndef PCDEFS_H
#define PCDEFS_H
/*
*  hardware address for Ethernet broadcast address (used for ARP)
*/
#ifdef MASTERDEF
unsigned char bseed[]={0xff,0xff,0xff,0xff,0xff,0xff},
#if defined(NET14)
    raw[7000];
#elif defined(__TURBOC__) || defined(__WATCOMC__)
	raw[10000] = {0};
#else
	raw[17000];
#endif
#else
extern unsigned char bseed[],raw[];
#endif

/*
*  timing information is machine dependent
*/
#ifndef REALTIME
#define time(A) n_clicks()
#endif
#define movenbytes(A,B,C) movebytes((A),(B),(C))

/*
*  timeout for response to ARP packet for Ethernet
*/
/* was 15 */
#define DLAYTIMEOUT 8
/*
*  how often to poke a TCP connection to keep it alive and make
*  sure other side hasn't crashed. (poke) in 1/18ths sec
*  And, timeout interval
*/
#define POKEINTERVAL     3000
#define MAXRTO  	  100
#define MINRTO              5
#define ARPTO  		   20
#define CACHETO 	 7000
#define WAITTIME	   35
#define LASTTIME 	 4000
#define CACHELEN 	   10
#define MAXSEG 		 1024
#define CREDIT 		 4096

#endif
