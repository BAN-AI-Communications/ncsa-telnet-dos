/*----------------------------------------------------------------------
 *
 *  memdebug.h -- Dynamic memory handler interface
 *  Description: memdebug.h provides the interface definitions for the dynamic
 *  memory handler.
 *  See memdebug.c for complete documentation.
 *
 */

/* Compilation options */
#define MEM_LIST		/* Build internal list */
#define MEM_WHERE		/* Keep track of memory block source */
#define MEM_HEADER		/* Keep headers and footers around for each block */
#define MEM_COMP_FREE	/* Complement the space free'd */

/* Interface functions */
unsigned long	Mem_Used(void);
void		Mem_Display(FILE *fp);

/* Interface functions to access only through macros */
#if defined(MEM_WHERE)
void	*mem_alloc(size_t size, char *fil, int lin);
void	*mem_calloc(size_t n,size_t size, char *fil, int lin);
void	*mem_realloc(void *old_ptr, size_t size, char *fil, int lin);
void	mem_free(void *ptr, char *fil, int lin);
char	*mem_strdup(char *ptr, char *fil, int lin);
#else
void	*mem_alloc(size_t size) ;
void	*mem_calloc(size_t n,size_t size);
void	*mem_realloc(void *old_ptr, size_t size) ;
void	mem_free(void *ptr) ;
char	*mem_strdup(char *ptr) ;
#endif

/* Interface macros */
#if !defined(__MALDEBUG__)
#if defined(MEM_WHERE)
#define malloc(a)		mem_alloc((a),__FILE__,__LINE__)
#define calloc(a,b)		mem_calloc((a),(b),__FILE__,__LINE__)
#define realloc(a,b)	mem_realloc((a),(b),__FILE__,__LINE__)
#define free(a)			mem_free((a),__FILE__,__LINE__)
#define strdup(a)		mem_strdup((a),__FILE__,__LINE__)
#else
#define malloc(a)		mem_alloc(a)
#define calloc(a,b)		mem_calloc((a),(b))
#define realloc(a,b)	mem_realloc((a),(b))
#define free(a)			mem_free(a)
#define strdup(a)		mem_strdup(a)
#endif
#endif
