/*----------------------------------------------------------------------
 *
 *  memdebug.c
 *  Memory management utilities
 *
 *  Description
 *
 *	memdebug.c contains routines to protect the programmer
 *	from errors in calling memory copying/moving routines.
 *	The programmer must use the memory calls defined
 *	in memdebug.h. When these calls are used, the
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

#define __MEMDEBUG__
/*#define DEBUG_LIST */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "memdebug.h"
#include "externs.h"

/* External procedures */
/* ------------------- */
extern void cdecl movebytes(void *to,void *from,int len);

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
static	void	mem_tag_err(void *, char *, int) ;	/* Tag error */
static	void	mem_int_err(int ,char *, int);
static	void	mem_size_err(unsigned int ,char *, int);
#if defined(MEM_LIST)
#define Mem_Tag_Err(a)	mem_tag_err((void *)a,fil,lin)
#define Mem_Size_Err(a)	mem_size_err(a,fil,lin)
#define Int_Err(a)		mem_int_err(a,fil,lin)
#else
#define Mem_Tag_Err(a)	mem_tag_err((void *)a,__FILE__,__LINE__)
#define Mem_Size_Err(a)	mem_size_err(a,__FILE__,__LINE__)
#define Int_Err(a)		mem_int_err(a,__FILE__,__LINE__)
#endif

/************************************************************************/
/**** Functions accessed only through macros ****************************/
/************************************************************************/

#ifdef UN_USED
/**********************************************************************
*  Function	:	mem_memccpy
*  Purpose	:	perform a protected call to memccpy()
*  Parameters	:
*		dst	- the pointer to the destination block of memory
*		src	- the pointer to the source block of memory
*		c	- the character to copy up to
*  Returns	:	a pointer to the byte in dst following the character c if
*				one is found, NULL otherwise
*  Calls	:	memccpy()
*  Called by	:	
**********************************************************************/
void *mem_memccpy(void *dst,const void *src,int c,unsigned cnt
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *dst_hdr,
void *src_hdr
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Mem_Tag_Err(dst);

	if(src==NULL)
		Mem_Tag_Err(src);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Mem_Tag_Err(dst);

#if defined(MEM_ALLOC)
	if(dst_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(dst_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<cnt)
			Mem_Size_Err(cnt);
	  }	/* end if */
#endif

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Mem_Tag_Err(src);

#if defined(MEM_ALLOC)
	if(src_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(src_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<cnt)
			Mem_Size_Err(cnt);
	  }	/* end if */
#endif

#endif

	if(c<0 || c>255)
		Int_Err(c);

	return(memccpy(dst,src,c,cnt));
}	/* end mem_memccpy() */

/**********************************************************************
*  Function	:	mem_memchr
*  Purpose	:	perform a protected call to memchr()
*  Parameters	:
*		buf	- the pointer to the block of memory to search
*		ch	- the character to search for
*		length - the number of characters to search
*  Returns	:	a pointer to the located character, NULL is the character
*				is not found
*  Calls	:	memchr()
*  Called by	:	
**********************************************************************/
void *mem_memchr(const void *buf,int ch,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *buf_hdr
#endif
)
{
#ifdef OLD_WAY
	if(buf==NULL)
		Mem_Tag_Err(buf);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(buf)<=_psp || FP_SEG(buf)>=0xA000)
		Mem_Tag_Err(buf);

#if defined(MEM_ALLOC)
	if(buf_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(buf_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	if(ch<0 || ch>255)
		Int_Err(ch);

	return(memchr(buf,ch,length));
}	/* end mem_memchr() */

/**********************************************************************
*  Function	:	mem_memcmp
*  Purpose	:	perform a protected call to memcmp()
*  Parameters	:
*		s1	- the pointer to the first block of memory
*		s2	- the pointer to the second block of memory
*		length - the number of characters to compare
*  Returns	:	the same as memcmp()
*  Calls	:	memcmp()
*  Called by	:	
**********************************************************************/
int mem_memcmp(const void *s1,const void *s2,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *s1_hdr,
void *s2_hdr
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Mem_Tag_Err(s1);

	if(s2==NULL)
		Mem_Tag_Err(s2);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Mem_Tag_Err(s1);

#if defined(MEM_ALLOC)
	if(s1_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(s1_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

	if(FP_SEG(s2)<=_psp || FP_SEG(s2)>=0xA000)
		Mem_Tag_Err(s2);

#if defined(MEM_ALLOC)
	if(s2_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(s2_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	return(memcmp(s1,s2,length));
}	/* end mem_memcmp() */

/**********************************************************************
*  Function :   real_memcmp
*  Purpose  :   perform an un-protected call to memcmp()
*  Parameters	:
*		s1	- the pointer to the first block of memory
*		s2	- the pointer to the second block of memory
*		length - the number of characters to compare
*  Returns	:	the same as memcmp()
*  Calls	:	memcmp()
*  Called by	:	
**********************************************************************/
int real_memcmp(const void *s1,const void *s2,size_t length)
{
	return(memcmp(s1,s2,length));
}   /* end real_memcmp() */
#endif

/**********************************************************************
*  Function	:	mem_memcpy
*  Purpose	:	perform a protected call to memcpy()
*  Parameters	:
*		dst	- the pointer to the destination block of memory
*		src	- the pointer to the source block of memory
*		length	- the number of characters to copy
*  Returns	:	the value of dst is returned
*  Calls	:	memcpy()
*  Called by	:	
**********************************************************************/
void *mem_memcpy(void *dst,const void *src,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *dst_hdr,
void *src_hdr
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Mem_Tag_Err(dst);

	if(src==NULL)
		Mem_Tag_Err(src);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Mem_Tag_Err(dst);

#if defined(MEM_ALLOC)
	if(dst_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(dst_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Mem_Tag_Err(src);

#if defined(MEM_ALLOC)
	if(src_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(src_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	return(memcpy(dst,src,length));
}	/* end mem_memcpy() */

/**********************************************************************
*  Function :   mem_movebytes
*  Purpose  :   perform a protected call to movebytes()
*  Parameters	:
*		dst	- the pointer to the destination block of memory
*		src	- the pointer to the source block of memory
*		length	- the number of characters to copy
*  Returns	:	the value of dst is returned
*  Calls    :   movebytes()
*  Called by	:	
**********************************************************************/
void mem_movebytes(void *dst,const void *src,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *dst_hdr,
void *src_hdr
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Mem_Tag_Err(dst);

	if(src==NULL)
		Mem_Tag_Err(src);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Mem_Tag_Err(dst);

#if defined(MEM_ALLOC)
	if(dst_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(dst_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Mem_Tag_Err(src);

#if defined(MEM_ALLOC)
	if(src_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(src_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

    movebytes(dst,src,length);
}   /* end mem_movebytes() */

#ifdef UN_USED
/**********************************************************************
*  Function	:	mem_memicmp
*  Purpose	:	perform a protected call to memicmp()
*  Parameters	:
*		s1	- the pointer to the first block of memory
*		s2	- the pointer to the second block of memory
*		length - the number of characters to compare
*  Returns	:	the same as memicmp()
*  Calls	:	memicmp()
*  Called by	:	
**********************************************************************/
int mem_memicmp(const void *s1,const void *s2,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *s1_hdr,
void *s2_hdr
#endif
)
{
#ifdef OLD_WAY
	if(s1==NULL)
		Mem_Tag_Err(s1);

	if(s2==NULL)
		Mem_Tag_Err(s2);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(s1)<=_psp || FP_SEG(s1)>=0xA000)
		Mem_Tag_Err(s1);

#if defined(MEM_ALLOC)
	if(s1_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(s1_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

	if(FP_SEG(s2)<=_psp || FP_SEG(s2)>=0xA000)
		Mem_Tag_Err(s2);

#if defined(MEM_ALLOC)
	if(s2_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(s2_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	return(memicmp(s1,s2,length));
}	/* end mem_memicmp() */
#endif

/**********************************************************************
*  Function	:	mem_memmove
*  Purpose	:	perform a protected call to memmove()
*  Parameters	:
*		dst	- the pointer to the destination block of memory
*		src	- the pointer to the source block of memory
*		length	- the number of characters to move
*  Returns	:	the value of dst is returned
*  Calls	:	memmove()
*  Called by	:	
**********************************************************************/
void *mem_memmove(void *dst,const void *src,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *dst_hdr,
void *src_hdr
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Mem_Tag_Err(dst);

	if(src==NULL)
		Mem_Tag_Err(src);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Mem_Tag_Err(dst);

#if defined(MEM_ALLOC)
	if(dst_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(dst_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Mem_Tag_Err(src);

#if defined(MEM_ALLOC)
	if(src_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(src_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	return(memmove(dst,src,length));
}	/* end mem_memmove() */

/**********************************************************************
*  Function	:	mem_memset
*  Purpose	:	perform a protected call to memset()
*  Parameters	:
*		s	- the pointer to the block of memory to fill
*		c	- the character to fill with
*		length - the number of characters to fill
*  Returns	:	the value of s
*  Calls	:	memset()
*  Called by	:	
**********************************************************************/
void *mem_memset(void *s,int c,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *s_hdr
#endif
)
{
#ifdef OLD_WAY
	if(s==NULL)
		Mem_Tag_Err(s);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(s)<=_psp || FP_SEG(s)>=0xA000)
		Mem_Tag_Err(s);

#if defined(MEM_ALLOC)
	if(s_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(s_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	if(c<0 || c>255)
		Int_Err(c);

	return(memset(s,c,length));
}	/* end mem_memset() */

/**********************************************************************
*  Function	:	mem_movedata
*  Purpose	:	perform a protected call to movedata()
*  Parameters	:
*		src_segment	- the value of the source segment of memory
*		src_offset	- the value of the source offset of memory
*		dst_segment	- the value of the destination segment of memory
*		dst_offset	- the value of the destination offset of memory
*		length	- the number of characters to move
*  Returns	:	none
*  Calls	:	movedata()
*  Called by	:	
**********************************************************************/
void mem_movedata(unsigned int src_segment,unsigned int src_offset,unsigned int dst_segment,unsigned int dst_offset,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
)
{
#ifdef OLD_WAY
	if(MK_FP(src_segment,src_offset)==NULL)
		Mem_Tag_Err(MK_FP(src_segment,src_offset));

	if(MK_FP(dst_segment,dst_offset)==NULL)
		Mem_Tag_Err(MK_FP(dst_segment,dst_offset));
#else
	if(src_segment<=_psp || src_segment>=0xA000)
		Mem_Tag_Err(MK_FP(src_segment,src_offset));

	if(dst_segment<=_psp || dst_segment>=0xA000)
		Mem_Tag_Err(MK_FP(dst_segment,dst_offset));
#endif

	movedata(src_segment,src_offset,dst_segment,dst_offset,length);
}	/* end mem_movedata() */

#ifdef UN_USED
/**********************************************************************
*  Function	:	mem_swab
*  Purpose	:	perform a protected call to swab()
*  Parameters	:
*		dst	- the pointer to the destination block of memory
*		src	- the pointer to the source block of memory
*		length	- the number of characters to copy & swap
*  Returns	:	none
*  Calls	:	swab()
*  Called by	:	
**********************************************************************/
void mem_swab(void *src,const void *dst,int num
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *src_hdr,
void *dst_hdr
#endif
)
{
#ifdef OLD_WAY
	if(src==NULL)
		Mem_Tag_Err(src);

	if(dst==NULL)
		Mem_Tag_Err(dst);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Mem_Tag_Err(src);

#if defined(MEM_ALLOC)
	if(src_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(src_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<num)
			Mem_Size_Err(num);
	  }	/* end if */
#endif

	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Mem_Tag_Err(dst);

#if defined(MEM_ALLOC)
	if(dst_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(dst_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<num)
			Mem_Size_Err(num);
	  }	/* end if */
#endif

#endif

	swab(src,dst,num);
}	/* end mem_swab() */
#endif

#ifdef QAK
/**********************************************************************
*  Function	:	mem_memqcpy
*  Purpose	:	perform a protected call to memqcpy()
*  Parameters	:
*		dst	- the pointer to the destination block of memory
*		src	- the pointer to the source block of memory
*		length	- the number of characters to copy
*  Returns	:	none currently (the value of dst is returned)
*  Calls	:	memqcpy()
*  Called by	:	
**********************************************************************/
void mem_memqcpy(void *dst,const void *src,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *dst_hdr,
void *src_hdr
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Mem_Tag_Err(dst);

	if(src==NULL)
		Mem_Tag_Err(src);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Mem_Tag_Err(dst);

#if defined(MEM_ALLOC)
	if(dst_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(dst_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Mem_Tag_Err(src);

#if defined(MEM_ALLOC)
	if(src_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(src_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	memqcpy(dst,src,length);
}	/* end mem_memqcpy() */

/**********************************************************************
*  Function	:	mem_mem2cpy
*  Purpose	:	perform a protected call to mem2cpy()
*  Parameters	:
*		dst	- the pointer to the destination block of memory
*		src	- the pointer to the source block of memory
*		length	- the number of characters to copy
*  Returns	:	none currently (the value of dst is returned)
*  Calls	:	mem2cpy()
*  Called by	:	
**********************************************************************/
void mem_mem2cpy(void *dst,const void *src,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *dst_hdr,
void *src_hdr
#endif
)
{
#ifdef OLD_WAY
	if(dst==NULL)
		Mem_Tag_Err(dst);

	if(src==NULL)
		Mem_Tag_Err(src);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(dst)<=_psp || FP_SEG(dst)>=0xA000)
		Mem_Tag_Err(dst);

#if defined(MEM_ALLOC)
	if(dst_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(dst_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

	if(FP_SEG(src)<=_psp || FP_SEG(src)>=0xA000)
		Mem_Tag_Err(src);

#if defined(MEM_ALLOC)
	if(src_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(src_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	mem2cpy(dst,src,length);
}	/* end mem_mem2cpy() */

/**********************************************************************
*  Function	:	mem_memqset
*  Purpose	:	perform a protected call to memqset()
*  Parameters	:
*		s	- the pointer to the block of memory to fill
*		c	- the character to fill with
*		length - the number of characters to fill
*  Returns	:	none currently (the value of s)
*  Calls	:	memqset()
*  Called by	:	
**********************************************************************/
void mem_memqset(void *s,int c,size_t length
#if defined(MEM_WHERE)
, char *fil,
int lin
#endif
#if defined(MEM_ALLOC)
, void *s_hdr
#endif
)
{
#ifdef OLD_WAY
	if(s==NULL)
		Mem_Tag_Err(s);
#else
#if defined(MEM_ALLOC)
	MEMHDR *hdr_block;		/* pointer to a memory header for debugging */
#endif

	if(FP_SEG(s)<=_psp || FP_SEG(s)>=0xA000)
		Mem_Tag_Err(s);

#if defined(MEM_ALLOC)
	if(s_hdr!=STATIC_PTR) {	/* check whether we should look at the header for this memory block */
		hdr_block=CLIENT_2_HDR(s_hdr);	/* get a pointer to the memory header for this memory block */
		if(hdr_block->mh_size<length)
			Mem_Size_Err(length);
	  }	/* end if */
#endif

#endif

	if(c<0 || c>255)
		Int_Err(c);

	memqset(s,c,length);
}	/* end mem_memqset() */
#endif

/************************************************************************/
/**** Error display *****************************************************/
/************************************************************************/

/*
 *  mem_tag_err()
 *  Display memory error
 */
static void mem_tag_err(void *p,char *fil,int lin)
{
#ifdef LATER
	FILE *fp;
#endif

    fprintf(stdaux,"Memory error - %p - %s(%d)\n",p,fil,lin);
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
}	/* end mem_tag_err() */

/*
 *  mem_int_err()
 *  Display integer error
 */
static void mem_int_err(int c,char *fil,int lin)
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
}	/* end mem_int_err() */

#if defined(MEM_ALLOC)
/*
 *  mem_size_err()
 *  Display size error
 */
static void mem_size_err(unsigned int c,char *fil,int lin)
{
#ifdef LATER
	FILE *fp;
#endif

    fprintf(stdaux,"Size error - %u - %s(%d)\n",c,fil,lin);
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
}	/* end mem_size_err() */
#endif
