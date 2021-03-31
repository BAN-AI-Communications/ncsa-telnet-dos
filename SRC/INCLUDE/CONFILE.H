/*
*    confile.h
*  Used for config.tel options
****************************************************************************
*                                                                          *
*                                                                          *
*      NCSA Telnet                                                         *
*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
*      Additions by Kurt Mahan, Heeren Pathak, & Quincey Koziol            *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
****************************************************************************
*   Quincey Koziol
*   Defines for config.tel options and variables
*/

#ifndef CONFILE_H
#define CONFILE_H

/*
*  Global Variables
*/
#ifdef LATER
#define	NETX25	1
#endif
 
/* Initial configuration.  struct config found in hostform.h */
#ifndef CONFIG_MASTER
extern
#endif
struct config Scon
#ifdef CONFIG_MASTER
    ={
		0,0,0,0,			/* subnet mask */
		3,					/* interrupt # */
		127,0,0,1,			/* ip num */
		255,255,255,255,	/* broadcast IP address */
		"",					/* my name */
		7,1,0x70,			/* default colors */
                "packet",                        /* network hardware */
		"ega",				/* graphics hardware */
        0,                  /* 43/50 screen status */
		"VT100",			/* terminal type */
		"*",				/* AppleTalk zone */
		NULL,				/* default domain */
		NULL,				/* capture file */
		NULL,				/* password file */
                NULL,                           /* key definitions file */
		NULL,				/* hp file */
		NULL,				/* ps file */
		NULL,				/* tek file */
		-1,					/* cursor start line */
		-1,					/* cursor end line */
		1,					/* name server */
		4,					/* time out of UDP domain request */
		3,					/* number of retries */
		0x0d000,			/* segment address */
		0x0300,				/* I/O address */
		0,					/* do we have a net mask */
		0,					/* bios flag */
		1,					/* tek flag */
        0,                  /* ftp flag */
        0,                  /* rcp flag */
		1,					/* thin wire default!  thick=0 used for 3c503*/
		0,					/* windows go away when closed (0) - no, (1) - yes */
		1,					/* display clock? (0) - no, (1) - yes */
        1                   /* autoscroll in scrollback is on */
}
#endif
;

#ifndef NET14
#ifndef CONFIG_MASTER
extern
#endif
char *commandLineOveride    /* for args to -e options */
#ifdef CONFIG_MASTER
=NULL
#endif
;
#endif

/*
*   States for config file reading state machine.
*   One for each type of keyword and some for controlling.
*/

#define	CONNAME		101
#define CONHOST		102
#define CONIP		103
#define CONGATE		104
#define CONCOLOR        105
#define CONBKSP		106
#define CONBKSC		107
#define CONRETR		108
#define CONWIND		109
#define CONSEG		110
#define CONMTU		111
#define CONNS		112
#define CONTO		113
#define CONCRMAP	114
#define CONDUP  	115
#define CONWRAP 	116
#define CONWIDE 	117
#define CONFONT 	118
#define CONFSIZE	119
#define CONNF		120
#define CONNB 		121
#define CONBF 		122
#define CONBB 		123
#define CONUF 		124
#define CONUB 		125
#define CONRF 		126
#define CONRB 		127
#define CONCLMODE 	128
#define CONPORT 	129
#define CONFTPBAK       130
#define CONDEBUGCONSOLE 131
#define CONMAPOUTPUT    132

#define NUMSPECS 132    /* last CON option for a machine specific option */

/*
*  above this line are per machine entries, below are configuration entries
*/
#define CONNDOM         (NUMSPECS+1)
#define CONMASK         (NUMSPECS+2)
#define CONMYIP         (NUMSPECS+3)
#define CONHPF          (NUMSPECS+4)
#define CONPSF          (NUMSPECS+5)
#define CONTEKF         (NUMSPECS+6)
#define CONME           (NUMSPECS+7)
#define CONCCOL         (NUMSPECS+8)
#define CONHW           (NUMSPECS+9)
#define CONADDR         (NUMSPECS+10)
#define CONIOA          (NUMSPECS+11)
#define CONDEF          (NUMSPECS+12)
#define CONINT          (NUMSPECS+13)
#define CONBIOS         (NUMSPECS+14)
#define CONTEK          (NUMSPECS+15)
#define CONVIDEO        (NUMSPECS+16)
#define CONFTP          (NUMSPECS+17)
#define CONRCP          (NUMSPECS+18)
#define CONPASS         (NUMSPECS+19)
#define CONCAP          (NUMSPECS+20)
#define CONTTYPE        (NUMSPECS+21)
#define CONNSTYPE       (NUMSPECS+22)
#define CONFROM         (NUMSPECS+23)
#define CONARPTO        (NUMSPECS+24)
#define CONZONE         (NUMSPECS+25)
#define CONDOMTO        (NUMSPECS+26)
#define CONKBFILE       (NUMSPECS+27)
#define CONWIRE         (NUMSPECS+28)
#define CONCURSORTOP    (NUMSPECS+29)
#define CONCURSORBOTTOM (NUMSPECS+30)
#define CONWINDOWGOAWAY (NUMSPECS+31)
#define CONAUTOSCROLL   (NUMSPECS+32)
#define CONCLOCK        (NUMSPECS+33)
#define CONBROADCAST    (NUMSPECS+34)
#define CONOUTPUTMAP    (NUMSPECS+35)
#define CONBEEP         (NUMSPECS+36)
#define CONSERVICES     (NUMSPECS+37)
#define CONFTPWRT       (NUMSPECS+38)

#ifndef CONFIG_MASTER
extern
#endif
char *Skeyw[]
#ifdef CONFIG_MASTER
    ={
		"",	
                "name",               /* name of session */
                "host",               /* name of host */
                "hostip",             /* IP number */
                "gateway",            /* gateway level */
                "color",              /* color code  ==5== */
                "erase",              /* value to use for backspace */
                "scrollback",         /* how many lines to backscroll */
                "retrans",            /* initial retrans time */
                "rwin",               /* window to allow for this host */
                "maxseg",             /* maximum transfer size(in) ==10== */
                "mtu",                /* transfer unit (out) */
                "nameserver",         /* name server level */
                "contime",            /* timeout for opening connection */
                "crmap",              /* map for Berkeley 4.3 compatibility */
                "duplex",             /* half duplex for IBM machines ==15==*/
                "vtwrap",             /* should VT wrap? */
                "vtwidth",            /* width of VT100 screen */
                "font",               /* font to use, when given a choice */
                "fsize",              /* font size, in points */
                "nfcolor",            /* normal foreground color ==20==*/
                "nbcolor",            /* normal background color */
                "bfcolor",            /* blink foreground color */
                "bbcolor",            /* blink background color */
                "ufcolor",            /* underline foreground color */
                "ubcolor",            /* underline background color ==25==*/
                "rfcolor",            /* reverse foreground color */
                "rbcolor",            /* reverse background color */
                "clearsave",          /* clear screen saves lines */
                "port",               /* TCP port to go for */
                "ftpoptions",         /* Options for ftp command line ==30==*/
                "consoledebug",       /* Debugging options for connections */
                "outputmap",          /* Flag for output mapping on a machine basis */
/*
*  following are one-time entries, above are part of the data structure
*/
        "domainretry",      /* # of retries */
		"netmask",			/* subnetting mask */
        "myip",             /* local machine's IP # ==35==*/
        "hpfile",           /* HPGL output file */
        "psfile",           /* postscript output file */
        "tekfile",          /* tektronix output file */
		"myname",			/* identifying info */
        "concolor",         /* console colors ==40==*/
        "hardware",         /* network hardware */
        "address",          /* Address of hardware */
		"ioaddr",			/* ioaddress of hardware */
		"domain",			/* default domain for lookup */
        "interrupt",        /* interrupt request 3 or 5 ==45==*/
        "bios",             /* use BIOS screen */
        "tek",              /* tektronix graphics */
		"video",			/* type of video hardware */
		"ftp",				/* enable ftp? */
        "rcp",              /* enable rcp? ==50==*/
        "passfile",         /* password file name */
        "capfile",          /* capture file name */
		"termtype",			/* terminal type */
		"nameservertype",	/* nameserver type */
        "copyfrom",         /* copy from another machine ==55==*/
        "arptime",          /* time-out for ARPs */
        "zone",             /* NBP zone for Macs */
		"domaintime",		/* time-out for DOMAIN */
		"keyfile",			/* file for keyboard map */
        "wire",             /* Thin or thick ether? Needed for 3c503 ==60==*/
        "cursortop",        /* Starting line for the cursor */
        "cursorbottom",     /* Ending line for the cursor */
        "windowgoaway",		/* Whether to wait for a keypress after a window closes, or not */
        "autoscroll",		/* Do we automatically scroll in scrollback */
        "clock",            /* Display the clock? ==65==*/
        "broadcast",        /* IP broadcast address */
        "outputfile",       /* File for the output mapping definition */
        "beep",             /* musical note display */
        "services",			/* specify path to services file */
        "ftpwrt",     /* allow write access to incoming connections ==70==*/
        ""
    }
#endif
    ;

#ifndef CONFIG_MASTER
extern
#endif
struct machinfo *Smachlist,*Smptr;

#ifndef CONFIG_MASTER
extern
#endif
struct machinfo *Sns
#ifdef CONFIG_MASTER
=NULL
#endif
;

#ifndef NET14
#ifndef CONFIG_MASTER
extern
#endif
int default_mapoutput
#ifdef CONFIG_MASTER
=0
#endif
;
#endif

#ifndef CONFIG_MASTER
extern
#endif
unsigned char *Smachfile
#ifdef CONFIG_MASTER
={"config.tel"}
#endif
,
#ifdef OLD_WAY
    *Smachpath
#ifdef CONFIG_MASTER
={""}
#endif
,
#endif
	Sflags[NUMSPECS-95],	/* which parms we have or have not */
	*Sspace;

#ifndef CONFIG_MASTER
extern
#endif
int mno
#ifdef CONFIG_MASTER
=0
#endif
,                  /* how many machines in host file */
#ifndef NET14
    beep_notify
#ifdef CONFIG_MASTER
=1
#endif
,                           /* whether to beep when text is received on a non-visible screen */
#endif
    lineno,                 /* line number in hosts file */
    position,               /* position for scanning string */
    constate,               /* state for config file parser */
    inquote;                /* flag, inside quotes now */

#endif  /* confile.h */

