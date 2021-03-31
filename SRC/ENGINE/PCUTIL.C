/*  PCUTIL.C
*   Utilities for the network library that are PC specific
****************************************************************************
*                                                                          *
*      part of:                                                            *
*      TCP/UDP/ICMP/IP Network kernel for NCSA Telnet                      *
*      by Tim Krauskopf                                                    *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
****************************************************************************
*/

/*
*	Includes
*/
#ifdef __TURBOC__
#include "turboc.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef MSC
#ifdef __TURBOC__
#include <alloc.h>
#include <dir.h>
#else
#include <malloc.h>
#include <direct.h>
#include <dos.h>
#endif
#endif
#include "whatami.h"
#include "externs.h"

static char lookcolor(char *);

/**********************************************************************/
/*
*   Find directory name -- return a code that indicates whether the
*   directory exists or not.
*   0 = dir name ok
*   -1 = error
*   > 0 = dos error code, no dir by this name
*
*   Accept certain unix conventions, like '/' for separator
*
*   Also, append a '\' to the name before returning
* 
*  Note:  There must be enough room in the string to append the '\'
*/
struct dosdta {
	char junk[21];
	char att;
	int time,date;
	long int size;
	char name[13];
};

extern struct dosdta *dtaptr;	/* declared in ncsaio.asm */

#ifndef NET14
int direxist(char dirname[])
{
	int i,ret;
	char *p;

	if(!strcmp(dirname,".") || !dirname[0]) {
		dirname[0]='\0';
		return(0);
	  }
	if(!strcmp(dirname,"\\"))
		return(0);
	p=dirname;
	while(*p) {
		switch(*p) {
			case '*':
			case '?':
				return(-1);

			case '/':
				*p='\\';
				break;
		  }
		p++;
	  }
/*
*  n_findfirst  will return normal files AND directories
*  must check attribute to see if it is really a directory
*/
	ret=n_findfirst(dirname,0x10);		/* find name */

	if(ret)
		return(ret);
	if(!(dtaptr->att&0x10))
		return(-2);							/* is a normal file */
	i=strlen(dirname);
	dirname[i]='\\';						/* extend with '\' */
	dirname[++i]='\0';
	return(0);
}

/**********************************************************************/
/* firstname
*  find the first name in the given directory which matches the wildcard
*  specification.  Skip . and ..
*
*  expand '*' (unix) to '*.*' (dos)
*/
char savepath[_MAX_DIR+_MAX_FNAME+_MAX_EXT+30]; /* allocate enough room for the pathname, filename, & extension, plus a little bit of slush */
int rootlen;

char *firstname(char path[],int type)
{
	int i,len;
	char *p,*q;

	if(!*path)
		return(NULL);

	len=strlen(path);
	i=0;
	rootlen=0;
	q=savepath;
	p=path;
	while(*q=*p) {				/* basic string copy with extras */
		if(*p=='\\')
			rootlen=i+1;			/* rootlen = position of last \ */
		p++;
		q++;
		i++;
	  }
	if(savepath[len-1]=='*' && rootlen==len-1) {
		savepath[len++]='.';
		savepath[len++]='*';
		savepath[len++]='\0';
	  }

      /* List files and directories  rmg 930408 */
      if(n_findfirst(savepath,0x10))
        return(NULL);

/*
*  copy file name, translate to lower case 
*/
	q=&savepath[rootlen];
	p=dtaptr->name;
	while(*p) {
		if(*p>='A' && *p<='Z')
			*q++= (*p++) + (char) 32;
		else
			*q++=*p++;
	  }
/*
*  if it is a directory then put <DIR> after it
*/

    if(type) {
      p=&savepath[rootlen+20];
      for(; q!=p; *q++=' ');
      if(dtaptr->att&0x10) {
        *q++=' ';
        *q++='<';
        *q++='D';
        *q++='I';
        *q++='R';
        *q++='>';
      }
      else {
        sprintf(q,"%8ld",dtaptr->size);
        return(savepath);
      } /* end else */
    }
	*q='\0';
	return(savepath);
}

/**********************************************************************/
/* nextname
*  modify the path spec to contain the next file name in the
*  sequence as given by DOS
*
*  if at the end of the sequence, return NULL
*/
char *nextname(int type)
{
	char *p,*q;

	if(n_findnext())	/* check if there are any more filenames */
		return(NULL);

/*
*  copy file name, translate to lower case 
*/
	q=&savepath[rootlen];
	p=dtaptr->name;
	while(*p) {
		if(*p >='A' && *p<='Z')
			*q++ = (*p++) + (char)32;
		else
			*q++=*p++;
	  }
/*
* if it is a directory, then put <DIR> after it
*/
	if(type) {
		p=&savepath[rootlen+20];
		for(; q!=p; *q++=' ');		
		if(dtaptr->att&0x10) {
			*q++=' ';
			*q++='<';
			*q++='D';
			*q++='I';
			*q++='R';
			*q++='>';
		  }
		else {
			sprintf(q,"%8ld",dtaptr->size);
			return(savepath);
		  }	/* end else */
   	  }		
	*q='\0';
	return(savepath);
}
/**********************************************************************/
/*  getdrive
*   get the current disk drive
*/
void getdrive(unsigned int *d)
{
#ifdef __TURBOC__
	*d=(unsigned) getdisk();
#else
	_dos_getdrive(d);
#endif
};

void setdrive(unsigned int d)
{
	unsigned temp;
#ifdef __TURBOC__
	setdisk((int) d);
#else
	_dos_setdrive(d, &temp);
#endif
};

/**********************************************************************/
/*  dopwd
*   get the current directory, including disk drive letter
*/
void dopwd(char *p,int l)
{
	getcwd(p,l);				/* get dir */
}

/**********************************************************************
*	Function	:	chgdir
*	Purpose	:	change to a different drive and directory
*	Parameters	:
*		file_name - handle of the name of the directory to change to 
*	Returns	:	0 for success, 1 for error
*	Calls	:	various string routines
*	Called by	:	ftp server
**********************************************************************/
int chgdir(char *file_name)
{
    unsigned old_drive,     /* the old drive we were in */
        temp_val;           /* temporary value */
    int ret_val=0,          /* the return value from the function */
		new_drive;			/* the drive to change to */
	char *name_ptr,			/* pointer to the file name data */
              /* chgdir string length changed from 64 to _MAX_DIR  rmg 931029 */
    cwd[_MAX_DIR],      /* handle of the current directory */
		*current,			/* pointer to the place in the file name */
    working[_MAX_DIR];    /* handle of a space for doing things */

	name_ptr=file_name;	/* set the pointer to the file name data */
#ifdef MSC
#ifdef __TURBOC__
	old_drive=getdisk();		/* get the old drive number */
#else
	_dos_getdrive(&old_drive);	/* get the old drive number */
#endif
#else
	old_drive=getdsk();
#endif
  getcwd(cwd,_MAX_DIR);   /* get the current directory */
	memmove(cwd,cwd+2,strlen(cwd)+1);	/* get rid of the drive specifier */
                            /* Pipe symbol for WWW access  rmg 931029 */
  if((*(name_ptr+1)==':') || (*(name_ptr+1)=='|')) {    /* do we have a drive specified */
		new_drive=toupper((int)*name_ptr)-'A';	/* get the new drive number */
#ifdef MSC
#ifdef __TURBOC__
		setdisk(new_drive);
#else
		_dos_setdrive(new_drive+1,&temp_val);
#endif
#else
		chgdsk(new_drive);
#endif
		name_ptr+=2;				/* increment the name pointer */
		if(*name_ptr=='\0')			/* check for simple drive change */
			name_ptr=NULL;
	  }	/* end if */
	while(*name_ptr=='\\') {		/* check for changing to the root directory */
		chdir("\\");				/* change to the root directory */
		name_ptr++;					/* advance the name pointer to the next character */
		if(*name_ptr=='\0')			/* check for simple drive change */
			name_ptr=NULL;
	  }	/* end if */
	while(name_ptr!=NULL && ret_val!=1) {		/* continue until the end of the string or an error occurs */
#ifdef MSC
		current=strchr(name_ptr,(int)'\\');		/* find the first occurence of the SLASH character */
#else
		current=strchr(name_ptr,(char)'\\');	/* find the first occurence of the SLASH character */
#endif
		if(current!=NULL) {							/* found the SLASH character */
			temp_val=current-name_ptr;					/* find out the length of the string */
			movebytes(working,name_ptr,temp_val);		/* copy the string into the working buffer */
			*(working+temp_val)=0;						/* terminate the string */
			name_ptr=current+1;						/* advance to the next part of the path name */
		  }	/* end if */
		else {										/* the SLASH character is not in the name */
			strcpy(working,name_ptr);
			name_ptr=NULL;
		  }	/* end else */
		if(chdir(working))		/* change the directory, but look for an error also */
			ret_val=1;
	  }	/* end while */
	if(ret_val==1) {					/* on error, reset the old drive */
#ifdef MSC
#ifdef __TURBOC__
		setdisk(old_drive);
#else
		_dos_setdrive(old_drive,&temp_val);
#endif
#else
		chgdsk(old_drive);
#endif
		chdir(cwd);					/* fix the directory */
	  }	/* end if */
	return(ret_val);				/* return the retuen value */
}	/* end chgdir() */
#endif

/**********************************************************************/
/*  Scolorset
*  setup the color value from the config file string
*/
void Scolorset(char *thecolor,char *st)
{
	*thecolor=lookcolor(st);
}

/**********************************************************************/
/* lookcolor
*  search a list for the given color name
*/
static char *colist[]={
	"black",
	"blue",
	"green",	
	"cyan",
	"red",
	"magenta",
	"yellow",
	"white",
	"BLACK",
	"BLUE",
	"GREEN",	
	"CYAN",
	"RED",
	"MAGENTA",
	"YELLOW",
	"WHITE"	};

static char lookcolor(char *s)
{
    char i;

	for(i=0; i<15; i++)
		if(!strcmp(colist[i],s))
			return(i);
	return(15);
}

#ifndef NET14
/**********************************************************************
*  Function	:	octal_to_int
*  Purpose	:	convert an octal string to an integer (like atoi())
*  Parameters	:
*			octal_str - the octal string to get the value of
*  Returns	:	0 to indicate that the input cannot be converted to an integer
*  Calls	:	none
*  Called by	:	parse_str()
**********************************************************************/
int octal_to_int(char *octal_str)
{
	unsigned int return_value=0;	/* the value to return from the function */

	while((*octal_str)>='0' && (*octal_str)<='7') {
		return_value*=(return_value*8);		/* bump up the value top return to the next multiple of eight */
		return_value+=((*octal_str)-'0');		/* increment by the digit found */
		octal_str++;
	  }	/* end while */
	return((int)return_value);
}	/* end octal_to_int() */

/**********************************************************************
*  Function	:	hex_to_int
*  Purpose	:	convert an hexadecimal string to an integer (like atoi())
*  Parameters	:
*			hex_str - the hexadecimal string to get the value of
*  Returns	:	0 to indicate that the input cannot be converted to an integer
*  Calls	:	none
*  Called by	:	parse_str()
**********************************************************************/
int hex_to_int(char *hex_str)
{
	unsigned int return_value=0;	/* the value to return from the function */

	while(isxdigit((int)(*hex_str))) {
		return_value*=(return_value*16);		/* bump up the value top return to the next multiple of sixteen */
		if(isdigit((int)(*hex_str)))	/* check whether this digit is numeric or alphabetic */
			return_value+=(unsigned int)((*hex_str)-'0');		/* increment by the digit found (for '0'-'9') */
		else
			return_value+=(unsigned int)(tolower((int)(*hex_str))-'a'+10);	/* increment by the digit found (for 'A'-'F' & 'a'-'f') */
		hex_str++;
	  }	/* end while */
	return((int)return_value);
}	/* end hex_to_int() */
#endif

