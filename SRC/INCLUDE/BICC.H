#ifndef BICC_H
#define BICC_H
/*
 *      Command codes for ISOLAN MPS SW
 */

#define PORT_STATUS 			0xB3	/* XPORT & SFB */

#define FAST_TIMER              0xF0    /* XPORT & SFB */
#define L_ACTIVATE              0xF2    /* XPORT & SFB */
#define L_DEACTIVATE            0xF3    /* XPORT & SFB */
#define L_DATA_SEND             0xF4    /* XPORT & SFB */
#define M_ACTIVATE              0xFA    /* XPORT & SFB */
#define M_DEACTIVATE            0xFB    /* XPORT & SFB */

#define L_ACTIVATE_CONF         0x03    /* Returned Event Code */
#define L_DEACTIVATE_CONF       0x05    /* Returned Event Code */
#define L_DATA_IND              0x08    /* Returned Event Code, LLC Data */
#define M_DATA_IND              0x0a    /* Returned Event Code, Blue Book MAC Data */
#define M_ACTIVATE_CONF         0x12    /* Returned Event Code */
#define M_DEACTIVATE_CONF       0x14    /* Returned Event Code */

#define FALSE                   0
#define TRUE                    1

#define BUF_SIZE                1504
#define MAX_VC                  32

/*
 *      DATA STRUCTURES
 */

union adr_ptr
{
        char    far *pt;
        struct  
        {
                unsigned short  start_type;
                unsigned short  end_type;
        } type;
        struct mps_status *status;
};

struct tcb         /* Transfer Control Block */
{
        unsigned char   tcbcommand;     /* command code                         */
        unsigned char   tcbcid;         /* command identity                     */
        unsigned char   tcbvcid;        /* virtual circuit id                   */
        unsigned short  tcblength;      /* buffer length                        */
        union adr_ptr   tcbbaddr;       /* address of data                      */
        unsigned char   tcbexpedited;   /* expedited data flag                  */
        unsigned char   tcbcancelable;  /* cancelable flag                      */
        unsigned char   tcbladdr[16];   /* local network address                */
        unsigned char   tcbraddr[16];   /* remote address                       */
        unsigned short  (far *tcbasync)(); /* address of async notification routine*/
        unsigned long   tcblnet;        /* local network number                 */
        unsigned long   tcbrnet;        /* remote network number                */
        unsigned char   tcbrto;         /* call timeout                         */
        unsigned char   tcbsto;         /* not used                             */
        unsigned char   tcbres2[8];     /* reserved                             */
        unsigned char   tcbcmdex;       /* command code extension               */
        unsigned short  tcbstatus;      /* Blue Book MAC type                   */
};


struct acb              /* Asynchronous Control Block */
{
        unsigned char   acbcid;         /* return cid from TCB          */
        unsigned char   acbvcid;        /* return vcid from TCB         */
        unsigned char   acberr;         /* error code                   */
        unsigned char   acbcmd;         /* return command from TCB      */
        unsigned short  acblen;         /* actual length of message     */
        unsigned char   acbraddr[16];   /* remote network address       */
        unsigned long   acbrnet;        /* remote network number        */
        unsigned char   acbladdr[16];   /* local network address        */
        unsigned long   acblnet;        /* local network number         */
        unsigned short  acbstatus;      /* Blue Book MAC type           */
        unsigned char   acbeventcode;   /* copy of board event code     */
        union adr_ptr   acbbaddr;       /* address of data              */
        unsigned char   acbexpedited;   /* expedited data flag          */
};

struct recany_rec               /* receive any record */
{
        unsigned char   recany_count;   
        unsigned char   recany_list[MAX_VC];
};


struct mps_status       
{
        unsigned char address[16];
        unsigned long network;
        unsigned char status;
        unsigned char version;
        unsigned int  maxcon;
        unsigned int  maxcom;
        unsigned long pack_sent;
        unsigned long pack_rec;
        unsigned int  pack_lost;
        unsigned int  pack_abort;
        unsigned int  pack_frame;
        unsigned int  pack_crc;
        unsigned char address_mask[16];
        unsigned int  board_segment;
        unsigned int  board_offset;
        unsigned int  gen_tsel;
        unsigned int  broad_tsel;
        unsigned char reserved[0x3e];
};

#endif

