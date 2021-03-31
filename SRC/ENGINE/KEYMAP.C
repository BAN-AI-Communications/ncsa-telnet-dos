/*
*	keymap.c
*
*   Keymapping functions for the real screen
*
*   Quincey Koziol
*
*	Date		Notes
*	--------------------------------------------
*	8/90		Started
*	10/90		Added functionality to allow octal & hex mappings
*				and mixed case keywords
*   6/91        Added functionality to allow '\' in curly braces,
*               set special codes for the cursor control codes
*               for Kermit cursor control codes,
*/

/*
* Includes
*/

#define KEYMASTER

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
#include "vskeys.h"
#include "keymap.h"
#include "externs.h"

#define MAX_LINE_LENGTH	160		/* the maximum length of a line in the keyboard mapping file */

/*
*	Global Variables
*/
extern char path_name[];		/* the path name of telbin.exe, used to find telnet.key file */

/* Local functions */
static char *parse_str(char *file_str,uint *ret_code);
static int parse_verb(char *verb_str,uint *ret_code);
static int add_key(unsigned int search_key,char *key_map_str,int is_special,byte special_code);
static int del_key(unsigned int search_key);

/* Local variables */
static char white_sp[]="\x009\x00a\x00b\x00c\x00d\x020";	/* string which contains all the white space characters */
static char end_token[]="\x000\x009\x00a\x00b\x00c\x00d\x020};";	/* string which contains all the white space characters, and the right curley brace & the semi-colon */

/**********************************************************************
*  Function	:	del_key
*  Purpose	:	delete a node from the list of mapped keys
*  Parameters	:
*			search_key - the keycode to add to the list
*  Returns	:	-1 to indicate the key is not found, 0 otherwise
*  Calls	:	none
*  Called by	:	read_keyboard_file()
**********************************************************************/
static int del_key(unsigned int search_key)
{
	key_node *temp_key,		/* temporary pointer for the key node to delete from the list */
		*temp_2key;			/* another temporary pointer to a key node */

	if(IS_KEY_MAPPED(search_key) && head_key!=NULL) {	/* check whether the key is actually mapped, and there are mapped keys in memory */
		temp_2key=temp_key=head_key;
		if((*temp_key).key_code==search_key) {	/* check whether the key to delete is the head of the list */
			head_key=(*head_key).next_node;		/* more around the deleted key */
    } /* end if */
		else {
			temp_key=(*temp_key).next_node;		/* go to the next node */
			while(temp_key!=NULL && (*temp_key).key_code!=search_key) {	/* search for the key to delete */
				temp_2key=temp_key;		/* advance the trailing pointer */
				temp_key=(*temp_key).next_node;	/* advance the leading pointer */
      } /* end while */
			if(temp_key==NULL)	/* check for the key not being in the list */
				return(-1);
			(*temp_2key).next_node=(*temp_key).next_node;	/* link around the key to delete */
    } /* end else */
		if(!IS_KEY_SPECIAL(search_key)) {		/* check for just a regular mapped key string */
			if((*temp_key).key_data.key_str!=NULL)
				free((*temp_key).key_data.key_str);
			else
				return(-1);
    } /* end if */
		RESET_KEY_MAPPED((int)search_key);	/* reset the mapped & special flags */
        RESET_KEY_SPECIAL((int)search_key);
        free(temp_key);         /* free the key node */
  } /* end if */
	else
		return(-1);
}	/* end del_key() */

/**********************************************************************
*  Function	:	add_key
*  Purpose	:	add a node to the list of mapped keys
*  Parameters	:
*			search_key - the keycode to add to the list
*			key_map_str - the string to map the key to
*			is_special - flag to indicate kermit verbs re-mapping
*			special_code - the kermit code to re-map to
*  Returns	:	-1 for an out of memory error, 0 otherwise
*  Calls	:	none
*  Called by	:	read_keyboard_file()
**********************************************************************/
static int add_key(unsigned int search_key,char *key_map_str,int is_special,byte special_code)
{
	key_node *temp_key;		/* temporary pointer for the key node to add to the list */

	if(IS_KEY_MAPPED(search_key)) {	/* check for strictly re-mapping this key */
		if((temp_key=find_key(search_key))!=NULL) {		/* get the key code to remap */
			if(IS_KEY_SPECIAL(search_key)) {		/* check whether the key we are re-mapping is a verb */
				if(is_special)		/* check whether the new key is special also */
					(*temp_key).key_data.vt100_code=special_code;	/* just change the vt100 code generated */
				else {			/* allocate room for the string to map */
					RESET_KEY_SPECIAL((int)search_key);	/* reset the special flag */
					if(((*temp_key).key_data.key_str=strdup(key_map_str))==NULL)	/* duplicate the string to re-map to */
						return(-1);	/* indicate no more memory */
				  }	/* end else */
			  }	/* end if */
			else {		/* not a special key, we need to free the old string */
				if((*temp_key).key_data.key_str!=NULL)	/* free the old string */
					free((*temp_key).key_data.key_str);
				if(is_special) {		/* check whether the new key is special also */
					(*temp_key).key_data.vt100_code=special_code;	/* just change the vt100 code generated */
					SET_KEY_SPECIAL((int)search_key);		/* set the special flag */
				  }	/* end if */
				else {			/* allocate room for the string to map */
					if(((*temp_key).key_data.key_str=strdup(key_map_str))==NULL)	/* duplicate the string to re-map to */
						return(-1);	/* indicate no more memory */
				  }	/* end else */
			  }	/* end else */
			return(0);			/* indicate no error */
		  }	/* end if */
		else		/* uh-uh, memory is messed up */
			return(-1);
	  }	/* end if */
    if((temp_key=(key_node *)malloc((size_t)sizeof(key_node)))!=NULL) {  /* allocate room for the key node */
		(*temp_key).key_code=search_key;	/* set the key code for this node */
		SET_KEY_MAPPED((int)search_key);	/* indicate this key is mapped */
		if(is_special) {	/* check for a kermit verb to re-map to */
			SET_KEY_SPECIAL((int)search_key);
			(*temp_key).key_data.vt100_code=special_code;
		  }	/* end if */
		else {
			if(((*temp_key).key_data.key_str=strdup(key_map_str))==NULL)	/* duplicate the string to re-map to */
				return(-1);	/* indicate no more memory */
		  }	/* end else */
		(*temp_key).next_node=head_key;	/* attach to the linked list */
		head_key=temp_key;
		return(0);
	  }	/* end if */
	else
		return(-1);		/* indicate out of memory */
}	/* end add_key() */

/**********************************************************************
*  Function	:	parse_verb()
*  Purpose	:	compare the verb_str against the various Kermit verbs
*				which we support, and set the correct (internal)
*				vt100 code for that key.  Otherwise, set an error code
*				to return.
*  Parameters	:
*			verb_str - pointer to a string to compare against the kermit verbs we support
*			ret_code - pointer to an unsigned int to return the (internal) vt100 code in
*  Returns	:	0xFFFF for error, otherwise, the length of the kermit verb
*				indentified.
*  Calls	:	none
*  Called by	:	parse_str()
**********************************************************************/
static int parse_verb(char *verb_str,uint *ret_code)
{
    int i;      /* local counting variable */

    for(i=0; i<NUM_KERMIT_VERBS; i++) {
        if(!strnicmp(verb_str,verb_table[i],verb_length[i])) {      /* check for sending each kermit verb */
            *ret_code=verb_num[i];      /* indicate verb found character */
            return(verb_length[i]);          /* indicate the number of characters to skip */
          } /* end if */
      } /* end for */
    *ret_code=0xFFFF;       /* indicate that we don't recognize this verb */
    return(strcspn(verb_str,end_token));    /* return the number of characters to skip */
}   /* end parse_verb() */

/**********************************************************************
*  Function	:	parse_str()
*  Purpose	:	parse the string to map a key to.
*  Parameters	:
*			file_str - pointer to a string from the file to parse
*			ret_code - pointer to an unsigned int to return an (internal)vt100 code in
*  Returns	:	0xFFFF for error, otherwise, the vt100 code for telbin
*  Calls	:	parse_verb()
*  Called by	:	read_keyboard_file()
**********************************************************************/
static char *parse_str(char *file_str,uint *ret_code)
{
	int buff_off=0,			/* current location in the buffer */
		ascii_num,			/* ascii character encoded after a backslash */
		kermit_length,		/* the length of a kermit verb returned from parsing it */
		done=0;				/* flag for dropping out of the loop */
	byte *ret_str,			/* the string variable to return the result in */
		*temp_str,			/* pointer to the current place in the string */
		buffer[MAX_LINE_LENGTH];		/* buffer to store the string we are unravelling */

	*ret_code=0xFFFF;		/* mark return code to indicate nothing special to return */
	temp_str=file_str;		/* start at the beginning of the string to parse */
	while((*temp_str)!='\0' && buff_off<MAX_LINE_LENGTH && !done) {		/* parse until the end of the string or until the buffer is full */
		while((*temp_str) && (*temp_str)!=';' && (*temp_str)!='\\' && (*temp_str)!='{' && (*temp_str)>' ') {	/* copy regular characters until a special one is hit */
			buffer[buff_off]=(*temp_str);		/* copy the character */
			buff_off++;		/* increment the position in the buffer */
			temp_str++;		/* increment the position in the parse string */
		  }	/* end while */
		if(*temp_str) {		/* check on the various special cases for dropping out of the loop */
			switch(*temp_str) {		/* switch for the special case */
				case '\\':	/* backslash for escape coding a character */
					temp_str++;		/* get the character after the backslash */
					if((*temp_str)=='{') {	/* check for curly brace around numbers */
						temp_str++;	/* increment to the next character */
						if((*temp_str)=='K' || (*temp_str)=='k') {		/* check for Kermit escape code here also */
							temp_str++;		/* increment past the kermit flag */
							kermit_length=parse_verb(temp_str,ret_code);		/* parse the kermit verb, and return the correct kermit code in the ret_code */
							if(*ret_code!=0xFFFF)		/* a kermit verb was specified that we recognize */
								return(NULL);		/* return now, indicating a kermit verb we recognize in the ret_code variable */
							temp_str+=kermit_length;	/* increment past the kermit verb */
						  }	/* end if */
						else if((*temp_str)=='o') {	/* check for octal number */
							ascii_num=octal_to_int(temp_str);	/* get the acii number after the escape code */
							buffer[buff_off]=(byte)ascii_num;	/* store the ascii code in the buffer */
							buff_off++;				/* increment our position */
							while((*temp_str)>='0' && (*temp_str)<='7')	/* increment our position past the digits */
								temp_str++;
						  }	/* end if */
						else if((*temp_str)=='x') {	/* check for hexadecimal number */
							ascii_num=hex_to_int(temp_str);	/* get the acii number after the escape code */
							buffer[buff_off]=(byte)ascii_num;	/* store the ascii code in the buffer */
							buff_off++;				/* increment our position */
							while(isxdigit((int)(*temp_str)))	/* increment our position past the digits */
								temp_str++;
						  }	/* end if */
						else {		/* must be an escape integer */
							if((*temp_str)=='d')	/* check for redundant decimal number specification */
								temp_str++;
							ascii_num=atoi(temp_str);	/* get the ascii number after the escape code */
							buffer[buff_off]=(byte)ascii_num;	/* store the ascii code in the buffer */
							buff_off++;				/* increment our position */
							while(isdigit((int)(*temp_str)))	/* increment our position past the digits */
								temp_str++;
						  }	/* end else */
						if((*temp_str)=='}')	/* found the closing curly brace */
							temp_str++;		/* jump over the closing curly brace */
						else {		/* closing curly brace missing, indicate error */
							buff_off=0;
							done=1;
						  }	/* end else */
					  }	/* end if */
					else if((*temp_str)=='K' || (*temp_str)=='k') {		/* check for Kermit escape code */
						temp_str++;		/* increment past the kermit flag */
						kermit_length=parse_verb(temp_str,ret_code);		/* parse the kermit verb, and return the correct kermit code in the ret_code */
						if(*ret_code!=0xFFFF)		/* a kermit verb was specified that we recognize */
							return(NULL);		/* return now, indicating a kermit verb we recognize in the ret_code variable */
						temp_str+=kermit_length;	/* increment past the kermit verb */
					  }	/* end if */
					else if((*temp_str)=='o') {	/* check for octal number */
						ascii_num=octal_to_int(temp_str);	/* get the acii number after the escape code */
						buffer[buff_off]=(byte)ascii_num;	/* store the ascii code in the buffer */
						buff_off++;				/* increment our position */
						while((*temp_str)>='0' && (*temp_str)<='7')	/* increment our position past the digits */
							temp_str++;
					  }	/* end if */
					else if((*temp_str)=='x') {	/* check for hexadecimal number */
						ascii_num=hex_to_int(temp_str);	/* get the acii number after the escape code */
						buffer[buff_off]=(byte)ascii_num;	/* store the ascii code in the buffer */
						buff_off++;				/* increment our position */
						while(isxdigit((int)(*temp_str)))	/* increment our position past the digits */
							temp_str++;
					  }	/* end if */
					else {		/* must be an escape integer */
						if((*temp_str)=='d')	/* check for redundant decimal number specification */
							temp_str++;
						ascii_num=atoi(temp_str);	/* get the ascii number after the escape flag */
						buffer[buff_off]=(byte)ascii_num;	/* store the ascii code in the buffer */
						buff_off++;				/* increment our position */
						while(isdigit((int)(*temp_str)))	/* increment our position past the digits */
							temp_str++;
					  }	/* end else */
					break;

				case '{':	/* curly brace opens a quoted string */
                    temp_str++;     /* jump over the brace itself */
                    while((*temp_str) && (*temp_str)!='}') {    /* copy regular characters until a special one is hit */
                        if((*temp_str)=='\\') {     /* check for a backslash in the curly braces */
                            temp_str++; /* increment to the next character */
                            if((*temp_str)=='o') { /* check for octal number */
                                ascii_num=octal_to_int(temp_str);   /* get the acii number after the escape code */
                                buffer[buff_off]=(byte)ascii_num;   /* store the ascii code in the buffer */
                                buff_off++;             /* increment our position */
                                while((*temp_str)>='0' && (*temp_str)<='7') /* increment our position past the digits */
                                    temp_str++;
                              } /* end if */
                            else if((*temp_str)=='x') { /* check for hexadecimal number */
                                ascii_num=hex_to_int(temp_str); /* get the acii number after the escape code */
                                buffer[buff_off]=(byte)ascii_num;   /* store the ascii code in the buffer */
                                buff_off++;             /* increment our position */
                                while(isxdigit((int)(*temp_str)))   /* increment our position past the digits */
                                    temp_str++;
                              } /* end if */
                            else {      /* must be an escape integer */
                                if((*temp_str)=='d')    /* check for redundant decimal number specification */
                                    temp_str++;
                                ascii_num=atoi(temp_str);   /* get the ascii number after the escape code */
                                buffer[buff_off]=(byte)ascii_num;   /* store the ascii code in the buffer */
                                buff_off++;             /* increment our position */
                                while(isdigit((int)(*temp_str)))    /* increment our position past the digits */
                                    temp_str++;
                              } /* end else */
                          } /* end if */
                        else {
                            buffer[buff_off]=(*temp_str);       /* copy the character */
                            buff_off++;     /* increment the position in the buffer */
                            temp_str++;     /* increment the position in the parse string */
                          } /* end else */
					  }	/* end while */
					if((*temp_str)=='}')		/* found the closing brace */
						temp_str++;		/* jump over the closing brace */
					else {		/* line terminated without closing brace */
						buff_off=0;		/* indicate error condition */
						done=1;			/* drop out of the loop */
					  }	/* end else */
					break;

				default:	/* ctrl characters, space, and semi-colon terminate a string */
                    buffer[buff_off]='\0';  /* terminate the string */
					done=1;
					break;

			  }	/* end switch */
		  }	/* end if */
		else
			buffer[buff_off]='\0';	/* terminate the string */
	  }	/* end while */
	if(buff_off>0) {
		if((ret_str=malloc(buff_off+1))!=NULL)
			strcpy(ret_str,buffer);		/* copy the parsed string */
		return(ret_str);
	  }	/* end if */
	return(NULL);
}	/* end parse_str() */

/**********************************************************************
*  Function	:	read_keyboard_file
*  Purpose	:	read in a keyboard mapping file, parse the input and 
*				map keys
*  Parameters	:
*			key_file - string containing the name of the keyboard mapping file
*  Returns	:	0 for no error, -1 for any errors which occur
*  Calls	:	parse_str(), & lots of library string functions
*  Called by	:	initkbfile(), Sconfile()
**********************************************************************/
int read_keyboard_file(char *key_file)
{
	FILE *key_fp;			/* pointer to the keyboard file */
	char key_line[MAX_LINE_LENGTH],		/* static array to store lines read from keyboard file */
		*map_str,			/* the parsed string from the keyboard file */
		*temp_str;			/* temporary pointer to a string */
	uint line_no=0,			/* what line in the file we are on */
		token_num,			/* the current token we are parsing */
		where,				/* pointer to the beginning of text */
		kermit_code,		/* the variable to return the possible 'kermit verb' code in */
		re_map_key,			/* the key to re-map */
		error=0;			/* error from the file reading */

	if((key_fp=fopen(key_file,"rt"))!=NULL) {
		while((temp_str=fgets(key_line,MAX_LINE_LENGTH,key_fp))!=NULL && !error) {	/* get a line of input */
			token_num=0;			/* initialize the token we are on */
			if((temp_str=strtok(key_line,white_sp))!=NULL) {	/* get the first token from the string */
				if((*temp_str)!=';') {		/* check for a comment line */
					do {
						switch(token_num) {		/* switch on which token we are processing */
							case 0:		/* the 'SET' token (we already know it is not a comment) */
								if(stricmp(temp_str,"SET")) {	/* make certain the first token is a SET token */
									printf("invalid token #%d:'%s' on line %d\n",token_num,temp_str,line_no);
									token_num=4;	/* bump the token count up to drop out of the loop */
								  }	/* end if */
								break;

							case 1:		/* the 'KEY' token */
								if(stricmp(temp_str,"KEY")) {	/* make certain the first token is a KEY token */
									printf("invalid token #%d:'%s' on line %d\n",token_num,temp_str,line_no);
									token_num=4;	/* bump the token count up to drop out of the loop */
								  }	/* end if */
								break;

							case 2:		/* the key to be re-mapped */
								if(!stricmp(temp_str,"CLEAR") || !stricmp(temp_str,"OFF") || !stricmp(temp_str,"ON") || (*temp_str)==';') {	/* ignore the rest of the line if the 'key' is one of the tokens we don't support */
									token_num=4;	/* bump the token count up to drop out of the loop */
								  }	/* end if */
								else {		/* the 'key' field is not a special command or a comment, must be a valid character */
									if(*(temp_str)!='\\') {	/* the key to re-map is not an escape code */
										re_map_key=*temp_str;	/* set the re-mapping key */
									  }	/* end if */
									else {
                                        if(*(temp_str+1)=='d')  /* walk a past the 'd' character */
                                            temp_str++;
										re_map_key=atoi(temp_str+1);	/* get the re-mapping key value from the string */
										if(re_map_key==0) {		/* invalid key code */
											printf("Error, invalid key code:%s, on line %d\n",temp_str,line_no);
											token_num=4;	/* bump the token count up to drop out of the loop */
										  }	/* end if */
									  }	/* end else */
								  }	/* end else */
								break;

							case 3:		/* the string to re-map the key to */
								if((*temp_str)==';') {	/* ignore the rest of the line if the 'key' is one of the tokens we don't support */
									if(IS_KEY_MAPPED(re_map_key))	/* check for the key being already re-mapped */
										error=del_key(re_map_key);
									token_num=4;	/* bump the token count up to drop out of the loop */
								  }	/* end if */
								else {
									if((map_str=parse_str(temp_str,&kermit_code))!=NULL) {		/* parse the re-mapping string */
										error=add_key(re_map_key,map_str,0,0);	/* add a regular key string to the key mapping list */
										free((char *)map_str);
									  }	/* end if */
									else {
										if(kermit_code!=0xFFFF) {	/* check for special kermit verb returned */
											error=add_key(re_map_key,NULL,1,(byte)kermit_code);	/* add a kermit code to the list */
										  }	/* end if */
										else {
											if(IS_KEY_MAPPED(re_map_key))	/* check for the key being already re-mapped */
												error=del_key(re_map_key);
											printf("Error, re-mapping string:%s invalid on line %d\n",temp_str,line_no);
											token_num=4;	/* bump the token count up to drop out of the loop */
										  }	/* end else */
									  }	/* end else */
								  }	/* end else */
								break;
						  }	/* end switch */
						token_num++;
						if(token_num<3)		/* if the next token is not the last one, then grab it */
							temp_str=strtok(NULL,white_sp);		/* get the next token */
						else if(token_num==3) {		/* for the last 'token' (after the remapping key to the end of the line), just get the next character in the line */
							temp_str+=(strlen(temp_str)+1);		/* jump over the previous token */
							where=strspn(temp_str,white_sp);	/* look for the first non-white space in the line */
							temp_str+=where;	/* jump to the first position with a character */
							if(!isgraph(*temp_str)) {		/* not more characters in the line */
								if(IS_KEY_MAPPED(re_map_key))	/* check for the key being already re-mapped */
									error=del_key(re_map_key);
								token_num=4;	/* bump the token count to drop out of the loop */
							  }	/* end if */
						  }	/* end if */
					  }	while(temp_str!=NULL && token_num<4);
				  }	/* end if */
				else {
				  }	/* end else */
			  }	/* end if */
			line_no++;			/* increment current line */
		  }	/* end while */
		fclose(key_fp);
	  }	/* end if */
	else
		error=(-1);		/* indicate an error */
	return(error);
}	/* end read_keyboard_file() */

/**********************************************************************
*  Function	:	find_key
*  Purpose	:	search through the list of mapped keys and return a pointer
*				to the node whose key_code matches the parameter passed
*  Parameters	:
*			search_key - the keycode to search the list for
*  Returns	:	NULL for a match not found, or a pointer to the key_node
*				containing the matched keycode
*  Calls	:	none
*  Called by	:	vt100key()
**********************************************************************/
key_node *find_key(unsigned int search_key)
{
	key_node *temp_key;		/* temporary pointer to a key node used to search to the matching key code */

	temp_key=head_key;		/* start at the head of the linked list */
	while(temp_key!=NULL) {	/* search the entire list */
		if((*temp_key).key_code==search_key)	/* check for match */
			return(temp_key);	/* return the pointer to the matched node */
		temp_key=(*temp_key).next_node;
	  }	/* end while */
	return(NULL);
}	/* end find_key() */

/**********************************************************************
*  Function	:	initkbfile()
*  Purpose	:	initialize the default keyboard settings and read in
*				the default keycodes from telnet.key
*  Parameters	:	none
*  Returns	:	0 for no error, <0 for various errors
*  Calls	:	none
*  Called by	:	main()
**********************************************************************/
int initkbfile(void )
{
	char kb_name[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT];	/* temporary variable to hold the entire pathname of "telnet.key" */
	int error;			/* the error code returned from reading in the keyboard mapping file */

	memset(key_map_flags,0,1024);		/* initialize all the keyboard mapped flags to zero (not mapped) */
  memset(key_special_flags,0,1024); /* initialize all the keyboard special flags to zero (not special) */
	strcpy(kb_name,path_name);	/* get the directory where telbin.exe is */
  strcat(kb_name,"telnet.key"); /* append the proper name */
  error=read_keyboard_file(kb_name);
	return(error);
}	/* end initkbfile() */

