; Driver for Ungermann-Bass NICps/2 Zypher interface
; Tim Krauskopf
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

	TITLE	NETSUPPORT -- LOW LEVEL DRIVERS FOR ETHERNET
;
;  Tim Krauskopf
;  National Center for Supercomputing Applications
;  9/1/87  Ungermann-Bass driver started, PC bus
;  12/28/87 UB NIC driver started, Zypher interface
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
ifdef MSC6
	INCLUDE NET\ENET\ZYPDEFS.INC
else
	INCLUDE ZYPDEFS.INC
endif
;
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

SAVECS	DW	00H		; where to save the old interrupt ptr
SAVEIP	DW	00H
LFPP	DB	00h		; Full Page pointer
DEAF	DB	00H		; when we can't handle any more packets
OFFS	DW	00H		; how many times the handler was turned off
;
;
; Zypher definitions to work with
;
Init_ZCB	LABEL	WORD
	db	Initialize_Cmd
	db	0	; Status
	db	0	; Result
	db	0	; Report_Code
	dw	0	; Options
	dd	0	; Post_Routine
	dw	2 dup (0)
	dw	0	; Modes
	dw	1100	; Max_Xmt_Length
	dw	1	; Num_Xmt_Buffers
	dw	1514	; Max_Rcv_Size
	dw	12	; Num_Rcv_Buffers
	dw	0	; Max_Multicast_Addresses
	dw	7 dup (0)	; reserved
	dw	0	; Acquired Modes
	dw	0	; Acquired Max_Xmt_Length
	dw	0	; Acquired Num_Xmt_Buffers
	dw	0	; Acquired Max_Rcv_Size
	dw	0	; Acquired Num_Rcv_Buffers
	dw	0	; Acquired Max_Multicast_Addresses
	dw	3 dup (0)	; reserved

Init2_ZCB	LABEL	WORD
	db	Initialize_Cmd
	db	0	; Status
	db	0	; Result
	db	0	; Report_Code
	dw	0	; Options
	dd	0	; Post_Routine
	dw	2 dup (0)
	dw	0	; Modes
	dw	1100	; Max_Xmt_Length
	dw	1	; Num_Xmt_Buffers
	dw	1514	; Max_Rcv_Size
	dw	12	; Num_Rcv_Buffers
	dw	0	; Max_Multicast_Addresses
	dw	7 dup (0)	; reserved
	dw	0	; Acquired Modes
	dw	0	; Acquired Max_Xmt_Length
	dw	0	; Acquired Num_Xmt_Buffers
	dw	0	; Acquired Max_Rcv_Size
	dw	0	; Acquired Num_Rcv_Buffers
	dw	0	; Acquired Max_Multicast_Addresses
	dw	3 dup (0)	; reserved


Stat_ZCB	LABEL	WORD
	db	Status_Cmd
	db	0	; Status
	db	0	; Result
	db	0	; Report_Code
	dw	0	; Options
	dd	0	; Post_Routine
	dw	2 dup (0)
	dw	0	; state
	dw	0	; modes
	dw	0	; Max_Xmt_Length
	dw	0	; Act Num_Xmt_Buffers
	dw	0	; Act Max_Rcv_Size
	dw	0	; Act Num_Rcv_Buffers
	db	6 dup (0)	; unique ID
	dw	0,0	; total xmts
	dw	0,0	; total rcvs
	dw	0,0	; CRC errors
	dw	0,0	; ALN errors
	dw	0,0	; RSC errors
	dw	0,0	; OVR errors
	dw	12 dup (0)	; reserved

Xmt_ZCB	LABEL	WORD
	db	Transmit_Cmd
	db	0	; Status
	db	0	; Result
	db	0	; Report_Code
	dw	0	; Options
	dd	0
	dw	2 dup (0)
	dw	0	; Xmt_Length
	dw	0,0	; Xmt_Buffer
	dw	0	; hardware status
	db	0,0	; Xmt_Bfr_ID and an unused byte
	dw	0,0	; Xmt_Bfr_Address (on-card transmit buffer address)

Cancel_ZCB	LABEL	WORD
	db	Cancel_Receives_Cmd
	db	0	; Status
	db	0	; Result
	db	0	; Report_Code
	dw	0	; Options
	dd	0	; Post_Routine
	dw	2 dup (0)


Recv_ZCB	LABEL	WORD
	db	Receive_Cmd
	db	0	; Status
	db	0	; Result
	db	0	; Report_Code
	dw	0	; Options
	dd	0
	dw	2 dup (0)
;			db	SIZE ZCB_Header dup (0)
;ZCB_Rcv_Mode		db	0
	db	0
;ZCB_Rcv_Status		db	0
	db	0
;ZCB_Rcv_Buffer_Size	dw	0	; Size of user's buffer.
	dw	1514			; always this much room
;ZCB_Rcv_Buffer_Address	dd	0	; Address of user's buffer.
	dd	0
;ZCB_Rcv_Data_Length	dw	0	; Bytes copied to user's buffer.
	dw	0
;ZCB_Rcv_Frame_Count	dw	0	; Count of as-yet-uncopied bytes left;
	dw	0
					;  in frame.
;ZCB_Rcv_Hdwr_Status	dw	0	; Status reported by 82586.
	dw	0
;ZCB_Rcv_Frame_ID	dw	0	; Frame ID for "incremental mode".
	dw	0
;ZCB_Rcv_Bfr_Ptr		dw	0,0	; Address of next as-yet-uncopied byte
	dw	0,0
					;  of frame in on-card receive buffer.
;ZCB_Rcv_Bfr_Count	dw	0	; Count of as-yet-uncopied bytes in
	dw	0
					;  current on-card receive buffer.
;ZCB_Rcv_Descriptor	dw	0,0	; Address of 82586 RBD (Receive Buffer
	dw	0,0
					;  Descriptor) for current on-card
					;  receive buffer.


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
	PUBLIC	_U2RECV,_U2ETOPEN,_U2ETCLOSE,_U2GETADDR
	PUBLIC	_U2XMIT,_U2ETUPDATE
else
	PSEG
	PUBLIC	U2RECV,U2ETOPEN,U2ETCLOSE,U2GETADDR
	PUBLIC	U2XMIT,U2ETUPDATE
endif

ZYP_Entry	LABEL	DWORD
	dw	0, 0D000h

;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,addr,ioaddr)
;           char s[6];       ethernet address
;           int irq,addr,ioaddr;     
;                interrupt number, base mem address and
;                i/o address to use
;
;
ifdef Microsoft
_U2ETOPEN	PROC	FAR
else
U2ETOPEN	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	push	es
	push	ds

	mov	ax,ds
	mov	es,ax		; set to base address for NCB
;
;   Set base address for board into ZYP_Entry
;
	mov	ax,[X+BP+6]	; base address for board as installed
	mov word ptr cs:[ZYP_Entry+2],ax	; store
;
;	try shutting the board down first
;
;	MOV	BX,offset Cancel_ZCB
;	CALL	ZYP_Entry
;closeit:
;	CMP	[BX].ZCB_Status,0FFh
;	JE	closeit
;
;	now reopen it
;
	MOV	BX,offset Init2_ZCB
	CALL	ZYP_Entry
Init_wait:
	CMP	[BX].ZCB_Status,0FFh
	JE	Init_wait

;
; call ZYP receive
;
	mov	bx,offset Recv_ZCB
ifdef Microsoft
	mov	ax,word ptr [_BUFPT+2]	; where packet should arrive
else
	mov	ax,word ptr [BUFPT+2]	; where packet should arrive
endif
ifdef Microsoft
	mov	di,word ptr [_BUFPT]
else
	mov	di,word ptr [BUFPT]
endif
	inc	di
	inc	di
	; address of packet into NCB
	mov	word ptr [bx].ZCB_Rcv_Buffer_Address,di
	mov	word ptr [bx].ZCB_Rcv_Buffer_Address+2,ax
	CALL	ZYP_Entry

	xor	ax,ax
getout:
	pop	ds
	POP	ES
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_U2ETOPEN	ENDP
else
U2ETOPEN	ENDP
endif
;
;
;*******************************************************************
;  GETADDR
;     get the Ethernet address off of the board
;
;   usage:  getaddr(s,address,ioaddr);
;	char s[6];           will get six bytes from the PROM
;       int address;
;       int ioaddr;     mem address and ioaddress to use
;
;
ifdef Microsoft
_U2GETADDR	PROC	FAR
else
U2GETADDR	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	PUSH 	ES		; save mine
	push	ds

	mov	ax,ds
	mov	es,ax
;
;   Set base address for board into ZYP_Entry
;
	mov	ax,[X+BP+4]	; base address for board as installed
	mov word ptr cs:[ZYP_Entry+2],ax	; store
;
;
	MOV	BX,offset Init_ZCB
	CALL	ZYP_Entry
ifdef OLD_WAY
Init_wt:
	CMP	[BX].ZCB_Status,0FFh
	JE	Init_wt
else
    MOV AX,0
Init_wt:
    INC AX
    CMP AX,0ffh
    JE  getaddr_timeout
    CMP [BX].ZCB_Status,0FFh
	JE	Init_wt
getaddr_timeout:
endif

;	CMP	[BX].ZCB_Result,Initialization_Complete
;	JNE	oh_well

	MOV	BX,offset Stat_ZCB
	CALL	ZYP_Entry
addr_wait:
	CMP	[BX].ZCB_Status,0FFh
	JE	addr_wait
	CMP	[BX].ZCB_Result,Status_Complete
	JE	get_addr
oh_well:
	mov	ax,-1
    jmp SHORT nomore

get_addr:
	MOV	AX,[BP+X+2]	; get new one
	MOV	ES,AX           ; set new one
	MOV	DI,[BP+X]	; get pointer, es:di is ready
	MOV	SI,BX
	add	si,ZCB_Stat_Unique_ID
	;
	mov	cx,3
	CLD
	rep movsw		; copy address
	xor	ax,ax
nomore:
	pop	ds
	POP 	ES
	POP	DI
	POP	SI
	POP	BP		
	RET
ifdef Microsoft
_U2GETADDR	ENDP
else
U2GETADDR	ENDP
endif
;
;***********************************************************************
;  ETCLOSE
;        shut it down
;
ifdef Microsoft
_U2ETCLOSE	PROC	FAR
else
U2ETCLOSE	PROC	FAR
endif
	push	bp
	push	si
	push	di
	push	es
	push	ds
	mov	ax,ds
	mov	es,ax

	MOV	BX,offset Cancel_ZCB
	CALL	ZYP_Entry
canwait:
	CMP	[BX].ZCB_Status,0FFh
	JE	canwait

	pop	ds
	pop	es
	pop	di
	pop	si
	pop	bp
	RET
ifdef Microsoft
_U2ETCLOSE	ENDP
else
U2ETCLOSE	ENDP
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
_U2XMIT	PROC	FAR
else
U2XMIT	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	SI
	PUSH	DI
	push	es
	push	ds

	MOV	CX,[BP+X+2]	; ds for buffer
	MOV	SI,[BP+X]	; offset for buffer

	MOV	AX,[BP+X+4]	; count of bytes
	cmp	ax,1100
	jle	oklen
	mov	ax,1100		; maximum for me
oklen:
;
;  place into Xmit parms
;
	MOV	BX,offset Xmt_ZCB	; Start the 1st transmit, using
	mov	word ptr [bx].ZCB_Xmt_Data_Address,si
	mov	word ptr [bx].ZCB_Xmt_Data_Address+2,CX
	mov	[bx].ZCB_Xmt_Data_Length,ax

	mov	ax,ds
	mov	es,ax		; base for ZCB block

	CALL	ZYP_Entry

ifdef OLD_WAY
waitstat:
	CMP	[BX].ZCB_Status,0FFh
	JE	waitstat
else
    MOV AX,0
waitstat:
    INC AX
    CMP AX,0ffh
    JE  timeout
    CMP [BX].ZCB_Status,0FFh
    JE  waitstat
timeout:
endif

	xor	ax,ax
;	CMP	[BX].ZCB_Result,Initialization_Complete
;	JE	xmitok
	
;	mov	al,[BX].ZCB_Result
xmitok:

	pop	ds
	pop	es
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_U2XMIT	ENDP
else
U2XMIT	ENDP
endif
;
;
;***********************************************************************
;   Receive
;   This is a CPU hook for boards that must be polled before we can
;   deliver packets into the receive buffer.  (i.e. no interrupts used)
;
;    usage:  recv();
;
;
ifdef Microsoft
_U2RECV	proc	far
else
U2RECV	proc	far
endif
	push	bp
	PUSH	SI
	PUSH	DI
	push	es
	push	ds
;
	mov	bx,offset Recv_ZCB
	CMP	[BX].ZCB_Status,0FFh ; status byte for outstanding read request

	JNE	newpkt
;
; no packet yet, skip
;
	pop	ds
	pop	es
	POP	DI
	POP	SI
	pop	bp
	ret			; no packet arrival yet

newpkt:
	mov	dx,[bx].ZCB_Rcv_Data_Length	; length of recieved packet

ifdef Microsoft
	MOV	aX,word ptr [_BUFPT+2]	; buffer's ds
else
	MOV	aX,word ptr [BUFPT+2]	; buffer's ds
endif
ifdef Microsoft
	mov	bx,word ptr [_BUFPT]	; get where size field for this packet goes
else
	mov	bx,word ptr [BUFPT]	; get where size field for this packet goes
endif

	mov	es,ax
	mov	es:[bx],dx		; put the accumulated size there

	add	bx,dx		; add length to bufpt
	inc	bx
	inc	bx

ifdef Microsoft
	MOV	word ptr [_BUFPT],bx	; it is here, now
else
	MOV	word ptr [BUFPT],bx	; it is here, now
endif

ifdef Microsoft
	MOV	AX,word ptr [_BUFBIG]	; total amount of stuff in buffer
else
	MOV	AX,word ptr [BUFBIG]	; total amount of stuff in buffer
endif

	ADD	AX,DX		; add in size of this packet
	INC	AX
	INC	AX		; to cover the length value

ifdef Microsoft
	MOV	word ptr [_BUFBIG],AX	; after adding in current packet size
else
	MOV	word ptr [BUFBIG],AX	; after adding in current packet size
endif

;
;  set up to read the next packet from the net
;

ifdef Microsoft
	MOV	aX,word ptr [_BUFPT+2]	; buffer's ds
else
	MOV	aX,word ptr [BUFPT+2]	; buffer's ds
endif
ifdef Microsoft
	MOV	DI,word ptr [_BUFPT]     ; where buffer is
else
	MOV	DI,word ptr [BUFPT]     ; where buffer is
endif
	mov	es,ax

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
    JMP SHORT ENDINT      ; can't do much, we lose packets until restarted

;
;  wrap pointer around at end, we know that we have room
;
ISROOM:
ifdef Microsoft
	MOV	DX,word ptr [_BUFEND]	; right before 2K safety area
else
	MOV	DX,word ptr [BUFEND]	; right before 2K safety area
endif
	CMP	DX,DI		; see if pointer is over limit
	JA	OKAYREAD	; we are not at wrap-around

ifdef Microsoft
	MOV	AX,word ptr [_BUFORG]	; wrap to here
else
	MOV	AX,word ptr [BUFORG]	; wrap to here
endif
ifdef Microsoft
	MOV	word ptr [_BUFPT],AX	; wrap-around
else
	MOV	word ptr [BUFPT],AX	; wrap-around
endif
	MOV	DI,AX		; di also
;
;  here, DI contains where we want to put the packet.
;
OKAYREAD:
	inc	di
	inc	di		; leave space for length of packet
;
;
; call receive again
;
	mov	bx,offset Recv_ZCB
	; address of packet into NCB
	mov	word ptr [bx].ZCB_Rcv_Buffer_Address,di
	mov	word ptr [bx].ZCB_Rcv_Buffer_Address+2,es
	mov	ax,ds
	mov	es,ax
	CALL	ZYP_Entry

STOPINT:

ENDINT:
	pop	ds
	pop	es
	POP	DI
	POP	SI
	POP	BP
	RET
ifdef Microsoft
_U2RECV	ENDP
else
U2RECV	ENDP
endif

;
;*************************************************************************
;  ETUPDATE
;      update pointers and/or restart receiver when read routine has
;      already removed the current packet
;
ifdef Microsoft
_U2ETUPDATE	PROC	FAR
else
U2ETUPDATE	PROC	FAR
endif
	PUSH 	ES

ifdef Microsoft
	MOV	AX,word ptr [_BUFPT+2] ; establish data segment to buffer
else
	MOV	AX,word ptr [BUFPT+2] ; establish data segment to buffer
endif
	MOV	ES,AX		; put that in es
;
ifdef Microsoft
	MOV	BX,word ptr [_BUFREAD]	; where read pointer is now
else
	MOV	BX,word ptr [BUFREAD]	; where read pointer is now
endif
	MOV	DX,ES:[BX]	; get size of this packet
	INC	DX
	INC	DX		; TWO MORE FOR LENGTH VALUE

	ADD	BX,DX		; increment bufread by size of packet

ifdef Microsoft
	MOV	CX,word ptr [_BUFEND]	; right before 2K safety area
else
	MOV	CX,word ptr [BUFEND]	; right before 2K safety area
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
_U2ETUPDATE	ENDP
else
U2ETUPDATE	ENDP
endif

;
ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END
