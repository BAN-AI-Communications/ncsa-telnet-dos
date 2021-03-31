/* cu-notic.txt		 NCSA Telnet version 2.2C	 2/3/89
   Notice:
		Portions of this file have been modified by
		The Educational Resources Center of Clarkson University.

		All modifications made by Clarkson University are hereby placed
		in the public domain, provided the following statement remain in
		all source files.

		"Portions Developed by the Educational Resources Center, 
				Clarkson University"

		Bugs and comments to bkc@omnigate.clarkson.edu
								bkc@clgw.bitnet

		Brad Clements
		Educational Resources Center
		Clarkson University
*/

/* packet.c - FTP Software Packet Interface for NCSA TELNET
   Author: Brad Clements  bkc@omnigate.clarkson.edu
		   Clarskon University
		   10/24/88

   Assumes Microsoft C large model
*/
/*
*  packet.c
*****************************************************************************
*																		  	*
*	  part of:																*
*	  TCP/UDP/ICMP/IP Network kernel for NCSA Telnet						*
*	  by Tim Krauskopf														*
*																		  	*
*	  National Center for Supercomputing Applications					 	*
*	  152 Computing Applications Building								 	*
*	  605 E. Springfield Ave.											 	*
*	  Champaign, IL  61820													*
*																		  	*
*****************************************************************************
*
*	Packet driver code modifed for Microsoft and Lattice C by
*		Quincey Koziol 8/18/89
*
*   SLIP driver support added by
*       Nelson B. Bolyard 9/91
*/
/* #define XDEBUG  1 */

#define PACKET_MASTER

/*#define DEBUG*/

#ifdef DEBUG	
#include <conio.h>
#endif

#include "debug.h"

#ifdef  USE_INLINE
#pragma inline
#endif
#include <stdio.h>
#include <string.h>
#include <dos.h>
#ifdef __ZTC__
#include <int.h>
#endif
#include "whatami.h"
#include "windat.h"
#include "packet.h"
#include "externs.h"

extern unsigned char rstat;					/* status from last read */
extern char *bufpt,*bufend,*bufread,*buforg;
extern int bufbig,buflim;

static int pkt_receive_fake(char *src,int len);

static int locate_pkt_vector(unsigned int vec)            /* search for the packet driver */
{
    struct vector_ptr {
        char far *real_vector;
      } *vptr;
    char    far  *xvptr;
    int vector,vmax;

    if(packet_vector)
        return(0);       /* already found!     */
    vector=0x60;
    vmax=0x7f;

#ifdef DEBUG
printf("%s[%d],vec = %d\n",__FILE__,__LINE__,vec);
printf("setting vec to 0x60\n");
printf("%s[%d],vec = %d\n",__FILE__,__LINE__,vec);
#endif
    if((vec>=0x60) && (vec<=0x7f))
        vmax=vector=vec;
#ifdef NET14
    else
        n_puts("Warning, packet driver vector incorrect, using default search\n\r");
#endif

    for(; vector<=vmax; vector++) {
#ifdef COMPILER_BUG
        vptr = (struct vector_ptr *)MK_FP(0,vector * 4);
#else
        vptr = (struct vector_ptr *)((unsigned long)vector * 4);
#endif
        xvptr = vptr->real_vector;
#ifdef DEBUG
printf("Checking vector %X\n", vector);
printf("vptr = %p\n",vptr);
printf("real vec = %p\n",xvptr);
printf("string = %s\n",xvptr+3);
#endif
        if(!real_strncmp(xvptr+3,"PKT DRVR",8)) {
            packet_vector=vector;
BUG("found vector");
            return(0);
          } /* end if */
      } /* end for */

BUG("didn't find vector");
    return(-1);
}

static int pkt_receive_fake(char *src,int len)
{
    char *dest=0;
    int pkt_size;
	
    pkt_size=len+sizeof(int);
#ifdef USE_INLINE
	asm cli			/* inline assembly  no ints during this operation	 */
#else
	clear_int();
#endif

    if(bufbig<=buflim) {  /* if there is space remaining */
        if(bufpt>bufend)   /* if at end of wrap area then wrap it */
            bufpt=buforg;
        dest=bufpt;
        bufpt+=pkt_size;
        bufbig+=pkt_size;
      } /* end if */

#ifdef USE_INLINE
	asm sti			/* inline assembly	  */
#else
	set_int();
#endif

    if(dest==0)
        return(-1);

    *(int *)dest=len;
    movebytes(dest+sizeof(int),src,len);
    return(0);
}

/* we try and locate the packet driver and open ARP and IP handles. */
/* also open RARP handle */
int CDECL pketopen(unsigned char *s,unsigned int irq,unsigned int address,unsigned int ioaddr)
/* unsigned char *s;            ethernet address */
/* unsigned int    irq;             don't need this    */
/* unsigned int    address;         address is packet class */
/* unsigned int    ioaddr;          packet int, or 0 */
{
	char buff[256];

	irq=irq;		/* get rid of compiler warning */
    if(locate_pkt_vector(ioaddr)) {
		n_puts("No Packet Driver found at specified location. Change ioaddr in config.tel\n\r");
		return(-1);
      } /* end if */

	if(ip_handle!=(-1))
		return(0);

    if(pkt_driver_info(0))  /* get the packet driver information */
        return(-1);

    if((ip_handle=pkt_access_type(driver_class,IT_ANY,0,iptype,(driver_class!=IC_SLIP ? IPLEN : 0),pkt_receiver))==-1) {
        sprintf(buff,"driver_version=%d, driver_class=%d, driver_type=%d, driver_number=%d\r\n",driver_version,(int)driver_class,driver_type,(int)driver_number);
		n_puts(buff);
        sprintf(buff,"Can't Access IP handle interface type %d\r\nPacket Driver probably not loaded, vector=%d\r\n",driver_class,packet_vector);
		n_puts(buff);
		return(-2);
	  }	/* end if */

    if(driver_class!=IC_SLIP) {
        if((arp_handle=pkt_access_type(driver_class,IT_ANY,0,arptype,ARPLEN,pkt_receiver))==-1) {
            sprintf(buff,"Can't Access ARP handle\r\n");
            n_puts(buff);
            pkt_release_type(ip_handle);
            return(-3);
          } /* end if */

    /* Grab the RARP handle also */
        if((rarp_handle=pkt_access_type(driver_class,IT_ANY,0,rarptype,RARPLEN,pkt_receiver))==-1) {
            sprintf(buff,"Can't Access RARP handle\r\n");
            n_puts(buff);
            pkt_release_type(ip_handle);
            pkt_release_type(arp_handle);   /* let go of the arp handle also */
            return(-4);
          } /* end if */
      } /* end if */

	pkt_get_address(ip_handle,s,6);
    pkt_set_recv_mode(ip_handle,3);     /* receive broadcasts also */
    return(0);
}

int CDECL pkgetaddr(unsigned char *s,unsigned int address,unsigned int ioaddr)    /* get the ethernet address */
/* unsigned char   *s;             ethernet address */
/* unsigned int     address;        address is packet class */
/* unsigned int     ioaddr;         packet int, or 0 */
{
BUG("pkgetaddr");
    if(ip_handle==(-1))
        return(pketopen(s,0,address,ioaddr));
BUG("about to pkt_get_address");
    pkt_get_address(ip_handle,s,6);
    return(0);
}

void CDECL pkrecv(void)                       /* no op for this interface  */
{
}

int CDECL pketclose(void)                  /* throw away our handles */
{
    pkt_release_type(ip_handle);
    if(driver_class!=IC_SLIP) {
        pkt_release_type(arp_handle);
        pkt_release_type(rarp_handle);
      } /* end if */
    return(0);
}

int CDECL pkxmit(DLAYER *packet,unsigned int length)          /* transmit a packet */
{
    if(driver_class==IC_SLIP) {
        ARPKT* rptr;
        ARPKT replyarp;

        switch(packet->type) {
             case EIP:   /* drop the Ethernet header, send the IP Datagram */
                packet++;
                length-=sizeof(*packet);
                break;

            case EARP:  /* if an arp reqeuest, convert to reply, fake receive */
                rptr=(ARPKT *)packet;
                if(rptr->op!=intswap(ARPREQ))
                    return(0);   /* pretend we sent it ;-) */

                movebytes(replyarp.d.dest,rptr->d.me,DADDLEN);
                movebytes(replyarp.d.me,rptr->spa,4);
                replyarp.d.me[4]=0;
                replyarp.d.me[5]=0;
                replyarp.d.type=rptr->d.type;
                movebytes(&replyarp.hrd,&rptr->hrd,6);
                replyarp.op=intswap(ARPREP);
                movebytes(replyarp.sha,replyarp.d.me,DADDLEN);
                movebytes(replyarp.spa,rptr->tpa,4);
                movebytes(replyarp.tha,rptr->sha,10);
                return(pkt_receive_fake((char *)&replyarp,sizeof(replyarp)));

            default:
                return(0);  /* pretend we sent it */
          } /* end switch */
      } /* end if */
    else {
      if(length<60)
        length=60;          /* what a terrible hack! */
      } /* end else */
    if(pkt_send_pkt((char *)packet,length))
        return(-1);
    return(0);
}

#ifdef __TURBOC__
void interrupt pkt_receiver2(unsigned int bp,unsigned int di,unsigned int si,unsigned int ds,unsigned int es,unsigned int dx,unsigned int cx,unsigned int bx,unsigned int ax)
#elif __WATCOMC__
void interrupt pkt_receiver2(union INTPACK r)
#elif __ZTC__
extern void pkt_receiver2(struct INT_DATA *pd);
#else
void interrupt pkt_receiver2(unsigned int es,unsigned int ds,unsigned int di,unsigned int si,unsigned int bp,unsigned int sp,unsigned int bx,unsigned int dx,unsigned int cx,unsigned int ax)
#endif
{
    char *where_to_write;
    int packet_size;

   /* this receiver function assumes that between the first and second call
	  from the packet driver, the underlying telnet code will not access
	  the buffer.
   */

   /* here's an incoming packet from the packet driver, first see if we
	  have enough space for it */

#ifdef __WATCOMC__
    if(!r.w.ax) {
		if(bufbig <= buflim) {  /* if there is space remaining */
			if(bufpt > bufend)   /* if at end of wrap area then wrap it */
				bufpt = buforg;

            if(driver_class==IC_SLIP) {
                DLAYER *packet;

                packet=(DLAYER *)(bufpt+sizeof(int));
                packet->type=EIP;
                packet_size=sizeof(*packet)+r.w.cx;
                where_to_write=bufpt+sizeof(int)+sizeof(*packet);
              } /* end if */
            else {
                packet_size=r.w.cx;
                where_to_write=bufpt+sizeof(int);
              } /* end else */

            *(int *)bufpt=packet_size;
            bufpt+=packet_size+sizeof(int);
            bufbig+=packet_size+sizeof(int);
            r.w.es=FP_SEG(where_to_write);
            r.w.di=FP_OFF(where_to_write);
		  }
		else {
            r.w.es = r.w.di = 0;                /* no room */
		}
	}
#elif __ZTC__
    if(!pd->regs.x.ax) {
		if(bufbig <= buflim) {  /* if there is space remaining */
			if(bufpt > bufend)   /* if at end of wrap area then wrap it */
				bufpt = buforg;

            if(driver_class==IC_SLIP) {
                DLAYER *packet;

                packet=(DLAYER *)(bufpt+sizeof(int));
                packet->type=EIP;
                packet_size=sizeof(*packet)+pd->regs.x.cx;
                where_to_write=bufpt+sizeof(int)+sizeof(*packet);
              } /* end if */
            else {
                packet_size=pd->regs.x.cx;
                where_to_write=bufpt+sizeof(int);
              } /* end else */

            *(int *)bufpt=packet_size;
            bufpt+=packet_size+sizeof(int);
            bufbig+=packet_size+sizeof(int);
            pd->regs.x.es=FP_SEG(where_to_write);
            pd->regs.x.di=FP_OFF(where_to_write);
		  }
		else {
            pd->regs.x.es = pd->regs.x.di = 0;                /* no room */
		}
	}
#else
#ifndef __TURBOC__
	ds=ds;		/* get rid of compiler warnings */
	si=si;
	bp=bp;
	sp=sp;
	bx=bx;
	dx=dx;
#endif
	if(!ax) {
        if(bufbig<=buflim) {  /* if there is space remaining */
            if(bufpt>bufend)   /* if at end of wrap area then wrap it */
                bufpt=buforg;
            if(driver_class==IC_SLIP) {
                DLAYER *packet;

                packet=(DLAYER *)(bufpt+sizeof(int));
                packet->type=EIP;
                packet_size=sizeof(*packet)+cx;
                where_to_write=bufpt+sizeof(int)+sizeof(*packet);
              } /* end if */
            else {
                packet_size=cx;
                where_to_write=bufpt+sizeof(int);
              } /* end else */

            *(int *)bufpt=packet_size;
            bufpt+=packet_size+sizeof(int);
            bufbig+=packet_size+sizeof(int);
            es=FP_SEG(where_to_write);
            di=FP_OFF(where_to_write);
		  }
		else {
			es = di = 0;				/* no room */
		}
	}
#endif
	return;					  /* we do nothing if its the second call */
}

void CDECL pketupdate(void)                   /* update the pointers */
{
    int packet_size;
    int *size_ptr;

    size_ptr=(int *)bufread;
    packet_size=*size_ptr;
    bufread+=packet_size+sizeof(int);
    if(bufread>bufend)
        bufread=buforg;

#ifdef USE_INLINE
    asm cli                   /* inline assembly  no ints during this operation  */
    bufbig-=packet_size+sizeof(int);
    asm sti                   /* inline assembly      */
#else
    clear_int();
    bufbig-=packet_size+sizeof(int);
	set_int();
#endif
}

