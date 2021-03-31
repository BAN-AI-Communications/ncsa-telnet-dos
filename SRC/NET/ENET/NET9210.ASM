;  MICOM NI 9210 driver code
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
;

	TITLE	NETSUPPORT -- LOW LEVEL DRIVERS FOR ETHERNET
;
;  Assembler support for Ethernet I/O on the PC
;
;  Tim Krauskopf
;  9/1/87  Ungermann-Bass driver started, PC bus
;  9/14/87 MICOM NI5210 driver started, PC bus
;  4/88    Modified for combined drivers, version 2.2
;  9/88    NI9210 mods, just some I/O addr changes
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
;  Equates for controlling the MICOM board
;
;  I/O addresses, writing anything in AL trips these gates
;
;  First six addresses are the EPROM board Ether address (read)
;
IORESET	EQU	0		; reset the board
IOCA	EQU	2		; execute command which is in SCB
IODIS	EQU	4		; disable network connect
IOENA	EQU	6		; enable network
;
;  Structure elements specific to the Intel 82586 chip
;
BDBASE	EQU	1874		; base address for 30 buffer descriptors
BUFBASE	EQU	2174		; base address for 30 200 byte buffers
BDSTAT	EQU	0		; status word in BD
BDLINK	EQU	2		; 16pointer to next BD
BDPTR	EQU	4		; 24pointer to actual buffer
BDSIZE	EQU	8		; size of the buffer
;
SCB	EQU	10		; system control block base
SSTAT	EQU	0		; status word for SCB
SCOM	EQU	2		; command word in SCB
SCBL	EQU	4		; 16pointer to command block list
SRFA	EQU	6		; 16pointer to receive frame list
SERRS	EQU	8		; 4 words of error counts
;
FDBASE	EQU	1214		; base addr for 30 frame descriptors
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
SCPTR	EQU	01ff6h		; hardwired address for SCP
ISCPTR	EQU	01feeh		; my address for ISCP, points to SCB
CCBPTR	EQU	26		; offset of configure command block
TCBPTR	EQU	44		; xmit CB offset
TBDPTR	EQU	60		; xmit BD offset
TBUFPTR	EQU	68		; xmit buffer offset
;
;  Data segment
;
ifdef Microsoft
	.data
;DGROUP	group	_DATA
;_DATA	segment	public 'DATA'
;	assume	DS:DGROUP
else
	DSEG
endif
;
;  The pointers below are actually DWORDs but we access them two
;  bytes at a time.
;
; STAT change to RSTAT because of name clash with MSC library routine
ifdef Microsoft
	RSTAT	EQU	<_RSTAT>
	BUFPT	EQU	<_BUFPT>
	BUFORG	EQU	<_BUFORG>
	BUFEND	EQU	<_BUFEND>
	BUFREAD	EQU	<_BUFREAD>
	BUFBIG	EQU	<_BUFBIG>
	BUFLIM	EQU	<_BUFLIM>
ifdef QAK
	EXTRN	_RSTAT:BYTE	; last status from read
	EXTRN	_BUFPT:WORD	; current buffer pointer
	EXTRN	_BUFORG:WORD	; pointer to beginning of buffer
	EXTRN	_BUFEND:WORD	; pointer to end of buffer
	EXTRN	_BUFREAD:WORD	; pointer to where program is reading
	EXTRN	_BUFBIG:WORD	; integer, how many bytes we have
	EXTRN	_BUFLIM:WORD	; integer, max bytes we can have
endif
endif
	EXTRN	RSTAT:BYTE	; last status from read
	EXTRN	BUFPT:WORD	; current buffer pointer
	EXTRN	BUFORG:WORD	; pointer to beginning of buffer
	EXTRN	BUFEND:WORD	; pointer to end of buffer
	EXTRN	BUFREAD:WORD	; pointer to where program is reading
	EXTRN	BUFBIG:WORD	; integer, how many bytes we have
	EXTRN	BUFLIM:WORD	; integer, max bytes we can have

ICNT	DB	00h

SAVECS	DW	00H		; where to save the old interrupt ptr
SAVEIP	DW	00H
LFPP	DB	00h		; Full Page pointer
DEAF	DB	00H		; when we can't handle any more packets
OFFS	DW	00H		; how many times the handler was turned off
FIRSTFD	dw	FDBASE		; start of FD queue
LASTFD	DW	0		; end of the FD chain
LASTBD	DW	0		; end of the BD chain
IOADDR	DW	0300h		; I/O address for card (jumpers)
UBASE	DW  	0d000h		; base segment for board (jumper set)
;
;  data for configuring and setting up the MICOM board
;
;  chip always looks at SCP for config info which points to ISCP for the
;  pointer to the CONTROL BLOCK which handles everything from there.
;  Kind of indirect, but it works.
;
SCP	DB	1		; bus use flag
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
	DW	0			; flags, set to 1 for promiscuous
	DW	40H			; min frame length=40h
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
	.code
	PUBLIC	_M9RECV,_M9ETOPEN,_M9ETCLOSE,_M9GETADDR,_M9XMIT,_M9ETUPDATE
else
	PSEG
	PUBLIC	M9RECV,M9ETOPEN,M9ETCLOSE,M9GETADDR,M9XMIT,M9ETUPDATE
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
_M9ETOPEN	PROC	FAR
else
M9ETOPEN	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	ES
	PUSH	SI
	PUSH	DI

	mov	ax,[bp+X+6]	; second parameter
	mov	UBASE,ax	; base address
;	mov	ax,UBASE	; get a copy (temporary)
	mov	es,ax		; set to base address
	mov	ax,[bp+X+8]	; get I/O address
	mov	IOADDR,ax	; save a copy
;	mov	ax,IOADDR	; get a copy (temporary)
;
;  check for correct EPROM location
;
	mov	dx,ax		; i/o address
	add	dx,12
	in	al,dx
	mov	bl,al		; assemble pattern to check
	inc	dx
	inc dx
	in	al,dx
	mov	bh,al
	cmp	bx,05500h	; pattern known to be there in ROM
	jz	goinit
	mov	ax,-1
	POP		DI
	POP		SI
	pop	es
	pop	bp
	ret
goinit:
;
;  Initialize MICOM 9210
;
;  Install 8K SCP, we only use 8K bytes no matter what
;
	mov	si,offset SCP	; get pre-set values
	mov	di,SCPTR
	mov	cx,5		; 5 words
	rep	movsw		; install SCP
;
;  Install 16K SCP
;
	mov	si,offset SCP	; get pre-set values
	mov	di,SCPTR+02000h		; offset for 16K board
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
;  Turn off interrupts, I don't want them
;  can't turn off on 9210, shouldn't get any anyway
;	mov	dx,IOADDR	; base for IO control
;	add	dx,IOINTOF	; turn ints off
;	out	dx,al		; any value in ax

	mov dx,IOADDR	; base address = reset
	out	dx,al		; reset the chip
;
;  Issue a CA to initialize the chip after reset
;
	add	dx,IOCA
	out	dx,al		; CA
;
;  Disconnect from network
	add	dx,2
	out	dx,al
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
	mov	word ptr es:[SCB+SCOM],0100h	; do-command command
	mov	word ptr es:[SCB+SCBL],CCBPTR	; where conf command is
	mov	word ptr es:[SCB+SERRS],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+2],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+4],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+6],0	; zero errs field
	mov	dx,IOADDR
	add	dx,IOCA			; CA
	out	dx,al			; send it
	xor	cx,cx			; timeout
waitconf:
	mov	ax,word ptr es:[CCBPTR]	; get status word
	test	ax,08000h		; is command complete?
	loopz	waitconf
	jnz	confok
	mov	ax,-3			; timeout problem
	POP		DI
	POP		SI
	pop	es
	pop	bp
	ret
;
confok:
;
;  Next step, load our address into the board
;     reuses the space that the configure command used, with different command 
;
	mov	di,CCBPTR		; start of config command block
	xor	ax,ax
ifdef QAK
	stosw				; zero status word for commmand
	mov	ax,8001h		; IA setup command + EL
	stosw
	xor	ax,ax
	dec	ax
	stosw				; set link value to -1 (unused)
else
		stosw				;set up ia command
		mov	[di], 8001h
		inc	di
		inc	di
		stosw
endif

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
ifndef QAK
	mov	word ptr es:[SCB+SCBL],CCBPTR	; where conf command is
	mov	word ptr es:[SCB+SERRS],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+2],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+4],0	; zero errs field
	mov	word ptr es:[SCB+SERRS+6],0	; zero errs field
endif
	mov	dx,IOADDR
	add	dx,IOCA			; CA
	out	dx,al			; send it
	xor	cx,cx			; timeout
waitia:
	mov	ax,word ptr es:[CCBPTR]	; get status word
	test	ax,08000h		; is command complete?
	loopz	waitia
	jnz	iaok
	mov	ax,-2			; timeout problem

	POP		DI
	POP		SI
	pop	es
	pop	bp
	ret
;
iaok:
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
	mov	dx,IOADDR
	add	dx,IOENA		; enable network
	out	dx,al			; any al value
;
;  Start the RU, doesn't need CB, only SCB parms.
;   command, to start receiving
;
	mov	word ptr es:[SCB],0		; clear status word
	mov	word ptr es:[SCB+SRFA],FDBASE	; set to frame descriptors
	mov	word ptr es:[SCB+SCOM],010h	; start RU
	mov	dx,IOADDR
	add	dx,IOCA			; issue CA
	out	dx,al
;
;  don't wait for response, we are done
;
	xor	ax,ax
	POP	DI
	POP	SI
	POP	ES
	POP	BP
	RET
ifdef Microsoft
_M9ETOPEN	ENDP
else
M9ETOPEN	ENDP
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
_M9GETADDR	PROC	FAR
else
M9GETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH 	ES		; save mine
	PUSH	DI

	mov	dx,[BP+X+6]	; get board's base io addr
;	mov	dx,IOADDR	; (temporary)
	MOV	AX,[BP+X+2]	; get new one
	MOV	ES,AX           ; set new one
	MOV	DI,[BP+X]	; get pointer, es:di is ready
	;
	mov	cx,6
	CLD
getonee:
	in	al,dx		; get a byte of the EPROM address
	STOSB			; put it away
	add	dx,2		; next register
	loop getonee		; go back for rest

	xor	ax,ax
	POP		DI
	POP 	ES
	POP	BP		
	RET
ifdef Microsoft
_M9GETADDR	ENDP
else
M9GETADDR	ENDP
endif
;
;***********************************************************************
;  ETCLOSE
;        shut it down if necessary
;        MICOM board never interrupts, so we can leave it running.
;        Who cares, right?
;
ifdef Microsoft
_M9ETCLOSE	PROC	FAR
else
M9ETCLOSE	PROC	FAR
endif
	RET
ifdef Microsoft
_M9ETCLOSE	ENDP
else
M9ETCLOSE	ENDP
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
_M9XMIT	PROC	FAR
else
M9XMIT	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	ES
	PUSH	SI
	PUSH	DI

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
	mov	dx,IOADDR
	add	dx,IOCA
	out	dx,al		; issue CA to get it going
	xor 	ax,ax
	POP		DI
	POP		SI
	POP	ES
	POP	BP
	RET
ifdef Microsoft
_M9XMIT	ENDP
else
M9XMIT	ENDP
endif
;
;
;***********************************************************************
;   Receive
;   This is a CPU hook for boards that must be polled before we can
;   deliver packets into the receive buffer.  (i.e. no interrupts used)
;
;   The 3COM 3C501 version uses interrupts, so this routine is a NOP
;   for that board.
;
;    usage:  recv();
;
;
ifdef Microsoft
_M9RECV	proc	far
else
M9RECV	proc	far
endif
	push	bp
	push	es
	PUSH	SI
	PUSH	DI

	MOV	AX,word ptr [BUFPT+2]	; buffer's ds
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
;  no room at the Inn. 
;
	JMP	ENDINT		; can't do much, we lose packets until restarted

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
	mov	bx,BUFPT	; get where size field for this packet goes
	mov	ax,di		; where pointer is now
	sub	ax,bx		; where is was then, difference is size
	MOV	DX,BUFBIG	; total amount of stuff in buffer
	ADD	DX,AX		; add in size of this packet
	MOV	BUFBIG,DX	; after adding in current packet size

	dec	ax
	dec	ax		; account for length field
	mov	[bx],ax		; put the accumulated size there

	MOV	BUFPT,DI	; it is here, now

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
;	MOV	AX,0B000H       ; screen
;	MOV	ES,AX
;	MOV	DI,3998		; lower right corner
;	INC	ICNT
;	MOV	al,ICNT		; character
;	STOSB

;
;  set up to read the next packet from the net
;
STOPINT:

ENDINT:
	POP		DI
	POP		SI
	POP	ES
	POP	BP
	RET
ifdef Microsoft
_M9RECV	ENDP
else
M9RECV	ENDP
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
	mov	dx,IOADDR
	add	dx,IOCA			; issue CA
	out	dx,al
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
_M9ETUPDATE	PROC	FAR
else
M9ETUPDATE	PROC	FAR
endif
	PUSH 	ES

	MOV	AX,word ptr [BUFPT+2]	; establish data segment to buffer
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

ALIVE:
	POP	ES
	RET
ifdef Microsoft
_M9ETUPDATE	ENDP
else
M9ETUPDATE	ENDP
endif


ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END
