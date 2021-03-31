;  3COM 3C523 driver code
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
;  Assembler support for Ethernet I/O on the PC
;
;  Tim Krauskopf
;  9/1/87  Ungermann-Bass driver started, PC bus
;  9/14/87 MICOM NI5210 driver started, PC bus
;  4/22/88 3COM 3C523 driver adapted from MICOM 82586 driver, MCA bus
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

	NAME	MNET
ifdef Microsoft
X	EQU	6
	DOSSEG
	.MODEL	LARGE
else
	INCLUDE	DOS.MAC
	SETX
endif

;
;  CA macro
;
DOCA	MACRO	FLGS
	mov	dx,IOADDR
	add	dx,IOC
	mov	al,FLGS+CA
	out	dx,al
	mov	al,FLGS
	jmp	$+2
	jmp	$+2
	jmp	$+2
	out	dx,al
	ENDM
;
;  Equates for controlling the 3c523 board
;
;  I/O addresses, seventh address controls everything
;
;  First six addresses are the EPROM board Ether address (read)
;
IOC	EQU	6
;
;  Register bits
;
GO	EQU	83h		; no reset plus bank select
CA	EQU	40h		; CA bit
LBK	EQU	20h		; loopback bit
ZRAM	EQU	03h		; memory bank enable
;
;  other bits are interrupts, not needed
;

;
;  Structure elements specific to the Intel 82586 chip
;
BDBASE	EQU	1874+0c000h		; base address for 30 buffer descriptors
BUFBASE	EQU	2174+0c000h		; base address for 30 200 byte buffers
BDSTAT	EQU	0		; status word in BD
BDLINK	EQU	2		; 16pointer to next BD
BDPTR	EQU	4		; 24pointer to actual buffer
BDSIZE	EQU	8		; size of the buffer
;
SCB	EQU	10+0c000h		; system control block base
SSTAT	EQU	0		; status word for SCB
SCOM	EQU	2		; command word in SCB
SCBL	EQU	4		; 16pointer to command block list
SRFA	EQU	6		; 16pointer to receive frame list
SERRS	EQU	8		; 4 words of error counts
;
FDBASE	EQU	1214+0c000h		; base addr for 30 frame descriptors
FDSTAT	EQU	0		; status word for frame
FDEOL	EQU	2		; end of FD list flag
FDLINK	EQU	4		; 16pointer to next FD
FDPTR	EQU	6		; 16pointer to list of BD's
;
TSTAT	EQU	0		; status word for xmit
TCOM	EQU	2		; command to transmit
TLINK	EQU	4		; 16pointer to next command (always ffff)
TPTR	EQU	6		; 16pointer to xmit TBD
TTRIES	EQU	8		; number of transmit retries
;
SCPTR	EQU	03ff6h+0c000h		; hardwired address for SCP
ISCPTR	EQU	03feeh+0c000h		; my address for ISCP, points to SCB
CCBPTR	EQU	26+0c000h		; offset of configure command block
TCBPTR	EQU	44+0c000h		; xmit CB offset
TBDPTR	EQU	60+0c000h		; xmit BD offset
TBUFPTR	EQU	68+0c000h		; xmit buffer offset
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
FIRSTFD	dw	FDBASE		; start of FD queue
LASTFD	DW	0		; end of the FD chain
LASTBD	DW	0		; end of the BD chain
IOADDR	DW	02300h		; I/O address for card (POS set)
UBASE	DW  	0bc00h		; base segment for board (POS set)
;
;  data for configuring and setting up the MICOM board
;
;  chip always looks at SCP for config info which points to ISCP for the
;  pointer to the CONTROL BLOCK which handles everything from there.
;  Kind of indirect, but it works.
;
SCP	DB	0		; bus use flag
	DB	5 DUP(0)	; unused
	DW	ISCPTR		; 24pointer to ISCP offset
	DW	0		; high part
;
; Intermediate SCP
;
ISCP	DW	1		; busy flag
	DW	SCB		; 16pointer to SCB
	DW	0,0		; base for all 16 pointers, lo, hi
				; board is hardwired to 0 for these values
;
; Configuration block for 82586, this comprises one config command
;  Parameters taken from MICOM driver
;
CBCONF	DW	0		; status word
	DW	8002H		; end of command list + configure command
	DW	0ffffh		; link to next command (not used)
	DW	080CH		; fifo=8, byte count=C
	DW	2E00H		; important! Addr (AL) not inserted on the fly!
	DW	6000H		; IFS = 60h
	DW	0F200H		; retry=F, slot time=200h
	DW	0		; flags, set to 1 for promiscuous
	DW	40H		; min frame length=40h
;
; CB for xmit, followed by BD for xmit, copied together
;
TCB	DW	0		; status word
	DW	08004H		; command word for xmit + EL
	DW	0ffffh		; no command link
	DW	TBDPTR		; 16pointer to xmit BD
	DW	0,0,0,0		; no addressing used here
;
; BD template for xmit
TBD	DW	0
	DW	0		; next BD pointer, unused
	DW	TBUFPTR		; 24pointer to xmit buffer
	DW	0		; high part of pointer

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
	PUBLIC	_E2RECV,_E2ETOPEN,_E2ETCLOSE,_E2GETADDR
	PUBLIC	_E2XMIT,_E2ETUPDATE
else
	PSEG
	PUBLIC	E2RECV,E2ETOPEN,E2ETCLOSE,E2GETADDR
	PUBLIC	E2XMIT,E2ETUPDATE
endif

;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,address,ioaddr)
;           char s[6];       ethernet address
;           int irq;         (unused, we don't use no interrupts)
;           int address	     base mem address
;           int ioaddr       io base address
;
ifdef Microsoft
_E2ETOPEN	PROC	FAR
else
E2ETOPEN	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	push	es
	mov	ax,[bp+X+6]	; second parameter
	sub	ax,0c00h	; set back to match 82586 addressing
	mov	UBASE,ax	; base address
;	mov	ax,UBASE	; get a copy (temporary)
	mov	es,ax		; set to base address
	mov	ax,[bp+X+8]	; get I/O address
	mov	IOADDR,ax	; save a copy
;	mov	ax,IOADDR	; get a copy (temporary)
;
;  Set the POS bit for card enable?
;  The section which hasn't been written here should query the POS,
;  set the card enable bit, and look up the base and ioaddrs so that
;  the user won't have to install them in the configuration file.
;  That's why we have PS/2s - but I got lazy (tk)
;

;
;  Bank select, so that the memory will be available
;  Bit settings also leave interrupts off, don't need them
;
	mov	dx,ax		; IO reg base
	add	dx,IOC		; control register
	mov	al,23h		; reset, plus enable bank 3, plus LBK
	out	dx,al

;
;  Initialize 82586
;
	DOCA	ZRAM+LBK	; hit the board with a reset
;
;  Install 16K SCP
;
nop
nop
nop
	mov	si,offset SCP	; get pre-set values
	mov	di,SCPTR		; offset for 16K board
	mov	cx,5		; 5 words
	rep	movsw		; install SCP
;
;  Intermediate SCP
;
	mov	si,offset ISCP	; addr of pre-set values
	mov	di,ISCPTR
	mov	cx,4		; 4 words
	rep	movsw		; install ISCP

;
;  configure 82586
;
	mov	si,offset CBCONF	; configure command
	mov	di,CCBPTR		; where command will reside
	mov	cx,9
	rep	movsw			; copy to board
;
;  issue the configure command
;
	mov	ax,word ptr es:[SCB+SSTAT]	; get old status
	and	ax,0f000h
	or	ax,0100h
	mov	word ptr es:[SCB+SCOM],ax	; do-command command
	mov	word ptr es:[SCB+SSTAT],0	; status word
	mov	word ptr es:[CCBPTR],0		; status word
	mov	word ptr es:[SCB+SCBL],CCBPTR	; where conf command is
	mov	word ptr es:[SCB+SERRS],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+2],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+4],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+6],0	; zero errs field

	DOCA	LBK+GO			; kick CA
	DOCA	LBK+GO			; kick CA

	xor	cx,cx			; timeout
waitconf:
	mov	ax,word ptr es:[CCBPTR]	; get status word
	test	ax,08000h
	loopz	waitconf
	jnz	confok
	mov	ax,-1			; timeout problem
	pop	es
	POP	DI
	POP	SI
	pop	bp
	ret
;
confok:
	and	ax,0f000h		; ack flags
	mov	word ptr es:[SCB+SCOM],ax	; ack bits

	DOCA	LBK+GO

	xor	cx,cx			; timeout
waitcack:
	mov	ax,word ptr es:[SCB+SCOM]	; get command word
	or	ax,ax
	loopnz	waitcack
	jz	cackok
	mov	ax,-1			; timeout problem
	pop	es
	POP	DI
	POP	SI
	pop	bp
	ret
;
cackok:
	
;
;  Next step, load our address into the board
;     reuses the space that the configure command used, with different command 
;
	mov	di,CCBPTR		; start of config command block
	xor	ax,ax
	stosw				; zero status word for commmand
	mov	ax,8001h		; IA setup command + EL
	stosw
	xor	ax,ax
	dec	ax
	stosw				; set link value to -1 (unused)

;  move my addr onto board location inside IA command
;  es:di is already set
;
	PUSH 	DS		; save my DS
	MOV	AX,[BP+X+2]	; get new one
	MOV	DS,AX           ; set new one
	MOV	SI,[BP+X]	; get pointer, ds:si is ready
	;
	movsw
	movsw
	movsw
	POP	DS		; get back DS of local data
;
;  start the IA setup command, 
;
	mov	word ptr es:[SCB+SCOM],0100h	; do-command command
	mov	word ptr es:[SCB+SSTAT],0	; clear status

	DOCA	LBK+GO			; kick CA, with loopback

	xor	cx,cx			; timeout
waitia:
	mov	ax,word ptr es:[SCB+SSTAT]	; get status word
	or	ax,ax
	loopz	waitia
	jnz	iaok
	mov	ax,-2			; timeout problem
	pop	es
	POP	DI
	POP	SI
	pop	bp
	ret
;
iaok:
	and	ax,0f000h		; ack flags
	mov	word ptr es:[SCB+SCOM],ax	; ack bits

	DOCA	LBK+GO

	xor	cx,cx			; timeout
waitiack:
	mov	ax,word ptr es:[SCB+SCOM]	; get command word
	or	ax,ax
	loopnz	waitiack
	jz	iackok
	mov	ax,-1			; timeout problem
	pop	es
	POP	DI
	POP	SI
	pop	bp
	ret
;
iackok:
;
;  IA sent, setup all of the other data structures on the board
;  start with xmit command descriptors
;
	mov	si,offset TCB		; template for xmit
	mov	di,TCBPTR		; where it goes on board
	mov	cx,12			; copies CB and BD for xmit
	rep	movsw
;
;  Set up frame and buffer descriptors, 30 each
;
	mov	cx,30			; # of FDs
	mov	di,FDBASE		; base addr for FDs
fdloop:
	xor	ax,ax
	mov	bx,di			; save pointer
	stosw				; clear status wd
	stosw				; clear EL field
	add	bx,22			; points to next one
	mov	es:[di],bx		; put in link ptr
	inc	di
	inc	di
	dec	ax
	stosw				; clear BD ptr to -1
	add	di,14
	loop	fdloop
;
	sub	di,20			; point back to last EL field
	mov	ax,08000h		; end of list
	stosw				; put into last FD
	sub	di,4			; back to beginning of last FD
	mov	LASTFD,di		; save the pointer
	mov	word ptr es:[di+FDLINK],FDBASE	; make list circular, from last to first
	
;
	mov	ax,BDBASE		; first BD
	mov	word ptr es:[FDBASE+FDPTR],ax	; put it in the first FD frame
;
;  now BDs
	mov	cx,30
	mov	di,BDBASE		; start of BD area
	mov	dx,BUFBASE		; start of buffer area
bdloop:
	xor	ax,ax
	mov	bx,di			; save pointer
	stosw				; zero status field
	add	bx,10			; point to next record
	mov	es:[di],bx		; put in link ptr
	inc	di
	inc	di
	mov	es:[di],dx		; address of buffer, lo part
	inc	di
	inc	di
	stosw				; zero out high part
	mov	ax,200
	stosw				; store length field	
	add	dx,ax			; add in length of buffer, updates ptr
	loop	bdloop
;
	sub	di,2			; back to last BD size field
	mov	ax,08000h+200		; end of list + 200
	stosw				; mark end of list
	sub	di,8			; back to last BDLINK field
	mov	ax,BDBASE
	stosw				; put link to beginning of list here
	sub	di,4			; back to beginning of last BD
	mov	LASTBD,di		; save pointer to end of list
;
;  minor detail, but important
;  Change SCB command block pointer to setup for xmit commands
;      = only commands needed when operational
;
	mov	word ptr es:[SCB+SCBL],TCBPTR	; where xmit command is
;
;  configure to connect to network
;
;	mov	dx,IOADDR
;	add	dx,IOC			; enable network
;	mov	al,GO			; network setting
;	out	dx,al			;
;
;  Start the RU, doesn't need CB, only SCB parms.
;   command, to start receiving
;
	mov	word ptr es:[SCB],0		; clear status word
	mov	word ptr es:[SCB+SRFA],FDBASE	; set to frame descriptors
	mov	word ptr es:[SCB+SCOM],010h	; start RU

	DOCA	GO			; kick CA
;
;  don't wait for response, we are done
;
	xor	ax,ax
	POP	ES
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_E2ETOPEN	ENDP
else
E2ETOPEN	ENDP
endif
;
;
;*******************************************************************
;  GETADDR
;     get the Ethernet address off of the board
;
;   usage:  getaddr(s,address,ioaddr);
;	char s[6];           will get six bytes from the PROM
;
ifdef Microsoft
_E2GETADDR	PROC	FAR
else
E2GETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	DI

	mov	dx,[BP+X+6]	; get board's base io addr
	PUSH 	ES		; save mine
	MOV	AX,[BP+X+2]	; get new one
	MOV	ES,AX           ; set new one
	MOV	DI,[BP+X]	; get pointer, es:di is ready
	;
	mov	cx,6
	CLD
getonee:
	in	al,dx		; get a byte of the EPROM address
	STOSB			; put it away
	inc	dx		; next register
	loop getonee		; go back for rest

	xor	ax,ax
	POP 	ES
	POP	DI
	POP	BP		
	RET
ifdef Microsoft
_E2GETADDR	ENDP
else
E2GETADDR	ENDP
endif
;
;***********************************************************************
;  ETCLOSE
;        shut it down if necessary
;        MICOM board never interrupts, so we can leave it running.
;        Who cares, right?
;
ifdef Microsoft
_E2ETCLOSE	PROC	FAR
	RET
_E2ETCLOSE	ENDP
else
E2ETCLOSE	PROC	FAR
	RET
E2ETCLOSE	ENDP
endif
;
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
_E2XMIT	PROC	FAR
else
E2XMIT	PROC	FAR
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

	MOV	DX,[BP+X+4]	; count of bytes
	MOV	CX,DX		; save a copy, might be less than 60, ok

	CMP	DX,60		; minimum length for Ether
	JNB	OKLEN
	MOV	DX,60		; make sure size at least 60
OKLEN:
	mov	di,TBUFPTR	; start of xmit buffer

;
;  check for previous xmit
xwait:
	mov	bx,word ptr es:[SCB+SCOM]	; is command zeroed yet?
	or	bx,bx
	jnz	xwait		; not there yet, wait for it
;
;  move the data
	rep	movsb		; copy into buffer
;
;  put the correct size into the TDB
;
	or	dx,08000h	; end of frame bit flag
	mov	word ptr es:[TBDPTR],dx		; store it
	mov	word ptr es:[TCBPTR],0		; zero status wd
	mov	word ptr es:[TCBPTR+TCOM],08004h; xmit command in TCB
	mov	word ptr es:[SCB+SCOM],0100h	; execute command
;	test	bx,0100h			; suspended?
;	jz	nosus
;	mov	word ptr es:[SCB+SCOM],0400h	; stop command
;nosus:
	pop	ds			; get back my ds

	DOCA	GO			; kick CA

	xor 	ax,ax
	pop	es
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_E2XMIT	ENDP
else
E2XMIT	ENDP
endif
;
;
;***********************************************************************
;   Receive
;   This is a CPU hook for boards that must be polled before we can
;   deliver packets into the receive buffer.  (i.e. no interrupts used)
;
;   The 3COM 3C501 version uses interrupts, so this routine is a NOP
;   for this board.
;
;    usage:  recv();
;
;
ifdef Microsoft
_E2RECV	proc	far
else
E2RECV	proc	far
endif
	push	bp
	PUSH	SI
	PUSH	DI
	push	es

ifdef Microsoft
	MOV	AX,word ptr [_BUFPT+2]	; buffer's ds
	MOV	DI,_BUFPT	; where buffer is
else
	MOV	AX,word ptr [BUFPT+2]	; buffer's ds
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
	MOV	_BUFPT,AX	; wrap-around
else
	MOV	AX,BUFORG	; wrap to here
	MOV	BUFPT,AX	; wrap-around
endif
	MOV	DI,AX		; di also
;
;  here, DI contains where we want to put the packet.
;  Add 2 to allow for space to put the size value
;
OKAYREAD:
	inc	di
	inc	di		; room for size word
	mov	bx,FIRSTFD	; get addr of first FD in list
	mov	dx,ubase	; base for board
	push	ds
	mov	ds,dx
;
;  
CKFRAME:
	mov	ax,[bx]		; status word of frame
	test	ax,08000h	; frame written?
	jnz	IREADONE	; yes, read it in
	pop	ds
	call	rust		; restore receiver if necessary
	jmp	STOPINT
;
;  we have a frame, read it in
;
IREADONE:
	test	ax,02000h	; check frame OK bit
	jnz	frameok		; ok, read it
	mov	cx,ds
	pop	ds
	call	rust		; preserves cx, restart RU if necessary
    jmp SHORT freespace   ; not OK, just free the frame

frameok:
	mov	si,[bx+FDPTR]	; get pointer to buffer descriptor

copybuf:			; es:di is already set to receive packet
	mov	dx,si		; save a copy of current BD ptr
	mov	ax,[si]		; get status and count word for BD
	push	ax		; save for EOF bit
	test	ax,04000h	; is count field there?
	jnz	okcount
	pop	ax	
	mov	cx,ds
	pop	ds
    jmp SHORT freespace   ; free the frame, etc
okcount:
	xor	cx,cx
	mov	cl,al		; 200 bytes is largest this can be
	mov	si,[si+BDPTR]	; get offset of data
	rep	movsb		; copy the bytes from this packet segment
	mov	si,dx		; get back current BD ptr
	pop	ax		; get back EOF bit
	test	ax,08000h	; check bit
	jnz	ptrupdate	; free the frame, no more data here
	mov	si,[si+BDLINK]	; go to next BD in list
	jmp	copybuf		; copy the next buffer
;
;
ptrupdate:
;
;  DI now contains updated value for BUFPT
;
	mov	cx,ds		; save board segment
	pop	ds
ifdef Microsoft
	mov	bx,_BUFPT	; get where size field for this packet goes
else
	mov	bx,BUFPT	; get where size field for this packet goes
endif
	mov	ax,di		; where pointer is now
	sub	ax,bx		; where is was then, difference is size
ifdef Microsoft
	MOV	DX,_BUFBIG	; total amount of stuff in buffer
else
	MOV	DX,BUFBIG	; total amount of stuff in buffer
endif
	ADD	DX,AX		; add in size of this packet
ifdef Microsoft
	MOV	_BUFBIG,DX	; after adding in current packet size
else
	MOV	BUFBIG,DX	; after adding in current packet size
endif

	dec	ax
	dec	ax		; account for length field
	mov	ES:[bx],ax		; put the accumulated size there

ifdef Microsoft
	MOV	_BUFPT,DI	; it is here, now
else
	MOV	BUFPT,DI	; it is here, now
endif

;
freespace:
;
;  we are done with the frame, do the list management
;    cx=DS for board  DS=DGROUP
;
	mov	bx,FIRSTFD	; the frame we are working with
	mov	di,LASTBD	; where end of BD list is now
	mov	es,cx		; reload board segment

	mov	si,es:[bx+FDPTR]	; first BD in frame list
nextbd:
	mov	cx,es:[si]	; count word for BD, EOF bit
	test	cx,08000h	; EOF bit, if set, save si in lastbd
	jnz	dolastbd
	mov	word ptr es:[si],0	; clear status word, EOF bit
	cmp	si,LASTBD		; see if we are wrapping
	jz	dolastbd		; yes, just undo it
	mov	si,es:[si+BDLINK]	; follow link
	jmp	nextbd
dolastbd:
	mov	LASTBD,si	; store last known BD
	mov	word ptr es:[si+BDSIZE],08000h+200	; end of list here
	mov	word ptr es:[si],0	; clear status word, EOF bit
; size field for not end of list
	mov	word ptr es:[di+BDSIZE],200	; remove old end-of-list

;
;  update the FD list flags, new end-of-list
;
	mov	di,LASTFD	; get old end-of-list
	mov	word ptr es:[bx+FDEOL],08000h	; store new EOL
	mov	word ptr es:[bx],0		; clear status word for frame
	mov	word ptr es:[di+FDEOL],0	; zero old one
	mov	LASTFD,bx	; update stored pointer
	mov	si,es:[bx+FDLINK]		; where next fd is
	mov	FIRSTFD,si	; store that info for next time
;	mov	bx,LASTBD	; last of the BD list
;	mov	ax,es:[bx+BDLINK]		; get link field from lastBD
;	mov	es:[si+FDPTR],ax		; store into next frame
;
;  signs that something is actually happening - for debugging
;
;	MOV	AX,0B800H       ; screen
;	MOV	ES,AX
;	MOV	DI,3998		; lower right corner
;	INC	ICNT
;	MOV	al,ICNT		; character
;	STOSB
;
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
_E2RECV	ENDP
else
E2RECV	ENDP
endif

rust	proc	near		; re-start receiver
;
;  check to see if the receiver went off because of no resources
;  and restart receiver if necessary
;  ds=dgroup, no other requirements
;
	push	es
	mov	ax,ubase
	mov	es,ax
	mov	ax,es:[SCB]	; status word for SCB
	and	ax,070h		; receiver status
	cmp	al,020h		; receiver has no resources
	jnz	hasres
;
;  setup lists for starting the RU on the chip
;  we know that there isn't anything in the buffer that we want
;
	mov	bx,FIRSTFD	; get first FD on free list (assume free)
	mov	word ptr es:[SCB+SRFA],bx	; put into SCB
	mov	si,LASTBD	; pointer to a BD, end of chain
	mov	ax,word ptr es:[si+BDLINK]	; pointer to next BD
	mov	word ptr es:[bx+FDPTR],ax	; set to start of BDs
;
;
;  Start the RU, doesn't need CB, only SCB parms.
;   command, to start receiving again
;
	mov	word ptr es:[SCB],0		; clear status word
	mov	word ptr es:[SCB+SCOM],010h	; start RU

	DOCA	GO
hasres:
	pop	es
	ret	
rust	endp
;
;*************************************************************************
;  ETUPDATE
;      update pointers and/or restart receiver when read routine has
;      already removed the current packet
;
ifdef Microsoft
_E2ETUPDATE	PROC	FAR
else
E2ETUPDATE	PROC	FAR
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
_E2ETUPDATE	ENDP
else
E2ETUPDATE	ENDP
endif


ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END
