/*
*		%W%	(NCSA)	%G%
*
*      Virtual Screen Kernel Data/Structure Definitions
*                          (vsdata.h)
*  
*    National Center for Supercomputing Applications
*      by Gaige B. Paulsen
*
*    This file contains the control and interface calls for the NCSA
*  Virtual Screen Kernel.
*
*      Version Date    Notes
*      ------- ------  ---------------------------------------------------
*      0.01    861102  Initial coding -GBP
*		2.1		871130	NCSA Telnet 2.1 -GBP
*		2.2 	880715	NCSA Telnet 2.2 -GBP
*		2.3		900527	NCSA Telnet 2.3 -QAK
*
*/

#ifndef VSDATA_H
#define VSDATA_H

#define MAXWID 132      		/* The absolute maximum number of chars/line */

typedef struct VSline {
    struct VSline *next,        /* Pointer to next line */
    	*prev;          		/* Pointer to previous line */
    char *text;  				/* Text for the line */
}VSline;

typedef struct VSscrn {
    VSline
         *scrntop,          /* top of the current screen 					*/
         *buftop,           /* top of the current buffer 					*/
         *vistop,			/* top of the visible area of screen 			*/
        **attrst,	        /* pointer the array of attribute lines         */
        **linest;       	/* pointer the array of  real screen lines      */
    int  lines,				/* How many lines are in the arrays above....   */
		 maxlines,          /* maximum number of lines to save off top      */
         numlines,          /* number of lines currently saved off top      */
         allwidth,          /* real maximum width for this window           */
         maxwidth,          /* current maximum width for this window        */
         savelines,         /* save lines off top? 0=no                     */
         ESscroll,			/* Scroll screen when ES received				*/
         attrib,            /* current attibute                             */
         x,y,               /* current cursor positon                       */
         Px,Py,Pattrib,     /* saved cursor position and attribute          */
         VSIDC,             /* Insert/delete character mode 0=draw line     */
         DECAWM,            /* Auto Wrap Mode 0=off 						*/
         DECCKM,            /* Cursor Key Mode      						*/
         DECPAM,            /* keyPad Application Mode						*/
		 G0,G1,				/* Character set identifiers 					*/
		 charset,			/* Character set mode 							*/
         IRM,               /* Insert/Replace Mode  						*/
         escflg,            /* Current Escape level							*/
         top, bottom,       /* Vertical bounds of the screen 				*/
         Rtop,Rbottom,		/* Vertical bounds of Region 					*/
         Rleft,Rright,		/* Horizontal bounds of Region 					*/
         parmptr,           /* pointer to current parm                      */
         localprint;        /* whether VT100 local printer is on            */
    int  parms[6];          /* Ansi Parameters 								*/
    char *tabs;             /* pointer to array for tab settings 			*/
}VSscrn;

typedef struct VSscrndata {
    VSscrn *loc;        	 /* Location of the Screen record for this scrn */
    int stat;      		     /* status of this screen (0=Uninitialized,     */
                             /*                        1=In Use             */
                             /*                        2=Inited, but not IU */
}VSscrndata;

#ifdef VSMASTER
VSscrn *VSIw;
int VSIwn;
#else
extern VSscrn *VSIw;
extern int VSIwn;
#endif

#define VSPBOTTOM (VSIw->lines) 

#endif
