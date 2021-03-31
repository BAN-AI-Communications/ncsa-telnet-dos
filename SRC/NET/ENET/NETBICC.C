/****************************************************************************
 *  BICC 411x Multi Protocol Software code                                  *
 *  Inge Arnesen 1989                                                       *
 *                                                                          *
 *  Module to interface NCSA Telnet to the BICC 411x cards through the      *
 *  ISOLAN Multi Protocol Software. This enables NCSA Telnet to coexist     *
 *  with Novell Netware.                                                    *
 *                                                                          *
 *  THIS IS NOT A PART OF NCSA TELNET                                       *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *                                                                          *
 *      (C) Inge Arnesen                                                    *
 *      Institute of Social Research                                        *
 *      Munthesgt. 31                                                       *
 *      N-0260 Oslo, NORWAY                                                 *
 *                                                                          *
 *                                                                          *
 *    DISCLAIMER: This code is not property of the institute, but of        *
 *                the author himself. No responsibility is claimed          *
 *                for anything whatsoever due to the use of this code!      *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *    Created      : 89.05.27  by Inge Arnesen                              *
 *    Last modified: 89.05.29  by Inge Arnesen                              *
 *                                                                          *
 *    History: 89.05.29 Initial alpha version                               *
 *             89.07.01 Beta started - no changes                           *
 *             89.10.06 Final release - no changes                          *
 *                                                                          *
 ****************************************************************************

        ILrecv          - Polled receive (dummy)
        ILetopen        - Initialize Ethernet SW
        ILetclose       - Close down Ethernet SW
        ILgetaddr       - Get Ethernet cards address (must be initialized first)
        ILetaddr        - Set Ethernet cards address (dummy)
        ILxmit          - Send data across the Ethernet
        ILetupdate      - Update buffer pointers


        COMPILE:
                cl -Ze -Zp -Gs iso.c
        Options are:    Enable language extentions, pack structure members,
                        Remove calls to Stack probe routine

 */
/* Standard include files */
#include        <conio.h>       /* Microsoft 'C' Include file   */
#include        <ctype.h>       /* Microsoft 'C' Include file   */
#include        <dos.h>         /* Microsoft 'C' Include file   */
#include        <stdio.h>       /* Microsoft 'C' Include file   */
#include        <string.h>      /* Microsoft 'C' Include file   */

/* NCSA include files */
#include        "protocol.h"
#include        "data.h"
#include		"bicc.h"
#include		"externs.h"

extern void ANR_ENTRY();
struct mps_status stat_buf;


extern unsigned char rstat;             /*  last status from read               */
extern unsigned char *bufpt;            /*  current buffer pointer              */
extern unsigned char *buforg;           /*  pointer to beginning of buffer      */
extern unsigned char *bufend;           /*  pointer to end of buffer            */
extern unsigned char *bufread;          /*  pointer to where program is reading */
extern unsigned int bufbig;             /*  integer, how many bytes we have     */
extern unsigned int buflim;             /*  integer, max bytes we can have      */

int CDECL ILgetaddr(unsigned char *ethaddr,unsigned int memaddr,unsigned int ioaddr)
{
        int i; 
        struct tcb t, *h;
        union REGS inregs, outregs;
        struct SREGS segregs;

        /* Then send status request */
		t.tcbcommand = PORT_STATUS;
		t.tcbcid = PORT_STATUS;
        t.tcbbaddr.status= &stat_buf;
        t.tcblength    = sizeof(struct mps_status);
        t.tcblnet= 0xffffffff;

        t.tcbasync= ANR_ENTRY;  /* This is a FUNCTION ptr */

        /*
         *      now issue the int5b, es:bx point at the tcb
         *
         */
        h= &t; 
        segregs.es= FP_SEG(h);
        inregs.x.bx= FP_OFF(h);
        int86x(0x5B, &inregs, &outregs, &segregs);

        for (i= 9; i < 15; i++)
                ethaddr[i - 9]= stat_buf.address[i];

		return(0);
}

void CDECL ILrecv(void )
{
}

void CDECL ILetupdate(void )
{
        /* Update all the pointers etc. */
        bufbig-= *((int *)bufread) + sizeof( int );
        bufread+= *((int *)bufread) + sizeof(int);
        if(bufread >= bufend)
                bufread= buforg;
}

int CDECL ILxmit(DLAYER *pack,unsigned int size)
{
        struct tcb t, *h;               
        int i;
        union REGS inregs, outregs;
        struct SREGS segregs;

        t.tcbraddr[2] = 0;
        t.tcbraddr[3] = 0;
        t.tcbraddr[7] = 0;
        t.tcbraddr[8] = 0;   /* LSAP */
        t.tcbraddr[9] = pack->dest[0];     /* ETHADDR */
        t.tcbraddr[10] = pack->dest[1];
        t.tcbraddr[11] = pack->dest[2];
        t.tcbraddr[12] = pack->dest[3];
        t.tcbraddr[13] = pack->dest[4];
        t.tcbraddr[14] = pack->dest[5];      /*--to here--*/
        t.tcbraddr[15] = 0;
        t.tcbcommand = L_DATA_SEND;

        t.tcbstatus = intswap(pack->type);
        for (i = 0; i < 16; ++i)
                t.tcbladdr[i] = 0;

        t.tcbasync= 0;   /* No ANR */

        /*      setup pointer to data buffer    */
        t.tcbbaddr.pt  = (char far *)pack + sizeof(DLAYER);
        t.tcblength    = size - sizeof(DLAYER);
        /*
         *      now issue the int5b, es:bx point at the tcb
         *
         */
        h= &t;
        segregs.es= FP_SEG(h);
        inregs.x.bx= FP_OFF(h);
        int86x(0x5B, &inregs, &outregs, &segregs);
        return(outregs.h.al);
}

int CDECL ILetopen(unsigned char *ethaddr,unsigned int ioirq,unsigned int memaddr,unsigned int ioaddr)
{
        struct tcb t, *h;
        int i;
        union REGS inregs, outregs;
        struct SREGS segregs;


        t.tcbcommand = L_ACTIVATE;
        t.tcbcid = L_ACTIVATE;

        for (i = 0; i < 16; ++i)
                t.tcbladdr[i] = 0;

        t.tcbasync= ANR_ENTRY;

        /*
         *      now issue the int5b, es:bx point at the tcb
         *
         */
        h= &t; 
        segregs.es= FP_SEG(h);
        inregs.x.bx= FP_OFF(h);
        int86x(0x5B, &inregs, &outregs, &segregs);

        return(outregs.h.al);
}

int CDECL ILetclose(void )
{
        union REGS inregs, outregs;
        struct SREGS segregs;
        struct tcb t, *h;               
        int i;

        t.tcbcommand = L_DEACTIVATE;
        t.tcbcid = 0; /* Not used */

        for (i = 0; i < 16; ++i)
                t.tcbladdr[i] = 0;

        /*
         *      now issue the int5b, es:bx point at the tcb
         *
         */
        h= &t; 
        segregs.es= FP_SEG(h);
        inregs.x.bx= FP_OFF(h);
        int86x(0x5B, &inregs, &outregs, &segregs);
        return(outregs.h.al);
}


/*
 *      Note: anr_c returns zero to the Multi Protocol Handler
 *
 *      Inside the ANR, Operating System calls must not be made, nor must
 *      space be grabbed from the Heap.  The use of Automatic variables
 *      is allowed, but make sure the stack is large enough.
 */

unsigned int CDECL anr_c(struct acb *acb_ptr)
{
        int i;
        int mine= TRUE;  /* We suspect all packets of coming from this card */
                        /* Loop back is supported on link level */      
                        /* If its mine, drop it like a hot potato */

        char    far     *buffer_pt;     /* buffer pointer       */

		if(acb_ptr->acbcmd != PORT_STATUS)
        {
                switch(acb_ptr->acbeventcode)
                {
                case L_ACTIVATE_CONF:
                        break;
                case L_DATA_IND:
                case M_DATA_IND:
                        /* If it is not one of mine, mark it as such */  
                        for(i= 0; i < 6 ; i++)
                                if(acb_ptr->acbraddr[i+9] != nnmyaddr[i])
                                {
                                        mine= FALSE; 
                                        break;
                                }       
                        if (!mine)
                        {
                        /* with llc and mac commands, it's neccessary to copy   */
                        /* the received data.                                   */

                        buffer_pt = acb_ptr->acbbaddr.pt;
                        /* The above instruction in theory should not be needed */
                        /* but is in practise                                   */ 

                        if(bufbig <= buflim) /* Enough room in the buffer ? */
                        {
                                if(bufpt >= bufend) /* Wrap around ? */
                                {
                                        bufpt= buforg;
                                }
                                /* Size of packet inc. DLAYER first in buffer */
                                *((int *)bufpt)= acb_ptr->acblen + sizeof(DLAYER);
                                bufpt+= 2;
                                ((DLAYER *)bufpt)->type= intswap(acb_ptr->acbstatus);
                                for(i= 0; i < 6 ; i++)
                                {
                                        ((DLAYER *)bufpt)->dest[i]= 
                                                acb_ptr->acbladdr[i+9];
                                        ((DLAYER *)bufpt)->me[i]= 
                                                acb_ptr->acbraddr[i+9];
                                }
                                bufpt+= sizeof(DLAYER);
                                movedata(FP_SEG( buffer_pt ), FP_OFF( buffer_pt ), 
                                        FP_SEG(bufpt), FP_OFF(bufpt), acb_ptr->acblen);
                                bufpt+= acb_ptr->acblen;
                                bufbig+= acb_ptr->acblen + sizeof(DLAYER) + sizeof(int);
                        }
                        /* else we are going to drop packets ! */
                        }
                        break;
                }
        }
        return(0);
}

