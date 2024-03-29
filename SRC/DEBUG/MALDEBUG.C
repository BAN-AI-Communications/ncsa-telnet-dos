/*----------------------------------------------------------------------
 *
 *  maldebug.c
 *  Memory management utilities
 *
 *  Description
 *
 *	maldebug.c contains routines to protect the programmer
 *	from errors in calling memory allocation/free routines.
 *	The programmer must use the memory calls defined
 *	in maldebug.h. When these calls are used, the
 *	allocation routines in this module add a data structure
 *	to the top of allocated memory blocks which tags them as
 *	legal memory blocks.
 *
 *	When the free routine is called, the memory block to
 *	be freed is checked for legality tag.  If the block
 *	is not legal, the memory list is dumped to stderr and
 *	the program is terminated.
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
 */

#define __MALDEBUG__
/*#define DEBUG_LIST */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "maldebug.h"

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
static unsigned long	mem_size = 0 ;	/* Amount of memory used */
#if defined(MEM_LIST)
static MEMHDR	*memlist = NULL ;	/* List of memory blocks */
#endif

/* Local functions */
/* --------------- */
static	void	mem_tag_err(void *, char *, int) ;	/* Tag error */
#if defined(MEM_LIST)
static	void	mem_list_add(MEMHDR *) ;		/* Add block to list */
static	void	mem_list_delete(MEMHDR *) ;		/* Delete block from list */
#define Mem_Tag_Err(a) mem_tag_err(a,fil,lin)
#else
#define Mem_Tag_Err(a) mem_tag_err(a,__FILE__,__LINE__)
#endif

/************************************************************************/
/**** Functions accessed only through macros ****************************/
/************************************************************************/

/*----------------------------------------------------------------------
 *
 *  mem_alloc()
 *  Allocate a memory block
 *
 *  Usage:
 *
 *	void *mem_alloc(size_t	size)
 *
 *  Parameters:
 *
 *	size		Size of block in bytes to allocate
 *
 *  Return Value:
 *
 *	Pointer to allocated memory block
 *	NULL if not enough memory
 *
 *  Description:
 *
 *	mem_alloc() makes a protected call to malloc()
 *
 *  Notes
 *
 *	Access this routine using the malloc() macro in memdebug.h
 *
 *
 */

void *mem_alloc(
#if defined(MEM_WHERE)
size_t	size,
char	*fil,
int		lin
#else
size_t	size
#endif
)

{
	MEMHDR	*p;

/* Allocate memory block */
/* --------------------- */
	p=malloc(RESERVE_SIZE + size + HEADER_SIZE);
	if(p==NULL) {
        fprintf(stdaux,"NULL pointer malloc'ed in %s, line %d\n",fil,lin);
		return(NULL);
	  }	/* end if */

/* Init header */
/* ----------- */
	p->mh_tag=MEMTAG;
	p->mh_size=size;
	mem_size+=size;
#if defined(MEM_WHERE)
	p->mh_file=fil;
	p->mh_line=lin;
#endif

#if defined(MEM_HEADER)
	memset((char *)HDR_2_CLIENT(p)-HEADER_SIZE,HEADERTAG,HEADER_SIZE);
	memset((char *)HDR_2_CLIENT(p)+size,HEADERTAG,HEADER_SIZE);
#endif

#if defined(MEM_LIST)
	mem_list_add(p);
#endif

/* Return pointer to client data */
/* ----------------------------- */
	return(HDR_2_CLIENT(p));
}	/* end mem_alloc() */

#ifdef UN_USED
/*----------------------------------------------------------------------
 *
 *  mem_calloc()
 *  Allocate & clear a memory block
 *
 *  Usage:
 *
 *	void *mem_calloc(size_t n,size_t	size)
 *
 *  Parameters:
 *
 *	n			number of blocks to allocate
 *	size		Size of block in bytes to allocate
 *
 *  Return Value:
 *
 *	Pointer to allocated memory block
 *	NULL if not enough memory
 *
 *  Description:
 *
 *	mem_calloc() makes a protected call to calloc()
 *
 *  Notes
 *
 *	Access this routine using the calloc() macro in memdebug.h
 *
 *
 */

void *mem_calloc(
#if defined(MEM_WHERE)
size_t	n,
size_t	size,
char	*fil,
int		lin
#else
size_t	n,
size_t	size
#endif
)

{
	MEMHDR	*p;

/* Allocate memory block */
/* --------------------- */
	p=malloc(RESERVE_SIZE + (n*size) + HEADER_SIZE);
	if(p==NULL)
		return(NULL);

/* Init block to zero */
/* ------------------ */
	memset((char *)HDR_2_CLIENT(p),0,(size*n));

/* Init header */
/* ----------- */
	p->mh_tag=MEMTAG;
	p->mh_size=(size*n);
	mem_size+=(size*n);
#if defined(MEM_WHERE)
	p->mh_file=fil;
	p->mh_line=lin;
#endif

#if defined(MEM_HEADER)
	memset((char *)HDR_2_CLIENT(p)-HEADER_SIZE,HEADERTAG,HEADER_SIZE);
	memset((char *)HDR_2_CLIENT(p)+(size*n),HEADERTAG,HEADER_SIZE);
#endif

#if defined(MEM_LIST)
	mem_list_add(p);
#endif

/* Return pointer to client data */
/* ----------------------------- */
	return(HDR_2_CLIENT(p));
}	/* end mem_calloc() */
#endif

/*----------------------------------------------------------------------
 *
 *  mem_realloc()
 *  Reallocate a memory block
 *
 *  Usage:
 *
 *	void *mem_realloc(void	*ptr,size_t	size)
 *
 *  Parameters:
 *
 *	ptr		Pointer to current block
 *	size	Size to adjust block to
 *
 *  Return Value:
 *
 *	Pointer to new memory block
 *	NULL if memory cannot be reallocated
 *
 *  Description:
 *
 *	mem_realloc() makes a protected call to realloc().
 *
 *  Notes:
 *
 *	Access this routine using the realloc() macro in memdebug.h
 *
 *
 */

void *mem_realloc(
#if defined(MEM_WHERE)
void		*ptr,
size_t		size,
char		*fil,
int		lin
#else
void		*ptr,
size_t		size
#endif
)

{
	MEMHDR	*p;
#if defined(MEM_HEADER) || defined(MEM_COMP_FREE)
	unsigned char *q;
    int i;
#endif

/* Convert client pointer to header pointer */
/* ---------------------------------------- */
	p=CLIENT_2_HDR(ptr);

/* Check for valid block */
/* --------------------- */
	if(p->mh_tag!=MEMTAG) {
		Mem_Tag_Err(p);
		return(NULL);
	  }	/* end if */

/* Check for overwrites into the header & footer */
/* --------------------------------------------- */
#if defined(MEM_HEADER)
	q=(unsigned char *)ptr-HEADER_SIZE;		/* Check the Header to consistancy */
	for(i=0; i<BLOCK_SIZE; i++) {
		if(q[i]!=HEADERTAG) {
			Mem_Tag_Err(p);
			return(NULL);
		  }	/* end if */
	  }	/* end for */
	q=(unsigned char *)ptr+p->mh_size;		/* Check the Footer for consistancy */
	for(i=0; i<BLOCK_SIZE; i++) {
		if(q[i]!=HEADERTAG) {
			Mem_Tag_Err(p);
			return(NULL);
		  }	/* end if */
	  }	/* end for */
#endif

/* Invalidate header */
/* ----------------- */
	p->mh_tag=~MEMTAG;
	mem_size-=p->mh_size;

/* Invalidate the block of memory to be free'd */
/* ------------------------------------------- */
#if defined(MEM_COMP_FREE)
	q=(unsigned char *)ptr;
	for(i=0; i<p->mh_size; i++)
		q[i]=~q[i];
#endif

#if defined(MEM_WHERE)
	mem_list_delete(p);	/* Remove block from list */
#endif

/* Reallocate memory block */
/* ----------------------- */
	p=(MEMHDR *)realloc(p,RESERVE_SIZE+size+HEADER_SIZE);
	if(p==NULL)
		return(NULL);

/* Update header */
/* ------------- */
	p->mh_tag=MEMTAG;
	p->mh_size=size;
	mem_size+=size;
#if defined(MEM_LIST)
	p->mh_file=fil;
	p->mh_line=lin;
#endif

#if defined(MEM_WHERE)
	mem_list_add(p);	/* Add block to list */
#endif

#if defined(MEM_HEADER)
	memset((char *)HDR_2_CLIENT(p)-HEADER_SIZE,HEADERTAG,HEADER_SIZE);
	memset((char *)HDR_2_CLIENT(p)+size,HEADERTAG,HEADER_SIZE);
#endif

/* Return pointer to client data */
/* ----------------------------- */
	return(HDR_2_CLIENT(p));
}	/* end mem_realloc() */

/*----------------------------------------------------------------------
 *
 *  mem_strdup()
 *  Save a string in dynamic memory
 *
 *  Usage:
 *
 *	char *mem_strdup(char *str)
 *
 *  Parameters:
 *
 *	str		String to save
 *
 *  Return Value:
 *
 *	Pointer to allocated string
 *	NULL if not enough memory
 *
 *  Description:
 *
 *	mem_strdup() saves the specified string in dynamic memory.
 *
 *  Notes:
 *
 *	Access this routine using the strdup() macro in memdebug.h
 *
 *
 */

char *mem_strdup(
#if defined(MEM_WHERE)
char		*str,
char		*fil,
int		lin
#else
char		*str
#endif
)

{
	char *s;

#if defined(MEM_WHERE)
	s=mem_alloc(strlen(str)+1,fil,lin);
#else
	s=mem_alloc(strlen(str)+1);
#endif

	if(s!=NULL)
		strcpy(s,str);

	return(s);
}	/* end mem_strdup() */

/*----------------------------------------------------------------------
 *
 *  mem_free()
 *  Free a memory block
 *
 *  Usage:
 *
 *	void mem_free(void	*ptr)
 *
 *  Parameters:
 *
 *	ptr		Pointer to memory to free
 *
 *  Return Value:
 *
 *	None
 *
 *  Description:
 *
 *	mem_free() frees the specified memory block. The
 *	block must be allocated using mem_alloc(), mem_realloc()
 *	or mem_strdup().
 *
 *  Notes
 *
 *	Access this routine using the free() macro in memdebug.h
 *
 *
 */

void mem_free(
#if defined(MEM_WHERE)
void		*ptr,
char		*fil,
int		lin
#else
void		*ptr
#endif
)

{
	MEMHDR *p;
#if defined(MEM_HEADER) || defined(MEM_COMP_FREE)
	unsigned char *q;
    int i;
#endif

/* Convert client pointer to header pointer */
/* ---------------------------------------- */
	p=CLIENT_2_HDR(ptr);

/* Check for valid block */
/* --------------------- */
	if(p->mh_tag!=MEMTAG) {
		Mem_Tag_Err(p);
		return;
	  }	/* end if */

/* Check for overwrites into the header & footer */
/* --------------------------------------------- */
#if defined(MEM_HEADER)
	q=(unsigned char *)ptr-HEADER_SIZE;		/* Check the Header to consistancy */
	for(i=0; i<BLOCK_SIZE; i++) {
		if(q[i]!=HEADERTAG) {
			Mem_Tag_Err(p);
			return;
		  }	/* end if */
	  }	/* end for */
	q=(unsigned char *)ptr+p->mh_size;		/* Check the Footer for consistancy */
	for(i=0; i<BLOCK_SIZE; i++) {
		if(q[i]!=HEADERTAG) {
			Mem_Tag_Err(p);
			return;
		  }	/* end if */
	  }	/* end for */
#endif

/* Invalidate header */
/* ----------------- */
	p->mh_tag=~MEMTAG;
	mem_size-=p->mh_size;

/* Invalidate the block of memory to be free'd */
/* ------------------------------------------- */
#if defined(MEM_COMP_FREE)
	q=(unsigned char *)ptr;
	for(i=0; i<p->mh_size; i++)
		q[i]=~q[i];
#endif

#if defined(MEM_LIST)
	mem_list_delete(p);	/* Remove block from list */
#endif

/* Free memory block */
/* ----------------- */
	free(p);
}	/* end mem_free() */

/************************************************************************/
/**** Functions accessed directly ***************************************/
/************************************************************************/

/*----------------------------------------------------------------------
 *
 *  Mem_Used()
 *  Return amount of memory currently allocated
 *
 *  Usage:
 *
 *	unsigned long Mem_Used()
 *
 *  Parameters:
 *
 *	None.
 *
 *  Description:
 *
 *	Mem_Used() returns the number of bytes currently allocated
 *	using the memory management system. The value returned is
 *	simply the sum of the size requests to allocation routines.
 *	It does not reflect any overhead required by the memory
 *	management system.
 *
 *  Notes:
 *
 *	None
 *
 *
 */

unsigned long Mem_Used(void)
{
	return(mem_size);
}	/* end Mem_Used() */

/*----------------------------------------------------------------------
 *
 *  Mem_Display()
 *  Display memory allocation list
 *
 *  Usage:
 *
 *	void Mem_Display(FILE *fp)
 *
 *  Parameters:
 *
 *	fp		File to output data to
 *
 *  Description:
 *
 *	Mem_Display() displays the contents of the memory
 *	allocation list.
 *
 *	This function is a no-op if MEM_LIST is not defined.
 *
 *  Notes:
 *
 *	None
 *
 *
 */

void Mem_Display(FILE *fp)
{
#if defined(MEM_LIST)
	MEMHDR	*p;
	int	idx;
#if defined(MEM_HEADER)
	unsigned char *q;
    int i;
#endif

#if defined(MEM_WHERE)
	fprintf(fp, "Index   Size  File(Line) - total size %lu\n",mem_size);
#else
	fprintf(fp, "Index   Size - total size %lu\n",mem_size);
#endif

	idx=0;
	p=memlist;
	while(p!=NULL) {
		fprintf(fp,"%-5d %6u",idx++,p->mh_size);
#if defined(MEM_WHERE)
		fprintf(fp, "  %s(%d)",p->mh_file,p->mh_line);
#endif
		if (p->mh_tag!=MEMTAG)
			fprintf(fp," INVALID TAG");

/* Check for overwrites into the header & footer */
/* --------------------------------------------- */
#if defined(MEM_HEADER)
        q=(unsigned char *)HDR_2_CLIENT(p)-HEADER_SIZE;     /* Check the Header to consistancy */
        for(i=0; i<BLOCK_SIZE; i++) {
            if(q[i]!=HEADERTAG) {
                fprintf(fp," HEADER OVERWRITTEN");
                break;
              } /* end if */
          } /* end for */
        q=(unsigned char *)HDR_2_CLIENT(p)+p->mh_size;      /* Check the Footer for consistancy */
        for(i=0; i<BLOCK_SIZE; i++) {
            if(q[i]!=HEADERTAG) {
                fprintf(fp," FOOTER OVERWRITTEN");
                break;
              } /* end if */
          } /* end for */
#endif
		fprintf(fp,"\n");
		p=p->mh_next;
	  }	/* end while */
#else
	fprintf(fp, "Memory list not compiled (MEM_LIST not defined)\n") ;
#endif
}	/* end Mem_Display() */

/************************************************************************/
/**** Memory list manipulation functions ********************************/
/************************************************************************/

/*
 * mem_list_add()
 * Add block to list
 */

#if defined(MEM_LIST)
static void mem_list_add(MEMHDR	*p)
{
	p->mh_next=memlist;
	p->mh_prev=NULL;
	if(memlist!=NULL)
		memlist->mh_prev=p;
	memlist=p;

#if defined(DEBUG_LIST)
	printf("mem_list_add()\n");
	Mem_Display(stdout);
#endif
}	/* end mem_list_add() */
#endif

/*----------------------------------------------------------------------*/

/*
 * mem_list_delete()
 * Delete block from list
 */

#if defined(MEM_LIST)
static void mem_list_delete(MEMHDR	*p)
{
	if(p->mh_next!=NULL)
		p->mh_next->mh_prev=p->mh_prev;
	if(p->mh_prev!=NULL)
		p->mh_prev->mh_next=p->mh_next;
    else
		memlist=p->mh_next;

#if defined(DEBUG_LIST)
	printf("mem_list_delete()\n");
	Mem_Display(stdout);
#endif
}	/* end mem_list_delete() */
#endif

/************************************************************************/
/**** Error display *****************************************************/
/************************************************************************/

/*
 *  mem_tag_err()
 *  Display memory tag error
 */
static void mem_tag_err(void *p,char *fil,int lin)
{
	FILE *fp;

    fprintf(stdaux,"Malloc tag error - %p - %s(%d)\n",p,fil,lin);
#ifndef OLD_WAY
	if((fp=fopen("impro.err","wt+"))!=NULL) {	/* open impro.err to output the error file */
		fprintf(fp,"Malloc tag error - %p - %s(%d)\n",p,fil,lin);
#if defined(MEM_LIST)
		Mem_Display(fp);
#endif
		fclose(fp);
	  }	/* end if */
#else
        fprintf(stdaux,"Malloc tag error - %p - %s(%d)\n",p,fil,lin);
#if defined(MEM_LIST)
		Mem_Display(stderr);
#endif
#endif
#ifdef QAK
	exit(1);
#endif
}	/* end mem_tag_err() */
