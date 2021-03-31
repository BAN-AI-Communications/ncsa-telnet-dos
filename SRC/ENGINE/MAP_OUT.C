/*
*	map_out.c
*
*   Output mapping functions for the real screen code
*
*   Quincey Koziol
*
*	Date		Notes
*	--------------------------------------------
*	11/90		Started
*/

/*
* Includes
*/

#define OUTPUTMASTER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <ctype.h>
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#else
#include <malloc.h>
#endif
#endif
#ifdef __TURBOC__
#include "turboc.h"
#endif
#include "vskeys.h"
#include "externs.h"
#include "map_out.h"

#define MAX_LINE_LENGTH	160		/* the maximum length of a line in the output mapping file */

/* Local functions */
static unsigned int parse_str(char *file_str);
static void add_output(unsigned int search_key,unsigned int key_map_code);
static void del_output(unsigned int search_key);

/* Local variables */
static char white_sp[]="\x009\x00a\x00b\x00c\x00d\x020";	/* string which contains all the white space characters */
#ifdef OLD_WAY
static char end_token[]="\x000\x009\x00a\x00b\x00c\x00d\x020};";	/* string which contains all the white space characters, and the right curley brace & the semi-colon */
#endif

/**********************************************************************
*  Function :	del_output
*  Purpose	:	delete a node from the list of mapped keys,
*				basicly just replaces the key code in the array with the
*				array index.
*  Parameters	:
*			search_key - the keycode to delete from the list
*  Returns	:	none
*  Calls	:	none
*  Called by	:	read_keyboard_file()
**********************************************************************/
static void del_output(unsigned int search_key)
{
	if(search_key<256)
		outputtable[search_key]=(unsigned char)search_key;
}	/* end del_output() */

/**********************************************************************
*  Function :	add_output
*  Purpose	:	add a node to the list of mapped output characters
*  Parameters	:
*			search_key - the character code to add to the list
*			key_map_code - the character code to map the key to
*  Returns	:	none
*  Calls	:	none
*  Called by	:	read_keyboard_file()
**********************************************************************/
static void add_output(unsigned int search_key,unsigned int key_map_code)
{
	if(search_key<256)
		outputtable[search_key]=(unsigned char)key_map_code;
}	/* end add_output() */

/**********************************************************************
*  Function	:	parse_str()
*  Purpose	:	parse the string to map a key to.
*  Parameters	:
*			file_str - pointer to a string from the file to parse
*  Returns	:	0xFFFF for error, otherwise, the vt100 code for telbin
*  Calls	:	none
*  Called by	:	read_keyboard_file()
**********************************************************************/
static unsigned int parse_str(char *file_str)
{
	unsigned int ret_code;	/* the code to return from this function */
	int done=0; 			/* flag for dropping out of the loop */
	byte *temp_str; 		/* pointer to the current place in the string */

	ret_code=0xFFFF;	   /* mark return code to indicate nothing special to return */
	temp_str=file_str;		/* start at the beginning of the string to parse */
	while((*temp_str)!='\0' && !done) {     /* parse until the end of the string or until the buffer is full */
		if((*temp_str) && (*temp_str)!=';' && (*temp_str)!='\\' && (*temp_str)!='{' && (*temp_str)>' ') {    /* copy regular characters until a special one is hit */
			ret_code=(*temp_str);	/* copy the character */
			done=1; 				/* stop parsing the string */
		  }	/* end while */
		if(*temp_str) {		/* check on the various special cases for dropping out of the loop */
			switch(*temp_str) {		/* switch for the special case */
				case '\\':	/* backslash for escape coding a character */
					temp_str++;		/* get the character after the backslash */
					if((*temp_str)=='{') {	/* check for curly brace around numbers */
						temp_str++;	/* increment to the next character */
						if((*temp_str)=='o') /* check for octal number */
							ret_code=octal_to_int(temp_str);   /* get the acii number after the escape code */
						else if((*temp_str)=='x') /* check for hexadecimal number */
							ret_code=hex_to_int(temp_str); /* get the acii number after the escape code */
						else {		/* must be an escape integer */
							if((*temp_str)=='d')	/* check for redundant decimal number specification */
								temp_str++;
							ret_code=atoi(temp_str);   /* get the ascii number after the escape code */
						  }	/* end else */
					  }	/* end if */
					else if((*temp_str)=='o') /* check for octal number */
						ret_code=octal_to_int(temp_str);   /* get the acii number after the escape code */
					else if((*temp_str)=='x') /* check for hexadecimal number */
						ret_code=hex_to_int(temp_str); /* get the acii number after the escape code */
					else {		/* must be an escape integer */
						if((*temp_str)=='d')	/* check for redundant decimal number specification */
							temp_str++;
						ret_code=atoi(temp_str);   /* get the ascii number after the escape flag */
					  }	/* end else */
					done=1;
                    break;

				case '{':	/* curly brace opens a quoted string */
					temp_str++;		/* jump over the brace itself */
					if((*temp_str) && (*temp_str)!='}')    /* copy regular characters until a special one is hit */
						ret_code=(*temp_str);		/* copy the character */
					done=1; 		/* drop out of the loop */
					break;

				default:	/* ctrl characters, space, and semi-colon terminate a string */
					done=1;
					break;

			  }	/* end switch */
		  }	/* end if */
		else
			done=1;
	  }	/* end while */
	return(ret_code);
}	/* end parse_str() */

/**********************************************************************
*  Function	:	read_output_file
*  Purpose	:	read in a output mapping file, parse the input and 
*				map the output characters
*  Parameters	:
*			output_file - string containing the name of the output mapping file
*  Returns	:	0 for no error, -1 for any errors which occur
*  Calls	:	parse_str(), & lots of library string functions
*  Called by	:	initoutputfile(), Sconfile()
**********************************************************************/
int read_output_file(char *output_file)
{
	FILE *output_fp;		/* pointer to the keyboard file */
	char output_line[MAX_LINE_LENGTH],	/* static array to store lines read from keyboard file */
		*temp_str;			/* temporary pointer to a string */
	uint line_no=0,			/* what line in the file we are on */
		map_code,			/* the output character to re-map to */
		token_num,			/* the current token we are parsing */
		where,				/* pointer to the beginning of text */
		re_map_key,			/* the key to re-map */
		error=0;			/* error from the file reading */

	if((output_fp=fopen(output_file,"rt"))!=NULL) {
		while((temp_str=fgets(output_line,MAX_LINE_LENGTH,output_fp))!=NULL && !error) {	/* get a line of input */
			token_num=0;			/* initialize the token we are on */
			if((temp_str=strtok(output_line,white_sp))!=NULL) {	/* get the first token from the string */
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
									outputtable[re_map_key]=(unsigned char)re_map_key;	   /* reset the output mapping definition */
									token_num=4;	/* bump the token count up to drop out of the loop */
								  }	/* end if */
								else {
									if((map_code=parse_str(temp_str))!=0xFFFF) {	  /* parse the re-mapping string */
										add_output(re_map_key,map_code);  /* add a regular key string to the key mapping list */
									  }	/* end if */
									else {
										del_output(re_map_key);
										printf("Error, re-mapping string:%s invalid on line %d\n",temp_str,line_no);
										token_num=4;	/* bump the token count up to drop out of the loop */
									  } /* end else */
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
								del_output(re_map_key);
								token_num=4;	/* bump the token count to drop out of the loop */
							  }	/* end if */
						  }	/* end if */
					  }	while(temp_str!=NULL && token_num<4);
				  }	/* end if */
			  }	/* end if */
			line_no++;			/* increment current line */
		  }	/* end while */
		fclose(output_fp);
	  }	/* end if */
	else
		error=(-1);		/* indicate an error */
	return(error);
}   /* end read_output_file() */

/**********************************************************************
*  Function	:	initoutputfile()
*  Purpose	:	initialize the default output mappings
*  Parameters	:	none
*  Returns	:	none
*  Calls	:	none
*  Called by	:	main()
**********************************************************************/
void initoutputfile(void )
{
	int i;				/* local counting variable */

	for(i=0; i<256; i++)	/* just set the mappings in the table to their one code */
		outputtable[i]=(unsigned char)i;
}	/* end initoutputfile() */

