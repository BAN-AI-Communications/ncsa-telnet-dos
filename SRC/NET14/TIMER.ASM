;
;  TCP/IP timer support routines
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
    NAME    TIMER
;Microsoft EQU 1
;Lattice EQU 1
	ifndef Microsoft
		ifndef Lattice
			if2
				%out
				%out ERROR: You have to specify "/DMicrosoft" or "/DLattice" on the
				%out 		MASM command line to determine the type of assembly.
				%out
			endif
			end
		endif
	endif
;
;******************************************************************
;*
;*	We need to set up a stack for netsleep when we exit to DOS.

NEWSTACK SEGMENT PARA STACK 'STACK'
    dw 4096 dup(?)
STACKEND label far 
NEWSTACK	ends

X	EQU		6
ifdef Microsoft
;DGROUP  group _DATA
;_DATA segment public 'DATA'
;    assume DS:DGROUP
.MODEL	LARGE
.DATA
else	
	INCLUDE	DOS.MAC
	SETX
	DSEG
endif

OLDSS dw 1 dup(?)
OLDSP dw 1 dup(?)

ifdef Microsoft
;_DATA ends
;
;_TEXT	segment public 'CODE'
;	assume CS:_TEXT
.CODE
    EXTRN   _int14netsleep:FAR   ; C routine which gets called from handler
    PUBLIC  _TIMEINST,_TIMEDEINST
else
	ENDDS
	PSEG
    EXTRN   int14netsleep:FAR    ; C routine which gets called from handler
    PUBLIC  TIMEINST,TIMEDEINST
endif   
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
TIMEINT		EQU	4*1CH		; User hook to timer int

;*************************************************************************
;
;  Take out the timer interrupt handler, restore previous value
;
ifdef Microsoft
_TIMEDEINST    PROC    FAR
else
TIMEDEINST PROC    FAR
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
_TIMEDEINST    ENDP
else
TIMEDEINST ENDP
endif

;
;
;  install the timer interrupt handler, the handler is technically
;  part of this procedure.
;
ifdef Microsoft
_TIMEINST  PROC    FAR
else
TIMEINST   PROC    FAR
endif
	XOR		AX,AX
	MOV		CS:TENTER,AL	; CLEAR THIS FLAG
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
TICNT		DB	0			; COUNTER OF 1/18THS SEC
TENTER 		DB  00
TIP  		DW  00
TCS  		DW  00
;
;   The handler itself.
;
THAND:			       		; not a public name, only handles ints
	STI
	PUSH	DS
	PUSH 	ES
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	DI
	PUSH	SI

	CLD						; ALL MOVES WILL BE FORWARD
	MOV		AL,CS:TENTER
	OR		AL,AL
	JNZ		TIME2
	MOV		AL,1
	MOV		CS:TENTER,AL	; SET FLAG TO INDICATE BUSY
	INC		CS:TICNT
	MOV		AL,CS:TICNT		; COUNTER FOR US
ifdef QAK
	AND		AL,7			; SEE IF # MOD 8 = 0
else
    AND     AL,3            ; SEE IF # MOD 8 = 0
endif
	JNZ		TSKIP			; SKIP 7 OUT OF 8 TIMES
;
	MOV		AL,60H			; EOI FOR TIMER INT
	OUT		20H,AL			; LET LOWER INTERRUPTS IN
;
;  SET UP CORRECT DS
;
	MOV		DS,CS:TMYDS		; GET CORRECT DS
;
;  do we have to set up our own stack here?
;
	MOV		AX,SS
	MOV		OLDSS,AX
	MOV		OLDSP,SP
	CLI
	MOV		AX,seg NEWSTACK
	MOV		SS,AX
	MOV		SP,OFFSET STACKEND
	STI
;    XOR     AX,AX
;    PUSH    AX
ifdef Microsoft
    CALL _int14netsleep
else	
    CALL int14netsleep
endif
;    POP     AX
	CLI	
	MOV		AX,OLDSS
	MOV		SS,AX
	MOV		SP,OLDSP
	STI
TSKIP:
	XOR		AL,AL
	MOV		CS:TENTER,AL	; REENTER FLAG, DONE NOW
TIME2:
	POP 	SI
	POP		DI
	POP		DX
	POP		CX
	POP		BX
	POP		AX
	POP		ES
	POP		DS
;
;   forward to any other existing routines
;
	JMP 	DWORD PTR CS:TIP
ifdef Microsoft
_TIMEINST      ENDP
else
TIMEINST       ENDP
endif
ifdef Microsoft
;_TEXT ends

else
	ENDPS
endif
	END
