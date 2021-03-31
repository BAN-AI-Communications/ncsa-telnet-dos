/***********************************************************************/
/*  menu support header file
*   New 5/22/88 - TK
*   Provide menus with a reasonable user interface for the user to
*   select colors or other parameters.
*
*   This menu system provides two types of entries.
*   1. Up to sixteen static choices, 0-15.  Each static choice is encoded
*      into the data structure and automatically rotated by the user.
*      The user cannot select an illegal value.
*   2. A string choice.  A maximum string length is honored.
*   There must be at least 20 characters open for each field, longer
*   if the field is longer.  Static choices cannot be longer than 20 chars.
*
*	Code Cleanup for 2.3 release - 6/89 QK
*/

#ifndef MENU_H
#define MENU_H

/*
*  structure for the menu entries
*/
struct pt {
	int posx,posy,		/* row and col position on screen for field */
		choice,			/* which is the currently visible selection */
		replen;			/* length of reply if a string is allowed to be entered */
	char *vals[17];		/* pointers to the actual choices */
};

#endif
