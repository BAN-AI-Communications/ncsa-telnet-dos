/*----------------------------------------------------------------------
 *
 *  strdebug.h -- Dynamic memory handler interface
 *  Description: strdebug.h provides the string interface definitions 
 *	for the dynamic memory handler.
 *  See strdebug.c for complete documentation.
 *
 */

/* Compilation options */
#define MEM_LIST		/* Build internal list */
#define MEM_WHERE		/* Keep track of memory block source */
#define MEM_HEADER		/* Keep headers and footers around for each block */
#define MEM_COMP_FREE	/* Complement the space free'd */

#if !defined(STATIC_PTR)
#define	STATIC_PTR		(~NULL)
#endif

/* Interface functions */

/* Interface functions to access only through macros */
#if defined(MEM_WHERE)
char	*str_strcat(char *dst,const char *src, char *fil, int lin);
char	*str_strchr(const char *s,int c, char *fil, int lin);
int		str_strcmp(const char *s1,const char *s2, char *fil, int lin);
int		str_strcoll(const char *s1,const char *s2, char *fil, int lin);
char	*str_strcpy(char *dst,const char *src, char *fil, int lin);
size_t	str_strcspn(const char *str,const char *charset, char *fil, int lin);
int		str_stricmp(const char *s1,const char *s2, char *fil, int lin);
size_t	str_strlen(const char *s, char *fil, int lin);
char	*str_strlwr(char *str, char *fil, int lin);
char	*str_strncat(char *dst,const char *src,size_t n, char *fil, int lin);
int     str_strncmp(const char *s1,const char *s2,size_t n, char *fil, int lin);
int     real_strncmp(const char *s1,const char *s2,size_t n);
char	*str_strncpy(char *dst,const char *src,size_t n, char *fil, int lin);
int		str_strnicmp(const char *s1,const char *s2,size_t len, char *fil, int lin);
char	*str_strnset(char *s1,int fill,const size_t len, char *fil, int lin);
char	*str_strpbrk(const char *str,const char *charset, char *fil, int lin);
char	*str_strrchr(const char *str,int c, char *fil, int lin);
char	*str_strrev(char *str, char *fil, int lin);
char	*str_strset(char *str,char fill, char *fil, int lin);
size_t	str_strspn(const char *str,const char *charset, char *fil, int lin);
char	*str_strstr(const char *str,const char *substr, char *fil, int lin);
char	*str_strtok(char *s1,const char *s2, char *fil, int lin);
char	*str_strupr(char *str, char *fil, int lin);
size_t	str_strxfrm(char *dst,const char *src,size_t n, char *fil, int lin);
#else
char	*str_strcat(char *dst,const char *src);
char	*str_strchr(const char *s,int c);
int		str_strcmp(const char *s1,const char *s2);
int		str_strcoll(const char *s1,const char *s2);
char	*str_strcpy(char *dst,const char *src);
size_t	str_strcspn(const char *str,const char *charset);
int		str_stricmp(const char *s1,const char *s2);
size_t	str_strlen(const char *s);
char	*str_strlwr(char *str);
char	*str_strncat(char *dst,const char *src,size_t n);
int		str_strncmp(const char *s1,const char *s2,size_t n);
int     real_strncmp(const char *s1,const char *s2,size_t n);
char    *str_strncpy(char *dst,const char *src,size_t n);
int		str_strnicmp(const char *s1,const char *s2,size_t len);
char	*str_strnset(char *s1,int fill,const size_t len);
char	*str_strpbrk(const char *str,const char *charset);
char	*str_strrchr(const char *str,int c);
char	*str_strrev(char *str);
char	*str_strset(char *str,char fill);
size_t	str_strspn(const char *str,const char *charset);
char	*str_strstr(const char *str,const char *substr);
char	*str_strtok(char *s1,const char *s2);
char	*str_strupr(char *str);
size_t	str_strxfrm(char *dst,const char *src,size_t n);
#endif

/* Interface macros */
#if !defined(__STRDEBUG__)
#if defined(MEM_WHERE)
#define	strcat(a,b)		str_strcat((a),(b),__FILE__,__LINE__)
#define	strchr(a,b)		str_strchr((a),(b),__FILE__,__LINE__)
#define	strcmp(a,b)		str_strcmp((a),(b),__FILE__,__LINE__)
#define	strcmpi(a,b)	str_stricmp((a),(b),__FILE__,__LINE__)
#define	strcoll(a,b)	str_strcoll((a),(b),__FILE__,__LINE__)
#define strcpy(a,b)		str_strcpy((a),(b),__FILE__,__LINE__)
#define	strcspn(a,b)	str_strcspn((a),(b),__FILE__,__LINE__)
#define	stricmp(a,b)	str_stricmp((a),(b),__FILE__,__LINE__)
#define	strlen(a)		str_strlen((a),__FILE__,__LINE__)
#define	strlwr(a)		str_strlwr((a),__FILE__,__LINE__)
#define	strncat(a,b,c)	str_strncat((a),(b),(c),__FILE__,__LINE__)
#define	strncmp(a,b,c)	str_strncmp((a),(b),(c),__FILE__,__LINE__)
#define strncpy(a,b,c)  str_strncpy((a),(b),(c),__FILE__,__LINE__)
#define	strnicmp(a,b,c)	str_strnicmp((a),(b),(c),__FILE__,__LINE__)
#define	strnset(a,b,c)	str_strnset((a),(b),(c),__FILE__,__LINE__)
#define	strpbrk(a,b)	str_strpbrk((a),(b),__FILE__,__LINE__)
#define	strrchr(a,b)	str_strrchr((a),(b),__FILE__,__LINE__)
#define	strrev(a)		str_strrev((a),__FILE__,__LINE__)
#define	strset(a,b)		str_strset((a),(b),__FILE__,__LINE__)
#define	strspn(a,b)		str_strspn((a),(b),__FILE__,__LINE__)
#define	strstr(a,b)		str_strstr((a),(b),__FILE__,__LINE__)
#define	strtok(a,b)		str_strtok((a),(b),__FILE__,__LINE__)
#define	strupr(a)		str_strupr((a),__FILE__,__LINE__)
#define	strxfrm(a,b,c)	str_strxfrm((a),(b),(c),__FILE__,__LINE__)
#else
#define	strcat(a,b)		str_strcat((a),(b))
#define	strchr(a,b)		str_strchr((a),(b))
#define	strcmp(a,b)		str_strcmp((a),(b))
#define	strcmpi(a,b)	str_stricmp((a),(b))
#define	strcoll(a,b)	str_strcoll((a),(b))
#define strcpy(a,b)		str_strcpy((a),(b))
#define	strcspn(a,b)	str_strcspn((a),(b))
#define	stricmp(a,b)	str_stricmp((a),(b))
#define	strlen(a)		str_strlen((a))
#define	strlwr(a)		str_strlwr((a))
#define	strncat(a,b,c)	str_strncat((a),(b),(c))
#define	strncmp(a,b,c)	str_strncmp((a),(b),(c))
#define	strncpy(a,b,c)	str_strncpy((a),(b),(c))
#define	strnicmp(a,b,c)	str_strnicmp((a),(b),(c))
#define	strnset(a,b,c)	str_strnset((a),(b),(c))
#define	strpbrk(a,b)	str_strpbrk((a),(b),(c))
#define	strrchr(a,b)	str_strrchr((a),(b))
#define	strrev(a)		str_strrev((a))
#define	strset(a,b)		str_strset((a),(b))
#define	strspn(a,b)		str_strspn((a),(b))
#define	strstr(a,b)		str_strstr((a),(b))
#define	strtok(a,b)		str_strtok((a),(b))
#define	strupr(a)		str_strupr((a))
#define	strxfrm(a,b,c)	str_strxfrm((a),(b),(c))
#endif
#endif
