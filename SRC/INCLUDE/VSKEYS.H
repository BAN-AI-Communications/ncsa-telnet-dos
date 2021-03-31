/*
*		@(#)vskeys.h	1.25	(NCSA)	4/29/88
*
*      Virtual Screen Kernel Keys and Attribute Definitions
*              (vskeys.c)
*  
*    National Center for Supercomputing Applications
*      by Gaige B. Paulsen
*
*      This file contains equates used by the program for specification of
*  special Keyboard characters and definition of the Attribute byte.
*
*      Version Date    Notes
*      ------- ------  ---------------------------------------------------
*      0.01    861102  Initial coding -GBP
*		2.1		871130	NCSA Telnet 2.1 -GBP
*		2.2 	880715	NCSA Telnet 2.2 -GBP
*
*/

#ifndef VSKEYS_H
#define VSKEYS_H

#define VSUP	129		/* Up Arrow */
#define VSDN    130		/* Down Arrow */
#define VSRT    131		/* Right Arrow */
#define VSLT    132		/* Left Arrow */

#define VSK0	133		/* Keypad 0 */
#define VSK1	134		/* Keypad 1 */
#define VSK2	135		/* Keypad 2 */
#define VSK3	136		/* Keypad 3 */
#define VSK4	137		/* Keypad 4 */
#define VSK5	138		/* Keypad 5 */
#define VSK6	139		/* Keypad 6 */
#define VSK7	140		/* Keypad 7 */
#define VSK8	141		/* Keypad 8 */
#define VSK9	142		/* Keypad 9 */
#define VSKC	143		/* Keypad , */
#define VSKM	144		/* Keypad - */
#define VSKP	145		/* Keypad . */
#define VSKE	146		/* Keypad Enter */

#define VSF1	147		/* Function 1 */
#define VSF2	148		/* Function 2 */
#define VSF3	149		/* Function 3 */
#define VSF4	150		/* Function 4 */
#define VSF5	151		/* Function 5 */
#define VSF6	152		/* Function 6 */
#define VSF7	153		/* Function 7 */
#define VSF8	154		/* Function 8 */
#define VSF9	155		/* Function 9 */
#define VSF10	156		/* Function 10 */

/*
*   Ok, here's the scoop about these next couple of codes.
*   The keymapping specifications called for certain keys
*   to always send cursor control VT100 strings.  These
*   next codes always send the correct cursor control
*   escape strings.  The problem with the regular VSUP,
*   VSDN, etc. keys was that depending on the current state
*   of other VT100 flags (mainly DECCKM and DECKPAM) they
*   could output other escape strings.  These codes are
*   hardwired to always send the cursor control codes.
*   the parse_serb() routine in keymap.c and the VSkbsend()
*   routine in vsinterf.c understand these codes.
*           QAK - 6/91
*/
#define VSUP_SPECIAL    193     /* Special Up Arrow Code */
#define VSDN_SPECIAL    194     /* Special Down Arrow Code */
#define VSRT_SPECIAL    195     /* Special Right Arrow Code */
#define VSLT_SPECIAL    196     /* Special Left Arrow Code */

#ifdef VSMASTER
char VSIkpxlate[2][29]={
	"ABCD0123456789,-.\15PQRSTUVWXY",
	"ABCDpqrstuvwxylmnMPQRSTUVWXY"
};
#else
extern char VSIkpxlate[2][29];
#endif

/*
*          Definition of attribute bits in the Virtual Screen
*
*          0   -   Bold
*          1   -   
*          2   -
*          3   -   Underline
*          4   -   Blink
*          5   -
*          6   -   Reverse
*          7   -   Graphics character set
*
*/
#define VSisbold(x)	  (x & 0x01)
#define VSisundl(x)	  (x & 0x08)
#define VSisblnk(x)	  (x & 0x10)
#define VSisrev(x) 	  (x & 0x40)
#define VSisgrph(x)	  (x & 0x80)
#define VSinattr(x)	  (x & 0xd9)
#define VSgraph(x) 	  (x | 0x80)
#define VSnotgraph(x) (x & 0x7F)

#endif
