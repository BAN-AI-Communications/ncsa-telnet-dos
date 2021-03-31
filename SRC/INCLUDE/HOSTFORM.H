/**************************************************************************/
/*
*  Host and local machine configuration information.
*
****************************************************************************
*                                                                          *
*      NCSA Telnet for the PC                                              *
*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
*      This program is in the public domain.                               *
*                                                                          *
****************************************************************************
*																		   *
*	5/88	clean up for 2.3 release, JKM	
*/

#ifndef HOSTFORM_H
#define HOSTFORM_H

/*
*  Linked list of structures which describe other machines.
*  Arranged one per session with unique session names.
*/
struct machinfo {
	unsigned char *sname,		/* pointer to name of session */
		*hname,                 /* pointer to name of that machine */
		*font,					/* font name, if we can do it */
		*ftpoptions,			/* options for FTP command line (ALT-F) */
		hostip[4],				/* IP number of this machine */
		gateway,				/* gateway preference, start with 1 */
		nameserv,				/* nameserver preference, start with 1 */
		bksp,					/* backspace value */
		halfdup,				/* half duplex required */
		crmap,					/* Strange Berkeley 4.3 CR mode needed */
        fsize,                  /* font size in points */
        vtwrap,                 /* flag on when need wrap mode */
		vtwidth;				/* how wide screen should be for this session */
   char nfcolor,                /* normal foreground */
		nbcolor,				/* normal background */
        bfcolor,                /* blink  foreground */
        bbcolor,                /* blink background */
        ufcolor,                /* underline forground */
        ubcolor;                /* underline background */
    int clearsave,              /* whether to save cleared lines */
		port,					/* TCP port number to access, default = 23 (telnet) */
		mno,					/* machine number for reference */
		mstat,					/* status of this machine entry */
		bkscroll,				/* how many lines to save */
		retrans,				/* initial retrans timeout */
		conto,					/* time out in seconds to wait for connect */
		window,					/* window, will be checked against buffers */
		maxseg,					/* maximum receivable segment size */
		mtu;					/* maximum transfer unit MTU (out) */
    unsigned int mapoutflag:1,  /* flag to indicate whether output mapping is on for this machine */
        consoledebug:2;         /* debugging levels for displaying information on the console */
    struct machinfo *next;      /* surprise, its a linked list! */
};

/*
*  status fields for mstat, what do we know about that machine?
*/
#define NOIP 1					/* we don't have IP number */
#define UDPDOM 3				/* there is a UDP request pending on it */
/*  The next 20 numbers are reserved for UDPDOM */
#define HAVEIP 50				/* at least we have the # */
#define HFILE 70				/* we have IP number from host file */
#define DOM 71					/* we have an IP number from DOMAIN */
#define FROMKIP 72				/* have IP# from KIP server */
/*
*   Configuration information which 
*   the calling program may want to obtain from the hosts file.
*   The calling program should include hostform.h and call
*   Sgetconfig(cp)
*     struct config *cp;
*   which will copy the information to the user's data structure.
*/
struct config {
    unsigned char netmask[4],   /* subnetting mask being used */
		irqnum,					/* which hardware interrupt */
		myipnum[4],				/* what is my IP #? */
		broadip[4],				/* IP broadcast address */
		me[32],					/* my name description (first 30 chars) */
                color[3],                               /* default colors to use */
		hw[10],					/* hardware type for network */
		video[10],				/* video graphics hardware available */
		ega43,					/* 43 line ega mode */
		*termtype,				/* terminal type specification */
		*zone,					/* AppleTalk zone for KIP NBP */
		*defdom,				/* default domain */
		*capture,				/* pointer to where the capture file name is */
		*pass,					/* pointer to where the password file name is */
                *keyfile,                               /* pointer to where the capture file name is */
		*hpfile,				/* HP file name */
		*psfile,				/* PS file name */
		*tekfile;				/* tek file name */
                int cursortop,              /* top line of cursor */
		cursorbottom,			/* bottom line of cursor */
		nstype,					/* nameserver = 1-domain 2-IEN116  */
		domto,					/* time out for UDP domain request */
                ndom;                   /* number of retries for domain requests */
                unsigned int address,       /* segment address */
		ioaddr;					/* I/O address */
                unsigned int havemask:1,    /* do we have a netmask? */
		bios:1,					/* flag, do we want to use BIOS for screen access */
		tek:1,					/* flag, enable tektronix graphics */
		ftp:1,					/* flag, enable ftp server */
		rcp:1,					/* flag, enable rcp server */
                ftpw:1,          /* flag, enable ftp writes */
		wire:1,					/* what type of connector? (thin or thick)*/
		wingoaway:1,			/* flag, whether a window goes away when session ends or waits for a keypress */
		clock:1,				/* flag, whether to display the clock or not */
        autoscroll:1;           /* flag, do we perform a fast scroll in scrollback */
};

#endif
