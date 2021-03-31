/*
*
*  tekstor.c by Aaron Contorer, NCSA
*
*  Character storage routines for use by Telnet VG 
*	(virtual graphics screen) routines.
*  Allocates storage incrementally as more data comes in.
*  Uses larger and larger increments to avoid inefficiency.
*  Full data abstraction is provided.
*
*  Data structure:  
*    A unique-type header node begins the data structure.  This points
*  to the head of a linked list of "handles".  Each handle contains:
*  > a pointer to a "pool" of storage memory created by malloc()
*  > an int stating the number of bytes in the pool
*  > a pointer to the next handle
*
*  IDEAS FOR IMPROVEMENT:
*  Store pool as part of handle, rather than pointed to by handle
*
*/

#define MINPOOL 0x0200	/* smallest allowable pool */
#define MAXPOOL 0x2000	/* largest allowable pool */
#define STORMASTER

#include <stdio.h>
#include <stdlib.h>
#ifdef MSC
#include <malloc.h>
#endif
#include "tekstor.h"
#include "externs.h"

#define TRUE 1
#define FALSE 0

/*
*	Create a new, empty store and return a pointer to it.
*	Returns NULL if not enough memory to create a new store.
*/
STOREP newstore(void )
{
	STOREP s;
	
	s=(STOREP) malloc(sizeof(STORE));
    if(s==NULL)
		return(NULL);
	else {
		s->lasth=s->thish=s->firsth=(HANDLEP) malloc(sizeof(HANDLE));
		if(s->firsth==NULL) {
			free(s);
			return(NULL);
		  }	/* end if */
		else {
			s->firsth->pool=malloc(MINPOOL);
			if(s->firsth->pool==NULL) {
				free(s->firsth);
				free(s);
				return(NULL);
			  }
			else {
				s->lastelnum=s->thiselnum=-1;
				s->firsth->poolsize=MINPOOL;
				s->firsth->next=NULL;
			  }
		  }
	  }
	return(s);
}

/*
	Frees all pools and other memory space associated with store s.
*/
void freestore(STOREP s)
{
	HANDLEP h,h2;
	h=s->firsth;
	while(h!=NULL) {
		h2=h;
		free(h->pool);
		h=h->next;
		free(h2);
	  }
	free(s);
}

/*
	Adds character d to the end of store s.
	Returns 0 if successful, -1 if unable to add character (no memory).
*/
int addstore(STOREP s,char d)
{
	unsigned int n; /* temp storage */
	int size;
	HANDLEP h;

	n=++(s->lastelnum);
	size=s->lasth->poolsize;
	if(n < (s->lasth->poolsize)) 
		s->lasth->pool[n]=d;
	else {			/* Pool full; allocate a new one. */
		if(size<MAXPOOL) 
			size <<= 1;
		h=(HANDLEP)malloc(sizeof(HANDLE));
		if(h==NULL) {
			(s->lastelnum)--;
			return(-1);
		  }
		else {
			h->pool=malloc(size);
			if(h->pool==NULL) {
				free(h);
				(s->lastelnum)--;
				return(-1);
			  }
			else {
				h->poolsize=size;
				h->next=NULL;
				s->lasth->next=h;
				s->lasth=h;
				s->lastelnum=0;
				h->pool[0]=d;
			  }
		  }
	  }			 /* end of new pool allocation */
	return(0);
}   /* end addstore() */

/*
	Reset stats so that a call to nextitem(s) will be retrieving the
	first item in store s.
*/
void topstore(STOREP s)
{
	s->thish=s->firsth;
	s->thiselnum=-1;
}

/*
	Increment the current location in store s.  Then return the
	character at that location.  Returns -1 if no more characters.
*/
int nextitem(STOREP s)
{
	HANDLEP h;

	if(s->thish==s->lasth&&s->thiselnum==s->lastelnum) 
		return(-1);
	else {
		h=s->thish;
		if((++(s->thiselnum)) < (int)(s->thish->poolsize)) 
			return((int)(s->thish->pool[s->thiselnum]));
		else {			/* move to next pool */
			h=h->next;
			s->thish=h;
			s->thiselnum=0;
			return((int)(h->pool[0]));
		  }
	  }
}   /* end nextitem() */

/*
	Removes ("pops") the last item from the specified store.
	Returns that item (in range 0-255), or returns -1 if there
	are no items in the store.
*/
int unstore(STOREP s)
{
	HANDLEP nextolast;

	if(s->lastelnum>-1)			  /* last pool not empty */
		return((int)(s->lasth->pool[(s->lastelnum)--]));
	else { 						/* last pool empty */
		if(s->lasth==s->firsth) 
			return(-1);
		else { 					/* move back one pool */
			nextolast=s->firsth;
			while(nextolast->next!=s->lasth)
				nextolast=nextolast->next;
			free(nextolast->next);
			s->lasth=nextolast;
			s->lastelnum=nextolast->poolsize-2;
			if(s->thish==nextolast->next) {
				s->thish=nextolast;
				s->thiselnum=s->lastelnum;
			  }
			nextolast->next=NULL;
			return((int)(nextolast->pool[s->lastelnum+1]));
		  }
	  }
}
