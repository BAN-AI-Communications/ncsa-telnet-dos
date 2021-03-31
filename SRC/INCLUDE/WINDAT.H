/*
*  Windat -- window data structures for NCSA Telnet
****************************************************************************
*                                                                          *
*                                                                          *
*      NCSA Telnet                                                         *
*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
*                                                                          *
****************************************************************************
*  Tim Krauskopf
*
*  structure to save capabilities of windows, in addition to the
*  virtual vt102 screen driver.
*/

#ifndef WINDAT_H
#define WINDAT_H

/*  
*  terminal modes, can be changed by external program
*  VTEK is a VT terminal that can accept graphics
*  TEK  is a tek graphics mode for a VT
*  VT   is a VT-only type, cannot be changed to graphics mode 
*/
#define VTEKTYPE 	1
#define DUMBTYPE 	2
#define VTTYPE 		3
#define TEKTYPE 	4
#define RASTYPE 	5

#define NCOLORS 	4

struct twin {
      unsigned short colors[NCOLORS];
      unsigned char mname[16],    /* name of the machine connected to */
        linemode_mask,          /* mask for editting modes in linemode connections */
                 /* RMG change for 132 */
        linemode[82];                   /* line mode buffer for session */
	int pnum,					/* port number associated */
        vs,                                             /* virtual screen associated with it */
        bkscroll,                               /* scroll back value */
        width,                                  /* width of the window */
        rows,                                   /* Number of rows in the window */
        telstate,                               /* telnet state for this connection */
        substate,                               /* telnet subnegotiation state */
        termstate,                              /* terminal type for this connection */
        teks,                                   /* tektronix window number */
        crfollow,                               /* what is supposed to follow a CR? NUL or LF? */
        sstat,                                  /* stat box on screen */
        bksp,                                   /* what keycode for backspace ?*/
        del,                                    /* for delete? */
        slc[31],                /* line mode sub-option characters */
        slm[31];                /* line mode sub-option modes */
	char *ftpopts;				/* FTP cmd line paramters */
      unsigned int mapoutput:1,       /* are we mapping the characters output for this window */
        vtwrap:1,               /* line wrapping flag */
        lmflag:1,               /* Are we in linemode? */
        lmedit:1,               /* Edit lines in linemode? */
        litflag:1,              /* Is the next character on this connection to be send literally (for linemode use) */
        litecho:1,              /* Is the next character on this connection to be echoed literally (for linemode use) */
        softtab:1,              /* Expand tabs on the client side? (for linemode use) */
        trapsig:1,              /* Trap signals on the client side? (for linemode use) */
        halfdup:1,              /* half duplex mode overrides line mode */
        termsent:1,             /* has terminal type been sent? */
        ibinary:1,              /* negotiate for binary traffic out */
        iwantbinary:1,          /* flag to indicate whether we asked for binary transmission out */
        ubinary:1,              /* negotiate for binary traffic in */
        uwantbinary:1,          /* flag to indicate whether we asked for binary transmission in */
        igoahead:1,             /* negotiation for suppress go-ahead */
        ugoahead:1,             /* neg. for his suppress go-ahead */
        echo:1,                 /* line mode or echo mode? */
        timing:1,               /* timing marker sent */
        capon:1,                /* does this session own a capture file? */
        condebug:2;             /* debugging level for console output */
      FILE *capfp;
      struct twin *next,*prev;
};

#define NUM_WINDOWS     20

#ifdef WINMASTER
    struct twin *screens[NUM_WINDOWS],
		*current=NULL,
		*console,
		*wins[NPORTS];
#else
    extern struct twin *screens[NUM_WINDOWS],
		*current,
		*console,
		*wins[NPORTS];
#endif

#endif
