/*----------------------------------------------------------------------
 *
 *  strdebug.c
 *  String management utilities
 *
 *  Description
 *
 *	strdebug.c contains routines to protect the programmer
 *	from errors in calling string copying/moving routines.
 *	The programmer must use the memory calls defined
 *	in strdebug.h. When these calls are used, the
 *	allocation routines in this module check a data structure
 *	at the top of allocated memory blocks which tags them as
 *	legal memory blocks.
 *
 *  Compilation Options
 *
 *	MEM_LIST	Link all allocated memory blocks onto
 *			an internal list. The list can be
 *			displayed using Mem_Display().
 *
 *	MEM_WHERE	Save the file/line number of allocated
 *			blocks in the header.
 *			Requires that the compilier supports
 *			__FILE__ and __LINE__ preprocessor
 *			directives.
 *			Also requires that the __FILE__ string
 *			have a static or global scope.
 *
 *	MEM_HEADER	Place a header and footer section around each
 *			allocated block to detect overwrites on the beginning
 *			and the ending of the allocated block.
 *
 *	MEM_COMP_FREE	Complement the free'd memory.
 *
 *	ImProVise Associates
 *	1105 W. Main #1
 *	Urbana, Illinois
 *	61801
 *
 *	Copyright:
 *	Quincey Koziol & Laura Kalman			July 1989
 */

#define __STRDEBUG__
/*#define DEBUG_LIST */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "strdebug.h"

/* Constants */
/* --------- */
#define MEMTAG	0xa55a			/* Value for mh_tag */
#define HEADERTAG	0x5a		/* Value for the header and footer data */

/* Structures */
/* ---------- */
typedef struct memnod {			/* Memory block header info	*/
	unsigned int	mh_tag ;	/* Special ident tag		*/
	size_t		mh_size ;		/* Size of allocation block	*/
#if defined(MEM_LIST)
	struct memnod	*mh_next ;	/* Next memory block		*/
	struct memnod	*mh_prev ;	/* Previous memory block	*/
#endif
#if defined(MEM_WHERE)
	char		*mh_file ;		/* File allocation was from	*/
	unsigned int	mh_line ;	/* Line allocation was from	*/
#endif
} MEMHDR;

/* Alignment macros */
/* ---------------- */
#define PC
#if defined(PC)
#define ALIGN_SIZE sizeof(unsigned char)
#else
#define ALIGN_SIZE sizeof(double)
#endif

#define HDR_SIZE sizeof(MEMHDR)

#if defined(MEM_HEADER)
#define BLOCK_SIZE	5
#define HEADER_SIZE	(sizeof(unsigned char)*BLOCK_SIZE)
#define RESERVE_SIZE ((((HDR_SIZE+(ALIGN_SIZE-1))/ALIGN_SIZE)*ALIGN_SIZE)+HEADER_SIZE)
#else
#define BLOCK_SIZE	0
#define HEADER_SIZE	0
#define RESERVE_SIZE (((HDR_SIZE+(ALIGN_SIZE-1))/ALIGN_SIZE)*ALIGN_SIZE)
#endif

/* Conversion macros */
/* ----------------- */
#define CLIENT_2_HDR(a) ((MEMHDR *) (((char *)(a)) - RESERVE_SIZE))
#define HDR_2_CLIENT(a) ((void *) (((char *)(a)) + RESERVE_SIZE))

/* Local variables */
/* --------------- */

/* Local functions */
/* --------------- */
static	void	str_tag_err(void *, char *, int) ;	/* Tag error */
static	void	str_int_err(int , char *, int) ;	/* Tag error */
#if defined(MEM_LIST)
#define Str_Tag_Err(a) str_tag_err((a),fil,lin)
#define Int_Err(a) str_int_err((a),fil,lin)
#else
#define Str_Tag_Err(a) str_tag_err((a),__FILE__,__LINE__)
#define Int_Err(a) str_int_err((a),__FILE__,__LINE__)
#endif

/************************************************************************/
/**** Functions accessed only through macros ****************************/
/************************************************************************/

/**********************************************************************
*  Function	:	str_strcat
*  Purpose	:	perform a protected call to strcat()
*  Parameters	:
*		dst - pointer to the string to concatenate onto
*		src	 - pointer to the string to concantenate
*  Returns	:	the value of dst is returned
*  Calls	:	strcat()
*  Called by	:	
**********************************************************************/
char *str_strcat(char *dst,const char *src
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Str_Tag_Err(dst);

	if(src==NULL)
		Str_Tag_Err(src);
#else
	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Str_Tag_Err(dst);

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Str_Tag_Err(src);
#endif

	return(strcat(dst,src));
}	/* end str_strcat() */

/**********************************************************************
*  Function	:	str_strchr
*  Purpose	:	perform a protected call to strchr()
*  Parameters	:
*		s	 - the string to search
*		c	 - the character to search for in the string
*  Returns	:	a pointer to the located character, or NULL
*  Calls	:	strchr()
*  Called by	:	
**********************************************************************/
char *str_strchr(const char *s,int c
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s==NULL)
		Str_Tag_Err(s);
#else
	if(FP_SEG(s)<=_psp || FP_SEG(s)>=0xA000)
		Str_Tag_Err(s);
#endif

	if(c<0 || c>255)
		Int_Err(c);

	return(strchr(s,c));
}	/* end str_strchr() */

/**********************************************************************
*  Function	:	str_strcmp
*  Purpose	:	perform a protected call to strcmp()
*  Parameters	:
*		s1 - pointer to the first string to compare
*		s2 - pointer to the second string to compare
*  Returns	:	same as strcmp()
*  Calls	:	strcmp()
*  Called by	:	
**********************************************************************/
int str_strcmp(const char *s1,const char *s2
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Str_Tag_Err(s1);

	if(s2==NULL)
		Str_Tag_Err(s2);
#else
	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Str_Tag_Err(s1);

	if(FP_SEG(s2)<=_psp || FP_SEG(s2)>=0xA000)
		Str_Tag_Err(s2);
#endif

	return(strcmp(s1,s2));
}	/* end str_strcmp() */

#ifdef UN_USED
/**********************************************************************
*  Function	:	str_strcoll
*  Purpose	:	perform a protected call to strcoll()
*  Parameters	:
*		s1 - pointer to the first string to compare
*		s2 - pointer to the second string to compare
*  Returns	:	same as strcoll()
*  Calls	:	strcoll()
*  Called by	:	
**********************************************************************/
int str_strcoll(const char *s1,const char *s2
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Str_Tag_Err(s1);

	if(s2==NULL)
		Str_Tag_Err(s2);
#else
	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Str_Tag_Err(s1);

	if(FP_SEG(s2)<=_psp || FP_SEG(s2)>=0xA000)
		Str_Tag_Err(s2);
#endif

	return(strcoll(s1,s2));
}	/* end str_strcoll() */
#endif

/**********************************************************************
*  Function	:	str_strcpy
*  Purpose	:	perform a protected call to strcpy()
*  Parameters	:
*		dst - pointer to the destination string
*		src - pointer to the source string
*  Returns	:	the value of dst is returned
*  Calls	:	strcpy()
*  Called by	:	
**********************************************************************/
char *str_strcpy(char *dst,const char *src
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Str_Tag_Err(dst);

	if(src==NULL)
		Str_Tag_Err(src);
#else
	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Str_Tag_Err(dst);

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Str_Tag_Err(src);
#endif

	return(strcpy(dst,src));
}	/* end str_strcpy() */

#ifdef UN_USED
/**********************************************************************
*  Function	:	str_strcspn
*  Purpose	:	perform a protected call to strcspn()
*  Parameters	:
*		str - pointer to the string to search
*		charset - pointer to the string of characters to search for
*  Returns	:	the value of the initial segment is returned
*  Calls	:	strcspn()
*  Called by	:	
**********************************************************************/
size_t str_strcspn(const char *str,const char *charset
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(str==NULL)
		Str_Tag_Err(str);

	if(charset==NULL)
		Str_Tag_Err(charset);
#else
	if(FP_SEG(str)<=_psp || FP_SEG(str)>=0xA000)
		Str_Tag_Err(str);

	if(FP_SEG(charset)<=_psp || FP_SEG(charset)>=0xA000)
		Str_Tag_Err(charset);
#endif

	return(strcspn(str,charset));
}	/* end str_strcspn() */

/**********************************************************************
*  Function	:	str_stricmp
*  Purpose	:	perform a protected call to stricmp()
*  Parameters	:
*		s1 - pointer to the first string to compare
*		s2 - pointer to the second string to compare
*  Returns	:	same as stricmp()
*  Calls	:	stricmp()
*  Called by	:	
**********************************************************************/
int str_stricmp(const char *s1,const char *s2
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Str_Tag_Err(s1);

	if(s2==NULL)
		Str_Tag_Err(s2);
#else
	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Str_Tag_Err(s1);

	if(FP_SEG(s2)<=_psp || FP_SEG(s2)>=0xA000)
		Str_Tag_Err(s2);
#endif

	return(stricmp(s1,s2));
}	/* end str_stricmp() */
#endif

/**********************************************************************
*  Function	:	str_strlen
*  Purpose	:	perform a protected call to strlen()
*  Parameters	:
*		s	 - the string to determine the length of
*  Returns	:	the number of characters before the terminating null char
*  Calls	:	strlen()
*  Called by	:	
**********************************************************************/
size_t str_strlen(const char *s
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s==NULL)
		Str_Tag_Err(s);
#else
	if(FP_SEG(s)<=_psp || FP_SEG(s)>=0xA000)
		Str_Tag_Err(s);
#endif

	return(strlen(s));
}	/* end str_strlen() */

#ifdef UN_USED
/**********************************************************************
*  Function	:	str_strlwr
*  Purpose	:	perform a protected call to strlwr()
*  Parameters	:
*		str	 - the string to convert to lowercase
*  Returns	:	address of str is returned
*  Calls	:	strlwr()
*  Called by	:	
**********************************************************************/
char *str_strlwr(char *str
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(str==NULL)
		Str_Tag_Err(str);
#else
	if(FP_SEG(str)<=_psp || FP_SEG(str)>=0xA000)
		Str_Tag_Err(str);
#endif

	return(strlwr(str));
}	/* end str_strlwr() */

/**********************************************************************
*  Function	:	str_strncat
*  Purpose	:	perform a protected call to strncat()
*  Parameters	:
*		dst - pointer to the string to concatenate onto
*		src	- pointer to the string to concantenate
*		n	- the number of characters to concantenate
*  Returns	:	the value of dst is returned
*  Calls	:	strncat()
*  Called by	:	
**********************************************************************/
char *str_strncat(char *dst,const char *src,size_t n
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Str_Tag_Err(dst);

	if(src==NULL)
		Str_Tag_Err(src);
#else
	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Str_Tag_Err(dst);

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Str_Tag_Err(src);
#endif

	return(strncat(dst,src,n));
}	/* end str_strncat() */
#endif

/**********************************************************************
*  Function	:	str_strncmp
*  Purpose	:	perform a protected call to strncmp()
*  Parameters	:
*		s1	- pointer to the first string to compare
*		s2	- pointer to the second string to compare
*		n	- the number of characters to compare
*  Returns	:	same as strncmp()
*  Calls	:	strncmp()
*  Called by	:	
**********************************************************************/
int str_strncmp(const char *s1,const char *s2,size_t n
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Str_Tag_Err(s1);

	if(s2==NULL)
		Str_Tag_Err(s2);
#else
	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Str_Tag_Err(s1);

	if(FP_SEG(s2)<=_psp || FP_SEG(s2)>=0xA000)
		Str_Tag_Err(s2);
#endif

	return(strncmp(s1,s2,n));
}	/* end str_strncmp() */

/**********************************************************************
*  Function :   real_strncmp
*  Purpose  :   perform an un-protected call to strncmp()
*  Parameters	:
*		s1	- pointer to the first string to compare
*		s2	- pointer to the second string to compare
*		n	- the number of characters to compare
*  Returns	:	same as strncmp()
*  Calls	:	strncmp()
*  Called by	:	
**********************************************************************/
int real_strncmp(const char *s1,const char *s2,size_t n)
{
	return(strncmp(s1,s2,n));
}   /* end real_strncmp() */

/**********************************************************************
*  Function	:	str_strncpy
*  Purpose	:	perform a protected call to strncpy()
*  Parameters	:
*		dst - pointer to the destination string
*		src - pointer to the source string
*		n	- the number of characters to copy
*  Returns	:	the value of dst is returned
*  Calls	:	strncpy()
*  Called by	:	
**********************************************************************/
char *str_strncpy(char *dst,const char *src,size_t n
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Str_Tag_Err(dst);

	if(src==NULL)
		Str_Tag_Err(src);
#else
	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Str_Tag_Err(dst);

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Str_Tag_Err(src);
#endif

	return(strncpy(dst,src,n));
}	/* end str_strncpy() */

#ifdef UN_USED
/**********************************************************************
*  Function	:	str_strnicmp
*  Purpose	:	perform a protected call to strnicmp()
*  Parameters	:
*		s1	- pointer to the first string to compare
*		s2	- pointer to the second string to compare
*		len	- the length of the strings to compare
*  Returns	:	same as strnicmp()
*  Calls	:	strnicmp()
*  Called by	:	
**********************************************************************/
int str_strnicmp(const char *s1,const char *s2,size_t len
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Str_Tag_Err(s1);

	if(s2==NULL)
		Str_Tag_Err(s2);
#else
	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Str_Tag_Err(s1);

	if(FP_SEG(s2)<=_psp || FP_SEG(s2)>=0xA000)
		Str_Tag_Err(s2);
#endif

	return(strnicmp(s1,s2,len));
}	/* end str_strnicmp() */

/**********************************************************************
*  Function	:	str_strnset
*  Purpose	:	perform a protected call to strnset()
*  Parameters	:
*		s1	- pointer to the string to fill
*		fill- the character to fill the string with
*		len	- the length of the string to fill
*  Returns	:	the pointer to the original string, s1, is returned
*  Calls	:	strnset()
*  Called by	:	
**********************************************************************/
char *str_strnset(char *s1,int fill,const size_t len
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Str_Tag_Err(s1);
#else
	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Str_Tag_Err(s1);
#endif

	if(fill<0 || fill>255)
		Int_Err(fill);

	return(strnset(s1,fill,len));
}	/* end str_strnset() */

/**********************************************************************
*  Function	:	str_strpbrk
*  Purpose	:	perform a protected call to strpbrk()
*  Parameters	:
*		str - pointer to the string to search
*		charset - pointer to the string of characters to search for
*  Returns	:	a pointer to the located character
*  Calls	:	strpbrk()
*  Called by	:	
**********************************************************************/
char *str_strpbrk(const char *str,const char *charset
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(str==NULL)
		Str_Tag_Err(str);

	if(charset==NULL)
		Str_Tag_Err(charset);
#else
	if(FP_SEG(str)<=_psp || FP_SEG(str)>=0xA000)
		Str_Tag_Err(str);

	if(FP_SEG(charset)<=_psp || FP_SEG(charset)>=0xA000)
		Str_Tag_Err(charset);
#endif

	return(strpbrk(str,charset));
}	/* end str_strpbrk() */

/**********************************************************************
*  Function	:	str_strchr
*  Purpose	:	perform a protected call to strrchr()
*  Parameters	:
*		s	 - the string to search
*		c	 - the character to search for in the string
*  Returns	:	a pointer to the located character, or NULL
*  Calls	:	strrchr()
*  Called by	:	
**********************************************************************/
char *str_strrchr(const char *s,int c
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s==NULL)
		Str_Tag_Err(s);
#else
	if(FP_SEG(s)<=_psp || FP_SEG(s)>=0xA000)
		Str_Tag_Err(s);
#endif

	if(c<0 || c>255)
		Int_Err(c);

	return(strrchr(s,c));
}	/* end str_strrchr() */

/**********************************************************************
*  Function	:	str_strrev
*  Purpose	:	perform a protected call to strrev()
*  Parameters	:
*		str	 - the string to reverse
*  Returns	:	address of str is returned
*  Calls	:	strrev()
*  Called by	:	
**********************************************************************/
char *str_strrev(char *str
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(str==NULL)
		Str_Tag_Err(str);
#else
	if(FP_SEG(str)<=_psp || FP_SEG(str)>=0xA000)
		Str_Tag_Err(str);
#endif

	return(strrev(str));
}	/* end str_strrev() */

/**********************************************************************
*  Function	:	str_strset
*  Purpose	:	perform a protected call to strset()
*  Parameters	:
*		s1	- pointer to the string to fill
*		fill- the character to fill the string with
*  Returns	:	the pointer to the original string, s1, is returned
*  Calls	:	strset()
*  Called by	:	
**********************************************************************/
char *str_strset(char *s1,char fill
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Str_Tag_Err(s1);
#else
	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Str_Tag_Err(s1);
#endif

	return(strset(s1,fill));
}	/* end str_strset() */

/**********************************************************************
*  Function	:	str_strspn
*  Purpose	:	perform a protected call to strspn()
*  Parameters	:
*		str - pointer to the string to search
*		charset - pointer to the string of characters to search for
*  Returns	:	the value of the initial segment is returned
*  Calls	:	strspn()
*  Called by	:	
**********************************************************************/
size_t str_strspn(const char *str,const char *charset
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(str==NULL)
		Str_Tag_Err(str);

	if(charset==NULL)
		Str_Tag_Err(charset);
#else
	if(FP_SEG(str)<=_psp || FP_SEG(str)>=0xA000)
		Str_Tag_Err(str);

	if(FP_SEG(charset)<=_psp || FP_SEG(charset)>=0xA000)
		Str_Tag_Err(charset);
#endif

	return(strspn(str,charset));
}	/* end str_strspn() */

/**********************************************************************
*  Function	:	str_strstr
*  Purpose	:	perform a protected call to strstr()
*  Parameters	:
*		str	- pointer to the string to search
*		substr - pointer to the string to search for
*  Returns	:	a pointer to the located string, or NULL if not found
*  Calls	:	strstr()
*  Called by	:	
**********************************************************************/
char *str_strstr(const char *str,const char *substr
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(str==NULL)
		Str_Tag_Err(str);

	if(substr==NULL)
		Str_Tag_Err(substr);
#else
	if(FP_SEG(str)<=_psp || FP_SEG(str)>=0xA000)
		Str_Tag_Err(str);

	if(FP_SEG(substr)<=_psp || FP_SEG(substr)>=0xA000)
		Str_Tag_Err(substr);
#endif

	return(strstr(str,substr));
}	/* end str_strstr() */

/**********************************************************************
*  Function	:	str_strtok
*  Purpose	:	perform a protected call to strtok()
*  Parameters	:
*		s1 - pointer to the string to tokenize
*		s2 - pointer to the string of tokens
*  Returns	:	same as strtok()
*  Calls	:	strtok()
*  Called by	:	
**********************************************************************/
char *str_strtok(char *s1,const char *s2
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{

/* Check for s1==NULL is _NOT_ missing, sometimes it is supposed to be NULL. */
/*	Read the Compiler's library manual for full details. */

#ifdef OLD_WAY
	if(s2==NULL)
		Str_Tag_Err(s2);
#else
	if(FP_SEG(s2)<=_psp || FP_SEG(s2)>=0xA000)
		Str_Tag_Err(s2);
#endif

	return(strtok(s1,s2));
}	/* end str_strtok() */
#endif

/**********************************************************************
*  Function	:	str_strupr
*  Purpose	:	perform a protected call to strupr()
*  Parameters	:
*		str	 - the string to convert to uppercase
*  Returns	:	address of str is returned
*  Calls	:	strupr()
*  Called by	:	
**********************************************************************/
char *str_strupr(char *str
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(str==NULL)
		Str_Tag_Err(str);
#else
	if(FP_SEG(str)<=_psp || FP_SEG(str)>=0xA000)
		Str_Tag_Err(str);
#endif

	return(strupr(str));
}	/* end str_strupr() */

#ifdef UN_USED
/**********************************************************************
*  Function	:	str_strxfrm
*  Purpose	:	perform a protected call to strxfrm()
*  Parameters	:
*		dst	- pointer to the string to transform to
*		src	- pointer to the string to transform
*		n	- the number of characters to transform
*  Returns	:	the length of the transformed string
*  Calls	:	strxfrm()
*  Called by	:	
**********************************************************************/
size_t	str_strxfrm(char *dst,const char *src,size_t n
#if defined(MEM_WHERE)
,char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Str_Tag_Err(dst);

	if(src==NULL)
		Str_Tag_Err(src);
#else
	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Str_Tag_Err(dst);

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Str_Tag_Err(src);
#endif

	return(strxfrm(dst,src,n));
}	/* end str_strxfrm() */
#endif

/************************************************************************/
/**** Error display *****************************************************/
/************************************************************************/

/*
 *  str_tag_err()
 *  Display string error
 */
static void str_tag_err(void *p,char *fil,int lin)
{
#ifdef LATER
	FILE *fp;
#endif

    fprintf(stdaux,"String error - %p - %s(%d)\n",p,fil,lin);
#ifdef LATER
	if((fp=fopen("impro.err","wt+"))!=NULL) {	/* open impro.err to output the error file */
		fprintf(fp,"Memory tag error - %p - %s(%d)\n",p,fil,lin);
#if defined(MEM_LIST)
		Mem_Display(fp);
#endif
		fclose(fp);
	  }	/* end if */
#endif
#ifdef QAK
	exit(1);
#endif
}	/* end str_tag_err() */

/*
 *  int_err()
 *  Display integer error
 */
static void str_int_err(int c,char *fil,int lin)
{
#ifdef LATER
	FILE *fp;
#endif

    fprintf(stdaux,"Integer error - %d - %s(%d)\n",c,fil,lin);
#ifdef LATER
	if((fp=fopen("impro.err","wt+"))!=NULL) {	/* open impro.err to output the error file */
		fprintf(fp,"Integer error - %d - %s(%d)\n",c,fil,lin);
#if defined(MEM_LIST)
		Mem_Display(fp);
#endif
		fclose(fp);
	  }	/* end if */
#endif
#ifdef QAK
	exit(1);
#endif
}	/* end str_int_err() */
