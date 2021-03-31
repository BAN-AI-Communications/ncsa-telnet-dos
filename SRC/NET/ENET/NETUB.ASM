;
;  Driver for Ungermann-Bass (IBM) NIC board
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
;****************************************************************************

	TITLE	NETSUPPORT -- LOW LEVEL DRIVERS FOR ETHERNET
;
;  Assembler support for interrupt-driven Ethernet I/O on the PC
;
;  Tim Krauskopf
;  National Center for Supercomputing Applications
;  9/1/87  Ungermann-Bass driver started, PC bus
;  4/88    Modified for combined - version 2.2
;
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

	NAME	NET
ifdef Microsoft
X	EQU	6
	DOSSEG
	.MODEL	LARGE
else
	INCLUDE	DOS.MAC
	SETX
endif
;
;  Equates for controlling the UB board
;
EADDR	EQU	010h		; where EPROM address is
RBUF	EQU	04000h		; where receive buffers start
TBUF	EQU	07000h		; where transmit buffers start
RPIDX	EQU	02100h		; receive page index starts here
;
;  Registers on UB board
;
TXINIT	EQU	02080h		; initiate transmit (read)
TSAMSB	EQU	02080h		; transmit high start address
TSALSB	EQU	02081h		; transmit low start address
CLRPAV	EQU	02081h		; Clear packet available (read)
INTSTAT	EQU	02082h		; interrupt and transmit status
INTCTL	EQU	02082h		; interrupt control
EPPPAV	EQU	02083h		; packet avail, and empty page ptr
FPP	EQU	02083h		; full page ptr, high bit clear=no interrupts
;
;  EDLC registers
;
TSTAT	EQU	02180h		; transmit status, write ff to clear
TMASK	EQU	02181h		; transmit mask, which ints can occur
RECSTAT	EQU	02182h		; rec status, write ff to clear
RMASK	EQU	02183h		; rec mask, which ints can occur
TMODE	EQU	02184h		; trans mode, high nyb=# of collisions
				;   enable loopback, write 00
				;   disable loopback, write 02
RMODE	EQU	02185h		; rec mode, see list
ERESET	EQU	02186h		; set 080h to reset, set 0 to clear
MADDR	EQU	02188h		; write my eaddr here,+1,+2,3,4,5

;
;  Transmit status that we are interested in
;
TDONE	EQU	01		; transmission done (when set)
TOK	EQU	02	      ; transmission done ok
;
;  mask for the packet available bit
;
PAV	EQU	080h		; packet is available
EPP	EQU	07fh		; inverse of PAV
;
;  Receive modes
;
PROMI	EQU	03		; promiscuous receive
ROFF	EQU	00		; don't receive
LMULT	EQU	01		; limited multicasts, bcast, mine
MULT	EQU	02		; mult, bcast, mine

;
;  Data segment
;
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

ICNT	DB	00h

SAVECS	DW	00H		; where to save the old interrupt ptr
SAVEIP	DW	00H
LFPP	DB	00h		; Full Page pointer
DEAF	DB	00H		; when we can't handle any more packets
OFFS	DW	00H		; how many times the handler was turned off
UBASE	DW  0b800h	; base segment for Ungermann-Bass board (jumper set)
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
	PUBLIC	_U1RECV,_U1ETOPEN,_U1ETCLOSE,_U1GETADDR
	PUBLIC	_U1XMIT,_U1ETUPDATE
else
	PSEG
	PUBLIC	U1RECV,U1ETOPEN,U1ETCLOSE,U1GETADDR
	PUBLIC	U1XMIT,U1ETUPDATE
endif

;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,address,ioaddr)
;           char s[6];       ethernet address
;           int irq;         (unused, we don't use no interrupts)
;           int address	     base mem address
;           int ioaddr       (unused for this board) io base address
;
ifdef Microsoft
_U1ETOPEN	PROC	FAR
else
U1ETOPEN	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	push	es
	mov	ax,[bp+X+6]	; parameter for
	mov	UBASE,ax	; base address
	mov	es,ax		; set to base address
;
;  reset the board by 
;  reset board, won't work until I set it back
	mov	byte ptr es:[ERESET],080h
;
;  loopback
;
	xor	ax,ax
	mov	byte ptr es:[TMODE],al
;
;  turn off receive 
	mov	byte ptr es:[RMODE],al
	mov	al,byte ptr es:[CLRPAV]	; clear PAV
;
;  clear interrupt control
	xor	ax,ax
	mov	byte ptr es:[INTCTL],al
;
;  rmask
	mov	byte ptr es:[RMASK],al
;
;  tmask
	mov	byte ptr es:[TMASK],al
;
;  rstat
	mov	byte ptr es:[RECSTAT],0ffh	; clear status
;
;  tstat
	mov	byte ptr es:[TSTAT],0ffh	; clear status
;
;  move my addr onto board
	PUSH 	DS		; save my DS
	MOV	AX,[BP+X+2]	; get new one
	MOV	DS,AX           ; set new one
	MOV	SI,[BP+X]	; get pointer, ds:si is ready
	;
	MOV	CX,6
	MOV	di,MADDR	; get base i/o reg for setting address
	CLD
	REP	MOVSB		; LOAD MY ADDR

	POP	DS		; get back DS of local data
;
;  write epp to fpp and clear PAV
	mov	al,byte ptr es:[EPPPAV]
	and	al,07fh		; mask PAV off
	mov	byte ptr es:[FPP],al	; no ints, set fpp=epp
	mov	al,byte ptr es:[CLRPAV]
;
;
;  set board back on from reset, causes xmit to loopback
	mov	es:[TSAMSB],0ff0fh	; set to zero length xmit
	mov	byte ptr es:[ERESET],00
;
;  wait for it
waitforit:
	mov	al,byte ptr es:[INTSTAT]	; status of xmit
	test	al,TDONE
	jz	waitforit
;
;  turn off loopback
	mov	byte ptr es:[TMODE],0ah		; turn on transmit
;
;  write epp to fpp and clear PAV
	mov	al,byte ptr es:[EPPPAV]
	and	al,07fh		; mask PAV off
	mov	byte ptr es:[FPP],al	; no ints, set fpp=epp
	mov	LFPP,al		; save copy of FPP
	mov	al,byte ptr es:[CLRPAV]
;
;  set to receive certain types of packets
	xor 	ax,ax		
	mov	al,MULT		; which mode
	mov	byte ptr es:[RMODE],al	; set into reg

	xor	ax,ax
	POP	ES
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_U1ETOPEN	ENDP
else
U1ETOPEN	ENDP
endif
;
;
;*******************************************************************
;  GETADDR
;     get the Ethernet address off of the board
;
;   usage:  getaddr(s,address,ioaddr);
;	char s[6];           will get six bytes from the PROM
;       int address;         segment base address
;
ifdef Microsoft
_U1GETADDR	PROC	FAR
else
U1GETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI

	mov	dx,[bp+X+4]	; get board's base addr
	PUSH 	ES		; save mine
	MOV	AX,[BP+X+2]	; get new one
	MOV	ES,AX           ; set new one
	MOV	DI,[BP+X]	; get pointer, es:di is ready
	MOV	SI,EADDR	; get ether address ptr
	;
	PUSH	DS
	mov	ds,dx		; put base addr in DS
	mov	cx,6
	CLD
	REP	MOVSB

	POP	DS

	POP 	ES
	POP	DI
	POP	SI
	POP	BP		
	RET
ifdef Microsoft
_U1GETADDR	ENDP
else
U1GETADDR	ENDP
endif
;
;***********************************************************************
;  ETCLOSE
;        shut it down
;
ifdef Microsoft
_U1ETCLOSE	PROC	FAR
else
U1ETCLOSE	PROC	FAR
endif
	RET
ifdef Microsoft
_U1ETCLOSE	ENDP
else
U1ETCLOSE	ENDP
endif
;
;
;************************************************************************
;  U1XMIT         
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
_U1XMIT	PROC	FAR
else
U1XMIT	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	push	es
	mov	ax,ubase
	mov	es,ax		; base for board
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
	mov	di,TBUF		; start of xmit buffer
	MOV	BX,2047		; total length of buffer-1
	SUB	BX,AX		; offset from beginning of buffer
	ADD	DI,BX		; add in to get buffer pointer
	MOV	BX,DI		; make a copy
;
;  check for previous xmit
xwait:
	mov	al,byte ptr es:[INTSTAT]
	and	al,3		; check TXOK and TXDONE
	cmp	al,3
	jnz	xwait		; not there yet, wait for it
;
;  move the data
	rep	movsb		; copy into buffer
;
;  set address regs and send it
	mov	byte ptr es:[TSTAT],0fh		; clear xmit status bits
	mov	byte ptr es:[TSALSB],bl		; address regs
	mov	byte ptr es:[TSAMSB],bh
	mov	al,byte ptr es:[TXINIT]	; start xmit
	xor 	ax,ax
	pop	ds
	pop	es
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_U1XMIT	ENDP
else
U1XMIT	ENDP
endif
;
;
;***********************************************************************
;  RECV
;  Get whatever packets are on the board
;
ifdef Microsoft
_U1RECV	proc	far
else
U1RECV	proc	far
endif
	push	bp
	PUSH	SI
	PUSH	DI
	push	es
ifdef Microsoft
	MOV	AX,WORD PTR [_BUFPT+2]	; buffer's ds
else
	MOV	AX,WORD PTR [BUFPT+2]	; buffer's ds
endif
ifdef Microsoft
	MOV	DI,_BUFPT	; where buffer is
else
	MOV	DI,BUFPT	; where buffer is
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
;  no room at the Inn. 
;
	JMP	ENDINT		; can't do much, we lose packets until restarted

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
	inc	di
	inc	di		; leave space for length of packet
	mov	bl,LFPP		; local copy of FPP value
	xor	bh,bh		; clear high byte
	mov	dx,ubase	; base for board
	push	ds
	mov	ds,dx

	mov	al,byte ptr ds:[EPPPAV]	; see if packet is available
	test	al,PAV		; see if bit is set
	jnz	IREADONE
	pop	ds
    JMP SHORT STOPINT

IREADONE:
	mov	dx,0		; size of packet is zero
step2:
	mov	cx,0		; size of packet segment is zero
	mov	si,bx		; save start of packet segment page # in si

step3:
	push	bx		; save LFPP value
	add	bx,RPIDX	; get into page index
	mov	al,byte ptr ds:[bx]		; get page index byte
	pop	bx

	xor	ah,ah
	push	ax		; save end of packet marker
	and	al,07fh		; mask off last packet bit
	inc	al		; size is value+1
	add	cx,ax		; making total size of packet segment
	inc	bl		; increment page pointer
	cmp	bl,05fh
	jng	nopwrap
	mov	bl,0		; wrap around at 60h (96)
nopwrap:
	mov	al,byte ptr ds:[EPPPAV]	; have we read all used pages?
	and	al,EPP
	cmp	al,bl		; is EPP = LFPP?
	jne	notend
	mov	al,byte ptr ds:[CLRPAV]	
				; clear packet available to make board contin.
notend:
	pop	ax		; get back end-of packet marker
	test	al,080h		; is it end of packet?
	jnz	dopack		; no, get next page of packet
	or	bl,bl		; test to see if packet wraps around end
	jnz	step3		; keep adding to size

dopack:
;  move the packet into our local buffer ds:si to es:di
;     cx is size, bl is page #, al is saving EOP mark, dx is accumulated size
;     of packet
	push	ax	
	push	bx		; save incremented bl
	mov	bx,si		; get back saved value from before increment
	xor	al,al		; clear low byte, load high byte
	mov	ah,bl		; multiply *256, then divide by 2 =
	shr	ax,1		; multiply * 128
	add	ax,RBUF		; add offset of rec buffer
	mov	si,ax		; this is where data resides

	add	dx,cx		; add in to size of packet

	rep	movsb		; copy the packet or portion of packet

	pop	bx		; get back incremented bl
	mov	byte ptr ds:[FPP],bl	; write full page pointer where it is now
	pop	ax
	test	al,080h		; was this the end of the packet?
	jz	step2		; if not, read next section of packet

	pop	ds		; work with local data again
;
;
;  DI now contains updated value for BUFPT, BX contains size of packet
;
	mov	LFPP,bl		; store local fpp where we need it
ifdef Microsoft
	mov	bx,_BUFPT	; get where size field for this packet goes
else
	mov	bx,BUFPT	; get where size field for this packet goes
endif
	mov	ES:[bx],dx		; put the accumulated size there

ifdef Microsoft
	MOV	_BUFPT,DI	; IT IS HERE, NOW
else
	MOV	BUFPT,DI	; IT IS HERE, NOW
endif

ifdef Microsoft
	MOV	AX,_BUFBIG	; TOTAL AMOUNT OF STUFF IN BUFFER
else
	MOV	AX,BUFBIG	; TOTAL AMOUNT OF STUFF IN BUFFER
endif
	ADD	AX,DX		; add in size of this packet
	INC	AX
	INC	AX		; TO COVER THE LENGTH VALUE
ifdef Microsoft
	MOV	_BUFBIG,AX	; AFTER ADDING IN CURRENT PACKET SIZE
else
	MOV	BUFBIG,AX	; AFTER ADDING IN CURRENT PACKET SIZE
endif
;
;  signs that something is actually happening
;
;	MOV	AX,0B000H       ; screen
;	MOV	ES,AX
;	MOV	DI,3998		; lower right corner
;	INC	ICNT
;	MOV	al,ICNT	; character
;	STOSB

;
;  set up to read the next packet from the net
;
STOPINT:

ENDINT:
	pop	es
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_U1RECV	ENDP
else
U1RECV	ENDP
endif

;
;*************************************************************************
;  ETUPDATE
;      update pointers and/or restart receiver when read routine has
;      already removed the current packet
;
ifdef Microsoft
_U1ETUPDATE	PROC	FAR
else
U1ETUPDATE	PROC	FAR
endif
	PUSH 	ES
ifdef Microsoft
	MOV	AX,WORD PTR [_BUFPT+2]	; establish data segment to buffer
else
	MOV	AX,WORD PTR [BUFPT+2]	; establish data segment to buffer
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
	INC	DX		; TWO MORE FOR LENGTH VALUE

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

	INC	OFFS		; keep count how many times this happened

ALIVE:
	POP	ES
	RET	
ifdef Microsoft
_U1ETUPDATE	ENDP
else
U1ETUPDATE	ENDP
endif

ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END
