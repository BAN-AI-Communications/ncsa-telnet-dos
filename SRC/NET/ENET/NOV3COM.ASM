;  3COM 3C501 driver code
;  Tim Krauskopf
;  with mods by Dave Thomson, Carleton University
;
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
;*																			*
;*		Novell compatibility modifications by Dave Thomson                  *
;*		Copyright (c) 1987
;*		Carleton University													*
;*		Department of Computer Science										*
;*		Ottawa CANADA														*
;*		K1S 5B6																*
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
	NAME	NET
	INCLUDE	DOS.MAC
	SETX
;
;  Equates for controlling the 3COM board
;
ICTRL	EQU	020H		; 8259 interrupt control register
IMASK	EQU	021H		; 8259 interrupt mask register
BASEA	EQU	300H		; Base I/O address on PC I/O bus
ENDOFI	EQU	020H		; end-of-interrupt
;
;  Controller registers
;
EADDR	EQU	BASEA+0H	; Network address for hardware checking
;   takes six bytes, this is the address that the Ethernet board will
;   match to find packets on the net.  (0-5h)
;
EREC	EQU	BASEA+6H	; Receive status (read)
				; Receive command (write)
ESEND	EQU	BASEA+7H	; Transmit status (read)
				; Transmit command (write)
EGLOW	EQU	BASEA+8H	; General purpose pointer for R/W to 
				; packet buffer, low byte
EGHI	EQU	BASEA+9H	; high byte, total of 11 bits for 2K buffer
ERLOW	EQU	BASEA+0AH	; Receive pointer, set by board (read) low byte
				; Receive buffer pointer clear (write)
ERHI	EQU	BASEA+0BH	; high byte of Receive pointer
EPROM	EQU	BASEA+0CH	; PROM window address
		;     DH ??
EAUX	EQU	BASEA+0EH	; Auxiliary Status (read)
				; Aux Command (write)
EBUF	EQU	BASEA+0FH	; Buffer window (where to I/O to net)
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

	DSEG
	PUBLIC	RSTAT,BUFPT,BUFORG,BUFEND,BUFREAD,BUFBIG,BUFLIM,OFFS
RSTAT	DB	00H     	; last status from read
BUFBIG	DW	00H		; buffer space used
BUFLIM	DW	05000H		; buffer space limit
BUFPT	DW	00000H		; where buffer pointer is, initialized safely
BUFDS	DW	0a000H		; where buffer is, ds
BUFORG	DW	00000H		; start of buffer space
BUFDS2	DW	0a000H		; another ds
BUFEND	DW	06000H		; end limit of allowable buffer space
BUFDS3	DW	0a000H
BUFREAD	DW	00000H		; where the read pointer is
BUFDS4	DW	0a000H

;DAVE - additions for Novell compatibility
VECTOR	DB	4 DUP (0)	;used to chain to previously installed driver
CHAINING	DW	0		;flag to indicate if we are chaining
EN_IP	EQU		800H
EN_ARP	EQU		806H

SAVECS	DW	00H		; where to save the old interrupt ptr
SAVEIP	DW	00H
OLDMASK	DB	00H		; save interrupt controller mask
DEAF	DB	00H		; when we can't handle any more packets
OFFS	DW	00H		; how many times the handler was turned off
;
;  use variables to access IRQ3 or IRQ5
;  3 is COM2, 5 is LPT2
;
;WHICHINT  EQU	4*0Bh		; Interrupt for interrupt I/O on IRQ3
;WHICHINT  EQU	4*0Dh		; Interrupt for interrupt I/O on IRQ5
;TURNOFF  EQU  	08H  		; IRQ3 bit mask for 8259 controller (1<<3)
;TURNOFF  EQU  	020H        	; IRQ5 bit mask for 8259 controller (1<<5)
;TURNON	EQU	0F7H		; IRQ3 enable bit mask for 8259 controller
;TURNON	EQU	0DFH		; IRQ5 enable bit mask for 8259 controller
INTNUM	DB	0BH		; Defaults to IRQ3, interrupt handler 0bh
WHICHINT  DW	4*0BH		; ETOPEN can change these values
TURNOFF	DB	08H
TURNON	DB	0F7H
	ENDDS
;
;
;
;   The subroutines to call from C
;
	PSEG
	PUBLIC	RECV,ETOPEN,ETCLOSE,GETADDR,SETADDR,XMIT,ETUPDATE

;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,dma)
;           char s[6];       ethernet address
;           int irq,dma;     interrupt number and dma channel to use
;
ETOPEN	PROC	FAR
	PUSH	BP
	MOV	BP,SP

;DAVE - determine if the board should be reset i.e. is it already hooked

	MOV		AH,30H
	INT		21H		;get DOS version
	CMP		AL,3	;must be 3.xx
	JE		ET_VERSION_OK1

ET_VERSION_ERROR:
	MOV		AX,0FFFFH
	RET

ET_VERSION_OK1:
	CMP		AH,10	;3.10?
	JE		ET_DOS310
	CMP		AH,20
	JE		ET_DOS320	;3.20?
	JMP		ET_VERSION_ERROR
ET_DOS310:
	MOV		DX,0F000H	;this vector defaults to rom segment under 3.10
    JMP     SHORT ET_GOT_SEGMENT
ET_DOS320:
	MOV		DX,0070H	;default is segment 0070H under dos 3.20

ET_GOT_SEGMENT:
	PUSH	DX			;save the segment for later checking agains vector
;
;  check the parameters for interrupt and dma
;
	MOV	AX,[BP+X+4]	; interrupt number
	CMP	AL,5		; If not 5, then use 3
	JNZ	USE3
	MOV	INTNUM,0DH	; Interrupt # for handler for IRQ5
	MOV	WHICHINT,4*0DH	; Interrupt handler location in vector table
	MOV	TURNOFF,020H	; mask for interrupt controller for IRQ5
	MOV	TURNON,0DFH	; opposite mask, for interrupt controller
USE3:
	MOV	AX,[BP+X+6]	; dma channel to use
	CMP	AL,3		; if not 3, then use 1
	JNZ	USE1
;
; Add DMA values for channel 3 here later
;
USE1:

;DAVE - save mask in case we don't go through initialization code
;
	MIN	IMASK		; get current int enable mask
	MOV	OLDMASK,AL

;DAVE - interrupts must be off for the entire installation
;		in case the card is in use by another driver
;
	CLI
	CALL	IINST		; DO THE PATCHING OF THE INTERRUPT TABLE
	POP		DX
	CMP		DX,SAVECS	;compare the default segment with the actual segment
						;the vector pointed to before we hooked it

	JNE		ET_CHAINING	;vector already hooked - don't initialize it

	MOUT	EAUX,ERESET	; reset the board
	MOUT	EAUX,0          ; Clear the reset bit, otherwise keeps resetting
;
;  install the interrupt handler
;
;
;  set up the net address
;
	PUSH 	DS		; save mine
	MOV	AX,[BP+X+2]	; get new one
	MOV	DS,AX           ; set new one
	MOV	SI,[BP+X]	; get pointer, ds:si is ready
	;
	MOV	CX,6
	MOV	DX,EADDR	; get base i/o reg for setting address
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

;DAVE - interrupts already off
;	CLI

	MOUT	EAUX,EGETEM+ERIDE	; Set for receive, interrupts

	MIN	IMASK		; get current int enable mask
	MOV	OLDMASK,AL
	MOV	BL,AL		; save a copy
	AND	AL,TURNON	; force bit for etherlink board off
	OUT	DX,AL		; put back the byte, IRQ enabled

;DAVE - zero chaining flag
;
	XOR	AX,AX
	MOV	CHAINING,AX	;set chaining flag	

    JMP SHORT ET_SKIP_INIT

ET_CHAINING:
	MOV	AX,1
	MOV	CHAINING,AX	;set chaining flag

ET_SKIP_INIT:
	STI

;DAVE - it appears we don't need this code and it will interfere with the
;		chaining case

;	AND	BL,TURNOFF	; isolate this bit only from oldmask
;	MOV	OLDMASK,BL	; save it
;
	POP	BP
	XOR	AX,AX
	RET
ETOPEN	ENDP
;
;******************************************************************
;  SETADDR
;    set the Ethernet address on the board to 6 byte ID code
SETADDR	PROC	FAR
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	MOV	AX,[BP+X+2]
	MOV	DS,AX
	MOV	SI,[BP+X]	; address of buffer to read
;
	MOV	CX,6
	MOV	DX,EADDR	; get base i/o reg for setting address
	CLD
SADDR2:
	LODSB			; get next one
	OUT	DX,AL		; send it
	INC	DX		; next position
	LOOP	SADDR2		; do 6 times

	POP	DS
	POP	BP
	RET
SETADDR	ENDP
;
;*******************************************************************
;  GETADDR
;     get the Ethernet address off of the board
;
;   usage:  getaddr(s);
;	char s[6];           will get six bytes from the PROM
;
GETADDR	PROC	FAR
	PUSH	BP
	MOV	BP,SP
	PUSH 	ES		; save mine
	MOV	AX,[BP+X+2]	; get new one
	MOV	ES,AX           ; set new one
	MOV	DI,[BP+X]	; get pointer, es:di is ready
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
	POP	BP		
	RET
GETADDR	ENDP
;
;***********************************************************************
;  ETCLOSE
;        shut it down
;
ETCLOSE	PROC	FAR

;DAVE - interrupts must be off for duration of close
;
	CLI

;	MOUT	EAUX,ERESET	; Turn off all pendings, cause reset
;	MOUT	EAUX,0          ; Turn off reset
;
;   Make sure that DMA transfers are not outstanding  (later)
;
;
;  mask out IRQ on interrupt controller
;
;	MIN	IMASK		; get current mask
;	OR	AL,TURNOFF	; force that bit on
;	OUT	DX,AL		; send it back to controller
;	STI

	CALL	DEINST		; restore old interrupt handler

;	MOV	BL,OLDMASK	; get back saved setting of irq
;	NOT	BL		; flip it
;	CLI			;should be already off
;	MIN	IMASK
;	AND	AL,BL		; restore setting of that bit
	MOV	AL,OLDMASK	;DAVE - restore to old mask
	OUT	DX,AL
	STI			;DAVE - turn interrupts on
	RET
ETCLOSE	ENDP
;
;************************************************************************
;   Receive
;      This version is unused.  It is a polled receive which is nearly
;  useless with this board.  It hasn't been debugged yet, either.
;      get those packets into a buffer
;
;    usage:  size = recv(where);
;             char *where;      at least 2048 bytes of room
;             returns # bytes in packet, -1 if no packet available
;
RECV	PROC	FAR
	PUSH	BP
	MOV	BP,SP
	PUSH	ES
	MOV	AX,[BP+X+2]	; get new es value
	MOV	ES,AX
	MOV	DI,[BP+X]	; set di for later movement
	;
	MOV	CX,10		; give it a few tries

FINDONE:
	MIN	EAUX		; get status to al
	MOV	RSTAT,AL
	AND	AL,ERBUSY	; is it still in receive state or done?
	JZ	READONE		; done, can read it
	LOOP	FINDONE

	MOV	AX,-1		; no packet yet, return
	POP	ES
	POP	BP
	RET
;
READONE:
	MOUT	EAUX,EBUS	; turn off receive, give buffer to bus
	MOUTW	EGLOW,0,0	; clear general purpose pointer for read

	MOV	DX,ERLOW	; receive buffer pointer
	IN	AL,DX
	MOV	CL,AL		; save low byte
	INC	DX
	IN	AL,DX
	MOV	CH,AL		; save high byte

	MOV	BX,CX           ; save another copy of the length

	MOV	DX,EBUF		; window to the data

DOBYTES:
	IN	AL,DX		; get a byte
	STOSB			; save it to es:di
	LOOP 	DOBYTES

	MIN	EREC		; get status to al, clears read
	MOV	RSTAT,AL		; KEEP LAST STATUS BYTE
;
;  set up to read the next packet from the net
;
	MOUT	ERLOW,0		; clear receive buffer pointer
	MOUT	EAUX,EGETEM+ERIDE	; set receive bit in aux

	MOV	AX,BX		; return value is # of bytes
	POP	ES
	POP	BP
	RET
RECV	ENDP
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
XMIT	PROC	FAR
	PUSH	BP
	MOV	BP,SP

;DAVE - we must turn interrupts of for the duration of the transmit so that
;		other drivers won't try to access the card
;
	CLI	
	PUSH	DS		; set up proper ds for the buffer
	MOV	AX,[BP+X+2]
	MOV	DS,AX
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
	CLD
FILLBUF:
	LODSB			; get value to go into buffer
	OUT	DX,AL		; put it into buffer (autoincrement)
	LOOP	FILLBUF		; do whole count

	POP	DS
;
;  PACKET IS IN BUFFER, READY TO BE SENT
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

;	MOV	AL,INTNUM
;	CMP	AL,0BH		; two choices of int to call
;	JNZ	TRYNINT
;	INT	0BH		; we do have a packet, read it
;	JMP	DONEMIT
;TRYNINT:
;	CMP	AL,0DH
;	JNZ	DONEMIT
;	INT	0DH

DONEMIT:
	MOV	AX,CX		; put return in ax

;DAVE - turn interrupts back on
;
	STI
	POP	BP
	RET
XMIT	ENDP
;
;*************************************************************************
;  INTERRUPT HANDLER, INSTALLATION AND DEINSTALLATION
;  IHAND
;     take the receive packet out of the input buffer
;
;DAVE - modified to not turn interrupts on - should be off before the call
;		the call
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

;DAVE - interrupts must be off the entire deinstall process
;
;	STI

	POP	DS
	RET
DEINST	ENDP
;
;DAVE - modified to not turn interrupts on - must be off before call
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

;DAVE - interrupts must be off for the entire install process
;
;	STI

	POP	DS
	MOV	SAVEIP,DX	; store them away
	MOV	SAVECS,CX

;DAVE - save the CS:DX in the jump vector in the correct order
;
	LEA	SI,VECTOR
	MOV	WORD PTR [SI],DX
	MOV	WORD PTR [SI+2],CX

	RET

MYDS	DW	00H		; the data segment for this assembly code
ICNT    DB      00H

IHAND:			       	; not a public name, only handles ints
;DAVE - mosified the way the "deaf" mode works to set a flag but keep the
;		card in receive mode so that we can still pass packets we don't
;		want to other installed drivers (if any)
;
;DAVE - interrupts must be off - we must have exclusive access to the card
;		for the duration of the service routine
;
;	STI

	PUSH	DS
	PUSH 	ES
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	DI
	PUSH	SI
	CLD			; all moves will be forward
;
;  SET UP CORRECT DS
;
	MOV	DS,CS:MYDS	; get correct ds
	MOV	AX,BUFDS	; buffer's ds
	MOV	DI,BUFPT	; where buffer is
	MOV	ES,AX
;
;  check for buffer overrun or catching up with reader
;
;  implicit 64K max buffer, should stop before 64K anyway
;
	MOV	AX,BUFBIG	; how much stuff is in buffer
	MOV	BX,BUFLIM	; what is our size limit?
	CMP	AX,BX
	JNA	ISROOM		; we are ok
;
;  no room at the Inn.  turn off receiver
;
;	MOUT	EAUX,EBUS+ERIDE	; refuse to read more packets until restarted

	MIN	EREC		; must clear interrupt

;DAVE - "deaf" mode no longer turns the card off
;
	MOV	AL,1		; SET FLAG
	MOV	DEAF,AL		; WE ARE NOW DEAF, READ ROUTINE MUST RESTART

CHAIN:
	MOV	AX,CHAINING
	OR	AX,AX
	JZ	STOPINT		;not chaining so reset receive mode and exit

	LEA	SI,VECTOR
	PUSHF
	CALL	DWORD PTR [SI]	;chain to previously installed driver
							;we assume that the other driver leaves the
							;card in receive mode
    JMP SHORT ENDINT

;
;  wrap pointer around at end, we know that we have room
;
ISROOM:
	MOV	DX,BUFEND	; right before 2K safety area
	CMP	DX,DI		; see if pointer is over limit
	JA	OKAYREAD	; we are not at wrap-around

	MOV	AX,BUFORG	; wrap to here
	MOV	BUFPT,AX	; wrap-around
	MOV	DI,AX		; di also
;
;  here, DI contains where we want to put the packet.
;
OKAYREAD:

;	MOV	CX,10		; give it a few tries

IFINDONE:
;	MIN	EAUX		; get status to al
;	AND	AL,ERBUSY	; is it still in receive state or done?
;	JZ	IREADONE	; done, can read it
;	LOOP	IFINDONE
;	MIN	EREC
;	AND	AL,ESTALE
;	JZ	IREADONE
;	jmp	ireadone

;	MOV	AX,0		; no packet yet, spurious int, return
;	STOSB

;	MIN	EREC		; clear interrupt condition
;	MIN	ESEND		; in case it was an xmit spurious int
;	JMP	STOPINT
;
IREADONE:
	MOUT	EAUX,EBUS+ERIDE	; turn off receive, give buffer to bus
;	MOUTW	EGLOW,0,0	; clear general purpose pointer for read

;DAVE - we now want a first peek at offset 12-13 the ethernet type field
;
	MOUTW	EGLOW,12,0	; clear general purpose pointer for read
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
	
	MOV	AX,BX		; SAVE LENGTH IN BUFFER, BEFORE PACKET
	STOSW

;DAVE - check the ethernet type to see if we are interested in it
;
	MOV	DX,EBUF		; window to the data
	IN	AL,DX
	MOV	AH,AL
	IN	AL,DX

	PUSH	AX		;type in AX

;set up the card for the packet read in case we want it
;
	MOUTW	EGLOW,0,0	; clear general purpose pointer for read
	MOV	DX,EBUF		; window to the data

	POP		AX		;get type back
	CMP	AX,EN_IP	;IP packet?
	JE	IDOBYTES	;if so get it
	CMP	AX,EN_ARP	;ARP?
	JE	IDOBYTES
	JMP	CHAIN		;otherwise chain to the previously installed driver
	

IDOBYTES:
	IN	AL,DX		; get a byte
	STOSB			; save it to es:di
	LOOP 	IDOBYTES
;
;
;  DI now contains updated value for BUFPT, BX contains size of packet
;
	MOV	BUFPT,DI	; IT IS HERE, NOW

	MOV	AX,BUFBIG	; TOTAL AMOUNT OF STUFF IN BUFFER
	ADD	AX,BX
	INC	AX
	INC	AX		; TO COVER THE LENGTH VALUE
	MOV	BUFBIG,AX	; AFTER ADDING IN CURRENT PACKET SIZE
;
;  signs that something is actually happening
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
	POP	SI
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
;
;DAVE - this routine no longer restarts the card - it is always on
;		thgis routine just resets the "deaf" flag
;
ETUPDATE	PROC	FAR
	PUSH 	ES
	MOV	AX,BUFDS	; establish data segment to buffer
	MOV	ES,AX		; put that in es
;
	MOV	BX,BUFREAD	; where read pointer is now
	MOV	DX,ES:[BX]	; get size of this packet
	INC	DX
	INC	DX		; TWO MORE FOR LENGTH VALUE

	ADD	BX,DX		; increment bufread by size of packet

	MOV	CX,BUFEND	; right before 2K safety area
	CMP	BX,CX		; see if pointer is over limit
	JB	NOWRAPRD	; we are not at wrap-around
	
	MOV	BX,BUFORG	; wrap to here
NOWRAPRD:
	MOV	BUFREAD,BX	; buffer pointer has been updated

;
;  DECREMENT TOTAL BUFFER SIZE
;
	CLI			; keep interrupt handler from bothering dec
	MOV	CX,BUFBIG	; size before removing packet
	SUB	CX,DX		; remove size of current packet
	MOV	BUFBIG,CX	; put it back
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
	MOV	AX,BUFLIM	; what is our limit?
	CMP	CX,AX		; compare to limit
	JA	ALIVE		; not really alive, but can't turn on yet

	XOR	AL,AL
	MOV	DEAF,AL		; reset flag

	INC	OFFS		; keep count how many times this happened

;DAVE - already on
;
;	MOUT	ERLOW,0		; reset receive buffer ptr
;	MOUT	EAUX,EGETEM+ERIDE	; turn on receiver

ALIVE:
	POP	ES
	RET	
ETUPDATE	ENDP

	ENDPS
	END
