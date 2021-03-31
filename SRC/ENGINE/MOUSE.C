/*  Mouse routines for NCSA Telnet */
/* Andrew Tridgell 10/17/90 */

/* Basically these can replace n_chkchar,n_getchar and n_scrlck
   with ones that do the following

	mouse up : up arrow
	mouse down : down arrow
	mouse right : right arrow
	mouse left : left arrow

	left button : space bar
	right button : scroll lock toggle

  This is achieved by including mouse.h
*/

#define MOUSE_H

#include <stdio.h>
#include <stdlib.h>
#include "nkeys.h"
#include "externs.h"

#ifdef MSC 
#define mousecl mousecml 
#endif 

int nm_getchar(void);
int nm_chkchar(void);
int nm_scrlck(void);
int nm_chkmouse(void);
int nm_initmouse(void);
int nm_mousespeed(int change);

extern int use_mouse;
extern int ginon;

struct 	{
	int speed,
		num_right,
		num_down;
	unsigned int installed:1,	/* boolean, whether the mouse has been installed */
		left_flag:1,			/* boolean, whether the left button is down */
		right_flag:1,			/* boolean, whether the right button is down */
		scrlck:1;				/* boolean, whether the scrl-lock is active */
  } mouse;

#ifdef GETCHAR_NEEDED
int nm_getchar(void )
{
	int mouse_char;

	if ((mouse_char=nm_chkmouse())!=-1)
		return(mouse_char);
	else
		return(n_getchar());
}	/* end nm_getchar() */
#endif

int nm_chkchar(void )
{
	int mouse_char;

	if((mouse_char=nm_chkmouse())!=-1)
		return(mouse_char);
	else
		return(n_chkchar());
}	/* end nm_chkchar() */

int nm_scrlck(void )
{
	if ((int)(mouse.scrlck) != !(!n_scrlck()))
		return(1);
	return(0);
}	/* end nm_scrlck() */

int nm_chkmouse(void )
{ 
	int m1=3, m2=0 ,m3=0 ,m4=0;		/* mouse variables */ 
	static int press_used=0;

 	if(!mouse.installed)  
		return(-1); 
 
	m1=11; 
 	mousecl(&m1,&m2,&m3,&m4);		/* read mickeys */
 
	mouse.num_right+=m3;
	mouse.num_down+=m4;
	m1=3; 
	mousecl(&m1,&m2,&m3,&m4); 
	if((m2&3)==0) press_used=0;
	if (press_used) return(-1);
	if(m2&1) {					/* left button */
		if((mouse.right_flag)&&(!ginon)) {
			press_used=1;
			mouse.right_flag=0;
			return(ALTV);
		}
		else mouse.left_flag=1;
	}
	else if(mouse.left_flag) {
		mouse.left_flag=0;
		return((int)'\n');
	}	/* end if */

	if (ginon) return(-1);
	if(m2&2) {					/* right button */
		if(mouse.left_flag) {
			press_used=1;
			mouse.left_flag=0;
			return(ALTC);
		}
		else mouse.right_flag=1;
	}
	else if(mouse.right_flag) {
/*		use_mouse=!mouse.scrlck;
*/		mouse.scrlck=!mouse.scrlck;
		mouse.right_flag=0;
	}	/* end if */

	if (!use_mouse && !nm_scrlck()) {
		mouse.num_right=0;
		mouse.num_down=0;
		return(-1);
	};
	if(abs(mouse.num_right)>abs(mouse.num_down)) {
		if(mouse.num_right>=mouse.speed) {
			mouse.num_right-=mouse.speed;
			return(E_CURRT);
		}	/* end if */
		if(mouse.num_right<=(-mouse.speed)) {
			mouse.num_right+=mouse.speed;
			return(E_CURLF);
		}	/* end if */
	}	/* end if */

	if(mouse.num_down>=mouse.speed) {
		mouse.num_down-=mouse.speed;
		return(E_CURDN);
	}	/* end if */
	if(mouse.num_down<=(-mouse.speed)) {
		mouse.num_down+=mouse.speed;
		return(E_CURUP);
	}	/* end if */
 	return(-1); 
}	/* end nm_chkmouse() */

int nm_initmouse(void ) 
{ 
	int m1=0, m2=0, m3=0, m4=0;  			/* mouse variables */

	mouse.installed=1;
	mouse.speed=10;
	mouse.num_right=0;
	mouse.num_down=0;
	mouse.left_flag=0;
	mouse.right_flag=0;
	mouse.scrlck=0;
 
	m1=0;	 					 
	mousecl(&m1,&m2,&m3,&m4);				/* initialize mouse driver */ 
	if(!m1) {
		mouse.installed=0;
		return(0); 
	  }	/* end if */
	m1=2; 
	mousecl(&m1,&m2,&m3,&m4);				/* turn off software cursor */ 
	return(1);
} 	/* end nm_initmouse() */

int nm_mousespeed(int change)
{
	mouse.speed+=change;
	if (mouse.speed<1) mouse.speed=1;
	return(1);
}	/* end nm_mousespeed() */
