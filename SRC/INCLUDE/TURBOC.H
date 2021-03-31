/*
*    TURBOC.H
*    Header file for Microsoft specific things which need to be defined
*		when compiling for Turbo C
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
*   Revisions:
*	10/20/90	 Version 2.3 Andrew Tridgell
*/

#ifndef TURBOC_H
#define TURBOC_H

#define _MAX_PATH      144  /* max. length of full pathname */
#define _MAX_DRIVE   3      /* max. length of drive component */
#define _MAX_DIR       130  /* max. length of path component */
#define _MAX_FNAME   9      /* max. length of file name component */
#define _MAX_EXT     5      /* max. length of extension component */

int _dos_setdate(struct dosdate_t *ddate);
int _dos_settime(struct dostime_t *dtime);

struct dosdate_t {
	unsigned char day;
	unsigned char month;
	unsigned int year;
	unsigned char dayofweek;
};

struct dostime_t {
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	unsigned char hsecond;
};

#endif	/* TURBOC_H */
