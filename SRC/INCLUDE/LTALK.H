/*
 * National Center for Supercomputing Applications (NCSA) Telnet source code
 * 
 * February 10, 1989
 * (C) Copyright 1989 Planning Research Corporation
 *
 * Permission is granted to any individual or institution to use, copy,
 * modify, or redistribute this software and its documentation provided this
 * notice and the copyright notices are retained.  This software may not be
 * distributed for profit, either in original form or in derivative works.
 * Planning Research Corporation makes no representations about the
 * suitability of this software for any purpose.
 * 
 * PLANNING RESEARCH CORPORATION GIVES NO WARRANTY, EITHER EXPRESS OR IMPLIED,
 * FOR THE PROGRAM AND/OR DOCUMENTATION PROVIDED, INCLUDING, WITHOUT
 * LIMITATION, WARRANTY OF MERCHANTABILITY AND WARRANTY OF FITNESS FOR A
 * PARTICULAR PURPOSE.
 */

/*
 * LTalk.h - Typedef's and equates for the PC AppleTalk driver.  
 * 
 * Copied from Appendix A of "AppleTalk PC Card and Driver Preliminary Note",
 * Final Draft 3 August, 1987, ADPA #KMB014, with corrections from the body of
 * the document.
 */

/*
 * Revision History:
 * 
 * Date		Initials	Comment
 * ----		--------	-------
 * 02-10-89	JGN@PRC		Initial version.
 * 
 * Initials	Name		Organization
 * --------	----		------------
 * JGN@PRC	Jim Noble	Planning Research Corporation
 */

#ifndef LTALK_H
#define LTALK_H

typedef	int		(*CPTR)();
typedef char  *DPTR;

#define AsyncMask		0x8000

typedef struct AddrBlk {
    unsigned int	network;	/* network number */
    unsigned char	nodeid;		/* node number */
    unsigned char	socket;		/* socket number */
} AddrBlk;

/*
 * Equates/typedefs for ATGetNetInfo/ATInit/ATKill
 */

typedef struct InfoParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    int			inf_network;	/* network number or zero */
    unsigned char	inf_nodeid;	/* current node number or zero */
    unsigned char	inf_abridge;	/* node number of a bridge or zero */
    int			inf_config;	/* configuration mask for driver */
    DPTR		inf_bptr;	/* segment:offset of buffer for error */
					/* info */
    int			inf_buffsize;	/* size of buffer */
} InfoParams;

typedef struct ATErrorStatus {
    int			bufflen;	/* number of valid bytes in buffer */
    long		packets;	/* number of packets received */
    long		crcerrs;	/* number of CRC errors detected */
    long		overruns;	/* number of overruns detected */
    long		deferrals;	/* number of deferrals detected */
    long		collisions;	/* number of collisions detected */
    long		tossed;		/* number of packets tossed */
    long		mem_free;	/* number of Bytes of Free Memory */
    long		largest_mem;	/* Largest contiguous chunk of memory */
					/* available */
    long		int_bytes;	/* number of bytes used by driver */
    long		ext_bytes;	/* number of bytes used by external aps */
} ATErrorStatus;

#define ATInit			1
#define ATKill			2
#define ATGetNetInfo		3

/*
 * Equates/typedefs for Timer Calls
 */

typedef struct TimerParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion (timer) routine or zero */
    long		tmr_ticks;	/* timer ticks (since init.) */
    int			tmr_time;	/* number of 1/3 seconds to wait */
    struct TimerParams  *
			tmr_params;	/* address of another parameter block */
} TimerParams;

#define ATGetClockTicks		4
#define ATStartTimer		5
#define ATResetTimer		6
#define ATCancelTimer		7

/*
 * Equates/typedefs for LAP Protocol
 */

typedef struct LAPParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    int			lap_fl1;	/* filler word */
    unsigned char	lap_destnode;	/* destination node number */
    int			lap_fl2;	/* filler word */
    unsigned char	lap_type;	/* LAP type for header */
    DPTR		lap_bptr;	/* segment:offset of buffer area or */
					/* protocol handler (LAPInstall) */
    int			lap_buffsize;	/* size of buffer area */
} LAPParams;

#define LAPInstall		0x10
#define LAPRemove		0x11
#define LAPWrite		0x12
#define LAPRead			0x13
#define LAPCancel		0x14

/*
 * Equates/typedefs for DDP Protocol
 */

typedef struct DDPParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    AddrBlk		ddp_addr;	/* destination address of packet */
    unsigned char	ddp_socket;	/* socket for command */
    unsigned char	ddp_type;	/* DDP type for header */
    DPTR		ddp_bptr;	/* segment:offset of buffer area or */
					/* socket listener (DDPOpenSocket) */
    int			ddp_buffsize;	/* size of buffer area */
    unsigned char	ddp_chksum;	/* checksum flag */
} DDPParams;

#define DDPOpenSocket		0x20
#define DDPCloseSocket		0x21
#define DDPWrite		0x22
#define DDPRead			0x23
#define DDPCancel		0x24

/*
 * Equates/typedefs for NBP Protocol
 */

typedef struct NBPTuple {
	AddrBlk		tup_address;
	char		tup_enum;
	char		tup_entname[99];
} NBPTuple;

typedef struct NBPTabEntry {
	struct NBPTabEntry
		*tab_nxtentry;
	NBPTuple
		tab_tuple;
} NBPTabEntry;

typedef struct NBPParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    AddrBlk		nbp_addr;	/* for confirm, original address */
    int			nbp_toget;	/* number of matches to get/were found */
    DPTR		nbp_bptr;	/* segment:offset of buffer area */
    int			nbp_buffsize;	/* size of buffer area */
    unsigned char	nbp_interval;	/* interval (seconds) between retries */
    unsigned char	nbp_retry;	/* number of tries */
    DPTR		nbp_entptr;	/* segment:offset of entity name */
} NBPParams;

#define NBPRegister		0x30
#define NBPRemove		0x31
#define NBPLookup		0x32
#define NBPConfirm		0x33
#define NBPCancel		0x34

/*
 * Equates/typedefs for ZIP Protocol
 */

typedef struct ZIPParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    long		zip_fl;	/* filler dword */
    int			zip_zones;	/* number of zone names in returned list */
    DPTR		zip_bptr;	/* segment:offset of buffer area */
    int			zip_buffsize;	/* size of buffer area */
} ZIPParams;

#define ZIPGetZoneList		0x35
#define ZIPGetMyZone		0x36

/*
 * Equates/typedefs for ATP Protocol
 */

typedef struct BDSElement {
    DPTR		bds_bptr;	/* segment:offset of data buffer */
    int			bds_buffsize;	/* size of data buffer in bytes */
    int			bds_datasize;	/* amount of data put in header */
    char		bds_usrbytes[4];/* four bytes for ATP header */
} BDSElement;

typedef struct ATPParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    AddrBlk		atp_addr;	/* embedded address block */
    char		atp_socket;	/* socket associated with this transaction */
    char		atp_fl;	/* filler byte */
    DPTR		atp_bptr;	/* segment:offset of request data buffer */
    int			atp_buffsize;	/* amount of data at above address */
    char		atp_interval;	/* interval (seconds) between tries */
    char		atp_retry;	/* number of times to try request */
    char		atp_flags;	/* control info  (XO, etc.) */
					/* (NoRelease = 02H) */
    char		atp_seqbit;	/* bitmap, sequence info */
    int			atp_tranid;	/* transaction ID */
    char		atp_userbytes[4];/* four user bytes for ATP header */
    char		atp_bdsbuffs;	/* number of BDSElements at atp_bdsptr */
    char		atp_bdsresps;	/* number of responses at atp_bdsptr */
    struct BDSElement *
			atp_bdsptr;	/* segment:offset of BDS for responses */
} ATPParams;

#define ChkSum			0x01
#define STSbit			0x08
#define EOMbit			0x10
#define XObit			0x20

#define ATPOpenSocket		0x40
#define ATPCloseSocket		0x41
#define ATPSndRequest		0x42
#define ATPGetRequest		0x43
#define ATPSendResponse		0x44
#define ATPAddResponse		0x45
#define ATPCanTrans		0x46
#define ATPCanResponse	0x47
#define ATPCnRequest	0x48
#define ATPRespond		0x49

/*
 * Equates/typedefs for PAP Protocol
 */

typedef struct PAPStatusRec {
    long		psr_system;	/* internal PAP information */
    char		psr_status[256];/* printer-dependent status string */
} PAPStatusRec;

typedef struct PAPParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    AddrBlk		pap_addr;	/* address for status, open calls */
    int			pap_refnum;	/* reference number for connection */
    DPTR		pap_bptr;	/* segment:offset of buffer area */
    int			pap_buffsize;	/* size of buffer area */
    unsigned char	pap_eof;	/* end-of-file indicator */
    unsigned char	pap_srefnum;	/* server reference number */
    DPTR		pap_entptr;	/* segment:offset of entity name */
} PAPParams;

#define PAPOpen			0x70
#define PAPClose		0x71
#define PAPRead			0x72
#define PAPWrite		0x73
#define PAPStatus		0x74
#define	PAPRegName		0x75
#define PAPRemName		0x76
#define PAPInit			0x77
#define PAPNewStatus		0x78
#define PAPGetNextJob		0x79
#define PAPKill			0x7a
#define PAPCancel		0x7b
#define PAPXStatus		0x7c

/*
 * Equates/typedefs for ASP Protocol
 */

typedef struct ASPParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    int			asp_maxcmdsize;	/* maximum size of command block */
    int			asp_quantum;	/* maximum data for reply or write */
} ASPParams;

typedef struct ASPWksParams {
    int			atd_command;	/* command code */
    int			atd_status;	/* status word/error return */
    CPTR		atd_compfun;	/* completion routine or zero */
    long		aspw_cresult;	/* result code from server */
    int			aspw_sesrefnum;	/* reference number for session */
    unsigned char	aspw_interval;	/* period of interval timer */
    unsigned char	aspw_retry;	/* number of retries to be made */
    DPTR		aspw_cmdblock;	/* command buffer pointer */
    int			aspw_cblocksize;/* size of command block */
    DPTR		aspw_breply;	/* address of reply buffer */
    int			aspw_replysize;	/* size of reply buffer */
    int			aspw_actreply;	/* actual size of reply data */
    DPTR		aspw_bwrite;	/* address of write buffer */
    int			aspw_writesize;	/* size of write buffer */
    int			aspw_byteswritten;/* actual number of bytes written */
} ASPWksParams;

#define ASPGetParms		0x50
#define ASPCloseSession		0x51
#define ASPCancel		0x52

#define ASPGetStatus		0x5d
#define ASPOpenSession		0x5e
#define ASPCommand		0x5f
#define ASPWrite		0x60
#define ASPGetAttention		0x61

/*
 * Equates for Error Codes
 */

#define NOERR			0	/* No errors yet encountered */
#define MAXCOLISERR		-1	/* Too many collisions detected */
#define MAXDEFERERR		-2	/* 32 deferrals encountered (ALAP) */

#define LAP_LENERR		-30	/* LAP data too long */
#define LAP_TYPERR		-31	/* Bad LAP type */
#define TABFULLERR		-32	/* No more LAP protocols available */
#define LAP_NOTFND		-33	/* LAP type not found in internal */
    					/* protocol table */
#define LAP_CANCELLED		-34	/* LAP command has been cancelled */
#define DDP_SKTERR		-40	/* Used unopen socket */
#define DDP_LENERR		-41	/* DDP data too long */
#define NOBRDGERR		-42	/* No bridge found */
#define DDP_CANCELLED		-43	/* DDP transaction cancelled */

#define ATP_REQFAILED		-100	/* No response received */
#define ATP_NOSENDRESP		-101	/* Original socket not found */
#define ATP_BADSOCKET		-102	/* ATP socket not found */
#define ATP_NORELEASE		-103	/* No release received on ATP request */
#define ATP_OVERFLOW		-104	/* Too much response data */
#define ATP_CANCELLED		-105	/* Transaction cancelled */
#define ATP_NO_IDS		-106	/* No more transaction IDs for ATP */

#define NO_MEM_ERROR		-120	/* Not enough memory to perform */
    					/* requested operation */
#define BAD_PARAMETER		-121	/* Illegal parameter or parameters */
#define STACKERROR		-122	/* Too many levels of nesting to */
    					/* complete call */
#define ATNOTINITIALIZED	-123	/* Driver not initialized */
#define CALLNOTSUPPORTED	-124	/* Requested call not supported */
#define HARDWARE_ERROR		-125	/* Unrecoverable hardware error */
#define SOFTWARE_ERROR		-126	/* Internal software error. */
    					/* Reinitialize driver */
#define MEMORY_CORRUPTED	-127	/* Internal memory pool corrupted. */
    					/* Reload driver */
#define BAD_SYNC_CALL		-128	/* Synchronous command was called at */
    					/* a time when the driver cannot */
    					/* handle it */

#define NBP_NEWSOCKET		-200	/* New entity address on confinn */
#define NBP_NOCONFIRM		-201	/* Entity not found on confirm */
#define NAME_IN_USE		-202	/* Name already in use */
#define NBP_NO_ROOM		-203	/* Buffer overflow */
#define BAD_NAME		-204	/* Illegal entity name */
#define NBP_NOTFOUND		-205	/* Name not found */
#define NBP_CANCELLED		-206	/* NBP comrnand cancelled */
#define NBP_NO_IDS		-207	/* No more IDs for NBP */

#define TMR_NOTFOUND		-215	/* Timer structure not found */
#define TMR_CANCELLED		-216	/* Timer command cancelled */

#define PAP_BADCONNID		-300	/* Invalid connection reference */
    					/* number */
#define PAP_NOCONNIDS		-301	/* No more connections available */
#define PAP_DIED		-302	/* Connection died */
#define PAP_LENERR		-303	/* Data length too large */
#define PAP_WRITE_ACTIVE	-310	/* Write already active on stream */
#define PAP_READ_ACIIVE		-311	/* Read already active on stream */

#define ASP_DIED		-401	/* ASP session tickle timer expired */
#define ASP_CANCELLED		-402	/* ASP command was cancelled */
#define ASP_BADCONNID		-403	/* ASP session reference number does */
    					/* not exist */


/***************************************/
/*  ARP packet structure for telnet
*/

/*
 * atalk.h - AppleTalk conversion structures.....
 */

#define ATADDLEN	4
#define ATHTYPE		3			/* AppleTalk type */

/*
 * AppleTalk Listener Interface structure
 */

#define	AIP		22
#define AARP	23

struct aTalk {
	short	count;
	unsigned char
			dest[ATADDLEN],	/* where the packet is going */
		 	me[ATADDLEN],
			type, paddingchars[3];		/* Give an even length so we will be at even */
	};

typedef struct aTalk ATdlayer;


/*
 * Here is the ARP packet structure  -  ATplummer
 */

struct ATplummer {
	ATdlayer d; 				/* data link layer packet header */

	unsigned short hrd,			/* hardware type, Ethernet = 1 */
			pro;			/* protocol type to resolve for */
	unsigned char	
			hln,			/* byte length of hardware addr = 6 for ETNET */
			pln;			/* byte length of protocol = 4 for IP */
	unsigned short op;			/* opcode, request = 1, reply = 2, RARP = 3,4 */
	unsigned char
			sha[ATADDLEN],
			spa[4],
			tha[ATADDLEN],
			tpa[4];
/*
*   the final four fields (contained in 'rest') are:
*      sender hardware address:   sha       hln bytes
*      sender protocol address:   spa       pln bytes
*      target hardware address:   tha       hln bytes
*      target protocol address:   tpa       pln bytes
*/
		char dummy[8];				/* for overrun */
};

typedef struct ATplummer ATARPKT;

#endif
