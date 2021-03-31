;  3COM 3C501 driver code
;  Tim Krauskopf
;
;  This version is virtually unused, replaced by the driver version 
;  which handles all of the different ioaddrs and interrupts.
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
	INCLUDE	DOS.MAC
	SETX
endif
	NAME	NET
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

ifdef Microsoft
;DGROUP	group	_DATA
;_DATA	segment	public 'DATA'
;	assume	DS:DGROUP
	.data
else
	DSEG
endif
;
;  The pointers below are actually DWORDs but we access them two
;  bytes at a time.
;
ifdef Microsoft
    PUBLIC  _RSTAT,_BUFPT,_BUFORG,_BUFEND,_BUFREAD,_BUFBIG,_BUFLIM
ifdef OLD_WAY
    PUBLIC  OFFS
endif
_RSTAT	DB	00H     	; last status from read
_BUFBIG	DW	00H		; buffer space used
_BUFLIM	DW	05000H		; buffer space limit
_BUFPT	DW	00000H		; where buffer pointer is, initialized safely
_BUFDS	DW	0a000H		; where buffer is, ds
_BUFORG	DW	00000H		; start of buffer space
_BUFDS2	DW	0a000H		; another ds
_BUFEND	DW	06000H		; end limit of allowable buffer space
_BUFDS3	DW	0a000H
_BUFREAD	DW	00000H		; where the read pointer is
_BUFDS4	DW	0a000H
else
	EXTRN	RSTAT:BYTE	; last status from read
	EXTRN	BUFPT:WORD	; current buffer pointer
	EXTRN	BUFORG:WORD	; pointer to beginning of buffer
	EXTRN	BUFEND:WORD	; pointer to end of buffer
	EXTRN	BUFREAD:WORD	; pointer to where program is reading
	EXTRN	BUFBIG:WORD	; integer, how many bytes we have
	EXTRN	BUFLIM:WORD	; integer, max bytes we can have
endif

SAVECS	DW	00H		; where to save the old interrupt ptr
SAVEIP	DW	00H
OLDMASK	DB	00H		; save interrupt controller mask
DEAF	DB	00H		; when we can't handle any more packets
ifdef OLD_WAY
OFFS	DW	00H		; how many times the handler was turned off
endif
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
    PUBLIC  _E3RECV,_E3ETOPEN,_E3ETCLOSE,_E3GETADDR
ifdef NOT_USED
    PUBLIC  _E3SETADDR
endif
    PUBLIC  _E3XMIT,_E3ETUPDATE
else
	PSEG
	PUBLIC	E3RECV,E3ETOPEN,E3ETCLOSE,E3GETADDR
ifdef NOT_USED
    PUBLIC  E3SETADDR
endif
    PUBLIC  E3XMIT,E3ETUPDATE
endif

;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,addr,ioaddr)
;           char s[6];       ethernet address
;           int irq,addr,ioaddr;     
;                interrupt number, base mem address (unused) and
;                i/o address to use (currently unused)
;
;
ifdef Microsoft
_E3ETOPEN	PROC	FAR
else
E3ETOPEN	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	MOUT	EAUX,ERESET	; reset the board
	MOUT	EAUX,0          ; Clear the reset bit, otherwise keeps resetting
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
;  probably never going to use DMA for this board - unreliable so far
;
USE1:
;
;  install the interrupt handler
;
	CALL	IINST		; do the patching of the interrupt table
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
_E3ETOPEN	ENDP
else
E3ETOPEN	ENDP
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
_E3SETADDR	PROC	FAR
else
E3SETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	PUSH	SI
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

	POP	SI
	POP	DS
	POP	BP
	RET
ifdef Microsoft
_E3SETADDR	ENDP
else
E3SETADDR	ENDP
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
_E3GETADDR	PROC	FAR
else
E3GETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	DI
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
	POP	DI
	POP	BP		
	RET
ifdef Microsoft
_E3GETADDR	ENDP
else
E3GETADDR	ENDP
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
_E3ETCLOSE	PROC	FAR
else
E3ETCLOSE	PROC	FAR
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
	RET
ifdef Microsoft
_E3ETCLOSE	ENDP
else
E3ETCLOSE	ENDP
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
_E3RECV	PROC	FAR
else
E3RECV	PROC	FAR
endif
	RET			; for compatibility with other drivers
ifdef Microsoft
_E3RECV	ENDP
else
E3RECV	ENDP
endif
ifdef unused_code
;      This version is unused.  It is a polled receive which is nearly
;      useless with this board.  It hasn't been debugged yet, either.
;             char *where;      at least 2048 bytes of room
;             returns # bytes in packet, -1 if no packet available
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
	MOV	STAT,AL
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
	MOV	STAT,AL		; KEEP LAST STATUS BYTE
;
;  set up to read the next packet from the net
;
	MOUT	ERLOW,0		; clear receive buffer pointer
	MOUT	EAUX,EGETEM+ERIDE	; set receive bit in aux

	MOV	AX,BX		; return value is # of bytes
	POP	ES
	POP	BP
	RET
ifdef Microsoft
_E3RECV	ENDP
else
E3RECV	ENDP
endif
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
_E3XMIT	PROC	FAR
else
E3XMIT	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
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
_E3XMIT	ENDP
else
E3XMIT	ENDP
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
    MOV DI,_BUFPT       ; where buffer is
else
	MOV	AX,word ptr [BUFPT+2]	; buffer's ds
    MOV DI,BUFPT        ; where buffer is
endif
	MOV	ES,AX
;
;  check for buffer overrun or catching up with reader
;
;  implicit 64K max buffer, should stop before 64K anyway
;
ifdef Microsoft
	MOV	AX,_BUFBIG	; how much stuff is in buffer
else
	MOV	AX,BUFBIG	; how much stuff is in buffer
endif
ifdef Microsoft
	MOV	BX,_BUFLIM	; what is our size limit?
else
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
else
	MOV	AX,BUFORG	; wrap to here
endif
ifdef Microsoft
	MOV	_BUFPT,AX	; wrap-around
else
	MOV	BUFPT,AX	; wrap-around
endif
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
else
	MOV	BUFPT,DI	; it is here, now
endif

ifdef Microsoft
	MOV	AX,_BUFBIG	; total amount of stuff in buffer
else
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
_E3ETUPDATE	PROC	FAR
else
E3ETUPDATE	PROC	FAR
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
else
	MOV	CX,BUFBIG	; size before removing packet
endif
	SUB	CX,DX		; remove size of current packet
ifdef Microsoft
	MOV	_BUFBIG,CX	; put it back
else
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

ifdef OLD_WAY
	INC	OFFS		; keep count how many times this happened
endif

	MOUT	ERLOW,0		; reset receive buffer ptr
	MOUT	EAUX,EGETEM+ERIDE	; turn on receiver

ALIVE:
	POP	ES
	RET	
ifdef Microsoft
_E3ETUPDATE	ENDP
else
E3ETUPDATE	ENDP
endif

ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END
