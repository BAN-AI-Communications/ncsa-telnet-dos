/* 1152, Wed 29 Aug 90

   DNDLL:  DECnet DLL driver for NCSA TELNET

   This driver allows TELNET to work through the DECNET-DOS
   Datalink Layer, using the DLL interface documented in the
   VAXmate Technical Reference Manual Volume 2, AA-HD95A-TK.

   Support for DECNET-DOS is provided on the PC by a set of TSRs.
   The minimum set of these is just SCH (the DECnet process scheduler)
   and DLL (the Datalink Layer driver).  Other TSRs can be loaded
   into memory to provide other services, e.g. LAT (Local Area Transport)
   and DNP (DECNET).

   DEC supply (as part of DECNET-DOS) DLL drivers for their own
   ethernet card (the DEPCA), and a range of other cards from
   MICOM and 3COM.  DLL drivers for the Western Digital 8003 card
   (marketed here in New Zealand as the Amtec Ethercard) are also
   available.  This TELNET driver will work with any of the DLL
   drivers - I have tested it with the WD8003 as well as the DEPCA.

   DLL allows you to open a 'portal' to ethernet, specifying which
   protocol id it will carry.  Any incoming packets with this
   protocol will then be passed to you as received packets.
   For TELNET I open three portals, one for IP, ARP and RARP.
   Since the TELNET interface routines expect a complete packet
   (i.e. source, dest, protocol + data), I have to move the header
   fields into and out of the DLL data structures, but this is no
   problem.

   One advantage of using DLL is that you can start a TELNET session,
   use Alt-E to get into DOS, run a short LAT or DECNET session, then
   exit back into TELNET.

   Nevil Brownlee,  n.brownlee@aukuni.ac.nz
   Computer Centre,  University of Auckland */

#define noDCBERR
#define noDCBTRACE

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#include <dos.h>

#include "protocol.h"
#include "decnet.h"		/* header information for the decnet driver */
#include "externs.h"
#include "data.h"

#ifdef Lattice
#include <stdlib.h>
#define far
#endif

/* external packet variables */
extern unsigned char
   rstat,     /* Last status from read */
   *buforg,   /* Pointer to beginning of buffer */
   *bufread,  /* Pointer to where program is reading */
   *bufpt,    /* Current buffer pointer */
   *bufend;   /* Pointer to end of buffer */
extern int
   bufbig,    /* Number of bytes currently in buffer */
   buflim;    /* Max nbr of bytes in buffer */

/* Headers for assembler interface routines */

#ifdef Lattice
int DLLfn();
#else
int DLLfn (                   /* Invoke DLL function */
   int fn,                    /* Function nbr */
   struct dcb far *dp);       /* Datalink Control Block */
#endif

#ifdef NOT_NEEDED
void a_dgroup();              /* Point ds to dgroup, i.e. access globals */
#endif

struct ucb far *ucb_addr();   /* Get ucb address in callback routines */


/* DEC DLL callback handlers */

struct cba r_cba;
struct cba far *r_cbp = &r_cba;

extern void r_callback();  /* ASM receive routine, calls c_r_callback() */

int tx_ncbp;  /* Nbr of pending tx callbacks */

struct cba t_cba;
struct cba far *t_cbp = &t_cba;

extern void t_callback();  /* ASM transmit routine, calls c_t_callback() */

void c_r_callback(struct ucb far *u)  /* Received data routine */
{
   int n;

   n = r_cbp->inx;
   memcpy(&r_cbp->uc[n], u,sizeof(struct ucb));
   r_cbp->inx = (n+1) & CBAMASK;
}

void c_t_callback(struct ucb far *u)  /* Transmitted data routine */
{
   int n;

   n = t_cbp->inx;
   memcpy(&t_cbp->uc[n], u,sizeof(struct ucb));
   t_cbp->inx = (n+1) & CBAMASK;
}


/* Trace routines */

int dn_errs;  /* Nbr of Decnet DLL failures observed */

#ifdef DCBERR
FILE *dnlog;  /* Diagnostic trace file */

void p_farptr(unsigned char far *fp)
{
   fprintf(dnlog,"%04x:%04x  ", FP_SEG(fp),FP_OFF(fp));
}
#endif

#ifdef DCBTRACE
void p_en_addr(unsigned char *e)
{
   fprintf(dnlog,"%02x:%02x:%02x:%02x:%02x:%02x  ", e[0],e[1],e[2],e[3],e[4],e[5]);
}

unsigned char *p_hex(unsigned char far *fp,int n)
{
	fprintf(dnlog,"   ");
	while (n != 0) {
		fprintf(dnlog,"%02x ", *fp++);
		--n;
	  }	/* end while */
	fprintf(dnlog, "\n");
	return(fp);
}

void dcbdump(struct dcb *d)
{
   fprintf(dnlog,"\n   %2d  ", d->portal_id);
   p_en_addr(d->source_addr);
   p_en_addr(d->dest_addr);
   p_farptr(d->bh);
   fprintf(dnlog,"%d\n", d->bl);
   fprintf(dnlog,"   %d %d %d  ", d->operation,d->pad,d->mode);
   p_farptr(d->line_state);
   p_farptr(d->rcv_callback);
   p_farptr(d->xmit_callback);
   fprintf(dnlog," %d %02x%02x %d\n",
      d->max_outstanding,d->ptype[0],d->ptype[1],d->buffers_lost);
   }
#endif


unsigned char *nbcpy(unsigned char *d,unsigned char *s,int n)
{
    while(n != 0) {
      *d++=*s++;
       --n;
    }
    return(d);
}

void dll_read_chan(struct dcb *d)
{
   unsigned int r;
   r = DLLfn(0x08, d);  /* Read Channel Status */
   if (r != 0) {
      ++dn_errs;
#ifdef DCBERR
      fprintf(dnlog,"CHANNEL STATUS failed: result = %04x",r);
#endif
      }
}

int dll_deallocate(struct dcb *d,unsigned char far *b)
{
   unsigned int r;
   d->bh = b;
   r = DLLfn(0x07, d);  /* Deallocate transmit buffer */
   if (r != 0) {
      ++dn_errs;
#ifdef DCBERR
      fprintf(dnlog,"DEALLOC BUF failed:\n");
      fprintf(dnlog,"   result %d  portal %d  prot %02x%02x  buf ",
         r, d->portal_id, d->ptype[0],d->ptype[1]);
      p_farptr(b);  fprintf(dnlog,"\n");
#endif
      }
   return r;
}

struct userdcb dcbs[4];  /* User info for the dcbs + zero end marker */

/* unsigned int prot;   Protocol (bytes reversed) */
/* int nb;   Nbr of dll buffers to use */
int dll_open(unsigned int prot,int nb)
{
   struct userdcb *ud;
   struct dcb *d;
   int r;

   for (ud = dcbs;  ud->ptype != 0;  ++ud) {
      if (ud->ptype == prot) return 0;  /* Already open */
      }
   d = &(ud->d);

   d->pad = 0;  /* 0 = NOPAD, 1 = PAD */
   d->mode = 1;  /* 0 = 802.3, 1 = Ethernet, 2 = promiscuous */
   d->ptype[0] = (char)(prot & 0x00FF);  /* Low-memory byte */
   d->ptype[1] = (char)(prot >> 8);  /* High-memory byte */
   d->line_state = NULL;
/*   d->line_state = MK_FP(0,0); */   /* CGW */
   d->rcv_callback = r_callback;
   d->xmit_callback = t_callback;
   d->max_outstanding = (unsigned char)nb;  /* 0 => Default, i.e. 1 rcv + 1 xmit */
   r = DLLfn(0x01, d);  /* Open portal */
   if (r != 0) {
      ++dn_errs;
#ifdef DCBERR
      fprintf(dnlog,"OPEN failed:\n");
      fprintf(dnlog,"   result %d  prot %02x%02x\n",
         r, d->ptype[0],d->ptype[1]);
#endif
#ifdef Lattice
      exit(1);
#else
      printf("DECNET OPEN failed:\n   result=%d, protocol=%02x%02x\n",
         r, d->ptype[0],d->ptype[1]);
      exit(1);
#endif
   }

   ud->portal_id = d->portal_id;
   ud->ptype = prot;
#ifdef DCBTRACE
   fprintf(dnlog,"Portal %d open for protocol %02x%02x\n",
      d->portal_id, d->ptype[0],d->ptype[1]);
#endif
   return 0;
   }

struct dcb *dcb_for_prot(unsigned int prot)
{
   struct userdcb *ud;
   for (ud = dcbs;  ud->portal_id != 0;  ++ud) {
      if (ud->ptype == prot) return &(ud->d);
      }
   ++dn_errs;
#ifdef DCBERR
   fprintf(dnlog,"DCB_FOR_PORT failed:\n   prot %02x%02x\n",
      prot & 0x00FF,prot >> 8);
#endif
   return &(dcbs[0].d);
   }

struct dcb *dcb_for_ucb(struct ucb far *u)
{
   struct userdcb *ud;
   unsigned int p = u->portal_id;
   for (ud = dcbs;  ud->portal_id != 0;  ++ud) {
      if (ud->portal_id == p) return &(ud->d);
      }
   ++dn_errs;
#ifdef DCBERR
   fprintf(dnlog,"DCB_FOR_UCB failed:\n   portal %d\n", p);
#endif
   return &(dcbs[0].d);
   }

int check_tx(void)  /* Returns 1 if there was a tx callback */
{
   int n,m;
   struct ucb far *u;
   struct dcb *du;

   n = t_cbp->outx;   m = t_cbp->inx;
   if (n == m) return 0;  /* No tx callbacks */

   du = dcb_for_ucb(u = &t_cbp->uc[n]);
#ifdef DCBTRACE
   fprintf(dnlog,"Tx callback: n %d  buf ", n);
   p_farptr(u->buffer);  fprintf(dnlog,"\n");
#endif
   dll_deallocate(du, u->buffer);
   t_cbp->outx = (n+1) & CBAMASK;
   --tx_ncbp;
   return 1;  /* We have processed a tx callback */
   }


extern unsigned char
   rstat,     /* Last status from read */
   *buforg,   /* Pointer to beginning of buffer */
   *bufread,  /* Pointer to where program is reading */
   *bufpt,    /* Current buffer pointer */
   *bufend;   /* Pointer to end of buffer */
extern int
   bufbig,    /* Number of bytes currently in buffer */
   buflim;    /* Max nbr of bytes in buffer */


int CDECL DNetopen(unsigned char *s,unsigned int irq,unsigned int addr,unsigned int ioaddr)  /* Initialise ethernet interface */
{
	s=s;		/* get rid of compiler warning */
	irq=irq;
	addr=addr;
	ioaddr=ioaddr;
#ifdef DCBERR
	dnlog = fopen("dndll.log","w");
#endif
	dll_open(EIP,4);  /* Open a dll portal for each packet type */
	dll_open(EARP,2);
    if(nnipnum[0] == 'R')  /* Don't open RARP portal if we don't need to! */
        dll_open(ERARP,2);
	return(0);
}

int CDECL DNgetaddr(unsigned char *s,unsigned int address,unsigned int ioaddr)  /* Get ethernet address from board */
{
	address=address;		/* get rid of compiler warning */
	ioaddr=ioaddr;
	dll_read_chan(&dcbs[0].d);  /* Check channel status */
	memcpy(s, dcbs[0].d.source_addr,6);
	return(0);
}

int CDECL DNetclose(void)  /* Shut down ethernet interface */
{
   struct userdcb *ud;  /* Close all the dll portals */
   int r;
   while (tx_ncbp != 0) check_tx();  /* Clear pending tx callbacks */
   for (ud = dcbs;  ud->portal_id != 0;  ++ud) {
      r = DLLfn(0x02, &(ud->d));  /* Close portal */
      if (r != 0) {
         ++dn_errs;
#ifdef DCBERR
	 fprintf(dnlog,"CLOSE failed:\n   result %d  portal %d\n",
	    r, ud->portal_id);
#endif
         }
#ifdef DCBTRACE
      else fprintf(dnlog,"Portal %d closed\n", ud->portal_id);
#endif
      }
#ifdef DCBERR
   if (dn_errs != 0) fprintf(dnlog,">>> %d DECnet DLL errors <<<\n", dn_errs);
   fclose(dnlog);
#endif
   if (dn_errs != 0) printf(">>> %d DECnet DLL errors <<<\n", dn_errs);
   return(0);
   }

void CDECL DNrecv(void)  /* Move any received packet(s) into buffer */
{
   int n,m, sz;
   struct ucb far *u;
   struct dcb *du;
   unsigned char far *ucp;
   unsigned int *uip;

   for (;;) {
      n = r_cbp->outx;   m = r_cbp->inx;
      if (n == m) return;  /* No receive callbacks */
      du = dcb_for_ucb(u = &r_cbp->uc[n]);

#ifdef DCBTRACE
      fprintf(dnlog,"Rx callback: n %d  buf ", n);
         p_farptr(u->buffer);
      fprintf(dnlog,"portal %d  prot %02x%02x  status %d  length %d\n",
	 du->portal_id, du->ptype[0],du->ptype[1], u->buffer_status, u->bl);
      ucp = p_hex(u->buffer,20);  ucp = p_hex(ucp,20);  p_hex(ucp,20);
#endif

      if (u->buffer_status == 1) {  /* Received with no errors */
         if (bufbig <= buflim) {  /* Room for packet in TELNET buffer */
            if (bufpt >= bufend)  /* Wraparound top of buffer */
               bufpt = buforg;
            uip = (unsigned int *)bufpt;  /* Length of received packet */
            ucp = nbcpy(bufpt+2, u->dest,6);
            ucp = nbcpy(ucp, u->source,6);
            ucp = nbcpy(ucp, du->ptype,2);
            ucp = nbcpy(ucp, u->buffer,u->bl);
	    sz = ucp-bufpt;
            if (dll_deallocate(du, u->buffer) == 0) {  /* No problems */
               *uip = sz;  bufpt = ucp;
               bufbig += sz;  /* Bytes in TELNET buffer */
               }
            }
         }
      else dll_deallocate(du, u->buffer);  /* Errors - discard packet */
      r_cbp->outx = (n+1) & CBAMASK;
      }
   }

void CDECL DNetupdate(void)  /* Update pointers and/or restart receiver
                     after read routine has handled the current packet */
{
   unsigned int *uip;
   int sz;

   uip = (unsigned int *)bufread;  /* Packet size */
   sz = *uip;
   bufread += sz;
   if (bufread >= bufend) bufread = buforg;
   bufbig -= sz;
   }

int CDECL DNxmit(DLAYER *pkt,unsigned int count)  /* Send an ethernet packet */
{
   int r;
   struct dcb *du;
   unsigned char far *buf;
   unsigned char *packet = (unsigned char *)pkt;
   unsigned int *uip;

   uip = (unsigned int *)packet;  /* Protocol type from packet header */
   du = dcb_for_prot(uip[6]);
#ifdef DCBTRACE
   fprintf(dnlog,"Send packet: prot %02x%02x  portal %d  count %d\n",
      du->ptype[0],du->ptype[1], du->portal_id,count);
#endif

   for (;;) {  /* Get a transmit buffer */
      while (check_tx() != 0 || tx_ncbp == 2) ;  /* Clear pending tx callbacks */
      r = DLLfn(0x06, du);  /* Request transmit buffer */
      if (r == 0) break;  /* Got the buffer */
      else if (r == 8) {  /* No resources */
         if (tx_ncbp == 0) return 1;  /* Couldn't get buffer */
         continue;  /* Wait for a tx callback */
         }
      else {
         ++dn_errs;
#ifdef DCBERR
         fprintf(dnlog,"REQ TX BUF failed:\n");
         fprintf(dnlog,"   result %d  portal %d  prot %02x%02x\n",
            r, du->portal_id,du->ptype[0],du->ptype[1]);
#endif
         return 1;  /* Couldn't get buffer */
         }
      }

   memcpy(du->dest_addr, packet, 6);
   buf = du->bh;
   memcpy(buf, &packet[14], count -= 14);  /* Allow for ethernet header */
   du->bl = (count <= 46) ? 46 : count;
#ifdef DCBTRACE
   fprintf(dnlog,"   dest ");  p_en_addr(du->dest_addr);
   fprintf(dnlog,"source ");  p_en_addr(&packet[6]);
   fprintf(dnlog,"buffer ");  p_farptr(du->bh);
   fprintf(dnlog,"\n");
   p_hex(du->bh,20);
#endif

   r = DLLfn(0x05, du);  /* Transmit */
   if (r != 0) {
      ++dn_errs;
#ifdef DCBERR
      fprintf(dnlog,"TRANSMIT failed:\n");
      fprintf(dnlog,"   result %d  portal %d  prot %02x%02x  buf ",
         r, du->portal_id,du->ptype[0],du->ptype[1]);
      p_farptr(du->bh);  fprintf(dnlog,"\n");
#endif
      return 2;  /* Transmit failed */
      }
   ++tx_ncbp;
   return 0;  /* No problems */
   }
