/*
*   linemode.c
*
*   Linemode Specific functions
*
*   Quincey Koziol
*
*	Date		Notes
*	--------------------------------------------
*   2/92       Started
*/

/*
* Includes
*/
#ifdef __TURBOC__
#include "turboc.h"
#endif
#ifdef MOUSE
#include "mouse.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <direct.h>
#include <time.h>
#include <conio.h>
#include <process.h>
#include <stdarg.h>
#include <bios.h>
#ifdef MSC
#include <malloc.h>
#endif
#ifdef CHECKNULL
#include <dos.h>
#endif

#include "whatami.h"
#include "nkeys.h"
#include "windat.h"
#include "hostform.h"
#include "protocol.h"
#include "data.h"
#include "confile.h"
#include "telopts.h"
#include "externs.h"

/*
* Macro definitions
*/
#define MAX_LINE_LEN        79

/*
* Static function declarations
*/
static int LMissep(int );

/*
* Static variable declarations
*/
static char bk[]={8,' ',8};

/**********************************************************************
*  Function :   LMissep()
*  Purpose  :   Checks if a character is a word seperator for line mode
*               word checking.
*  Parameters   :
*           c - the character to check
*  Returns  :   (1) for the character is a word seperator, (0) for false
*  Calls    :   none
*  Called by    :   LMgets()
**********************************************************************/
static int LMissep(int c)
{
    if(isspace(c) || c=='_' || c=='*' || c=='!')
		return(1);
	return(0);
}

/**********************************************************************
*  Function :   LMgets()
*  Purpose  :   Non-blocking gets(),  This routine will continually add
*               to a string that is re-submitted until a special
*               character is hit.  It never block the network from
*               being serviced.
*               This routine is structurally similar to RSgets() (in rspc.c)
*               except that it uses the currently defined line mode
*               editting characters instead of hard-wiring backspace,etc.
*  Parameters   :
*           t - the telnet window structure
*  Returns  :   none
*  Calls    :
*  Called by    :   newkey()
**********************************************************************/
int LMgets(struct twin *t)
{
    int lm_char,        /* whether the character was handled by a linemode special character */
        echo,           /* whether to echo the characters typed */
        vs,             /* the virtual screen this telnet session is attached to */
        c,              /* the character typed */
        count,          /* the length of the line */
        line_len,       /* length of the line before we start erasing words */
        found_sep,      /* flag for end of word movements when editting line mode stuff */
        found_nonsep;   /* flag for word movements when editting line mode stuff */
    char *save,
        *s;             /* pointer to the end of the line */
    int i;              /* local counting variable */

    vs=t->vs;           /* get the virtual screen */
    echo=!t->echo;      /* get the echo status */

    count=strlen(t->linemode);  /* set up the string variables */
    save=t->linemode;
    s=save+count;

	while(0<(c=n_chkchar())) {
        lm_char=0;
        if(t->litflag) {    /* check if the "literal character" flag is on */
            if(c>31 && c<127) { /* check for printable ASCII */
                if(echo)
                    VSwrite(vs,(char *)&c,1);
              } /* end if */
            else {
                if(c>0 && c<27) {   /* print ASCII control char */
                    if(echo) {
                        if(t->litecho)  /* check for a literal echo of the character */
                            VSwrite(vs,(char *)&c,1);
                        else {
                            c+=64;
                            VSwrite(vs,"^",1);
                            VSwrite(vs,(char *)&c,1);
                            c-=64;
                          } /* end if */
                      } /* end if */
                  } /* end if */
              } /* end else */
            *s++=(char)c;       /* add to string */
            count++;            /* length of string */
            t->litflag=0;       /* reset the literal flag */

            continue;           /* don't try to do anything else with the character */
          } /* end if */
        if(c==BACKSPACE || c==t->slc[SLC_EC]) { /* check for an "erase character" character */
#ifdef QAK
tprintf(console->vs,"erasing a character\r\n");
#endif
            if(count) {
                if(echo)
                    VSwrite(vs,bk,3);
                count--;        /* one less character */
                s--;            /* move pointer backward */
              } /* end if */
            lm_char=1;  /* set the flag so we avoid the large switch */
          } /* end if */
        if(c==t->slc[SLC_EL]) { /* check for an "erase line" character */
            if(echo)
                for(i=0; i<s-save; i++)
                    VSwrite(vs,bk,3);
            s=save;
            lm_char=1;  /* set the flag so we avoid the large switch */
          } /* end if */
        if(c==t->slc[SLC_EW]) { /* check for an "erase word" character */
#ifdef QAK
tprintf(console->vs,"erasing a word, s=%p\r\n",s);
#endif
            if(count) {
                found_sep=0;    /* set the flag to indicate that we found another word seperator */
                found_nonsep=0; /* set the flag to indicate we haven't found any non-seperators yet */
                line_len=s-save;
                s--;            /* back to to the previous character */
                for(i=0; i<line_len; i++) {
                    if(LMissep(*s)) {   /* this is a line mode seperator */
                        if(found_nonsep) {  /* if we've actually erased part of a word, stop now */
                            found_sep=1;    /* indicate we found another word seperator */
                            s++;    /* move to the character after the seperator */
                            break;
                          } /* end if */
                      } /* end if */
                    else {
                        if(!found_nonsep)   /* check if we've previous found a word seperator */
                            found_nonsep=1;
                      } /* end if */
                    if(echo)
                        VSwrite(vs,bk,3);
                    count--;        /* one less character */
                    s--;            /* move pointer backward */
                  } /* end for */
              } /* end if */
            if(!found_sep)  /* check for a word seperator found */
                s=save;
#ifdef QAK
tprintf(console->vs,"done erasing a word, s=%p\r\n",s);
#endif
            lm_char=1;  /* set the flag so we avoid the large switch */
          } /* end if */
        if(!lm_char) {  /* check if this character was already handled by a line mode special character */
            switch(c) {                 /* allow certain editing chars */
#ifndef OLD_WAY
                case 13:
                case 9:
                case TAB:
                    *s='\0';            /* terminate the string */
                    return(c);
#else

                case TAB:       /* check if we are doing local tab expansion */
                case 9:
                    if(t->softtab) {    /* check whether we are supposed to expand this tab */
                      } /* end if */
                    else {
                      } /* end else */
                    break;

                case 13:
                    *s='\0';            /* terminate the string */
                    return(c);
#endif
                default:
                    if(count==MAX_LINE_LEN) {            /* to length limit */
                        RSbell(vs);
                        *s='\0';                /* terminate */
                        return(0);
                      } /* end if */
                    if(c>31 && c<127) { /* check for printable ASCII */
                        if(echo)
                            VSwrite(vs,(char *)&c,1);
                        *s++=(char)c;           /* add to string */
                        count++;                /* length of string */
                      } /* end if */
                    else {
                        if(c>0 && c<27) {   /* print ASCII control char */
                            if(echo) {
                                if(t->litecho)  /* check if we are supposed to echo this echo literally */
                                    VSwrite(vs,(char *)&c,1);
                                else {
                                    c+=64;
                                    VSwrite(vs,"^",1);
                                    VSwrite(vs,(char *)&c,1);
                                    c-=64;
                                  } /* end else */
                              } /* end if */
                          } /* end if */
                        *s='\0';            /* terminate the string */
                        return(c);
                      } /* end else */
                break;
              } /* end switch */
          } /* end if */
      } /* end while */
	*s='\0';			/* terminate the string */
	return(c);
}   /* end LMgets() */

/**********************************************************************
*  Function :   LMinterp_char()
*  Purpose  :   Interpret a character according to the current settings
*               for the line mode special characters
*  Parameters   :
*           t - the telnet window structure
*           c - the character to interpret
*  Returns  :   (1) - for no further process on the character being
*                   necessary, (0) for further processing allowed
*  Calls    :
*  Called by    :   newkey()
**********************************************************************/
int LMinterp_char(struct twin *t,int c)
{
    int char_handled=0;     /* flag to indicate that no further processing is need for the character */

    if(t->litflag) {       /* transmit this character with no processing */
        t->litflag=0;      /* reset the literal flag */
        strchar(t->linemode,(char)c);
        char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
      } /* end if */
    else {  /* interpret this character */
        if(c==13) {
#ifdef QAK
tprintf(console->vs,"c=%d, t->linemode=%s\r\n",(int)c,t->linemode);
#endif
            parse(t,"\r\n",2);     /* echo the return */
            strcat(t->linemode,"\n");
            netpush(t->pnum);
            netwrite(t->pnum,t->linemode,strlen(t->linemode));
            t->linemode[0]='\0';
            char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
          } /* end if */
        if(t->trapsig) {    /* check if we are supposed to trap telnet signals */
            if(c==t->slc[SLC_BRK]) {  /* send telnet BRK command */
                netprintf(t->pnum,"%c%c",IAC,BREAK);
                tprintf(console->vs,"SEND: IAC BREAK\r\n");
                char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
              } /* end if */
            if(c==t->slc[SLC_IP]) {  /* send telnet interrupt process command */
                netprintf(t->pnum,"%c%c",IAC,IP);
                tprintf(console->vs,"SEND: IAC IP\r\n");
                char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
              } /* end if */
            if(c==t->slc[SLC_AO]) {   /* send telnet about output command */
                netprintf(t->pnum,"%c%c",IAC,AO);
                tprintf(console->vs,"SEND: IAC AO\r\n");
                char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
              } /* end if */
            if(c==t->slc[SLC_AYT]) {  /* send telnet AYT command */
                netprintf(t->pnum,"%c%c",IAC,AYT);
                tprintf(console->vs,"SEND: IAC AYT\r\n");
                char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
              } /* end if */
            if(c==t->slc[SLC_ABORT]) {  /* send telnet abort process command */
                netprintf(t->pnum,"%c%c",IAC,ABORT);
                tprintf(console->vs,"SEND: IAC ABORT\r\n");
                char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
              } /* end if */
            if(c==t->slc[SLC_EOF]) {  /* send telnet EOF command */
                netprintf(t->pnum,"%c%c",IAC,TEL_EOF);
                tprintf(console->vs,"SEND: IAC EOF\r\n");
                char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
              } /* end if */
            if(c==t->slc[SLC_SUSP]) {  /* send telnet suspend command */
                netprintf(t->pnum,"%c%c",IAC,SUSP);
                tprintf(console->vs,"SEND: IAC SUSP\r\n");
                char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
              } /* end if */
          } /* end if */
        if(c==t->slc[SLC_FORW1] || c==t->slc[SLC_FORW2]) { /* pass the current string and the "forward" character immediately */
            strchar(t->linemode,(char)c);
            netpush(t->pnum);
            netwrite(t->pnum,t->linemode,strlen(t->linemode));
            t->linemode[0]='\0';
            char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
          } /* end if */
        if(c==t->slc[SLC_LNEXT]) { /* check for sending a literal character */
            t->litflag=1;      /* set the literal character flag */
            char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
          } /* end if */
        if(c==t->slc[SLC_XON]) {    /* check for XON character */
            strchar(t->linemode,(char)c);   /* we don't support flow of control right now, so just append the character to the line */
            char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
          } /* end if */
        if(c==t->slc[SLC_XOFF]) {   /* check for XOFF character */
            strchar(t->linemode,(char)c);   /* we don't support flow of control right now, so just append the character to the line */
            char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
          } /* end if */
        if(c==t->slc[SLC_RP]) {   /* check for RP character */
            VSredraw(t->vs,0,0,79,numline); /*   kinda a kludge, instead of just re-displaying the line, we re-display the entire screen */
            char_handled=1;     /* set the flag to indicate to further processing is not needed for this character */
          } /* end if */
      } /* end else */

    return(char_handled);   /* return whether we handled the character */
}   /* end LMinterp_char() */

