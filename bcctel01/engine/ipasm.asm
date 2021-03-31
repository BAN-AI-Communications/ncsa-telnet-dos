;
;  TCP/IP support routines
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
	NAME	IPASM

ifndef NET14
ifdef Microsoft
;ifdef Watcom
;    EXTRN   netsleep_:FAR   ; C routine which gets called from handler
;else
    EXTRN   _netsleep:FAR   ; C routine which gets called from handler
;endif
	PUBLIC	_TINST,_TDEINST
else	
        EXTRN   netsleep:FAR    ; C routine which gets called from handler
	PUBLIC	TINST,TDEINST
endif
endif ;Net14

;Microsoft EQU 1
;Lattice EQU 1
ifndef Microsoft
    ifndef Lattice
        if2
            %out
            %out ERROR: You have to specify "/DMicrosoft" or "/DLattice" on the
            %out        MASM command line to determine the type of assembly.
            %out
        endif
        end
    endif
endif

;******************************************************************
;*
;*	We need to set up a stack for netsleep when we exit to DOS.
;NEWSTACK SEGMENT PARA STACK 'STACK'
;    dw 2048 dup(?)
;STACKEND label far
;NEWSTACK    ends

 X   EQU     6
ifdef Microsoft
.MODEL  LARGE
.DATA
else	
	INCLUDE	DOS.MAC
	SETX
	DSEG
endif

OLDSS dw 1 dup(?)
OLDSP dw 1 dup(?)

ifndef NET14
NEWSTACK dw 2048 dup(?)     ; define a stack for netsleep when we shell to DOS
STACKEND label word
endif

ifdef Microsoft
;_DATA ends
;
;_TEXT	segment public 'CODE'
;	assume CS:_TEXT
.CODE
    PUBLIC  _IPCHECK, _TCPCHECK, _MOVEBYTES, _LONGSWAP, _INTSWAP
    PUBLIC  _COMPAREN
;ifdef Watcom
    PUBLIC  __STK
;endif
;   PUBLIC  _DBG

;was above tinst

else
	ENDDS
	PSEG
    PUBLIC  IPCHECK,TCPCHECK,MOVEBYTES,LONGSWAP,INTSWAP
    PUBLIC  COMPAREN
;   PUBLIC  DBG
endif	
;
;  Routines for general use by the communications programs
;
;
ifdef NOT_USED
;************************************************************************
;  DBG
;  provides a synch point for debugging
;
ifdef Microsoft
_dbg 	proc	far
else
dbg	proc	far
endif
	nop
	nop
	nop
	ret
ifdef Microsoft
_dbg endp
else
dbg endp
endif
endif           ; NOT_USED
;
;*************************************************************************
;  Internet header checksum
;    header checksum is calculated for a higher level program to verify
;
;  USAGE:  ipcheck((IPKT *)ptr,(int)len)
;
;  this proc knows that the IP header length is found in the first byte
;
ifdef Microsoft
_IPCHECK	PROC	FAR
else
IPCHECK	PROC	FAR
endif
	PUSH	BP
	MOV		BP,SP
	PUSH	DS
;    PUSH    ES
	PUSH	SI
;    PUSH    DI

ifdef OLD_WAY
	MOV		AX,[BP+X+2]		; ds for input data
	MOV		DS,AX
	MOV		SI,[BP+X]		; pointer to data
else
    LDS     SI,[BP+X]       ; Get the pointer to the data
endif
    MOV     CX,[BP+X+4]     ; count of words to test
	XOR		BX,BX
	CLC
CHKSUM:
	LODSW					; get next word
	ADC		BX,AX			; keep adding
	LOOP	CHKSUM			; til' done
	ADC		BX,0			; adds the carry bit in
;
	NOT		BX				; take one more 1-complement
	MOV		AX,BX

;    POP     DI
	POP		SI
;    POP     ES
	POP		DS
	POP		BP
	RET
ifdef Microsoft
_IPCHECK	ENDP
else
IPCHECK	ENDP
endif
;
;  TCP checksum, has two parts, including support for a pseudo-header
;
;  usage:   tcpcheck(psptr,tcpptr,tcplen)
;            char *psptr,*tcpptr;  pointers to pseudo header and real header
;            int tcplen            length of tcp packet in checksum
;
ifdef Microsoft
_TCPCHECK	PROC	FAR
else
TCPCHECK	PROC	FAR
endif
	PUSH	BP
	MOV		BP,SP
	PUSH	DS
;    PUSH    ES
	PUSH	SI
;    PUSH    DI

ifdef OLD_WAY
	MOV		AX,[BP+X+2]		; ds for input data for pseudo-hdr
	MOV		DS,AX
	MOV		SI,[BP+X]		; pointer to data
    MOV     CX,6            ; length of p-hdr in words
	XOR		BX,BX           ; clear to begin
	CLC
PCHKSUM:
	LODSW					; get next word
	ADC		BX,AX			; keep adding
	LOOP	PCHKSUM			; til' done
	ADC		BX,0			; adds the carry bit in
else
    LDS     SI,[BP+X]       ; Get the pointer to the data
;    MOV     CX,6            ; length of p-hdr in words
	XOR		BX,BX           ; clear to begin
	CLC
;PCHKSUM:
; Un-roll the loop for faster execution.
	LODSW					; get next word
	ADC		BX,AX			; keep adding
    LODSW                   ; get next word
	ADC		BX,AX			; keep adding
    LODSW                   ; get next word
	ADC		BX,AX			; keep adding
    LODSW                   ; get next word
	ADC		BX,AX			; keep adding
    LODSW                   ; get next word
	ADC		BX,AX			; keep adding
    LODSW                   ; get next word
	ADC		BX,AX			; keep adding
;    LOOP    PCHKSUM         ; til' done
	ADC		BX,0			; adds the carry bit in
endif
;
; NOW THE REAL THING
;
ifdef OLD_WAY
	MOV		AX,[BP+X+6]		; ds of real stuff
	MOV		DS,AX
	MOV		SI,[BP+X+4]		; pointer
else
    LDS     SI,[BP+X+4]     ; Get the pointer to the real stuff
endif
	MOV		CX,[BP+X+8]		; count of bytes to test
	MOV		DX,CX			; keep a copy
	SHR		CX,1			; divide by two, round down
	CLC
RCHKSUM:
	LODSW
	ADC		BX,AX			; add to previous running sum
	LOOP	RCHKSUM	
	ADC		BX,0			; add the last carry in again
	AND		DX,1			; odd # of bytes?
	JZ		NOTODD
	LODSB					; get that last byte
	XOR		AH,AH			; clear the high portion
	ADD		BX,AX			; add the last one in
	ADC		BX,0			; add the carry in, too
NOTODD:
	NOT		BX				; take one more 1-complement
	MOV		AX,BX

;    POP     DI
	POP		SI
;    POP     ES
	POP		DS
	POP		BP
	RET
ifdef Microsoft	
_TCPCHECK	ENDP
else
TCPCHECK	ENDP
endif

;
;********************************************************************
;  New movebytes
;  Move an arbitrary number of bytes from one location to another.
;
;  Usage:
;  movebytes(to,from,count)
;   char *to,*from;
;   int16 count
;   moves < 64K from one 4 byte pointer to another.  Does not handle
;   overlap, but does copy quickly.
;
ifdef Microsoft
_MOVEBYTES	PROC	FAR
else
MOVEBYTES	PROC	FAR
endif
        PUSH    BP
	MOV		BP,SP
	PUSH	DS
	PUSH	ES
	PUSH	SI
	PUSH	DI

	LES		DI,[BP+X]			; WHERE TO PUT IT
	LDS		SI,[BP+X+4]			; WHERE TO GET IT
	MOV		CX,[BP+X+8]			; HOW MANY TO MOVE
	SHR 	CX,1				; MAKE INTO A WORD COUNT
	REP 	MOVSW
	ADC		CX,CX				; GET THE ODD BYTE COUNT BACK
	REP 	MOVSB
	POP		DI
	POP		SI
	POP		ES
	POP		DS
	POP		BP
	RET
ifdef Microsoft
_MOVEBYTES	ENDP
else
MOVEBYTES	ENDP
endif

;
;********************************************************************
;  New comparen
;  Take n bytes and return identical (true, not zero) or not identical (false=0)
;
;  Usage:
;  comparen(s1,s2,count)
;   char *s1,*s2;
;   int16 count
;
ifdef Microsoft
_COMPAREN  PROC    FAR
else
COMPAREN   PROC    FAR
endif
	PUSH	BP
	MOV		BP,SP
	PUSH	DS
	PUSH	ES
	PUSH	SI
	PUSH	DI

    LES     DI,[BP+X]           ; First string
    LDS     SI,[BP+X+4]         ; Second string
    MOV     CX,[BP+X+8]         ; How many to compare
    REPE    CMPSB               ; Compare the strings
    LAHF                        ; Get the flags
    AND     AX,4000h            ; Mask all the flags except the zero flag

	POP		DI
	POP		SI
	POP		ES
	POP		DS
	POP		BP
	RET
ifdef Microsoft
_COMPAREN   ENDP
else
COMPAREN    ENDP
endif

;
;*************************************************************************
;  longswap
;    swap the bytes of a long integer from PC
;  order (reverse) to in-order.  This will work both ways.
;  returns the new long value
;  usage:
;      l2 = longswap(l)
;	long l;
;
ifdef Microsoft
_LONGSWAP	PROC	FAR
	PUSH	BP
	MOV		BP,SP

	MOV		AX,[BP+X+2]		; HIGH BYTES OF THE LONG INT
	MOV		DX,[BP+X]		; LOW BYTES OF THE LONG INT
;
;  GET THE DATA
;
	XCHG	AH,AL			; SWAP THEM, THESE ARE NOW LOW
	XCHG	DH,DL			; SWAP THE OTHERS
	POP		BP
	RET
_LONGSWAP	ENDP
else
LONGSWAP	PROC	FAR
	PUSH	BP
	MOV		BP,SP
	MOV		BX,[BP+X+2]		; HIGH BYTES OF THE LONG INT
	MOV		AX,[BP+X]		; LOW BYTES OF THE LONG INT
;
;  GET THE DATA
;
	XCHG	AH,AL			; SWAP THEM, THESE ARE NOW LOW
	XCHG	BH,BL			; SWAP THE OTHERS
	POP		BP
	RET
LONGSWAP	ENDP
endif
;
;*************************************************************************
;  INTSWAP
;    swap the bytes of an integer, returns the swapped integer
;
;   usage:      i = intswap(i);
;
ifdef Microsoft
_INTSWAP	PROC	FAR
else
INTSWAP	PROC	FAR
endif
	MOV		BX,SP
	MOV 	AX,SS:[BX+4]
	XCHG	AH,AL
	RET
ifdef Microsoft
_INTSWAP	ENDP
else
INTSWAP	ENDP
endif

ifndef NET14
;
;**************************************************************************
;
;  Routines to install and deinstall a timer routine which calls
;  netsleep(0);
;  The timer is set to go off every 1/2 second to check for packets 
;  in the incoming packet buffer.  We use the user-hook into the system 
;  timer which occurs every 1/18th of a second.
;
;
TIMEINT     EQU 4*1CH       ; User hook to timer int

;*************************************************************************
;
;  Take out the timer interrupt handler, restore previous value
;
ifdef Microsoft
_TDEINST	PROC	FAR
else
TDEINST	PROC	FAR
endif
	MOV		CX,CS:TIP		; GET OLD IP FROM SAVE SPOT
	MOV		DX,CS:TCS		; GET OLD CS FROM SAVE SPOT
	MOV		BX,TIMEINT		; INTERRUPT IN TABLE FOR TIMER
	PUSH	DS
	XOR		AX,AX			; SYSTEM INTERRUPT TABLE
	MOV		DS,AX		
	CLI
	MOV		[BX],CX			; STORE OLD IP INTO THE TABLE
	INC		BX
	INC		BX				; MOVE POINTER IN INTERRUPT TABLE
	MOV		[BX],DX			; STORE OLD CS INTO THE TABLE
	STI
	POP		DS
	RET
ifdef Microsoft
_TDEINST	ENDP
else
TDEINST	ENDP
endif
;
;
;  install the timer interrupt handler, the handler is technically
;  part of this procedure.
;
ifdef Microsoft
_TINST	PROC	FAR
else
TINST	PROC	FAR
endif
    XOR     AX,AX
    MOV     CS:TENTER,AL    ; CLEAR THIS FLAG
	MOV		CS:TMYDS,DS		; STORE FOR USE BY HANDLER
	MOV		BX,TIMEINT		; INTERRUPT IN TABLE FOR TIMER (1c)
	PUSH	DS
	XOR		AX,AX			; SYSTEM INTERRUPT TABLE
	MOV		DS,AX		
	MOV		AX,OFFSET THAND	; WHERE THE HANDLER IS
	CLI
	MOV		DX,[BX]			; KEEP COPY OF THE IP
	MOV		[BX],AX			; STORE IP INTO THE TABLE
	INC		BX
	INC		BX				; MOVE POINTER IN INTERRUPT TABLE
	MOV		CX,[BX]			; KEEP COPY OF THE CS, TOO
	MOV		AX,CS
	MOV		[BX],AX			; STORE NEW CS INTO THE TABLE
	STI
	POP	DS
	MOV	CS:TIP,DX			; STORE THEM AWAY
	MOV	CS:TCS,CX
	RET
;
;  Code segment addressable data for keeping track of the interrupt handler
;  stuff
;
TMYDS		DW	00H			; THE DATA SEGMENT FOR THIS ASSEMBLY CODE
TICNT       DB  00          ; COUNTER OF 1/18THS SEC
TENTER      DB  00
TIP  		DW  00
TCS  		DW  00

;
;   The handler itself.
;
THAND:                      ; not a public name, only handles ints
    STI
    PUSH    DS
	PUSH 	ES
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	DI
	PUSH	SI

    CLD                     ; ALL MOVES WILL BE FORWARD
    MOV     AL,CS:TENTER
    OR      AL,AL
    JNZ     TIME2
	MOV		AL,1
    MOV     CS:TENTER,AL    ; SET FLAG TO INDICATE BUSY
	INC		CS:TICNT
    MOV     AL,CS:TICNT     ; COUNTER FOR US
    AND     AL,7            ; SEE IF # MOD 8 = 0
    JNZ     TSKIP           ; SKIP 7 OUT OF 8 TIMES

;    MOV     AL,60H          ; EOI FOR TIMER INT
;    OUT     20H,AL          ; LET LOWER INTERRUPTS IN

;  SET UP CORRECT DS
    MOV     DS,CS:TMYDS     ; GET CORRECT DS

;  do we have to set up our own stack here?
ifdef HellWhyNot
    CLI
    MOV     AX,SS
	MOV		OLDSS,AX
    MOV     OLDSP,SP
;    MOV     AX,seg NEWSTACK
    MOV     AX,seg STACKEND
	MOV		SS,AX
    MOV     SP,OFFSET DGROUP:STACKEND
    STI
endif   ;HellWhyNot
    XOR     AX,AX
	PUSH 	AX

ifdef Microsoft
    CALL _netsleep
else
    CALL netsleep
endif

    POP     AX
ifdef HellWhyNot
    CLI
    MOV     AX,OLDSS
	MOV		SS,AX
    MOV     SP,OLDSP
    STI
endif

TSKIP:
	XOR		AL,AL
	MOV		CS:TENTER,AL	; REENTER FLAG, DONE NOW
TIME2:
    POP     SI
	POP		DI
	POP		DX
	POP		CX
	POP		BX
	POP		AX
	POP		ES
	POP		DS

; Pass through to other routines
	JMP 	DWORD PTR CS:TIP

ifdef Microsoft
_TINST		ENDP
else
TINST		ENDP
endif

endif       ; NET14

;************************************************************************
;  __STK
;  Routine stub to avoid stack overflow errors in Watcom
;
;ifdef Watcom
__STK    proc    far
	ret
__STK endp
;endif

ifdef Microsoft
;_TEXT ends
else
	ENDPS
endif
	END
