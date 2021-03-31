;  3C503 Driver
;
;	Kurt Mahan ( based on WD8003 by Tim Krauskopf )
;
;****************************************************************************
;*                                                                          *
;*                                                                          *
;*      part of NCSA Telnet                                                 *
;*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
;*		neat other things by Kurt Mahan
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

ifdef Microsoft
X	EQU	6
	DOSSEG
	.MODEL	LARGE
else
    	NAME	NET503
  	INCLUDE	DOS.MAC
  	SETX
endif

;
;	EQUATES for the 3C503 board
;

CMDR	equ	00h			  ; ports on the 8390 ( page 0 )
PSTART	equ	01h
PSTOP	equ	02h
BNRY	equ	03h
TPSR	equ	04h
TBCR0	equ	05h
TBCR1	equ	06h
ISR	equ	07h
RSAR0	equ	08h
RSAR1	equ	09h
RBCR0	equ	0ah
RBCR1	equ	0bh
XRCR	equ	0ch
TCR	equ	0dh
DCR	equ	0eh
IMR	equ	0fh

PAR0	equ	01h			  ; ports on the 8390 ( page 1 )
PAR1	equ	02h
PAR2	equ	03h
PAR3	equ	04h
PAR4	equ	05h
PAR5	equ	06h
CURR	equ	07h
MAR0	equ	08h
MAR1	equ	09h
MAR2	equ	0ah
MAR3	equ	0bh
MAR4	equ	0ch
MAR5	equ	0dh
MAR6	equ	0eh
MAR7	equ	0fh

PSTR	equ	400h			  ; ports on the gate array
PSPR	equ	401h
DQTR	equ	402h
BCFR	equ	403h
PCFR	equ	404h
GACFR	equ	405h
CTRL	equ	406h
STREG	equ	407h
IDCFR	equ	408h
DAMSB	equ	409h
DALSM	equ	40ah
VPTR2	equ	40bh
VPTR1	equ	40ch
VPTR0	equ	40dh
RFMSB	equ	40eh
RFLSB	equ	40fh

STRT_PG	equ	26h			  ; start at page 6
STOP_PG	equ	40h			  ; end at page 31
TSTRT_PG equ	20h			  ; transmit start page

;
;  macros for writing to NET503 board
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
	push	dx
	mov	DX, WDBASE
	add	DX, port		; DX contains address of port
	in	AL, DX			; AL contains data read from port
	pop	dx
	ENDM

;
; MACRO wr_wd8
;   Writes byte in AL to port specified on macro call.
;

wr_wd8	MACRO	port
	push	dx
	mov	DX, WDBASE
	add	DX, port		; DX contains address of port
	out	DX, AL			; AL contains data to be written to port
	pop	dx
	ENDM

;
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

WDBASE	DW	00h		; base ioaddr
WDADD	DW	00h		; base shared mem addr
DEAF	DB	00H		; when we can't handle any more packets
OFFS	DW	00H		; how many times the handler was turned off
WIRE	DB	02h		; thin as the default
;
ifdef Microsoft
;_DATA	ends
else
	ENDDS
endif
;
;   The subroutines to call from C
;
ifdef Microsoft
;_TEXT	segment	public	'CODE'
;	assume CS:_TEXT
	.code
	PUBLIC	_E4RECV,_E4ETOPEN,_E4ETCLOSE,_E4GETADDR
	PUBLIC	_E4SETADDR,_E4XMIT,_E4ETUPDATE
	PUBLIC _E4SETWIRE
else
	PSEG
	PUBLIC	E4RECV,E4ETOPEN,E4ETCLOSE,E4GETADDR
	PUBLIC	E4SETADDR,E4XMIT,E4ETUPDATE, E4SETWIRE
endif


;*****************************************************************
;
; E4SETWIRE
;
; WHAT TYPE OF WIRE ARE WE USING?  THIN OR THICK
;
; USAGE: E4SETWIRE(INT WIRE)
;

ifdef Microsoft
_E4SETWIRE	PROC FAR
else
E4SETWIRE	PROC FAR
endif

	PUSH BP
	MOV  BP,SP
	push ds
	mov  ax,seg WIRE
	mov  ds,ax
	XOR  AX,AX
	MOV  AL,[BP+X]		; GET WIRE TYPE
	MOV  WIRE,AL 		; STORE IT!
	pop  ds
	POP  BP
	RET

ifdef	Microsoft
_E4SETWIRE	ENDP
else
E4SETWIRE	ENDP
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
ifdef	Microsoft
_E4ETOPEN	PROC	FAR
else
E4ETOPEN	PROC	FAR
endif
;
;	init addresses and things
;

	PUSH	BP
	MOV	BP,SP
	mov	AX,[BP+X+8]		; install ioaddr
	mov	WDBASE,AX
	mov	AX,[BP+X+6]		; install shared mem addr
	mov	WDADD,AX
;
;	initialize the gate array
;
	push	ds
	mov	ax,seg WIRE
	mov	ds,ax
	mov	al,1			; reset the board -- onboard xcvr
	or 	al,WIRE
	wr_wd8	CTRL
	mov	al,0			; done with reset -- still onboard xcvr
	or	al,WIRE
	wr_wd8	CTRL
	wr_wd8	CTRL
	pop	ds
;
;	set the vector pointers ( VPTR0 - VPTR2 ) so that the shared memory
;	won't go byebye on us accidentally ( point to ffff00h )
;
	mov	al,0ffh
	wr_wd8	VPTR2
	wr_wd8	VPTR1
	xor	al,al
	wr_wd8	VPTR0
;
;	lookup the shared memory address ( such a neat card, huh -- we can
;	actually read it up from the gate array )
;
;	PCFR : high bits = ( 7 .. 0 )
;				bit 7 = dc00
;				bit 6 = d800
;				bit 5 = cc00
;				bit 4 = c800
;
;
	rd_wd8	PCFR			; read prom config register
	test	al,80h			; dc00?
	jz	eto100
	mov	WDADD,0da00h
	jmp	etok
eto100:	test	al,40h			; d800?
	jz	eto200
	mov	WDADD,0d600h
	jmp	etok
eto200:	test	al,20h			; cc00?
	jz	eto300
	mov	WDADD,0ca00h
	jmp	etok
eto300:	test	al,10h			; c800?
	jz	eto400
	mov	WDADD,0c600h
	jmp	etok
;
eto400:	mov	ax,-1			; mem not enabled
	pop	bp
	ret
;
;	ok, WDBASE is setup as the base memory address
;
etok :	mov	al,0c9h			; dis ints, ram sel, bank 0
	wr_wd8	GACFR
;
;	setup the start/stop pages 
;
	mov	al,STRT_PG		; starting page
	wr_wd8	PSTR
	mov	al,STOP_PG		; ending page
	wr_wd8	PSPR
;
;	setup the interrupt and dma request stuff
;
	xor	al,10h			; no ints, no dma channels
	wr_wd8	IDCFR			; interrupt/dma channel ctrl
;
;	set DRQ timer to 8
;
	mov	al,8
	wr_wd8	DQTR
;
;	set dma msb to 20h
;
	mov	al,TSTRT_PG		; transmit start page
	wr_wd8	DAMSB
	xor	al,al			; 0
	wr_wd8	DALSM
;
	cld
;
; initialize the LAN Controller register
;					
	push	ds
	mov	ax,seg WIRE
	mov	ds,ax
	mov	al,0
	or	al,WIRE
	wr_wd8	CTRL			; make sure its switched in
	pop	ds
;
; program for page 0
;
	mov	AL, 21h			  ; page 0, abort any pending dma
	wr_wd8	CMDR
;
;	clear interrupt regs
;
	mov	al, 0ffh
	wr_wd8	ISR
;
; initial DCR data configuration
;
	mov	AL, 48h			   ; burst dma, fifo thresh = 8 bytes
	wr_wd8	DCR
;
; initial TCR
;
	xor	AL, AL			
	wr_wd8	TCR			; normal operation
;
; initial RCR to monitor mode
;
	mov	AL, 20h				; monitor mode
	wr_wd8	XRCR
;
; set page start/page stop/ boundary
;
	mov	AL,STRT_PG
	wr_wd8	PSTART
	mov	al,STOP_PG
	wr_wd8	PSTOP
	mov	al,STRT_PG		; go with the WD style of things
	wr_wd8	BNRY
;
; program for page 1
;
	mov	AL, 60h	 		  ; page 1, abort any pending dma
	wr_wd8	CMDR
;
; initial physical addr
;
	mov	DX, WDBASE		; get board io base
	push	DS
	mov	ax,[bp+X+2]		; get seg from parms
	mov	ds,ax

	mov	CX, 6			; should be 6 for Ethernet
	mov	BX, [BP+X]		; ptr to adr in BX
	add	DX, PAR0		; i/o address of PAR0 in DX
lopa:
	mov	AL, [BX]		; get 1 byte into AL
	out	DX, AL			; write to PAR
	inc	BX
	inc	DX
	loop	lopa
	pop	DS
;
; initial multicast filter,  write all 0's  into MAR0 - MAR7
;
	mov	CX, 8
	mov	DX, WDBASE
	add	DX, MAR0		; i/o address of MAR0 in DX
	xor	AL, AL			
lopb:
	out	DX, AL
	inc	DX
	loop	lopb					
;
;	set CURR page
;
	mov	al,STRT_PG+1
	wr_wd8	CURR
;
; program for page 0
;
	mov	al,21h			; page 0, stop any dma stuff
	wr_wd8	CMDR
;
; initial IMR
;
	xor	al,al			; ***NCSA Telnet does not need interrupts
	wr_wd8	IMR			; enable interrupt
	mov	al,0ffh
	wr_wd8	ISR
;
;	clear byte counts
;
	xor	AL, AL
	wr_wd8	RBCR0
	wr_wd8	RBCR1
;	
; put 8390 on line
;
	mov	al,22h			; start up the 8390
	wr_wd8	CMDR
;
; program RCR to normal operation (MSK_AB, no MSK_AM)
;
	mov	al,4				; accept broadcast packets
	wr_wd8	XRCR
;
	mov	al,0c9h
	wr_wd8	GACFR
;
;	I'm happy to keep GACFR at c9 ( no interrupts )
;
;	return NO ERROR
;
	XOR	AX,AX
	POP	BP
	RET
;
ifdef	Microsoft
_E4ETOPEN	ENDP
else
E4ETOPEN	ENDP
endif
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
ifdef	Microsoft
_E4SETADDR	PROC	FAR
else
E4SETADDR	PROC	FAR
endif

	PUSH	BP
	MOV	BP,SP
;
;  not used for this board, set during etopen
;
	POP	BP
	RET
ifdef	Microsoft
_E4SETADDR	ENDP
else
E4SETADDR	ENDP
endif
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
ifdef	Microsoft
_E4GETADDR	PROC	FAR
else
E4GETADDR	PROC	FAR
endif

	PUSH	BP
	MOV	BP,SP
	PUSH	DS
	MOV	AX,[BP+X+2]	; SEG of where to put info
	MOV	DS,AX
	MOV	BX,[BP+X]	; address of where to put info
	mov	cx,6
	mov	dx,[BP+X+6]	; ioaddr for board
;
;	set the gate array to look at the lower 16 bytes of the prom
;
	push	ds
	mov	ax,seg WIRE
	mov	ds,ax
	mov	al,1
	or	al,WIRE
	pop	ds
	push	dx
	add	dx,CTRL
	out	dx,al
	mov	al,2
	out	dx,al
	mov	al,6
	out	dx,al
	
	pop	dx
	push	dx
;
getloop:
	in	al,dx
	mov	[bx],al		; store where we want
	inc	dx
	inc	bx
	loop	getloop
;
;	reset the gate array to normal mapping
;
	pop	dx
	push	ds
	mov	ax,seg WIRE
	mov	ds,ax
	mov	al,0		; still onboard xcvr 
	or	al,WIRE
	pop	ds
	add	dx,CTRL
	out	dx,al
	POP	DS
	POP	BP		
	RET
ifdef	Microsoft
_E4GETADDR	ENDP
else
E4GETADDR	ENDP
endif

;
;***********************************************************************
;  ETCLOSE
;        shut it down, remove the interrupt handler
;
;  usage:  etclose();
;
;
ifdef	Microsoft
_E4ETCLOSE	PROC	FAR
else
E4ETCLOSE	PROC	FAR
endif

	RET
ifdef	Microsoft
_E4ETCLOSE	ENDP
else
E4ETCLOSE	ENDP
endif
;
;************************************************************************
;   Receive
;   This is a CPU hook for boards that must be polled before we can
;   deliver packets into the receive buffer.  (i.e. no interrupts used)
;
;    usage:  recv();
;
ifdef	Microsoft
_E4RECV	PROC	FAR
else
E4RECV	PROC	FAR
endif

	push	bp
	push	es
;
;  check for data which can be read
;
	rd_wd8	XRCR			; read the Rx Status Register
	and	al,01h			; check for a packet
	jnz	snore			; none -- go onto something else
	jmp	end_rx
snore:
;
;	ok, the chip thinks we have a packet to read
;
	mov	AL, 61h			; page 1
	wr_wd8	CMDR
	rd_wd8	CURR
	mov	BL, AL			; CURR in BL 
;
	mov	AL, 20h			; page 0
	wr_wd8  CMDR
	rd_wd8	BNRY			; BNRY in AL
;
	add	AL, 1			; start page of frm in AL
	cmp	AL, STOP_PG		; check boundary
	jne	go_cmp
	mov	AL, STRT_PG		
go_cmp:
	cmp	AL, BL			
	jne	gotone
	jmp 	end_rx			; buff ring empty
gotone:
;
; ring not empty
;
	mov	BH, AL
	xor	BL, BL			; BX has the rx_frm pointer
	push	BX			; save the frm ptr
        mov	AX, WDADD		; shared mem base
	mov 	ES, AX			; ES has the shr seg paragraph
	mov	AL, ES:[BX]		; AL has the status byte
;	test	AL, SMK_PRX		; if rx good
	test	al,1
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
ifdef	Microsoft
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
	JMP	fd_bnry		; can't do much, we lose packets until restarted

;
;  wrap pointer around at end, we know that we have room
;
ISROOM:
ifdef	Microsoft
	MOV	DI,word ptr [_BUFPT]     ; where buffer is
	MOV	DX,word ptr [_BUFEND]	; right before 2K safety area
else
	MOV	DI,word ptr [BUFPT]     ; where buffer is
	MOV	DX,word ptr [BUFEND]	; right before 2K safety area
endif

	CMP	DX,DI			; see if pointer is over limit
	JA	OKAYREAD		; we are not at wrap-around

ifdef	Microsoft
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
ifdef	Microsoft
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

	mov	[di],dx		; put the accumulated size there
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
ifdef	Microsoft
	MOV	word ptr [_BUFPT],di	; it is here, now
	MOV	AX,word ptr [_BUFBIG]	; total amount of stuff in buffer
else
	MOV	word ptr [BUFPT],di	; it is here, now
	MOV	AX,word ptr [BUFBIG]	; total amount of stuff in buffer
endif
	ADD	AX,BX		; add in size of this packet
	INC	AX
	INC	AX		; to cover the length value
ifdef	Microsoft
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
	POP	BP

	RET			; for compatibility with other drivers
ICNT	db	0
ifdef	Microsoft
_E4RECV	ENDP
else
E4RECV	ENDP
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
ifdef	Microsoft
_E4XMIT	PROC	FAR
else
E4XMIT	PROC	FAR
endif

	PUSH	BP
	MOV	BP,SP
	push	es
	PUSH	DS		; set up proper ds for the buffer
;
;	ok, let's hangout here waiting for the transmitter
;
	mov	bx,8000h	; adequate timeout
twait:	rd_wd8	CMDR		; read up the command reg
	and	al,04h		; xmit flag
	jz    	tfree
	dec	bx
	jnz	twait		; keep going till timeout
	mov	ax,-1		; bummer, it hates me
	pop	ds
	pop	es
	pop	bp
	ret
tfree:
	cld
;
	mov	al,0c9h
	wr_wd8	GACFR
;
	mov	ax,WDADD	; shared memory address in ax
	mov	es,ax		; use es for this
;
;  move packet into position, set up regs
;
	MOV	AX,[BP+X+2]	; get data ds
	MOV	DS,AX
	MOV	SI,[BP+X]	; DS:SI points to data buffer

	MOV    	CX,[BP+X+4]	; count of bytes
	CMP	CX,60		; minimum length for Ether
	JNB	OKLEN
	MOV	CX,60		; make sure size at least 60
OKLEN:
;
;  Copy Packet : 
;
;	DS:SI = real data
;	ES    = shared memory address
;	CX    = number of bytes
;
	mov	al,0c9h		; reset the gate array
	wr_wd8	GACFR
;
	push	cx		; save xmit length
	xor	ax,ax
	mov	ah,TSTRT_PG	; starting page number
	mov	di,ax		; ES:DI = shared mem buff start
	shr	cx,1		; gonna do 16 bits
	jnc	evenx
	movsb			; pick up odd byte
evenx:
	rep	movsw		; copy all data into xmit buf
;
	mov	al,0
	wr_wd8	TCR
;
;  set up xmit length registers
;
	pop	cx		; len restored in cx
	pop	ds
	mov	al,cl		; length
	wr_wd8	TBCR0			; lower byte to TBCR0
	mov	al,ch
	wr_wd8	TBCR1			; higher byte to TBCR1
;
; set page number
;
	mov	al,TSTRT_PG
	wr_wd8	TPSR			; write start page into TPSR
;
; issue tx command
;
	mov	al,24h
	wr_wd8	CMDR			; start xmit
;
;  check to see if the last packet xmitted ok
;
	xor	cx,cx			; set a timeout
;
waitxmit:
;
	rd_wd8	CMDR		; command register
	and	al,4		; xmit bit
	jz	oktogo		; xmit is finished
	loop	waitxmit	; waiting for xmit to complete
	mov	ax,-1
	jmp	getout
oktogo:
	xor	ax,ax
;
; go back for more
;
getout:
	pop	es
	POP	BP
	RET
ifdef	Microsoft
_E4XMIT	ENDP
else
E4XMIT	ENDP
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
ifdef	Microsoft
_E4ETUPDATE	PROC	FAR
else
E4ETUPDATE	PROC	FAR
endif

	PUSH 	ES
ifdef	Microsoft
	MOV	AX,word ptr [_BUFPT+2]	; establish data segment to buffer
else
	MOV	AX,word ptr [BUFPT+2]	; establish data segment to buffer
endif
	MOV	ES,AX		; put that in es
;
ifdef	Microsoft
	MOV	BX,_BUFREAD	; where read pointer is now
else
	MOV	BX,BUFREAD	; where read pointer is now
endif
	MOV	DX,ES:[BX]	; get size of this packet
	INC	DX
	INC	DX		; two more for length value

	ADD	BX,DX		; increment bufread by size of packet

ifdef	Microsoft
	MOV	CX,_BUFEND	; right before 2K safety area
else
	MOV	CX,BUFEND	; right before 2K safety area
endif
	CMP	BX,CX		; see if pointer is over limit
	JB	NOWRAPRD	; we are not at wrap-around

ifdef	Microsoft	
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
ifdef	Microsoft
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
ifdef	Microsoft
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

ifdef	Microsoft
_E4ETUPDATE	ENDP
else
E4ETUPDATE	ENDP
endif

ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END
