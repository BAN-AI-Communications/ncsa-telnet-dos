#include <stdio.h>
#if !defined(__TURBOC__) && !defined(__WATCOMC__)
#include <memory.h>
#else
#include <dos.h>
#include <string.h>
#endif
#include <stdlib.h>
#include <malloc.h>
#include "vidinfo.h"
#include "externs.h"

extern int save_screen;

static char *vid_buffer=NULL;
static char *old_screen;
	
void init_text(void)
{
    unsigned int size;

    vid_buffer=(char far *)MK_FP(0,tel_vid_info.segment);

    size=tel_vid_info.cols*tel_vid_info.rows*2;
    if((old_screen=(char *)malloc(size))==NULL) {
      printf("Couldn't get enough memory to save video page - exiting\n");
      exit(-1);
    } /* end if */
    memcpy(old_screen,vid_buffer,size);
}   /* end init_text() */

void end_text(void)
{
    memcpy(vid_buffer,old_screen,tel_vid_info.cols*tel_vid_info.rows*2);
    free(old_screen);
}   /* end end_text() */

