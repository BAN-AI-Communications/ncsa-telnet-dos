/*
*   negotiat.c
*
*   Telnet option negotiation functions
*
*   Quincey Koziol
*
*	Date		Notes
*	--------------------------------------------
*   11/91       Started
*/

/*
* Includes
*/

#ifndef NOTEK
#define USETEK
#endif
/* #define USERAS */

#define NEGOTIATEDEBUG  /* define this to print the raw network data to the console */

#ifdef __TURBOC__
#include "turboc.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <ctype.h>
#ifdef MSC
#include <malloc.h>
#endif
#include "telopts.h"
#include "externs.h"

/*
*	Global Variables
*/
extern unsigned char s[550],
    parsedat[256];

extern int temptek,             /* where drawings go by default */
    rgdevice;                   /* tektronix device to draw on */

extern struct config def;       /* Default settings obtained from host file */

/* Local functions */
static void parse_subnegotiat(struct twin *tw,int end_sub);

/* Local variables */
static char *telstates[]={
    "EOF",
    "Suspend Process",
    "Abort Process",
    "Unknown (239)",
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

static char *teloptions[256]={      /* ascii strings for Telnet options */
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
    "Environment",
    "Authentication",
    "Data Encryption",
    "39",
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

static char *LMoptions[]={      /* ascii strings for Linemode sub-options */
    "None",
    "MODE",
    "FORWARDMASK",
    "SLC"
};

static char *ModeOptions[]={      /* ascii strings for Linemode edit options */
    "None",
    "EDIT",
    "TRAPSIG",
    "ACK",
    "SOFT TAB",
    "LIT ECHO"
};

static char *SLCoptions[]={     /* ascii strings for Linemode SLC characters */
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
	"FORW2",
	"MCL",
	"MCR",
	"MCWL",
	"MCWR",
	"MCBOL",
	"MCEOL",
	"INSRT",
	"OVER",
	"ECR",
	"EWR",
	"EBOL",
	"EEOL"
};

static char *SLCflags[]={      /* ascii strings for Linemode SLC flags */
    "SLC_NOSUPPORT",
    "SLC_CANTCHANGE",
    "SLC_VALUE",
    "SLC_DEFAULT"
};

static unsigned char LMdefaults[NUMLMODEOPTIONS+1]={   /* Linemode default character for each function */
    -1,         /* zero isn't used */
    -1,         /* we don't support SYNCH */
    3,          /* ^C is default for BRK */
    3,          /* ^C is default for IP */
    15,         /* ^O is default for AO */
    25,         /* ^Y is default for AYT */             /* 5 */
    -1,         /* we don't support EOR */
    3,          /* ^C is default for ABORT */
    4,          /* ^D is default for EOF */
    26,         /* ^Z is default for SUSP */
    8,          /* ^H is default for EC */              /* 10 */
    21,         /* ^U is default for EL */
    23,         /* ^W is default for EW */
    18,         /* ^R is default for RP */
    22,         /* ^V is default for LNEXT */
    17,         /* ^Q is default for XON */             /* 15 */
    19,         /* ^S is default for XOFF */
    22,         /* ^V is default for FORW1 */
    5,          /* ^E is default for FORW2 */
    -1,         /* we don't support MCL */
    -1,         /* we don't support MCR */              /* 20 */
    -1,         /* we don't support MCWL */
    -1,         /* we don't support MCWR */
    -1,         /* we don't support MCBOL */
    -1,         /* we don't support MCEOL */
    -1,         /* we don't support INSRT */            /* 25 */
    -1,         /* we don't support OVER */
    -1,         /* we don't support ECR */
    -1,         /* we don't support EWR */
    -1,         /* we don't support EBOL */
    -1          /* we don't support EEOL */             /* 30 */
};


/**********************************************************************
*  Function :   start_negotiation()
*  Purpose  :   Send the initial negotiations on the network and print
*               the negotitations to the console screen.
*  Parameters   :
*           dat - the port number to write to
*           cvs - the console's virtual screen
*  Returns  :   none
*  Calls    :   tprintf(), netprintf()
*  Called by    :   dosessions()
*********************************************************************/
void start_negotiation(struct twin *tw,int cvs)
{
    /* Send the initial tlnet negotiations */
    netprintf(tw->pnum,"%c%c%c",IAC,DOTEL,ECHO);
    netprintf(tw->pnum,"%c%c%c",IAC,DOTEL,SGA);
    netprintf(tw->pnum,"%c%c%c",IAC,WILLTEL,NAWS);
    netprintf(tw->pnum,"%c%c%c",IAC,DONTTEL,AUTHENTICATION);
    netprintf(tw->pnum,"%c%c%c",IAC,WONTTEL,AUTHENTICATION);
    if(tw->mapoutput) { /* check whether we are going to be output mapping */
        netprintf(tw->pnum,"%c%c%c",IAC,DOTEL,BINARY);
        tw->uwantbinary=1;  /* set the flag indicating we wanted server to start transmitting binary */
        netprintf(tw->pnum,"%c%c%c",IAC,WILLTEL,BINARY);
        tw->iwantbinary=1;  /* set the flag indicating we want to start transmitting binary */
      } /* end if */

    /* Print to the console what we just did */
    if(tw->condebug>0) {
        tprintf(cvs,"SEND: %s %s\r\n",telstates[DOTEL-LOW_TEL_OPT],teloptions[ECHO]);
        tprintf(cvs,"SEND: %s %s\r\n",telstates[DOTEL-LOW_TEL_OPT],teloptions[SGA]);
        tprintf(cvs,"SEND: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[NAWS]);
        if(tw->mapoutput) { /* check whether we are going to be output mapping */
            tprintf(cvs,"SEND: %s %s\r\n",telstates[DOTEL-LOW_TEL_OPT],teloptions[BINARY]);
            tprintf(cvs,"SEND: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[BINARY]);
          } /* end if */
#ifdef QAK
        tprintf(cvs,"SEND: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[TERMTYPE]);
        tprintf(cvs,"SEND: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[LINEMODE]);
#endif
      } /* end if */
}   /* end start_negotiation() */

/**********************************************************************
*  Function :   parse_subnegotiat()
*  Purpose  :   Parse the telnet sub-negotiations read into the parsedat
*               array.
*  Parameters   :
*           end_sub - index of the character in the 'parsedat' array which
*                           is the last byte in a sub-negotiation
*  Returns  :   none
*  Calls    :
*  Called by    :   parse()
**********************************************************************/
static void parse_subnegotiat(struct twin *tw,int end_sub)
{
    int cv,                     /* virtual screen of the console */
        i;                      /* local counting variable */
    int line_opt_flag;          /* flag to indicate that line mode options were sent and need to be finished */

    cv=console->vs;             /* get the virtual screen of the console screen */

    switch(parsedat[0]) {
        case TERMTYPE:
            if(parsedat[1]==1) {
/* QAK!!! */    netprintf(tw->pnum,"%c%c%c%c%s%c%c",IAC,SB,TERMTYPE,0,def.termtype,IAC,SE);
                if(tw->condebug>0)
                    tprintf(cv,"RECV: SB TERMINAL-TYPE SEND\r\nSEND: SB TERMINAL-TYPE IS %s\r\n",def.termtype);
              } /* end if */
            break;

        case LINEMODE:
            switch(parsedat[1]) {
                case MODE:
                    if(tw->condebug>0) {    /* check for debugging ouput */
                        tprintf(cv,"RECV: SB LINEMODE %s ",LMoptions[parsedat[1]]);
                        if(parsedat[2]&MODE_EDIT)
                            tprintf(cv,"%s|",ModeOptions[1]);
                        if(parsedat[2]&MODE_TRAPSIG)
                            tprintf(cv,"%s|",ModeOptions[2]);
                        if(parsedat[2]&MODE_ACK)
                            tprintf(cv,"%s|",ModeOptions[3]);
                        if(parsedat[2]&MODE_SOFT_TAB)
                            tprintf(cv,"%s|",ModeOptions[4]);
                        if(parsedat[2]&MODE_LIT_ECHO)
                            tprintf(cv,"%s",ModeOptions[5]);
                        tprintf(cv,"\r\n");
                      } /* end if */
                    if((parsedat[2]&(~MODE_ACK))!=tw->linemode_mask && !(parsedat[2]&MODE_ACK)) { /* ignore the mode change if it is the same as the current one, or if the MODE_ACK bit is set */
                        if(parsedat[2]&MODE_EDIT)   /* check for line buffering/editting */
                            tw->lmedit=1;
                        else    /* character buffering */
                            tw->lmedit=0;
                        if(parsedat[2]&MODE_TRAPSIG)   /* check for local signal trapping */
                            tw->trapsig=1;
                        else
                            tw->trapsig=0;
                        if(parsedat[2]&MODE_SOFT_TAB)   /* check for tab expansion */
                            tw->softtab=1;
                        else
                            tw->softtab=0;
                        if(parsedat[2]&MODE_SOFT_TAB)   /* check for literal echo */
                            tw->litecho=1;
                        else
                            tw->litecho=0;
                        parsedat[2]&=LINEMODE_MODES_SUPPORTED;  /* mask off any modes we don't support */

                        tw->linemode_mask=parsedat[2];  /* save the new linemode MODE */
                        netprintf(tw->pnum,"%c%c%c%c%c%c%c",IAC,SB,LINEMODE,MODE,parsedat[2]|MODE_ACK,IAC,SE);
                        if(tw->condebug>0) {    /* check for debugging ouput */
                            tprintf(cv,"SEND: SB LINEMODE MODE ");
                            if(parsedat[2]&MODE_EDIT)
                                tprintf(cv,"%s|",ModeOptions[1]);
                            if(parsedat[2]&MODE_TRAPSIG)
                                tprintf(cv,"%s|",ModeOptions[2]);
                            if(parsedat[2]&MODE_SOFT_TAB)
                                tprintf(cv,"%s|",ModeOptions[4]);
                            if(parsedat[2]&MODE_LIT_ECHO)
                                tprintf(cv,"%s|",ModeOptions[5]);
                            tprintf(cv,"MODE_ACK IAC SE\r\n");
                          } /* end if */
                      } /* end if */
                    break;

                case DOTEL:
                    netprintf(tw->pnum,"%c%c%c%c%c%c%c",IAC,SB,LINEMODE,WONTTEL,parsedat[2],IAC,SE);
                    if(tw->condebug>0) {    /* check for debugging ouput */
                        tprintf(cv,"RECV: SB LINEMODE %s %s\r\n",telstates[DOTEL-LOW_TEL_OPT],LMoptions[parsedat[2]]);
                        tprintf(cv,"SEND: SB LINEMODE WONTTEL %s IAC SE\r\n",LMoptions[parsedat[2]]);
                      } /* end if */
                    break;

                case WILLTEL:
                    netprintf(tw->pnum,"%c%c%c%c%c%c%c",IAC,SB,LINEMODE,DONTTEL,parsedat[2],IAC,SE);
                    if(tw->condebug>0) {    /* check for debugging ouput */
                        tprintf(cv,"RECV: SB LINEMODE %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],LMoptions[parsedat[2]]);
                        tprintf(cv,"SEND: SB LINEMODE DONTTEL %s IAC SE\r\n",LMoptions[parsedat[2]]);
                      } /* end if */
                    break;

                case SLC:
                    line_opt_flag=0;    /* reset the flag indicating that we sent the line mode begin option */
                    if(tw->condebug>0)  /* check for debugging ouput */
                        tprintf(cv,"RECV: SB LINEMODE SLC\r\n");
                    for(i=2; i<end_sub; i+=3) {
                        if(tw->condebug>0) {    /* check for debugging ouput */
                            tprintf(cv,"     [%2.2X %2.2X %2.2X] (%2d) %s %s",(unsigned)parsedat[i],(unsigned)parsedat[i+1],(unsigned)parsedat[i+2],(unsigned)parsedat[i],SLCoptions[parsedat[i]],SLCflags[parsedat[i+1]&SLC_LEVELBITS]);
                            if(parsedat[i+1]&SLC_FLUSHIN)   /* check for FLUSH INPUT flag */
                                tprintf(cv,"|SLC_FLUSHIN");
                            if(parsedat[i+1]&SLC_FLUSHOUT)  /* check for FLUSH OUTPUT flag */
                                tprintf(cv,"|SLC_FLUSHOUT");
                            if(parsedat[i+1]&SLC_ACK)       /* check for ACK flag */
                                tprintf(cv,"|SLC_ACK");
                            tprintf(cv," %d\r\n",(unsigned)parsedat[i+2]);
                          } /* end if */
                        if(tw->slc[parsedat[i]]!=parsedat[i+2]) { /* check the new character for the function against our current character for that function */
                            if((tw->slm[parsedat[i]]&SLC_LEVELBITS)==(parsedat[i+1]&SLC_LEVELBITS) && (parsedat[i+1]&SLC_ACK)) {   /* if it's the same level as we have, and the ACK flag is set, just use the new value */
                                tw->slc[parsedat[i]]=parsedat[i+2];
                              } /* end if */
                            else {  /* level for the function is not the same, or the ACK flag is not set */
                                if(!(parsedat[i+1]&SLC_ACK)) {      /* if the ACK bit isn't set, then work with it */
                                    if(line_opt_flag==0) { /* check whether we've started the negotiations yet */
                                        line_opt_flag=1;
                                        netprintf(tw->pnum,"%c%c%c%c",IAC,SB,LINEMODE,SLC);
                                        if(tw->condebug>0)  /* check for debugging ouput */
                                            tprintf(cv,"SEND: SB LINEMODE SLC\r\n");
                                      } /* end if */
                                    if((parsedat[i]<=NUMLMODEOPTIONS) && (tw->slm[parsedat[i]]&SLC_SUPPORTED)) {  /* only try to parse linemode sub-options we have a clue about, and we support */
                                        switch(parsedat[i+1]&SLC_LEVELBITS) {
                                            case SLC_NOSUPPORT:     /* other side doesn't support this function */
                                                tw->slc[parsedat[i]]=(-1);  /* make certain we don't match it */
                                                tw->slm[parsedat[i]]&=~SLC_LEVELBITS;    /* mask off the correct characters */
                                                netprintf(tw->pnum,"%c%c%c",parsedat[i],(unsigned char)(SLC_NOSUPPORT|SLC_ACK),0);
                                                if(tw->condebug>0)  /* check for debugging ouput */
                                                    tprintf(cv, "        [%2.2X %2.2X %2.2X] (%d) %s SLC_NOSUPPORT|SLC_ACK %d\r\n",(unsigned)parsedat[i],(unsigned)parsedat[i+1],(unsigned)parsedat[i+2],(unsigned)parsedat[i],SLCoptions[parsedat[i]],0);
                                                break;

                                            case SLC_CANTCHANGE:    /* other side can't change the character for this function */
                                                tw->slc[parsedat[i]]=parsedat[i+2]; /* set the character */
                                                tw->slm[parsedat[i]]&=~SLC_LEVELBITS;    /* mask off the correct characters */
                                                tw->slm[parsedat[i]]|=SLC_CANTCHANGE;   /* turn on the cant change mode */
                                                if(parsedat[i+2]!=IAC)  /* check for character being a 255 */
                                                    netprintf(tw->pnum,"%c%c%c",parsedat[i],(unsigned char)(SLC_CANTCHANGE|SLC_ACK),parsedat[i+2]);
                                                else
                                                    netprintf(tw->pnum,"%c%c%c%c",parsedat[i],(unsigned char)(SLC_CANTCHANGE|SLC_ACK),IAC,parsedat[i+2]);
                                                if(tw->condebug>0)  /* check for debugging ouput */
                                                    tprintf(cv, "        [%2.2X %2.2X %2.2X] (%d) %s SLC_CANTCHANGE|SLC_ACK %d\r\n",(unsigned)parsedat[i],(unsigned)parsedat[i+1],(unsigned)parsedat[i+2],(unsigned)parsedat[i],SLCoptions[parsedat[i]],parsedat[i+2]);
                                                break;

                                            case SLC_VALUE:     /* other side has a value it wants us to use */
                                                tw->slc[parsedat[i]]=parsedat[i+2]; /* set the character */
                                                tw->slm[parsedat[i]]&=~SLC_LEVELBITS;    /* mask off the correct characters */
                                                tw->slm[parsedat[i]]|=SLC_VALUE;   /* turn on the cant change mode */
                                                if(parsedat[i+2]!=IAC)  /* check for character being a 255 */
                                                    netprintf(tw->pnum,"%c%c%c",parsedat[i],(unsigned char)(SLC_VALUE|SLC_ACK),parsedat[i+2]);
                                                else
                                                    netprintf(tw->pnum,"%c%c%c%c",parsedat[i],(unsigned char)(SLC_VALUE|SLC_ACK),IAC,parsedat[i+2]);
                                                if(tw->condebug>0)  /* check for debugging ouput */
                                                    tprintf(cv, "        [%2.2X %2.2X %2.2X] (%d) %s SLC_VALUE|SLC_ACK %d\r\n",(unsigned)parsedat[i],(unsigned)parsedat[i+1],(unsigned)parsedat[i+2],(unsigned)parsedat[i],SLCoptions[parsedat[i]],parsedat[i+2]);
                                                break;

                                            case SLC_DEFAULT:   /* other side wants us to use our default */
                                                tw->slc[parsedat[i]]=LMdefaults[parsedat[i]]; /* set the character */
                                                tw->slm[parsedat[i]]&=~SLC_LEVELBITS;    /* mask off the correct characters */
                                                tw->slm[parsedat[i]]|=SLC_VALUE;   /* turn on the cant change mode */
                                                netprintf(tw->pnum,"%c%c%c",parsedat[i],(unsigned char)SLC_VALUE,LMdefaults[parsedat[i]]);
                                                if(tw->condebug>0)  /* check for debugging ouput */
                                                    tprintf(cv, "        [%2.2X %2.2X %2.2X] (%d) %s SLC_VALUE %d\r\n",(unsigned)parsedat[i],(unsigned)parsedat[i+1],(unsigned)LMdefaults[parsedat[i]],(unsigned)parsedat[i],SLCoptions[parsedat[i]],LMdefaults[parsedat[i]]);
                                                break;
                                          } /* end switch */
                                      } /* end if */
                                    else {  /* we don't know what this linemode sub-option is, or we don't support it */
                                        if((parsedat[i+1]&SLC_LEVELBITS)==SLC_NOSUPPORT) {  /* check if we doesn't support it either */
                                            netprintf(tw->pnum,"%c%c%c",parsedat[i],(unsigned char)(SLC_NOSUPPORT|SLC_ACK),0);
                                            if(tw->condebug>0)  /* check for debugging ouput */
                                                tprintf(cv, "        [%2.2X %2.2X %2.2X] (%d) %s SLC_NOSUPPORT|SLC_ACK %d\r\n",(unsigned)parsedat[i],(unsigned)parsedat[i+1],(unsigned)parsedat[i+2],(unsigned)parsedat[i],SLCoptions[parsedat[i]],0);
                                          } /* end if */
                                        else {
                                            netprintf(tw->pnum,"%c%c%c",parsedat[i],(unsigned char)SLC_NOSUPPORT,0);
                                            if(tw->condebug>0)  /* check for debugging ouput */
                                                tprintf(cv, "        [%2.2X %2.2X %2.2X] (%d) %s SLC_NOSUPPORT %d\r\n",(unsigned)parsedat[i],(unsigned)parsedat[i+1],(unsigned)parsedat[i+2],(unsigned)parsedat[i],SLCoptions[parsedat[i]],0);
                                          } /* end else */
                                      } /* end else */
                                  } /* end if */
                              } /* end else */
                          } /* end if */
                      } /* end for */
                    if(line_opt_flag) {    /* check if we had to reply */
                        netprintf(tw->pnum,"%c%c",IAC,SE);
                        if(tw->condebug>0)  /* check for debugging ouput */
                            tprintf(cv,"SEND: IAC SE\r\n");
                      } /* end if */
                    break;

                default:    /* otherwise just exit */
                    break;
              } /* end switch */
            break;

        default:
            break;
      } /* end switch */
}   /* end parse_subnegotiat() */

/*********************************************************************/
/*  parse
*   Do the telnet negotiation parsing.
*
*   look at the string which has just come in from outside and
*   check for special sequences that we are interested in.
*
*   Tries to pass through routine strings immediately, waiting for special
*   characters ESC and IAC to change modes.
*/
void parse(struct twin *tw,unsigned char *st,int cnt)
{
    int cv,                     /* virtual screen of the console */
        i;                      /* local counting variable */
    static int sub_pos;         /* the position we are in the subnegotiation parsing */
    int end_sub;                /* index of the character in the 'parsedat' array which is the last byte in a subnegotiation */
	unsigned char *mark,*orig;

	cv=console->vs;

#ifdef NEGOTIATEDEBUG
    if(tw->condebug>1) {  /* check for debugging ouput */
        tprintf(cv,"\r\n");
        for(i=0; i<cnt; i++) {
            int j;

            for(j=0; (j < 16) && ((i + j) < cnt); j++)
                tprintf(cv,"%2.2X  ", *(unsigned char *) (st + i + j));
            i+=j-1;
            tprintf(cv,"\r\n");
          } /* end for */
        tprintf(cv,"\r\n");
      } /* end if */
#endif /* NEGOTIATEDEBUG */

	orig=st;				/* remember beginning point */
	mark=st+cnt;			/* set to end of input string */
	netpush(tw->pnum);

/*
*  traverse string, looking for any special characters which indicate that
*  we need to change modes.
*/
	while(st<mark) {
        while(tw->telstate!=STNORM && st<mark) {   /* try to handle the negotiations better */
            switch(tw->telstate) {
                case ESCFOUND:
#ifdef USETEK
                    if((*st==12) && (def.tek))    {               /* esc-FF */
                        if(tw->termstate==VTEKTYPE) {
                            tprintf(cv,"\r\n Entering Tek mode \r\n");
                            tw->termstate=TEKTYPE;
                            VGgmode(rgdevice);
                            VGuncover(temptek);
                            current=tw;
                          } /* end if */
                        VGwrite(temptek,"\033\014",2);
                        orig=++st;                  /* pass by ESC-FF in data */
                        tw->telstate=STNORM;
                        break;
                      } /* end if */
#endif

#ifdef USERAS
                    if(*st=='^') {                  /* esc-^ */
                        tw->termstate=RASTYPE;
                        tw->telstate=STNORM;
                        current=tw;
                        VRwrite("\033^",2);          /* echo ^ */
                        orig=++st;
                        break;
                      } /* end if */
#endif

                    parsewrite(tw,"\033",1);        /* send the missing ESC */
                    tw->telstate=STNORM;
                    break;

                case IACFOUND:              /* telnet option negotiation */
                    if(*st==IAC) {          /* real data=255 */
                        st++;               /* real 255 will get sent */
                        tw->telstate=STNORM;
                        break;
                      } /* end if */

                    if(*st>239) {
                        tw->telstate=*st++; /* by what the option is */
                        break;
                      } /* end if */

                    tprintf(cv,"\r\n strange telnet option %s",itoa(*st,s,10));
                    orig=++st;
                    tw->telstate=STNORM;
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
                case ABORT:     /* received a telnet Abort Process command */
                case SUSP:      /* received a telnet Suspend Process command */
                case TEL_EOF:   /* received a telnet EOF command */
                    if(tw->condebug>0)    /* check for debugging ouput */
                        tprintf(cv,"RECV: %s\r\n",telstates[tw->telstate-LOW_TEL_OPT]);
                    tw->telstate=STNORM;
#ifdef OLD_WAY
                    orig=++st;
#else
                    orig=st; /* NJT Mod was ++st, caused loss of 1 character */ /* rmg 930917 */
#endif
                    break;

                case GOAHEAD:       /* telnet go ahead option*/
                    if(tw->condebug>0)    /* check for debugging ouput */
                        tprintf(cv,"RECV: %s\r\n",telstates[tw->telstate-LOW_TEL_OPT]);
                    tw->telstate=STNORM;
#ifdef OLD_WAY
                    orig=++st;
#else
                    orig=st; /* NJT Mod was ++st, caused loss of 1 character */
#endif
                    break;

                case DOTEL:     /* received a telnet DO negotiation */
                    if(tw->condebug>0)    /* check for debugging ouput */
                        tprintf(cv,"RECV: %s %s\r\n",telstates[tw->telstate-LOW_TEL_OPT],teloptions[*st]);
                    switch(*st) {
                        case BINARY:       /* binary transmission */
                            if(!tw->ibinary) { /* binary */
                                if(!tw->iwantbinary) {   /* check whether we asked for this */
                                    netprintf(tw->pnum,"%c%c%c",IAC,WILLTEL,BINARY);
                                    if(tw->condebug>0)    /* check for debugging ouput */
                                        tprintf(cv,"SEND: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[BINARY]);
                                  } /* end if */
                                else
                                    tw->iwantbinary=0;  /* turn off this now */
                                tw->ibinary=1;
                              } /* end if */
                            else {
                                if(tw->condebug>0)    /* check for debugging ouput */
                                    tprintf(cv,"NO REPLY NEEDED: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[BINARY]);
                              } /* end else */
                            break;

                        case SGA:       /* Suppress go-ahead */
                            if(!tw->igoahead) { /* suppress go-ahead */
                                netprintf(tw->pnum,"%c%c%c",IAC,WILLTEL,SGA);
                                if(tw->condebug>0)    /* check for debugging ouput */
                                    tprintf(cv,"SEND: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[SGA]);
                                tw->igoahead=1;
                              } /* end if */
                            else {
                                if(tw->condebug>0)    /* check for debugging ouput */
                                    tprintf(cv,"NO REPLY NEEDED: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[SGA]);
                              } /* end else */
                            break;

                        case TERMTYPE:      /* terminal type negotiation */
                            if(!tw->termsent) {
                                tw->termsent=1;
                                netprintf(tw->pnum,"%c%c%c",IAC,WILLTEL,TERMTYPE);
                                if(tw->condebug>0)    /* check for debugging ouput */
                                    tprintf(cv,"SEND: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[TERMTYPE]);
                              } /* end if */
                            else {
                                if(tw->condebug>0)    /* check for debugging ouput */
                                    tprintf(cv,"NO REPLY NEEDED: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[TERMTYPE]);
                              } /* end else */
                            break;

                        case LINEMODE:      /* linemode negotiation */
                            tw->lmflag=1;   /* set the linemode flag for this connection */
                            netprintf(tw->pnum,"%c%c%c",IAC,WILLTEL,LINEMODE);
                            netprintf(tw->pnum,"%c%c%c%c",IAC,SB,LINEMODE,SLC,0,SLC_DEFAULT,0,IAC,SE);  /* Tell the other side to send us it's default character set */
                            if(tw->condebug>0) {    /* check for debugging ouput */
                                tprintf(cv,"SEND: %s %s\r\n",telstates[WILLTEL-LOW_TEL_OPT],teloptions[LINEMODE]);
                                tprintf(cv,"SEND: SB LINEMODE SLC 0 SLC_DEFAULT 0 IAC SE\r\n");
                              } /* end if */
                            break;

                        case NAWS:      /* Negotiate About Window Size */
                            netprintf(tw->pnum,"%c%c%c%c%c%c%c%c%c",IAC,SB,NAWS,(char)0,(char)tw->width,(char)0,(char)tw->rows,IAC,SE);
                            if(tw->condebug>0)      /* check for debugging ouput */
                                tprintf(cv,"SEND: SB NAWS 0 %d 0 %d IAC SE\r\n",tw->width,tw->rows);
                            break;

                        default:
                            netprintf(tw->pnum,"%c%c%c",IAC,WONTTEL,*st);
                            if(tw->condebug>0)      /* check for debugging ouput */
                                tprintf(cv,"SEND: %s %s\r\n",telstates[WONTTEL-LOW_TEL_OPT],teloptions[*st]);
                            break;

                      } /* end switch */
                    tw->telstate=STNORM;
                    orig=++st;
                    break;

                case DONTTEL:       /* Received a telnet DONT option */
                    if(tw->condebug>0)      /* check for debugging ouput */
                        tprintf(cv,"RECV: %s %s\r\n",telstates[tw->telstate-LOW_TEL_OPT],teloptions[*st]);
                    if((*st)==BINARY) {     /* check for binary negoatiations */
                        if(tw->ibinary) {   /* binary */
                            if(!tw->iwantbinary) {   /* check whether we asked for this */
                                netprintf(tw->pnum,"%c%c%c",IAC,WONTTEL,BINARY);
                                if(tw->condebug>0)    /* check for debugging ouput */
                                    tprintf(cv,"SEND: %s %s\r\n",telstates[WONTTEL-LOW_TEL_OPT],teloptions[BINARY]);
                              } /* end if */
                            else
                                tw->iwantbinary=0;  /* turn off this now */
                            tw->ibinary=0;
                            tw->mapoutput=0;    /* turn output mapping off */
                          } /* end if */
                        else {
                            if(tw->condebug>0)    /* check for debugging ouput */
                                tprintf(cv,"NO REPLY NEEDED: %s %s\r\n",telstates[WONTTEL-LOW_TEL_OPT],teloptions[BINARY]);
                          } /* end else */
                      } /* end if */

                    tw->telstate=STNORM;
                    orig=++st;
                    break;

                case WILLTEL:       /* received a telnet WILL option */
                    if(tw->condebug>0)      /* check for debugging ouput */
                        tprintf(cv,"RECV: %s %s\r\n",telstates[tw->telstate-LOW_TEL_OPT],teloptions[*st]);
                    switch(*st) {
                        case BINARY:                /* binary */
                            if(!tw->ubinary) {   /* binary */
                                if(!tw->uwantbinary) {   /* check whether we asked for this */
                                    netprintf(tw->pnum,"%c%c%c",IAC,DOTEL,BINARY);
                                    if(tw->condebug>0)    /* check for debugging ouput */
                                        tprintf(cv,"SEND: %s %s\r\n",telstates[DOTEL-LOW_TEL_OPT],teloptions[BINARY]);
                                  } /* end if */
                                else
                                    tw->uwantbinary=0;  /* turn off this now */
                                tw->ubinary=1;
                              } /* end if */
                            else {
                                if(tw->condebug>0)    /* check for debugging ouput */
                                    tprintf(cv,"NO REPLY NEEDED: %s %s\r\n",telstates[DOTEL-LOW_TEL_OPT],teloptions[BINARY]);
                              } /* end else */
                            break;

                        case SGA:                   /* suppress go-ahead */
                            if(!tw->ugoahead) {
                                tw->ugoahead=1;
                                netprintf(tw->pnum,"%c%c%c",IAC,DOTEL,SGA); /* ack */
                                if(tw->condebug>0)      /* check for debugging ouput */
                                    tprintf(cv,"SEND: %s %s\r\n",telstates[DOTEL-LOW_TEL_OPT],teloptions[SGA]);
                              } /* end if */
                            break;

                        case ECHO:                      /* echo */
                            if(!tw->echo) {
                                tw->echo=1;
                                netprintf(tw->pnum,"%c%c%c",IAC,DOTEL,ECHO); /* ack */
                                if(tw->condebug>0)      /* check for debugging ouput */
                                    tprintf(cv,"SEND: %s %s\r\n",telstates[DOTEL-LOW_TEL_OPT],teloptions[ECHO]);
#ifdef OLD_WAY
/* QAK!!! */                    netwrite(tw->pnum,tw->linemode,strlen(tw->linemode));
                                tw->linemode[0]='\0';
#endif
                              } /* end if */
                            break;

                        case TIMING:        /* Timing mark */
                            tw->timing=0;
                            break;

                        default:
                            netprintf(tw->pnum,"%c%c%c",IAC,DONTTEL,*st);
                            if(tw->condebug>0)      /* check for debugging ouput */
                                tprintf(cv,"SEND: %s %s\r\n",telstates[DONTTEL-LOW_TEL_OPT],teloptions[*st]);
                            break;
                      } /* end switch */
                    tw->telstate=STNORM;
                    orig=++st;
                    break;

                case WONTTEL:       /* Received a telnet WONT option */
                    if(tw->condebug>0)      /* check for debugging ouput */
                        tprintf(cv,"RECV: %s %s\r\n",telstates[tw->telstate-LOW_TEL_OPT],teloptions[*st]);
                    tw->telstate=STNORM;
                    switch(*st++) {     /* which option? */
                        case BINARY:            /* binary */
                            if(tw->ubinary) {  /* binary */
                                if(!tw->uwantbinary) {   /* check whether we asked for this */
                                    netprintf(tw->pnum,"%c%c%c",IAC,DONTTEL,BINARY);
                                    if(tw->condebug>0)    /* check for debugging ouput */
                                        tprintf(cv,"SEND: %s %s\r\n",telstates[DONTTEL-LOW_TEL_OPT],teloptions[BINARY]);
                                  } /* end if */
                                else
                                    tw->uwantbinary=0;  /* turn off this now */
                                tw->ubinary=0;
                                tw->mapoutput=0;    /* turn output mapping off */
                              } /* end if */
                            else {
                                if(tw->condebug>0)    /* check for debugging ouput */
                                    tprintf(cv,"NO REPLY NEEDED: %s %s\r\n",telstates[DONTTEL-LOW_TEL_OPT],teloptions[BINARY]);
                              } /* end else */
                            break;

                        case ECHO:              /* echo */
                            if(tw->echo) {
                                tw->echo=0;
                                netprintf(tw->pnum,"%c%c%c",IAC,DONTTEL,ECHO);
                                if(tw->condebug>0)      /* check for debugging ouput */
                                    tprintf(cv,"SEND: %s %s\r\n",telstates[DONTTEL-LOW_TEL_OPT],teloptions[ECHO]);
                              } /* end if */
                            break;

                        case TIMING:    /* Telnet timing mark option */
                            tw->timing=0;
                            break;

                        default:
                            break;
                      } /* end switch */
                    orig=st;
                    break;

                case SB:        /* telnet sub-options negotiation */
                    tw->telstate=NEGOTIATE;
                    orig=st;
                    end_sub=0;
                    sub_pos=tw->substate=0;     /* Defined for each */
#ifdef OLD_WAY
                    break;
#endif

                case NEGOTIATE:
                    if(tw->substate==0) { /* until we change sub-negotiation states, accumulate bytes */
                        if(*st==IAC) {  /* check if we found an IAC byte */
                            if(*(st+1)==IAC) {  /* skip over double IAC's */
                                st++;
                                parsedat[sub_pos++]=*st++;
                              } /* end if */
                            else {
                                end_sub=sub_pos;
                                tw->substate=*st++;
                              } /* end else */
                          } /* end if */
                        else     /* otherwise, just stash the byte */
                            parsedat[sub_pos++]=*st++;
                      } /* end if */
                    else {
                        tw->substate=*st++;
                        if(tw->substate==SE)    /* check if we've really ended the sub-negotiations */
                            parse_subnegotiat(tw,end_sub);
                        orig=st;
                        tw->telstate=STNORM;
                      } /* end else */
                    break;

                default:
                    tw->telstate=STNORM;
                    break;
              } /* end switch */
          } /* end while */

/*
* quick scan of the remaining string, skip chars while they are
* uninteresting
*/
        if(tw->telstate==STNORM && st<mark) {
/*
*  skip along as fast as possible until an interesting character is found
*/
            while(st<mark && *st!=27 && *st!=IAC) {
                if(!tw->ubinary)
                    *st&=127;                 /* mask off high bit */
				st++;
              } /* end while */
			if(!tw->timing) 
				parsewrite(tw,orig,st-orig);
			orig=st;				/* forget what we have sent already */
			if(st<mark)
                switch(*st) {
					case IAC:			/* telnet IAC */
						tw->telstate=IACFOUND;
						st++;
						break;

					case 27:			/* ESCape code */
						if(st==mark-1 || *(st+1)==12 || *(st+1)=='^')
							tw->telstate=ESCFOUND;
						st++;			/* strip or accept ESC char */
						break;

					default:
                        tprintf(cv," strange char>128\r\n");
						st++;
						break;
				  }	/* end switch */
		  }	/* end if */
      } /* end while */
}   /* end parse() */

