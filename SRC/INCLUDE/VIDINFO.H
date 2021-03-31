/*
*    vidinfo.h
*   Declarations of global video structures for Telnet
*
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
*      This program is in the public domain.                               *
*                                                                          *
****************************************************************************
*         
*/

#ifndef VIDINFO_H
#define VIDINFO_H

/* Structure containing the video information */
struct vidinfo {
    unsigned int segment;
    int type;
    int iscolor;
    int mode;
    unsigned int page;
    unsigned int rows;
    unsigned int cols;
    unsigned int cursor;
    unsigned int font_func;
};

struct vidstate {
    char mode;
    unsigned char page;
    unsigned int cursor;
    unsigned int curpos;
    unsigned int font_func;
};

#if defined(TELBIN) || defined(FTPBIN)
#if !defined(WINMASTER) && !defined(FTPMASTER)
extern
#endif
struct vidinfo tel_vid_info;
#endif

#if defined(TELBIN) || defined(FTPBIN)
#if !defined(WINMASTER) && !defined(FTPMASTER)
extern
#endif
struct vidstate tel_vid_state;
#endif

#endif  /* VIDINFO_H */

