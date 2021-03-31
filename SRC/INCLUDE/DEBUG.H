#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
# define BUG(__s) printf("%s[%d]: %s\n",__FILE__,__LINE__,__s);
#else
# define BUG(__s)
#endif

#endif /*DEBUG_H*/
