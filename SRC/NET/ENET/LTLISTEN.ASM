; National Center for Supercomputing Applications (NCSA) Telnet source code
; 
; February 10, 1989
; (C) Copyright 1989 Planning Research Corporation
; 
; Permission is granted to any individual or institution to use, copy,
; modify, or redistribute this software and its documentation provided this
; notice and the copyright notices are retained.  This software may not be
; distributed for profit, either in original form or in derivative works.
; Planning Research Corporation makes no representations about the
; suitability of this software for any purpose.
; 
; PLANNING RESEARCH CORPORATION GIVES NO WARRANTY, EITHER EXPRESS OR IMPLIED,
; FOR THE PROGRAM AND/OR DOCUMENTATION PROVIDED, INCLUDING, WITHOUT
; LIMITATION, WARRANTY OF MERCHANTABILITY AND WARRANTY OF FITNESS FOR A
; PARTICULAR PURPOSE.

; Revision History:
; 
; Date		Initials	Comment
; ----		--------	-------
; 02-10-89	JGN@PRC		Initial version.
; 
; Initials	Name		Organization
; --------	----		------------
; JGN@PRC	Jim Noble	Planning Research Corporation

	TITLE   LTListen
	NAME    LTListen

;
; Check which type of assembly to do.
;
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
	PUBLIC	_listen1
	PUBLIC	_listen2

    EXTRN   _listen1_c:FAR
	EXTRN	_listen2_c:FAR
;ifdef Watcom
;    EXTRN   listen_ds_:FAR
;else
	EXTRN	_listen_ds:FAR
;endif
else
	PUBLIC	listen1
	PUBLIC	listen2

	EXTRN	listen1_:FAR
	EXTRN	listen2_:FAR
	EXTRN	listen_d:FAR
endif

LTListen_TEXT	SEGMENT	WORD PUBLIC 'CODE'

	ASSUME	CS: LTListen_TEXT

; First-half DDP socket listener (asm part)
; 
; The first-half of the listener is called with the following inputs passed
; via registers:
;
; 	AH	Socket
; 	CX	Length of data
; 	DS:BX	Segment:Offset pointer of (LAP & DDP) header
; 
; The first-half of the listener must decide whether to keep or discard the 
; data.  If the packet is to be discarded, the first-half listener must return 
; with CX equal to zero; in this case, the second-half of the listener will not 
; be called.
; 
; If the data is to be retained, the first-half listener must return with 
; registers holding the following values:
; 
; 	DS:BX	Segment:Offset of buffer for packet data
; 	CX	Size of the buffer at DS:BX
; 	ES:DX	Segment:Offset of second-half listener

ifdef Microsoft
_listen1	PROC FAR
else
listen1		PROC FAR
endif
	push	bp			; Create stack frame
	mov	bp, sp
	sub	sp, 6
;
;	datalen = -6
;	dataptr = -4
;
;	unsigned	datalen;
;	char		*dataptr;

;
;	listen1_c (socket, headlen, header, &datalen, &dataptr);
;
	lea	dx, WORD PTR [bp-4]	; Push
	push	ss			;  Segment:
	push	dx			;  Offset of dataptr
	lea	dx, WORD PTR [bp-6]	; Push
	push	ss			;  Segment:
	push	dx			;  Offset of datalen
	push	ds			; Segment:
	push	bx			;  Offset of pointer to LAP & DDP header
	push	cx			; Length of data
	mov	al, ah			; Socket
	xor	ah, ah			;  widened
	push	ax			;  to int
ifdef Microsoft
    mov ax, seg _listen_ds  ; Load DS with data segment for
	mov	ds, ax			;  listen1_c
	mov	ds, word ptr _listen_ds	; 
	call	FAR PTR _listen1_c	; call listen1_c
else
	mov	ax, seg listen_d	; Load DS with data segment for 
	mov	ds, ax			;  listen1_c
	mov	ds, word ptr listen_d	; 
	call	FAR PTR listen1_	; call listen1_c
endif
	add	sp, 16			; Remove argument list

	mov	ds, [bp-2]		; Segment:
	mov	bx, [bp-4]		;  Offset of buffer for packet data
	mov	cx, [bp-6]		; Size of buffer at DS:BX
	mov	dx, cs			; Segment:
	mov	es, dx			;  ...
ifdef Microsoft
	lea	dx, _listen2		;  Offset of second-half listener
else
	lea	dx, listen2		;  Offset of second-half listener
endif

	mov	sp, bp			; Remove stack frame
	pop	bp
	ret	
ifdef Microsoft
_listen1	ENDP
else
listen1		ENDP
endif

; Second-half DDP socket listener (asm part)
; 
; The second-half of the listener is called (through a FAR CALL) with
; registers set as follows:
;
; 	DS:BX	Segment:Offset of buffer containing data
; 	CX	Number of bytes of data in buffer at DS:BX
; 
; The second-half listener may enable interrupts, and should exit with a FAR 
; RETURN.

ifdef Microsoft
_listen2	PROC FAR
else
listen2		PROC FAR
endif
;
;	listen2_c (datalen, dataptr);
;
	push	ds			; Segment:
	push	bx			;  Offset of packet data buffer 
	push	cx			; Number of bytes of data in buffer
ifdef Microsoft
	mov	ax, seg _listen_ds	; Load DS with data segment for 
    mov ds, ax              ;  listen2_c
    mov ds, word ptr _listen_ds
	call	FAR PTR _listen2_c	; call listen2_c
else
	mov	ax, seg listen_d	; Load DS with data segment for 
	mov	ds, ax			;  listen2_c
	mov	ds, word ptr listen_d	; 
	call	FAR PTR listen2_	; call listen2_c
endif
	add	sp, 6			; Remove argument list

	ret	
ifdef Microsoft
_listen2	ENDP
else
listen2		ENDP
endif

LTListen_TEXT   ENDS

	END

