/*
*	tekstor.h
*	Declarations to allow use of tekstor.c
*/

#ifndef TEKSTOR_H
#define TEKSTOR_H

struct handle {
	char *pool;
	unsigned int poolsize;
	struct handle *next;
};

typedef struct handle HANDLE, *HANDLEP;

typedef struct {
	HANDLE	*firsth, /* beginning of list */
			*thish,	 /* used for scanning thru data */
			*lasth;	 /* last handle in store */
	int thiselnum,	 /* number of currently-viewing element */
		lastelnum;	 /* number of last element in store */
		/* element numbers are in [0..(poolsize-1)] */
} STORE, *STOREP;

#endif
