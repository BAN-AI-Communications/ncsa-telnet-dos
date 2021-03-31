;  WD8003E driver code
;  Tim Krauskopf
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
;  Reads and writes from the 8K buffer on the WD card.
;  Started 4/11/88
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

	NAME	NET8003
ifdef MSC6
	INCLUDE NET\ENET\NET8003.INC
else
	INCLUDE NET8003.INC
endif
ifdef Microsoft
X	EQU	6
	DOSSEG
	.MODEL	LARGE
else
	INCLUDE	DOS.MAC
	SETX
endif
;
;  macros for writing to WD board
;
;***********************************************************************
;
;	Macros, from example driver
;
;***********************************************************************

;
; MACRO rd_wd8
;   Reads port specified on macro call. Leaves byte in AL.
;

rd_wd8	MACRO	port
	mov	DX, WDBASE
	add	DX, port		; DX contains address of port
	in	AL, DX			; AL contains data read from port
	ENDM

;
; MACRO wr_wd8
;   Writes byte in AL to port specified on macro call.
;

wr_wd8	MACRO	port
	mov	DX, WDBASE
	add	DX, port		; DX contains address of port
	out	DX, AL			; AL contains data to be written to port
	ENDM

;
ifdef Microsoft
;DGROUP	group	_DATA
;_DATA	segment	public 'DATA'
;	assume	DS:DGROUP
	.data
else
	DSEG
;	PUBLIC	RSTAT,BUFPT,BUFORG,BUFEND,BUFREAD,BUFBIG,BUFLIM
endif
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
;RSTAT	DB	00H     	; last status from read
;BUFBIG	DW	00H		; buffer space used
;BUFLIM	DW	05000H		; buffer space limit
;BUFPT	DW	00000H		; where buffer pointer is, initialized safely
;BUFDS	DW	0a000H		; where buffer is, ds
;BUFORG	DW	00000H		; start of buffer space
;BUFDS2	DW	0a000H		; another ds
;BUFEND	DW	06000H		; end limit of allowable buffer space
;BUFDS3	DW	0a000H
;BUFREAD	DW	00000H		; where the read pointer is
;BUFDS4	DW	0a000H

WDBASE	DW	00h		; base ioaddr
WDADD	DW	00h		; base shared mem addr
DEAF	DB	00H		; when we can't handle any more packets
OFFS	DW	00H		; how many times the handler was turned off
;
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
	PUBLIC	_WDRECV,_WDETOPEN,_WDETCLOSE,_WDGETADDR
ifdef NOT_USED
    PUBLIC  _WDSETADDR
endif
    PUBLIC  _WDXMIT,_WDETUPDATE
else
	PSEG
	PUBLIC	WDRECV,WDETOPEN,WDETCLOSE,WDGETADDR
ifdef NOT_USED
    PUBLIC  WDSETADDR
endif
    PUBLIC  WDXMIT,WDETUPDATE
endif

;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,addr,ioaddr)
;           char s[6];       ethernet address
;           int irq,addr,ioaddr;     
;                interrupt number (unused), base mem address and
;                i/o address to use
;
ifdef Microsoft
_WDETOPEN	PROC	FAR
else
WDETOPEN	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	mov	AX,[BP+X+8]		; install ioaddr
	mov	WDBASE,AX
	mov	AX,[BP+X+6]		; install shared mem addr
	mov	WDADD,AX
;
;  put shared mem address into memory select register
;
	mov	cl,9
	shr	ax,cl			; adapt for MSR reg
	wr_wd8	0			; ship it to offset 0 (MSR)

;
;  portions adapted from WD8STAR2.ASM example driver
;  Initializations as recommended by manufacturer and National Semi
;
	cld				; Clear direction flag for movs...
;
; initial the LAN Controller register
;					

; program for page 0
	mov	AL, MSK_PG0 + MSK_RD2
	wr_wd8	CMDR
; initial DCR data configuration
	mov	AL, MSK_BMS + MSK_FT10	; select FIFO threshold = 8 bytes
	wr_wd8	DCR
; clr RBCR0,1
	xor	AL, AL
	wr_wd8	RBCR0
	wr_wd8	RBCR1
; initial RCR to monitor mode
	mov	AL, MSK_MON
	wr_wd8	XRCR			; disable the rxer
; initial TCR
	xor	AL, AL			
	wr_wd8	TCR			; normal operation
; initial rev buffer ring
	mov	AL, STOP_PG
	wr_wd8	PSTOP			; init PSTOP
	mov	AL, STRT_PG		
	wr_wd8	PSTART			; init PSTART to the 1st page of ring
	wr_wd8	BNRY			; init BNRY
; clr ISR by 1's
	mov	AL, -1 			; write FF
	wr_wd8	ISR
; initial IMR
	mov	AL, 00h			; ***NCSA Telnet does not
					; ***need interrupts on

	wr_wd8	IMR			; enable interrupt
; program for page 1
	mov	AL, MSK_PG1 + MSK_RD2
	wr_wd8	CMDR
; initial physical addr
	mov	DX, WDBASE		; get board io base
	push	DS
	mov	ax,[bp+X+2]		; get seg from parms
	mov	ds,ax

	mov	CX, BPNA		; should be 6 for Ethernet
	mov	BX, [BP+X]		; ptr to adr in BX
	add	DX, PAR0		; i/o address of PAR0 in DX
lopa:
	mov	AL, [BX]		; get 1 byte into AL
	out	DX, AL			; write to PAR
	inc	BX
	inc	DX
	loop	lopa
	pop	DS

; initial multicast filter,  write all 0's  into MAR0 - MAR7
	mov	CX, 8
	mov	DX, WDBASE
	add	DX, MAR0		; i/o address of MAR0 in DX
	xor	AL, AL			
lopb:
	out	DX, AL
	inc	DX
	loop	lopb					
; initial CURR = PSTART + 1
	mov	AL, STRT_PG + 1
	wr_wd8	CURR
; program for page 0
	mov	AL, MSK_PG0 + MSK_RD2
	wr_wd8	CMDR
	
; put 8390 on line
	mov	AL, MSK_STA + MSK_RD2		; activate 8390
	wr_wd8	CMDR
; program RCR to normal operation (MSK_AB, no MSK_AM)
	mov	AL, MSK_AB			; accept broadcast
	wr_wd8	XRCR
	
;
		
	XOR	AX,AX
	POP	BP
	RET
ifdef Microsoft
_WDETOPEN	ENDP
else
WDETOPEN	ENDP
endif

ifdef NOT_USED
;
;******************************************************************
;  SETADDR
;    set the Ethernet address on the board to 6 byte ID code
;
;   usage:   setaddr(s,basea,ioa);
;             char s[6];           ethernet address to use
;             int basea;           shared memory base address 
;             int ioa;             io address for board
;
ifdef Microsoft
_WDSETADDR	PROC	FAR
else
WDSETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
;
;  not used for this board, set during etopen
;
	POP	BP
	RET
ifdef Microsoft
_WDSETADDR	ENDP
else
WDSETADDR	ENDP
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
;       int ioaddr;      mem address and ioaddress to use
;
ifdef Microsoft
_WDGETADDR	PROC	FAR
else
WDGETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	MOV	AX,[BP+X+2]	; SEG of where to put info
	MOV	DS,AX
	MOV	BX,[BP+X]	; address of where to put info
	mov	cx,6
	mov	dx,[BP+X+6]	; ioaddr for board, offset 0 for PROM addr
	add 	dx,8		; instruction added of WD8003e

getloop:
	in	al,dx
	mov	[bx],al		; store where we want
	inc	dx
	inc	bx
	loop	getloop
;
	XOR	AX,AX
	in	al,dx
	sub	al,3		; verification of board's existence
	jz	noerr		; compare went ok
	mov	ax,-1		; error return
noerr:
	POP	DS
	POP	BP		
	RET
ifdef Microsoft
_WDGETADDR	ENDP
else
WDGETADDR	ENDP
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
_WDETCLOSE	PROC	FAR
else
WDETCLOSE	PROC	FAR
endif
	RET
ifdef Microsoft
_WDETCLOSE	ENDP
else
WDETCLOSE	ENDP
endif
;
;************************************************************************
;   Receive
;   This is a CPU hook for boards that must be polled before we can
;   deliver packets into the receive buffer.  (i.e. no interrupts used)
;
;    usage:  recv();
;
ifdef Microsoft
_WDRECV	PROC	FAR
else
WDRECV	PROC	FAR
endif
	push	bp
	PUSH	SI
	PUSH	DI
	push	es
;
;  check for data which can be read
;
	mov	AL, MSK_PG1 + MSK_RD2	; read CURR reg
	wr_wd8	CMDR
	rd_wd8	CURR
	mov	BL, AL			; CURR in BL 
	mov	AL, MSK_PG0 + MSK_RD2		; read BNRY reg
	wr_wd8  CMDR
	rd_wd8	BNRY			; BNRY in AL
	add	AL, 1			; start page of frm in AL
	cmp	AL, STOP_PG		; check boundary
	jne	go_cmp
	mov	AL, STRT_PG		
go_cmp:
	cmp	AL, BL			
	jne	gotone
	jmp 	end_rx			; buff ring empty
gotone:
; ring not empty
	mov	BH, AL
	xor	BL, BL			; BX has the rx_frm pointer
	push	BX			; save the frm ptr
        mov	AX, WDADD		; shared mem base
	mov 	ES, AX			; ES has the shr seg paragraph
	mov	AL, ES:[BX]		; AL has the status byte
	test	AL, SMK_PRX		; if rx good
	jnz	readit
	jmp	fd_bnry			; rx error, drop frm by forward bnry
readit:
;
;  set up to read the next packet from the net
;
;
;  get ready for next packet
;
	cld			; moves in fwd dir
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
;  no room at the Inn. 
;
    JMP SHORT fd_bnry     ; can't do much, we lose packets until restarted

;
;  wrap pointer around at end, we know that we have room
;
ISROOM:
ifdef Microsoft
	MOV	DI,word ptr [_BUFPT]     ; where buffer is
	MOV	DX,word ptr [_BUFEND]	; right before 2K safety area
else
	MOV	DI,word ptr [BUFPT]     ; where buffer is
	MOV	DX,word ptr [BUFEND]	; right before 2K safety area
endif
	CMP	DX,DI			; see if pointer is over limit
	JA	OKAYREAD		; we are not at wrap-around

ifdef Microsoft
	MOV	AX,word ptr [_BUFORG]	; wrap to here
	MOV	word ptr [_BUFPT],AX	; wrap-around
else
	MOV	AX,word ptr [BUFORG]	; wrap to here
	MOV	word ptr [BUFPT],AX	; wrap-around
endif
	MOV	DI,AX			; di also

OKAYREAD:
;
;
;  start the copy of the new packet
;  pointer to the shared memory offset is in BX
;  At this offset, you will find:
;    1 byte - read status, usually 21h
;    1 byte - pointer, page # of next packet
;    2 bytes - length of data in packet, swapped for you already
;    n bytes - that many bytes of Ethernet packet spread
;       over n div 256 pages (allowing four lost bytes in first packet)
;
;
	pop	si		; get packet pointer back into si
	push	si		; restore for fd_bnry to read
;
;  save regs while moving packet to buffer
;  set up ds for buffer, even though we switch it later
;
	push	es
	push	ds
ifdef Microsoft
	MOV	AX,word ptr [_BUFPT+2]	; buffer's ds
else
	MOV	AX,word ptr [BUFPT+2]	; buffer's ds
endif
	mov	ds,ax
;
;  here, DS:DI contains where we want to put the packet.
;
newpkt:
	add	si,2		; offset for length field
	mov	dx,es:[si]	; value of length of recd packet

	mov	ds:[di],dx		; put the accumulated size there
	inc	si
	inc	si
	inc	di
	inc	di		; now it is the data pointer
;
;
;  Actually move the data
;    DX has packet size in bytes
;    ES:SI has the source pointer  } need to switch
;    DS:DI has the dest pointer    } es and ds
;    Remember, 256 byte pages wrap around at STOP_PG and there
;    are max 252 bytes in the first page
;
	mov	cx,dx
	cmp	cx,252
	jng	shrt
	mov	cx,252		; first page len
shrt:
	mov	ax,ds
	mov	bx,es
	mov	ds,bx
	mov	es,ax		; swap them

	mov	bx,dx		; save a copy of data length

mvpg:				; start of page move loop
	sub	dx,cx
	shr	cx,1		; convert to words
	jnc	iseven
	movsb			; move odd one if needed
iseven:
	rep	movsw		; move all words in one page

	cmp	dx,0		; how many left to move?
	jng	donepg
	mov	cx,dx
	cmp	cx,256
	jng	shrtr
	mov	cx,256		; one more page
shrtr:
	mov	ax,si		; look at source page
	cmp	ah,STOP_PG
	jl	mvpg
	mov	ah,STRT_PG	; wrap around at this page boundary
	mov	si,ax		; put back in si for rest of packet
	jmp	mvpg

donepg:

	pop	ds
	pop	es		; put regs back so ES is shared mem

;
; update the pointer and length in the buffer
;  DI already points just past end of data just placed there
;
ifdef Microsoft
	MOV	word ptr [_BUFPT],di	; it is here, now
	MOV	AX,word ptr [_BUFBIG]	; total amount of stuff in buffer
else
	MOV	word ptr [BUFPT],di	; it is here, now
	MOV	AX,word ptr [BUFBIG]	; total amount of stuff in buffer
endif
	ADD	AX,BX		; add in size of this packet
	INC	AX
	INC	AX		; to cover the length value
ifdef Microsoft
	MOV	word ptr [_BUFBIG],AX	; after adding in current packet size
else
	MOV	word ptr [BUFBIG],AX	; after adding in current packet size
endif
;
;
;  signs that something is actually happening
;
;	push	es
;	MOV	AX,0B000H       ; screen
;	MOV	ES,AX
;	MOV	DI,3998		; lower right corner
;	INC	cs:ICNT
;	MOV	al,cs:ICNT	; character
;	STOSB
;	pop	es
;


; drop bad frame by forwarding the BNRY register
;  or just normal BNRY update after frame read
;
fd_bnry:				; drop frm by forward BNRY
	pop	BX			; restore frm ptr in BX
	add	BX, 1
	mov	AL, ES:[BX]		; next frm start page in AL
	sub	AL, 1			; new BNRY in AL
	cmp	AL, STRT_PG		; check boundary
	jge	wrbnry
	mov	AL, STOP_PG - 1
wrbnry:
	wr_wd8	BNRY

end_rx:
	pop	es
	POP	DI
	POP	SI
	POP	BP

	RET			; for compatibility with other drivers
ICNT	db	0
ifdef Microsoft
_WDRECV	ENDP
else
WDRECV	ENDP
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
_WDXMIT	PROC	FAR
else
WDXMIT	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	cld
	push	es
	PUSH	DS		; set up proper ds for the buffer
;
	mov	dx,WDADD	; shared memory address in dx
	mov	es,dx		; use es for this
;
;
;  move packet into position, set up regs
;
	MOV	AX,[BP+X+2]
	MOV	DS,AX
	MOV	SI,[BP+X]	; offset for buffer

	MOV	AX,[BP+X+4]	; count of bytes
	MOV	CX,AX		; save a copy, might be less than 60, ok

	CMP	AX,60		; minimum length for Ether
	JNB	OKLEN
	MOV	AX,60		; make sure size at least 60
OKLEN:
;
;  Copy packet into transmit buffer
;  xmit buffer starts at offset zero in shared mem
;
	push	ax		; xmit size here, CX has data size
	mov	di,0		; set di to start of xmit buffer, 0 page
	shr	cx,1
	jnc	evenx
	movsb
evenx:
	rep	movsw		; copy all data into xmit buf
;
;  set up xmit length registers
;
	pop	ax
	pop	ds			; get back  DS for wr_wd8 macro
; set up TBCR0,1					 
	wr_wd8	TBCR0			; lower byte to TBCR0
	mov	AL, AH
	wr_wd8	TBCR1			; higher byte to TBCR1

; set page number to page 0
;
	xor	al,al			; page number

; set up TPSR
	wr_wd8	TPSR			; write start page into TPSR

; issue tx command
	mov	AL, MSK_TXP + MSK_RD2
	wr_wd8	CMDR			; start xmit
;
;
;  check to see if the last packet xmitted ok
;
	xor	cx,cx
waitxmit:
	rd_wd8	CMDR		; command register
	test	al,MSK_TXP	; xmit bit
	jz	oktogo		; xmit is finished
	loop	waitxmit	; waiting for xmit to complete
	mov	ax,-1
    jmp SHORT getout
oktogo:
	xor	ax,ax
;
; go back for more
;
getout:
	pop	es
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_WDXMIT	ENDP
else
WDXMIT	ENDP
endif
;
;
;*************************************************************************
;  ETUPDATE
;      update pointers and/or restart receiver when read routine has
;      already removed the current packet
;
;   usage:  etupdate();
;
ifdef Microsoft
_WDETUPDATE	PROC	FAR
else
WDETUPDATE	PROC	FAR
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
NOWRAPRD:
	MOV	_BUFREAD,BX	; buffer pointer has been updated
else
	MOV	BX,BUFORG	; wrap to here
NOWRAPRD:
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

	INC	OFFS		; keep count how many times this happened

;
;  turn receiver back on
;

ALIVE:
	POP	ES
	RET	
ifdef Microsoft
_WDETUPDATE	ENDP
else
WDETUPDATE	ENDP
endif

ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END
