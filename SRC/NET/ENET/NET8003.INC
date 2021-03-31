		SUBTTL	WD8STAR2.EQU - StarLAN equates


COMMENT \
/************************************************************************
* WD8STAR2.EQU								*
*									*
* Provides constants and data definitions for WD8STAR2.ASM file		*
* ( For WD8003 StarLAN PC Adapter Board ).				*
*									*
* CONTENTS:								*
* --------								*
*	external definition						*
*	I/O port offset definition					*
*	I/O register mask definition					*
*	miscellaneous constants	definition				*
*	structure definition						*
*									*
* NOTE:									*
* ----									*
*	variable definition and reserved patch area are in asm file now	*
*									*
* HISTORY:								*
* -------								*
*									*
*									*
*************************************************************************\


;*****************************************************************************
;
;	StarLAN controller board offsets
;	IO port definition (BASE in WD8_base)
;*****************************************************************************

W83CREG  EQU	00h			; 8003 control register
W83CREG  EQU	00h			; 8003 status register
ADDROM   EQU	08h			; LAN Address ROM
; 8390 LAN Controller (page0) register offset for read and write 
CMDR	EQU	10h			; command register for read & write
CLDA0	EQU	11h			; current local dma addr 0 for read
PSTART	EQU	11h			; page start register for write
CLDA1	EQU	12h			; current local dma addr 1 for read
PSTOP	EQU	12h			; page stop register for write
BNRY	EQU	13h			; boundary reg for rd and wr
TSR	EQU	14h			; tx status reg for rd
TPSR	EQU	14h			; tx start page start reg for wr	
NCR	EQU	15h			; number of collision reg for rd
TBCR0	EQU	15h			; tx byte count 0 reg for wr
FIFO	EQU	16h			; FIFO for rd
TBCR1	EQU	16h			; tx byte count 1 reg for wr
ISR	EQU	17h			; interrupt status reg for rd and wr
CRDA0	EQU	18h			; current remote dma address 0 for rd
RSAR0	EQU	18h			; remote start address reg 0  for wr
CRDA1	EQU	19h			; current remote dma address 1 for rd
RSAR1	EQU	19h			; remote start address reg 1 for wr
RBCR0	EQU	1Ah			; remote byte count reg 0 for wr
RBCR1	EQU	1Bh			; remote byte count reg 1 for wr
RSR	EQU	1Ch			; rx status reg for rd
XRCR	EQU	1Ch			; rx configuration reg for wr
CNTR0	EQU	1Dh			; tally cnt 0 for frm alg err for rd
TCR	EQU	1Dh			; tx configuration reg for wr
CNTR1	EQU	1Eh			; tally cnt 1 for crc err for rd
DCR	EQU	1Eh			; data configuration reg for wr
CNTR2	EQU	1Fh			; tally cnt 2 for missed pkt for rd
IMR	EQU	1Fh			; interrupt mask reg for wr
; 8390 LAN Controller (page1) register offset for read and write 
PAR0	EQU	11h 			; physical addr reg 0 for rd and wr
PAR1	EQU	12h 			; physical addr reg 1 for rd and wr
PAR2	EQU	13h 			; physical addr reg 2 for rd and wr
PAR3	EQU	14h 			; physical addr reg 3 for rd and wr
PAR4	EQU	15h 			; physical addr reg 4 for rd and wr
PAR5	EQU	16h 			; physical addr reg 5 for rd and wr
CURR	EQU	17h			; current page reg for rd and wr
MAR0	EQU	18h			; multicast addr reg 0 fro rd and WR
MAR1	EQU	19h			; multicast addr reg 1 fro rd and WR
MAR2	EQU	1Ah			; multicast addr reg 2 fro rd and WR
MAR3	EQU	1Bh			; multicast addr reg 3 fro rd and WR
MAR4	EQU	1Ch			; multicast addr reg 4 fro rd and WR
MAR5	EQU	1Dh			; multicast addr reg 5 fro rd and WR
MAR6	EQU	1Eh			; multicast addr reg 6 fro rd and WR
MAR7	EQU	1Fh			; multicast addr reg 7 fro rd and WR

;***********************************************************************
;
;	8003 control register operations
;***********************************************************************

MSK_RESET	EQU	80h	        ; reset LAN controller
MSK_ENASH	EQU	40h		; enable PC access to shared mem
MSK_DECOD	EQU	3Fh 		; ???? memory decode bits, corresponding
					; to SA 18-13. SA 19 assumed to be 1
;***********************************************************************
;
;	8390 CMDR MASK
;***********************************************************************

MSK_STP		EQU	01h		; software reset, take 8390 off line
MSK_STA		EQU	02h		; activate the 8390 NIC
MSK_TXP		EQU	04h		; initial txing of a frm
MSK_RD2		EQU	20h		; abort remote DMA
MSK_PG0		EQU	00h		; select register page 0
MSK_PG1		EQU	40h		; select register page 1

;***********************************************************************
;
;	8390 ISR & IMR MASK
;***********************************************************************

MSK_PRX  EQU	01h		; rx with no error
MSK_PTX  EQU	02h		; tx with no error
MSK_RXE  EQU	04h		; rx with error
MSK_TXE  EQU	08h		; tx with error
MSK_OVW  EQU	10h		; overwrite warning
MSK_CNT  EQU	20h		; MSB of one of the tally counters is set
MSK_RDC  EQU	40h		; remote dma completed
MSK_RST	 EQU	80h		; reset state indicator

;***********************************************************************
;
;	8390 DCR MASK
;***********************************************************************

MSK_WTS EQU	01h		; word transfer mode selection
MSK_BOS	EQU	02h		; byte order selection
MSK_LAS	EQU	04h		; long addr selection
MSK_BMS	EQU	08h		; burst mode selection
MSK_ARM	EQU	10h		; atuoinitialize remote
MSK_FT00 EQU	00h		; burst lrngth selection
MSK_FT01 EQU	20h		; burst lrngth selection
MSK_FT10 EQU	40h		; burst lrngth selection
MSK_FT11 EQU	60h		; burst lrngth selection

;***********************************************************************
;
;	8390 RCR MASK
;***********************************************************************

MSK_SEP EQU	01h		; save error pkts
MSK_AR 	EQU	02h		; accept runt pkt
MSK_AB 	EQU	04h		; accept broadcast 
MSK_AM 	EQU	08h		; accept multicast 
MSK_PRO	EQU	10h		; promiscuous physical
				; accept all pkt with physical adr
MSK_MON EQU	20h		; monitor mode

;***********************************************************************
;
;	8390 TCR MASK
;***********************************************************************

MSK_CRC EQU	01h		; inhibit CRC, do not append crc
MSK_LB01 EQU	06h		; encoded loopback control
MSK_ATD	EQU	08h		; auto tx disable
MSK_OFST EQU	10h		; collision offset enable 

;***********************************************************************
;
;	8390 RSR MASK
;***********************************************************************

SMK_PRX  EQU	01h		; rx without error
SMK_CRC  EQU	02h		; CRC error
SMK_FAE  EQU	04h		; frame alignment error
SMK_FO   EQU	08h		; FIFO overrun
SMK_MPA  EQU	10h		; missed pkt
SMK_PHY  EQU	20h		; physical/multicase address
SMK_DIS  EQU	40h		; receiver disable. set in monitor mode
SMK_DEF	 EQU	80h		; deferring

;***********************************************************************
;
;	8390 TSR MASK
;***********************************************************************

SMK_PTX  EQU	01h		; tx without error
SMK_DFR  EQU	02h		; non deferred tx
SMK_COL  EQU	04h		; tx collided
SMK_ABT  EQU	08h		; tx aboort because of excessive collisions
SMK_CRS  EQU	10h		; carrier sense lost
SMK_FU   EQU	20h		; FIFO underrun
SMK_CDH  EQU	40h		; collision detect heartbeat
SMK_OWC	 EQU	80h		; out of window collision



;***********************************************************************
;
;	Miscellaneous Constants
;***********************************************************************

;
; PIC (8259) Information 
;
EOI	EQU	20h			; End Of Interrupt
INTA00	EQU	20h			; 8259 port
INTA01	EQU	21h			; 8259 port
;
; Buffer Length and Field Definition Info
;
BPNA	EQU	6			; Bytes Per Network Address
MIN_DATA  EQU   52 + BPNA		; 52 bytes data + 6 bytes address
TX_BLK_LEN	EQU	2		; Offset to Dest Address in TX Block
TX_BLK_ADD	EQU	BPNA		; Length of TX block dest addr field
TX_HDR_LEN   EQU  TX_BLK_LEN + TX_BLK_ADD	; Offset to Data in TX Block
RX_TRAIL	EQU	02		; Trailer def for Received Frame Status
RX_HDR_LEN	EQU	12		; Offset to Data in Received Frame

;***********************************************************************
;
;	shared memory constant definition
;***********************************************************************
; for rcv buff ring of shr mem
STRT_PG EQU	6	      		; start at page 6
STOP_PG EQU	32			; end at page 31 
; for tx buff of shr mem
TB_SIZE EQU	2			; number of tb buff in shr mem
TB_PGNO EQU	3			; number of pages in one tb buff

;***********************************************************************
;
;	Structure definitions
;***********************************************************************

;
; MAC LAYER STATS
;
mstats struc			; MAC Layer Statistics
	t0	dd	0		; number of attempts to transmit 
	txok	dd	0		; number of successful transmissions 
	txbad	dd	0		; number of failed transmits 
	collsn	dd	0		; number of collisions occured 
	lostcrs	dd	0		; number of times the CRS was lost during tx 
	lostcts	dd	0		; lost CTS 
	underrun	dd	0	; number of underrun errors 

	rxrd	dd	0		; number of times the receiver was ready to receive 
	rxok	dd	0		; number of mpdus rxed w/o error 
	rxnrd	dd	0		; number rx's aborted for receive processing 
	crcerr	dd	0		; number of mpdus received with CRC error 
	overrun	dd	0		; number of overrun errors 
	algerr	dd	0		; number of alignment errors 
	srtfrm	dd	0		; number of short frame receiver error 
	rxnom	dd	0		; number of receives lost due to lack of mem 
	rxblkd	dd	0		; NOT USED

;  The following are not used right now....
	ex_lockup	dd	0	; number of times the execution unit of the chip was in lockup situation 
	ia_corrup	dd	0	; number of times the IA is corrupted 
	spur_int	dd	0	; # of spurious interrupts
mstats ENDS

;
; BUFFER DESCRIPTOR
;
bufp    struc			; Buffer Descriptor Block
	bd_next	dw	0		; Pointer to next block in chain
	bd_prev	dw	0		; Pointer to previous block in chain
	bd_global dw	0		;
	bd_addr dw	0		; Address of buffer associated w/this block
	bd_seq dd	2 DUP (?)	;
	bd_info dw	0		;
bufp    ENDS
