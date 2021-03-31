/*
	Header information for Decnet driver

	Quincey Koziol, Sept. 6, 1990
*/

#ifndef DECNET_H
#define DECNET_H

/* Declarations for DEC's Datalink Driver (from VAXmate Technical Manual) */

struct dcb {  /* DEC Datalink Communication Block */
   int portal_id;                  /*  0 Portal id for this request */
   unsigned char source_addr[6];   /*  2 Source ethernet address */
   unsigned char dest_addr[6];     /*  8 Destination ethernet address */
   unsigned char *bh;              /* 14 Pointer to buffer header */
   int bl;                         /* 18 Buffer length */
   int operation;                  /* 20 Used by each function differently */
   unsigned char pad;              /* 22 Pad flag for open */
   unsigned char mode;             /* 23 Mode flag for open */
   void (*line_state)();           /* 24 Line state change routine */
   void (*rcv_callback)();         /* 28 Received data routine */
   void (*xmit_callback)();        /* 32 Transmitted data routine */
   unsigned char max_outstanding;  /* 36 Nbr of outstanding xmits/receives */
   unsigned char ptype[2];         /* 37 Protocol type */
   int buffers_lost;               /* 39 Nbr of buffers lost */
   };                              /* 40 */

struct ucb {  /* DEC User Callback Block */
   int portal_id;                /*  0 Portal id from the request */
   unsigned char dest[6];        /*  2 Dest ethernet addr from buffer header */
   unsigned char source[6];      /*  8 Source ethernet addr from buffer header */
   unsigned char *buffer;        /* 14 Pointer to client buffer being returned */
   int bl;                       /* 18 Null  or  length of received message */
   unsigned char buffer_status;  /* 20 COMPLETE  or  ERROR */
   unsigned char buffer_reason;  /* 21 ERROR reason  or  LINE_STATE_CHANGE */
   };                            /* 22 */

#define CBASIZE  8  /* Must be a power of 2 */
#define CBAMASK  7  /* CBASIZE-1 */

struct cba {  /* Callback Address array */
   int inx;                      /* Index to put next ucb into */
   int outx;                     /* Index to take next ucb from */
   struct ucb uc[CBASIZE];       /* Copy of ucbs */
   };

struct userdcb {  /* dcb + user info about it */
   unsigned int portal_id;       /* Portal id (from dll_open) */
   unsigned int ptype;           /* Protocol (in memory byte order) */
   struct dcb d;                 /* The actual dcb */
   };

#endif
