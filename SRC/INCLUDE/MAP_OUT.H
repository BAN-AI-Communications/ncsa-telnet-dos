/*
*    map_out.h
*  Used for Character output mapping things
****************************************************************************
*                                                                          *
*                                                                          *
*      NCSA Telnet                                                         *
*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
*			Additions by Kurt Mahan, Heeren Pathak, & Quincey Koziol	   *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
****************************************************************************
*	Quincey Koziol
*   Defines for output character mapping variables
*/

#ifndef MAP_OUT_H
#define MAP_OUT_H

#ifdef OUTPUTMASTER
/* array of unsigned chars which are really used as a translation table for */
/*	characters that are output while output mapping is on */
unsigned char outputtable[256];

#else
/* array of unsigned chars which are really used as a translation table for */
/*	characters that are output while output mapping is on */
extern unsigned char outputtable[256];

#endif

#endif	/* map_out.h */
