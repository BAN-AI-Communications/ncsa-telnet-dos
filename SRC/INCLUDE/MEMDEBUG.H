/*----------------------------------------------------------------------
 *
 *  memdebug.h -- Memory movement handler interface
 *  Description: memdebug.h provides the interface definitions for 
 *  memory functions to interface dynamic memory handler.
 *  See memdebug.c for complete documentation.
 *
 */

/* Compilation options */
#define MEM_LIST		/* Build internal list */
#define MEM_WHERE		/* Keep track of memory block source */
#define MEM_HEADER		/* Keep headers and footers around for each block */
#define MEM_COMP_FREE	/* Complement the space free'd */

#if !defined(STATIC_PTR)
#define STATIC_PTR	(~NULL)
#endif

/* Interface functions */

/* Interface functions to access only through macros */
#if defined(MEM_WHERE)
void	*mem_memccpy(void *dst,const void *src,int c,unsigned cnt, char *fil, int lin);
void	*mem_memchr(const void *buf,int ch,size_t length, char *fil, int lin);
int     mem_memcmp(const void *s1,const void *s2,size_t length, char *fil, int lin);
int     real_memcmp(const void *s1,const void *s2,size_t length);
void	*mem_memcpy(void *dst,const void *src,size_t length, char *fil, int lin);
int		mem_memicmp(const void *s1,const void *s2,size_t length, char *fil, int lin);
void	*mem_memmove(void *dst,const void *src, size_t length, char *fil, int lin);
void	*mem_memset(void *s,int c,size_t length, char *fil, int lin);
void	mem_movedata(unsigned int src_segment,unsigned int src_offset,unsigned int dst_segment,unsigned int dst_offset,size_t length, char *fil, int lin);
void	mem_swab(char *src,char *dst,int num, char *fil, int lin);

void	mem_memqcpy(void *dst,const void *src,size_t length, char *fil, int lin);
void	mem_mem2cpy(void *dst,const void *src,size_t length, char *fil, int lin);
void	mem_memqset(void *s,int c,size_t length, char *fil, int lin);
#else
void	*mem_memccpy(void *dst,const void *src,int c,unsigned cnt);
void	*mem_memchr(const void *buf,int ch,size_t length);
int     mem_memcmp(const void *s1,const void *s2,size_t length);
int     real_memcmp(const void *s1,const void *s2,size_t length);
void    *mem_memcpy(void *dst,const void *src,size_t length);
void    mem_movebytes(void *dst,const void *src,size_t length);
int		mem_memicmp(const void *s1,const void *s2,size_t length);
void	*mem_memmove(void *dst,const void *src,size_t length);
void	*mem_memset(void *s,int c,size_t length);
void	mem_movedata(unsigned int src_segment,unsigned int src_offset,unsigned int dst_segment,unsigned int dst_offset,size_t length);
void	mem_swab(char *src,char *dst,int num);

void	mem_memqcpy(void *dst,const void *src,size_t length);
void	mem_mem2cpy(void *dst,const void *src,size_t length);
void	mem_memqset(void *s,int c,size_t length);
#endif

/* Interface macros */
#if !defined(__MEMDEBUG__)
#if defined(MEM_WHERE)
#define memccpy(a,b,c,d)	mem_memccpy((a),(b),(c),(d),__FILE__,__LINE__)
#define memchr(a,b,c)	mem_memchr((a),(b),(c),__FILE__,__LINE__)
#define memcmp(a,b,c)	mem_memcmp((a),(b),(c),__FILE__,__LINE__)
#define memcpy(a,b,c)   mem_memcpy((a),(b),(c),__FILE__,__LINE__)
#define movebytes(a,b,c)   mem_movebytes((a),(b),(c),__FILE__,__LINE__)
#define memicmp(a,b,c)  mem_memicmp((a),(b),(c),__FILE__,__LINE__)
#define	memmove(a,b,c)	mem_memmove((a),(b),(c),__FILE__,__LINE__)
#define	memset(a,b,c)	mem_memset((a),(b),(c),__FILE__,__LINE__)
#define movedata(a,b,c,d,e)	mem_movedata((a),(b),(c),(d),(e),__FILE__,__LINE__)
#define swab(a,b,c)		mem_swab((a),(b),(c),__FILE__,__LINE__)

#define memqcpy(a,b,c)	mem_memqcpy((a),(b),(c),__FILE__,__LINE__)
#define mem2cpy(a,b,c)	mem_mem2cpy((a),(b),(c),__FILE__,__LINE__)
#define	memqset(a,b,c)	mem_memqset((a),(b),(c),__FILE__,__LINE__)
#else
#define memccpy(a,b,c,d)	mem_memccpy((a),(b),(c),(d))
#define memchr(a,b,c)	mem_memchr((a),(b),(c))
#define memcmp(a,b,c)	mem_memcmp((a),(b),(c))
#define memcpy(a,b,c)	mem_memcpy((a),(b),(c))
#define memicmp(a,b,c)	mem_memicmp((a),(b),(c))
#define	memmove(a,b,c)	mem_memmove((a),(b),(c))
#define	memset(a,b,c)	mem_memset((a),(b),(c))
#define movedata(a,b,c,d,e)	mem_movedata((a),(b),(c),(d),(e))
#define swab(a,b,c)		mem_swab((a),(b),(c))

#define memqcpy(a,b,c)	mem_memqcpy((a),(b),(c))
#define mem2cpy(a,b,c)	mem_mem2cpy((a),(b),(c))
#define	memqset(a,b,c)	mem_memqset((a),(b),(c))
#endif
#endif
