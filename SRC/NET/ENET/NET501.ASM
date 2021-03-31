;  3COM 3C501 driver code
;  Tim Krauskopf
;
;  Thanks to Bruce Orchard for mods to allow more I/O addresses and INTs
;  5/18/88
;****************************************************************************
;*                                                                          *
;*                                                                          *
;*      part of NCSA Telnet                                                 *
;*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
;*                                                                          *
;*      National Center for Supercomputing Applications                     *
;*      152 Computing Applications Building                                 *
;*      605 E. Springfield Ave.                                             *
;*      Champaign, IL  61820                                                *
;*                                                                          *
;*                                                                          *
;****************************************************************************
;

	TITLE	NETSUPPORT -- LOW LEVEL DRIVERS FOR ETHERNET
;
;  Assembler support for interrupt-driven Ethernet I/O on the PC
;
;  Will read and write packets from the 2K packet buffer on the
;  Etherlink card.  Provides hooks for higher layer protocols.
;
;Microsoft EQU 1
;Lattice EQU 1
ifndef Microsoft
    ifndef Lattice
        if2
            %out
            %out ERROR: You have to specify "/DMicrosoft" OR "/DLattice" on the
            %out        MASM command line to determine the type of assembly.
            %out
        endif
        end
    endif
endif

ifdef Microsoft
X	EQU	6
	DOSSEG
	.MODEL	LARGE
else
    	NAME	NET
  	INCLUDE	DOS.MAC
  	SETX
endif
;
;  Equates for controlling the 3COM board
;
ICTRL	EQU	020H		; 8259 interrupt control register
IMASK	EQU	021H		; 8259 interrupt mask register
ENDOFI	EQU	020H		; end-of-interrupt
;
;  Controller registers
;
O_EADDR	EQU	0H		; Network address for hardware checking
;   takes six bytes, this is the address that the Ethernet board will
;   match to find packets on the net.  (0-5h)
;
O_EREC	EQU	6H		; Receive status (read)
				; Receive command (write)
O_ESEND	EQU	7H		; Transmit status (read)
				; Transmit command (write)
O_EGLOW	EQU	8H		; General purpose pointer for R/W to
				; packet buffer, low byte
O_EGHI	EQU	9H		; high byte, total of 11 bits for 2K buffer
O_ERLOW	EQU	0AH		; Receive pointer, set by board (read) low byte
				; Receive buffer pointer clear (write)
O_ERHI	EQU	0BH		; high byte of Receive pointer
O_EPROM	EQU	0CH		; PROM window address
O_XXX	EQU	0DH 		; ??
O_EAUX	EQU	0EH		; Auxiliary Status (read)
				; Aux Command (write)
O_EBUF	EQU	0FH		; Buffer window (where to I/O to net)
;
;  Transmit command options
;     what conditions we wish to be interrupted on after transmit
;
EDTUNDER  EQU	01H		; Detect underflow (bad CRC), not used
EDTCOLL	EQU	02H		; Detect collision on xmit
EDTC16	EQU	04H		; Detect 16 consecutive collisions (net down)
EDTOK	EQU	08H		; Detect successful transmission
				; other 4 bits unused in this reg
EXMITNORM  EQU	00H		; Normal use for interrupt-driven XMIT
;
;  Transmit status results
;
;  Use same values as commands, 08h means OK to xmit again
;
;*****************************
;  Receive Command Options
;
;    If something is not detected for, the receiver automatically discards
;        those packets.
;
EDTOVER	EQU	01H		; Detect Overflow condition
EDTFCS	EQU	02H		; Detect FCS error, bad CRC on packet
EDTDRB	EQU	04H		; Detect dribble errors and accept them
EDTSHORT EQU	08H		; Detect short frames (< 60 bytes)
EDTEOF	EQU	10H		; Detect no overflow (end-of-frame found)
EGOOD	EQU	20H		; Accept good frames 
; four values legal for the last two bits:
ECLOSE	EQU	00H		; Turn off receiver
EGETALL	EQU	40H		; Get all packets, no matter what address
EBROAD	EQU	80H		; get those for me or for broadcast
EMULTI	EQU	0C0H		; get those for me or for multicast
EWANT	EQU	0A0h		; EGOOD OR EBROAD
				; which packets we will look for on net
;
;  Receive Status results
;
;  errors are not detected unless asked for...otherwise the board just
;  won't take bad packets off of the net.
;
ERROVER	EQU	01H		; overflow error
ERRFCS	EQU	02H		; FCS (checksum) error
ERRDRB	EQU	04H		; Dribble error
ERRSHORT  EQU	08H		; Short frame error
ENOOVER	EQU	10H		; Received without overflow error 
				; means that we didn't miss any by being slow
;EGOOD	EQU	20H		; as above, we received a valid frame
; undefined 40h
ESTALE	EQU	80H		; stale receive condition, already read me
;
;  Aux command register
;
EIRE	EQU	01H		; interrupt request enable (no DMA) new boards
EBADFCS	EQU	02H		; create bad checksum for testing only
;
;  Next two bits tell who has access to packet buffer
;
EBUS	EQU	00H		; System bus has control of buffer
EXMIT	EQU	04H		; Transmit packet in buffer, auto kick
				; back to recieve status
EGETEM	EQU	08H		; Receive state, look for packets
ELOOP	EQU	0CH		; Transmit and loopback into xmit buffer
;  10H  unused
EDMA	EQU	20H		; Starts a DMA xfer
ERIDE	EQU	40H		; Interrupt and DMA enable
ERESET	EQU	80H		; Reset the Ethernet board
;
;  Aux status register
;
ERBUSY	EQU	01H		; Receive busy, receiver looking for packets
;               02H		; echos command status EBADFCS
;               04,08h		; echos command status for EXMIT,EGETEM,EBUS
EDMADONE  EQU	10H		; goes to one when DMA finishes
;               20H		; echos DMA request bit
;               40h		; echos RIDE bit
EXBUSY	EQU	80H		; for polled xmit, goes to 0 when xmit is done
;
;
;  Macros for in and out
;
MOUT	MACRO	REG,STUFF       ; one byte to the given I/O register
	MOV	DX,REG
	MOV	AL,STUFF
	OUT	DX,AL
	ENDM
;
MOUTW	MACRO	REG,LO,HI  	; two bytes to the I/O double port
	MOV	DX,REG
	MOV	AL,LO
	OUT	DX,AL
	INC	DX
	MOV	AL,HI
	OUT	DX,AL
	ENDM
;
MIN	MACRO	REG         	; get one byte to al
	MOV	DX,REG
	IN	AL,DX
	ENDM

ifdef Microsoft
;DGROUP	group	_DATA
;_DATA	segment	public 'DATA'
;	assume	DS:DGROUP
	.data
else
	DSEG
endif
;	PUBLIC	STAT,BUFPT,BUFORG,BUFEND,BUFREAD,BUFBIG,BUFLIM,OFFS
;
;  The pointers below are actually DWORDs but we access them two
;  bytes at a time.
;
; STAT change to RSTAT because of name clash with MSC library routine
ifdef Microsoft
	EXTRN	_RSTAT:BYTE	; last status from read
	EXTRN	_BUFPT:WORD	; current buffer pointer
	EXTRN	_BUFORG:WORD	; pointer to beginning of buffer
	EXTRN	_BUFEND:WORD	; pointer to end of buffer
	EXTRN	_BUFREAD:WORD	; pointer to where program is reading
	EXTRN	_BUFBIG:WORD	; integer, how many bytes we have
	EXTRN	_BUFLIM:WORD	; integer, max bytes we can have
else
	EXTRN	RSTAT:BYTE	; last status from read
	EXTRN	BUFPT:WORD	; current buffer pointer
	EXTRN	BUFORG:WORD	; pointer to beginning of buffer
	EXTRN	BUFEND:WORD	; pointer to end of buffer
	EXTRN	BUFREAD:WORD	; pointer to where program is reading
	EXTRN	BUFBIG:WORD	; integer, how many bytes we have
	EXTRN	BUFLIM:WORD	; integer, max bytes we can have
endif
;
;
BASEA	DW	?		; Base I/O address on PC I/O bus
EADDR	DW	?		; Network address for hardware checking
;   takes six bytes, this is the address that the Ethernet board will
;   match to find packets on the net.  (0-5h)
;
EREC	DW	?		; Receive status (read)
				; Receive command (write)
ESEND	DW	?		; Transmit status (read)
				; Transmit command (write)
EGLOW	DW	?		; General purpose pointer for R/W to
				; packet buffer, low byte
EGHI	DW	?		; high byte, total of 11 bits for 2K buffer
ERLOW	DW	?		; Receive pointer, set by board (read) low byte
				; Receive buffer pointer clear (write)
ERHI	DW	?		; high byte of Receive pointer
EPROM	DW	?		; PROM window address
EAUX	DW	?		; Auxiliary Status (read)
				; Aux Command (write)
EBUF	DW	?		; Buffer window (where to I/O to net)


SAVECS	DW	00H		; where to save the old interrupt ptr
SAVEIP	DW	00H
OLDMASK	DB	00H		; save interrupt controller mask
DEAF	DB	00H		; when we can't handle any more packets
OFFS	DW	00H		; how many times the handler was turned off
;
;  use variables to access IRQ3 or IRQ5
;  3 is COM2, 5 is LPT2
;
CINTNUM	db	3
INTNUM	DB	0BH		; Defaults to IRQ3, interrupt handler 0bh
WHICHINT  DW	4*0BH		; ETOPEN can change these values
TURNOFF	DB	08H
TURNON	DB	0F7H
ifdef Microsoft
;_DATA	ends
else
	ENDDS
endif
;
;
;
;   The subroutines to call from C
;
ifdef Microsoft
;_TEXT	segment	public	'CODE'
;	assume CS:_TEXT
	.code
	PUBLIC	_E1RECV,_E1ETOPEN,_E1ETCLOSE,_E1GETADDR
ifdef NOT_USED
    PUBLIC  _E1SETADDR
endif
    PUBLIC  _E1XMIT,_E1ETUPDATE
else
	PSEG
	PUBLIC	E1RECV,E1ETOPEN,E1ETCLOSE,E1GETADDR
ifdef NOT_USED
    PUBLIC  E1SETADDR
endif
    PUBLIC  E1XMIT,E1ETUPDATE
endif

;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,addr,ioaddr)
;           char s[6];       ethernet address
;           int irq,addr,ioaddr;     
;                interrupt number, base mem address (unused) and
;                i/o address to use
;
;
ifdef Microsoft
_E1ETOPEN	PROC	FAR
else
E1ETOPEN	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
;
;  set up all of the I/O register values in memory
;
	MOV	AX,[BP+X+8]	; i/o address -> ax
	MOV	BASEA,AX	; save base address
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EADDR	; + offset of Ethernet address
	MOV	EADDR,BX	; store address of Ethernet address
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EREC	; + offset of receive command/status
	MOV	EREC,BX		; store address of receive command status
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_ESEND	; + offset of transmit command/status
	MOV	ESEND,BX	; store address of transmit command status
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EGLOW	; + offset of general pointer low byte
	MOV	EGLOW,BX	; store address of general pointer low byte
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EGHI	; + offset of general pointer high byte
	MOV	EGHI,BX		; store address of general pointer high byte
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_ERLOW	; + offset of receive pointer low byte
	MOV	ERLOW,BX	; store address of receive pointer low byte
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_ERHI	; + offset of receive pointer high byte
	MOV	ERHI,BX		; store address of receive pointer high byte
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EPROM	; + offset of PROM window
	MOV	EPROM,BX	; store address of PROM window
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EAUX	; + offset of auxiliary command/status
	MOV	EAUX,BX		; store address of auxiliary command/status
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EBUF	; + offset of buffer window
	MOV	EBUF,BX		; store address of buffer window

;
;  check the parameters for interrupt and dma
;
	MOV	AX,[BP+X+4]	; interrupt number
	or	ax,ax
	JG	IOK1		; if 0 or negative, use 3
	mov	ax,3
IOK1:
	CMP	AX,7		; too big?
	JNG	IOK2		; yes:  use 3
	mov	ax,3
IOK2:
	MOV	CINTNUM,AL	; save 8259 interrupt number
	ADD	AX,8		; convert to 8086 vector number
	MOV	INTNUM,AL	; save vector number
	SHL	AX,1		; * 2
	SHL	AX,1		; * 2 = vector address
	MOV	WHICHINT,AX	; save vector address
	MOV	CL,CINTNUM	; interrupt number -> cl
	MOV	AX,1		; 1 -> ax
	SHL	AX,CL		; make interrupt mask
	MOV	TURNOFF,AL	; store interrupt disable mask
	NOT	AX		; make enable mask
	MOV	TURNON,AL	; store interrupt enable mask

;
;  DMA not used for 3C501
;
	MOUT	EAUX,ERESET	; reset the board
	MOUT	EAUX,0          ; Clear the reset bit, otherwise keeps resetting
;
;  install the interrupt handler
;
	CALL	IINST		; do the patching of the interrupt table
;
;  set up the net address
;
	MOV	DX,EADDR	; get base i/o reg for setting address

	PUSH 	DS		; save mine
	MOV	AX,[BP+X+2]	; get new one
	MOV	DS,AX           ; set new one
	MOV	SI,[BP+X]	; get pointer, ds:si is ready
	;
	MOV	CX,6
	CLD
SADDR:
	LODSB			; get next one
	OUT	DX,AL		; send it
	INC	DX		; next position
	LOOP	SADDR		; do 6 times

	POP	DS		; get back DS of local data
;
;  enable interrupts here with interrupt handler 
;  already set up.
;
	MOUT	ESEND,0		; xmit command = 0 for no interrupts
	IN	AL,DX

	MOUT	EREC,EWANT	; Set receiver for which packets we want
	IN	AL,DX		; reset 'stale'

	MOUT	ERLOW,0		; Clear the receive buffer pointer

	CLI
	MOUT	EAUX,EGETEM+ERIDE	; Set for receive, interrupts

	MIN	IMASK		; get current int enable mask
	MOV	BL,AL		; save a copy
	AND	AL,TURNON	; force bit for etherlink board off
	OUT	DX,AL		; put back the byte, IRQ enabled

	STI
	AND	BL,TURNOFF	; isolate this bit only from oldmask
	MOV	OLDMASK,BL	; save it
;
	POP	SI
	POP	BP
	XOR	AX,AX
	RET
ifdef Microsoft
_E1ETOPEN	ENDP
else
E1ETOPEN	ENDP
endif

ifdef NOT_USED
;
;******************************************************************
;  SETADDR
;    set the Ethernet address on the board to 6 byte ID code
;
;   usage:   setaddr(s,basea,ioa);
;             char s[6];           ethernet address to use
;             int basea;           shared memory base address (unused)
;             int ioa;             io address for board (unused)
;
ifdef Microsoft
_E1SETADDR	PROC	FAR
else
E1SETADDR	PROC	FAR
endif
	ret
;	PUSH	BP
;	MOV	BP,SP
;	PUSH	SI
;	PUSH	DS
;	MOV	AX,[BP+X+2]
;	MOV	DS,AX
;	MOV	SI,[BP+X]	; address of buffer to read
;
;	MOV	CX,6
;	MOV	DX,EADDR	; get base i/o reg for setting address
;	CLD
SADDR2:
;	LODSB			; get next one
;	OUT	DX,AL		; send it
;	INC	DX		; next position
;	LOOP	SADDR2		; do 6 times
;
;	POP	DS
;	POP	SI
;	POP	BP
;	RET
ifdef Microsoft
_E1SETADDR	ENDP
else
E1SETADDR	ENDP
endif
endif       ; NOT_USED
;
;*******************************************************************
;  GETADDR
;     get the Ethernet address off of the board
;
;   usage:  getaddr(s,address,ioaddr);
;	char s[6];           will get six bytes from the PROM
;       int address;
;       int ioaddr;     (unused here) mem address and ioaddress to use
;
ifdef Microsoft
_E1GETADDR	PROC	FAR
else
E1GETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	DI
	PUSH 	ES		; save mine
	MOV	AX,[BP+X+2]	; get new one
	MOV	ES,AX           ; set new one
	MOV	DI,[BP+X]	; get pointer, es:di is ready
	;
;
;  set up all of the I/O register values in memory
;
	MOV	AX,[BP+X+6]	; i/o address -> ax
	MOV	BASEA,AX	; save base address
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EADDR	; + offset of Ethernet address
	MOV	EADDR,BX	; store address of Ethernet address
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EREC	; + offset of receive command/status
	MOV	EREC,BX		; store address of receive command status
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_ESEND	; + offset of transmit command/status
	MOV	ESEND,BX	; store address of transmit command status
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EGLOW	; + offset of general pointer low byte
	MOV	EGLOW,BX	; store address of general pointer low byte
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EGHI	; + offset of general pointer high byte
	MOV	EGHI,BX		; store address of general pointer high byte
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_ERLOW	; + offset of receive pointer low byte
	MOV	ERLOW,BX	; store address of receive pointer low byte
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_ERHI	; + offset of receive pointer high byte
	MOV	ERHI,BX		; store address of receive pointer high byte
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EPROM	; + offset of PROM window
	MOV	EPROM,BX	; store address of PROM window
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EAUX	; + offset of auxiliary command/status
	MOV	EAUX,BX		; store address of auxiliary command/status
	MOV	BX,AX		; Base address -> BX
	ADD	BX,O_EBUF	; + offset of buffer window
	MOV	EBUF,BX		; store address of buffer window
;
;
	MOV	BX,0            ; start location 
	MOV	CX,EPROM	; address window
GADDR:
	CLD
	MOUTW	EGLOW,BL,BH  	; set gp to the right value
	MIN	CX		; get value from prom address window
	STOSB                   ; put into given buffer
	INC	BX		; next position
	CMP	BX,6
	JNZ 	GADDR  		; do 6 times
	POP 	ES
	POP	DI
	POP	BP		
	xor	ax,ax
	RET
ifdef Microsoft
_E1GETADDR	ENDP
else
E1GETADDR	ENDP
endif
;
;***********************************************************************
;  ETCLOSE
;        shut it down, remove the interrupt handler
;
;  usage:  etclose();
;
;
ifdef Microsoft
_E1ETCLOSE	PROC	FAR
else
E1ETCLOSE	PROC	FAR
endif
	CLI
	MOUT	EAUX,ERESET	; Turn off all pendings, cause reset
	MOUT	EAUX,0          ; Turn off reset
;
;
;  mask out IRQ on interrupt controller
;
	MIN	IMASK		; get current mask
	OR	AL,TURNOFF	; force that bit on
	OUT	DX,AL		; send it back to controller
	STI

	CALL	DEINST		; restore old interrupt handler

	MOV	BL,OLDMASK	; get back saved setting of irq
	NOT	BL		; flip it
	CLI
	MIN	IMASK
	AND	AL,BL		; restore setting of that bit
	OUT	DX,AL
	STI	
	xor	ax,ax
	RET
ifdef Microsoft
_E1ETCLOSE	ENDP
else
E1ETCLOSE	ENDP
endif
;
;************************************************************************
;   Receive
;   This is a CPU hook for boards that must be polled before we can
;   deliver packets into the receive buffer.  (i.e. no interrupts used)
;
;   The 3COM 3C501 version uses interrupts, so this routine is a NOP
;   for this board.
;
;    usage:  recv();
;
ifdef Microsoft
_E1RECV	PROC	FAR
else
E1RECV	PROC	FAR
endif
	RET			; for compatibility with other drivers
ifdef Microsoft
_E1RECV	ENDP
else
E1RECV	ENDP
endif
;
;************************************************************************
;  XMIT         
;     send a packet to Ethernet
;     Is not interrupt driven, just call it when you need it.
;
;  usage:   xmit(packet,count)
;		char *packet;
;		int count;
;
;   Takes a packet raw, Ethernet packets start with destination address,
;   and puts it out onto the wire.  Count is the length of packet < 2048
;
;   checks for packets under the Ethernet size limit of 60 and handles them
;
ifdef Microsoft
_E1XMIT	PROC	FAR
else
E1XMIT	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	MOV	SI,[BP+X]	; offset for buffer

	MOV	AX,[BP+X+4]	; count of bytes
	MOV	CX,AX		; save a copy, might be less than 60, ok

	CMP	AX,60		; minimum length for Ether
	JNB	OKLEN
	MOV	AX,60		; make sure size at least 60
OKLEN:
	MOV	BX,2048		; total length of buffer
	SUB	BX,AX		; offset of for buffer pointer to start
	MOV	DI,BX		; save a copy of the buffer pointer
;
;  TAKE CONTROL OF THE INPUT BUFFER
;
	MOUT	EAUX,EBUS+ERIDE	; take buffer away from receiver
	MOUT	ERLOW,0		; clear receive pointer for next read
	MOUTW	EGLOW,BL,BH	; set the general purpose pointer

	MOV	DX,EBUF		; window to packet buffer
	PUSH	DS		; set up proper ds for the buffer
	MOV	AX,[BP+X+2]
	MOV	DS,AX
	CLD
FILLBUF:
	LODSB			; get value to go into buffer
	OUT	DX,AL		; put it into buffer (autoincrement)
	LOOP	FILLBUF		; do whole count

	POP	DS
;
;  packet is in buffer, ready to be sent
;
TRYAGAIN:
	MOV	BX,DI		; retrieve copy of offset pointer
	MOUTW	EGLOW,BL,BH	; set the general purpose pointer (again)
;
	MOUT	EAUX,EXMIT+ERIDE    ; tell the board to send it and start receiving
	
NOTDONEX:
	MIN	EAUX		; waiting for transmit to finish
	AND	AL,EXBUSY	; is it done yet?
	JNZ	NOTDONEX	; no, wait some more

	MOV	CX,0		; return value, ok
	MIN	ESEND		; get xmit status
	MOV	BL,AL		; save status
	AND	AL,EDTOK	; was it ok?
	JNZ	DONEX		; yes, successful xmit
;
;  handle the possible errors, return 1 on coll16
;     coll16 generally means that the network has failed
;
	MOV	AL,BL		; get copy of status back
	AND	AL,EDTC16	; check collision 16
	JNZ	RET16		; yes, network probably down
	MOV	AL,BL		; get copy back again
	AND	AL,EDTCOLL	; check for collision status
	JZ	UNK		; no, unknown problem
	MOUT	EAUX,EBUS+ERIDE		; collision, reset buffer control
	JMP	TRYAGAIN	; go for it
UNK:
	MOV	CX,2		; unknown problem return code
    JMP SHORT DONEX
RET16:
	MOV	CX,1		; failure return
DONEX:
	MOUT	EREC,EWANT	; reset receive register filter necessary
	MIN	EAUX	
	AND	AL,ERBUSY	; is it still in receive state or done?
	JNZ	DONEMIT		; not ready now, return instead

	MOV	AL,INTNUM
	CMP	AL,0BH		; two choices of int to call
	JNZ	TRYNINT
	INT	0BH		; we do have a packet, read it
    JMP SHORT DONEMIT
TRYNINT:
	CMP	AL,0DH
	JNZ	DONEMIT
	INT	0DH

DONEMIT:
	MOV	AX,CX		; put return in ax
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_E1XMIT	ENDP
else
E1XMIT	ENDP
endif
;
;*************************************************************************
;  Interrupt Handler
;  installation and deinstallation
;
;     the handler takes the receive packet out of the input buffer
;
DEINST	PROC	NEAR
	MOV	CX,SAVEIP	; get old ip from save spot
	MOV	DX,SAVECS	; get old cs from save spot
	MOV	BX,WHICHINT	; interrupt in table for 3com board
	PUSH	DS
	XOR	AX,AX		; system interrupt table
	MOV	DS,AX		
	CLI
	MOV	[BX],CX		; store old ip into the table
	INC	BX
	INC	BX		; move pointer in interrupt table
	MOV	[BX],DX		; store old cs into the table
	STI
	POP	DS
	RET
DEINST	ENDP
;
IINST	PROC	NEAR
	MOV	CS:MYDS,DS	; store for use by handler
	MOV	BX,WHICHINT	; interrupt in table for 3com board
	PUSH	DS
	XOR	AX,AX		; system interrupt table
	MOV	DS,AX		
	MOV	AX,OFFSET IHAND	; where the handler is
	CLI
	MOV	DX,[BX]		; keep copy of the ip
	MOV	[BX],AX		; store ip into the table
	INC	BX
	INC	BX		; move pointer in interrupt table
	MOV	CX,[BX]		; keep copy of the cs, too
	MOV	AX,CS
	MOV	[BX],AX		; store new cs into the table
	STI
	POP	DS
	MOV	SAVEIP,DX	; store them away
	MOV	SAVECS,CX
	RET
MYDS	DW	00H		; the data segment for this assembly code
ICNT    DB      00H
IHAND:			       	; not a public name, only handles ints
	STI
	PUSH	DS
	PUSH 	ES
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	DI
	CLD			; all moves will be forward
;
;  SET UP CORRECT DS
;
	MOV	DS,CS:MYDS		; get correct ds
ifdef Microsoft
	MOV	AX,word ptr [_BUFPT+2]	; buffer's ds
	MOV	DI,_BUFPT		; where buffer is
else
	MOV	AX,word ptr [BUFPT+2]	; buffer's ds
	MOV	DI,BUFPT		; where buffer is
endif
	MOV	ES,AX
;
;  check for buffer overrun or catching up with reader
;
;  implicit 64K max buffer, should stop before 64K anyway
;
ifdef Microsoft
	MOV	AX,_BUFBIG	; how much stuff is in buffer
	MOV	BX,_BUFLIM	; what is our size limit?
else
	MOV	AX,BUFBIG	; how much stuff is in buffer
	MOV	BX,BUFLIM	; what is our size limit?
endif
	CMP	AX,BX
	JNA	ISROOM		; we are ok
;
;  no room at the Inn.  turn off receiver
;
	MOUT	EAUX,EBUS+ERIDE	; refuse to read more packets until restarted

	MIN	EREC		; must clear interrupt

	MOV	AL,1		; set flag
	MOV	DEAF,AL		; we are now deaf, read routine must restart

    JMP SHORT ENDINT      ; can't do much, we lose packets until restarted

;
;  wrap pointer around at end, we know that we have room
;
ISROOM:
ifdef Microsoft
	MOV	DX,_BUFEND	; right before 2K safety area
else
	MOV	DX,BUFEND	; right before 2K safety area
endif
	CMP	DX,DI		; see if pointer is over limit
	JA	OKAYREAD	; we are not at wrap-around

ifdef Microsoft
	MOV	AX,_BUFORG	; wrap to here
	MOV	_BUFPT,AX	; wrap-around
else
	MOV	AX,BUFORG	; wrap to here
	MOV	BUFPT,AX	; wrap-around
endif
	MOV	DI,AX		; di also
;
;  here, DI contains where we want to put the packet.
;
OKAYREAD:

;
IREADONE:
	MOUT	EAUX,EBUS+ERIDE	; turn off receive, give buffer to bus
	MOUTW	EGLOW,0,0	; clear general purpose pointer for read
	MIN	EREC		; get status to al, clears read

	MOV	DX,ERLOW	; receive buffer pointer
	IN	AL,DX
	MOV	CL,AL		; save low byte
	INC	DX
	IN	AL,DX
	MOV	CH,AL		; save high byte

	MOV	BX,CX           ; save another copy of the length
	OR	BX,BX		; check for non-zero
	JZ	STOPINT		; no packet
	
	MOV	AX,BX		; save length in buffer, before packet
	STOSW

	MOV	DX,EBUF		; window to the data

IDOBYTES:
	IN	AL,DX		; get a byte
	STOSB			; save it to es:di
	LOOP 	IDOBYTES
;
;
;  DI now contains updated value for BUFPT, BX contains size of packet
;

ifdef Microsoft
	MOV	_BUFPT,DI	; it is here, now
	MOV	AX,_BUFBIG	; total amount of stuff in buffer
else
	MOV	BUFPT,DI	; it is here, now
	MOV	AX,BUFBIG	; total amount of stuff in buffer
endif
	ADD	AX,BX
	INC	AX
	INC	AX		; to cover the length value
ifdef Microsoft
	MOV	_BUFBIG,AX	; after adding in current packet size
else
	MOV	BUFBIG,AX	; after adding in current packet size
endif
;
;  signs that something is actually happening - used for debugging
;
;	MOV	AX,0B000H       ; screen
;	MOV	ES,AX
;	MOV	DI,3998		; lower right corner
;	INC	CS:ICNT
;	MOV	Al,CS:ICNT	; character
;	STOSB

;
;  set up to read the next packet from the net
;
STOPINT:
	MOUT	ERLOW,0		; clear receive buffer pointer
	MOUT	EAUX,EGETEM+ERIDE	; set receive bit in aux

ENDINT:

	MOUT	ICTRL,ENDOFI	; signal end of interrupt
	POP	DI
	POP	DX
	POP	CX
	POP	BX
	POP	AX
	POP	ES
	POP	DS
	IRET
IINST	ENDP

;
;*************************************************************************
;  ETUPDATE
;      update pointers and/or restart receiver when read routine has
;      already removed the current packet
;
;   usage:  etupdate();
;
ifdef Microsoft
_E1ETUPDATE	PROC	FAR
else
E1ETUPDATE	PROC	FAR
endif
	PUSH 	ES
ifdef Microsoft
	MOV	AX,word ptr [_BUFPT+2]	; establish data segment to buffer
else
	MOV	AX,word ptr [BUFPT+2]	; establish data segment to buffer
endif
	MOV	ES,AX		; put that in es
;
ifdef Microsoft
	MOV	BX,_BUFREAD	; where read pointer is now
else
	MOV	BX,BUFREAD	; where read pointer is now
endif
	MOV	DX,ES:[BX]	; get size of this packet
	INC	DX
	INC	DX		; two more for length value

	ADD	BX,DX		; increment bufread by size of packet

ifdef Microsoft
	MOV	CX,_BUFEND	; right before 2K safety area
else
	MOV	CX,BUFEND	; right before 2K safety area
endif
	CMP	BX,CX		; see if pointer is over limit
	JB	NOWRAPRD	; we are not at wrap-around
	
ifdef Microsoft
	MOV	BX,_BUFORG	; wrap to here
else
	MOV	BX,BUFORG	; wrap to here
endif
NOWRAPRD:
ifdef Microsoft
	MOV	_BUFREAD,BX	; buffer pointer has been updated
else
	MOV	BUFREAD,BX	; buffer pointer has been updated
endif

;
;  DECREMENT TOTAL BUFFER SIZE
;
	CLI			; keep interrupt handler from bothering dec
ifdef Microsoft
	MOV	CX,_BUFBIG	; size before removing packet
	SUB	CX,DX		; remove size of current packet
	MOV	_BUFBIG,CX	; put it back
else
	MOV	CX,BUFBIG	; size before removing packet
	SUB	CX,DX		; remove size of current packet
	MOV	BUFBIG,CX	; put it back
endif
	STI
;
;  IF RECEIVER IS ON, THEN CHECKING BUFLIM IS UNNECESSARY.
;
	MOV	AL,DEAF		; is the receiver turned off?
	OR	AL,AL		; 0 = reading, 1 = deaf
	JZ	ALIVE
;
;  CHECK FOR ROOM IN THE BUFFER, IF THERE IS, TURN ON RECEIVER
;
ifdef Microsoft
	MOV	AX,_BUFLIM	; what is our limit?
else
	MOV	AX,BUFLIM	; what is our limit?
endif
	CMP	CX,AX		; compare to limit
	JA	ALIVE		; not really alive, but can't turn on yet

	XOR	AL,AL
	MOV	DEAF,AL		; reset flag

	INC	OFFS		; keep count how many times this happened

	MOUT	ERLOW,0		; reset receive buffer ptr
	MOUT	EAUX,EGETEM+ERIDE	; turn on receiver

ALIVE:
	POP	ES
	RET	
ifdef Microsoft
_E1ETUPDATE	ENDP
else
E1ETUPDATE	ENDP
endif

ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END
