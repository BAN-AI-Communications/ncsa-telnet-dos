		page 60,132
;
;  Int 14h support routines
;****************************************************************************
;*                                                                          *
;*                                                                          *
;*      part of NCSA Telnet                                                 *
;*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
;*          Kurt Mahan, Heeren Pathak, Quincey Koziol, & Chris Wilson       *
;*                                                                          *
;*      National Center for Supercomputing Applications                     *
;*      152 Computing Applications Building                                 *
;*      605 E. Springfield Ave.                                             *
;*      Champaign, IL  61820                                                *
;*                                                                          *
;****************************************************************************
;*                                                                *
;* DATE                   REASON                      DEVELOPER   *
;* ----                   ------                      ---------   *
;* 01/03/92   Update the timer ISR to prevent lock ups    LJD     *
;*                                                                *
;******************************************************************

;
    NAME    INT14
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

X   EQU     6
PORT    EQU 1

ifdef Microsoft
.8086
.MODEL	LARGE
.DATA
else
	INCLUDE	DOS.MAC
	SETX
	DSEG
endif
	even
TIM_OLDSS           dw ?
TIM_OLDSP           dw ?
ifndef Watcom
TIM_OLDSTKHQQ       dw 0
endif

SER_OLDSS           dw ?
SER_OLDSP           dw ?
ifndef Watcom
SER_OLDSTKHQQ       dw 0
endif

    even            ; Align the stack on an even boundary
SER_NEWSTACK dw 1024 dup(?)     ; define a stack for netsleep when we shell to DOS
SER_STCK label word

;/* rmg  Added TIMstack.  Having both interrupts use the same stack didn't
;                         look completely safe.  I could be wrong.  931100
    even            ; Align the stack on an even boundary
TIM_NEWSTACK dw 1024 dup(?)     ; define a stack for netsleep when we shell to DOS
TIM_STCK label word

ifndef Watcom
extrn _STKHQQ:word           ; Microsoft heap/stack bound
endif

NUM_COMM_PORTS  EQU     4

ifdef Microsoft
ifdef QAK
PUBLIC  _INITIALIZED_FLAGS
endif
PUBLIC  _CONNECTED_FLAGS
PUBLIC  _PORT_BUFFER
ifdef QAK
PUBLIC  _OPENING_FLAGS       ; FOR DEBUGGING
PUBLIC  _BUFFER_OFFSET       ; FOR DEBUGGING
endif
PUBLIC  _PNUM,_DATA_BEGIN,_DATA_MAX,_DATA_END,_DATA_START
EXTRN   _PRINT_INT:PROC         ; FOR DEBUGGING
EXTRN   _PRINT_INT2:PROC        ; FOR DEBUGGING
EXTRN   _GET_COMM_CHAR:PROC     ; GET A CHARACTER FROM A COMM BUFFER
EXTRN   _NETCLOSE:PROC          ; EXTERNAL PROCEDURE TO CLOSE A CONNECTION
EXTRN   _NETWRITE:PROC          ; EXTERNAL PROCEDURE TO WRITE TO A CONNECTION
EXTRN   _INT14OPEN:PROC         ; EXTERNAL PROCEDURE TO OPEN A CONNECTION
EXTRN   _SPEEDUP:WORD           ; EXTERNAL TIMER SPEEDUP FACTOR

_PNUM   DW NUM_COMM_PORTS DUP(0)    ; PORT WE ARE CONNECTED TO

ifdef QAK
_INITIALIZED_FLAGS DB 0         ; A FLAG IN EACH BIT POSITION TO INDICATE THAT
                                ; COMM. PORT IS INITIALIZED (BIT 0 IS COMM PORT
                                ; 0, BIT 1 IF COMM. PORT 1, ETC..)
endif
_CONNECTED_FLAGS DB 0           ; A FLAG IN EACH BIT POSITION TO INDICATE THAT
                                ; COMM. PORT IS CONNECTED (BIT 0 IS COMM PORT
                                ; 0, BIT 1 IS COMM. PORT 1, ETC..)
_OPENING_FLAGS DB 0             ; A FLAG IN EACH BIT POSITION TO INDICATE THAT
                                ; COMM. PORT IS OPENING (BIT 0 IS COMM PORT
                                ; 0, BIT 1 IS COMM. PORT 1, ETC..)
_PORT_BUFFER    DB  NUM_COMM_PORTS DUP (64 DUP (0)) ; SPECIFY THE FOUR BUFFERS FOR THE CONNECTION NAMES
_BUFFER_OFFSET  DB  NUM_COMM_PORTS DUP (0)   ; THE OFFSETS INTO THE FOUR PORT BUFFERS
_CHAR_TO_SEND   DB 0            ; SPACE TO STORE THE CHARACTER TO SEND ON THE NET
_DATA_BEGIN     DD NUM_COMM_PORTS DUP(0); ARRAY OF POINTERS TO THE BEGINNING OF
                                ;   THE DATA BUFFER FOR EACH COMM. PORT
_DATA_END       DD NUM_COMM_PORTS DUP(0); ARRAY OF POINTERS TO THE END OF
                                ;   THE DATA BUFFER FOR EACH COMM. PORT
_DATA_MAX       DD NUM_COMM_PORTS DUP(0); ARRAY OF POINTERS TO THE MAX OF
                                ;   THE DATA BUFFERS FOR EACH COMM. PORT
_DATA_START     DD NUM_COMM_PORTS DUP(0); ARRAY OF POINTERS TO THE START OF
                                ;   THE DATA BUFFERS FOR EACH COMM. PORT
_CLOCK_TICK     DW  0           ;   The number of clock ticks which have elapsed
else
PUBLIC  CONNECTED_FLAGS
PUBLIC  PORT_BUFFER
ifdef QAK
PUBLIC  INITIALIZED_FLAGS
PUBLIC  OPENING_FLAGS       ; FOR DEBUGGING
PUBLIC  BUFFER_OFFSET       ; FOR DEBUGGING
endif
PUBLIC  PNUM,DATA_BEGIN,DATA_MAX,DATA_END,DATA_START
EXTRN   PRINT_INT:PROC          ; FOR DEBUGGING
EXTRN   PRINT_INT2:PROC         ; FOR DEBUGGING
EXTRN   GET_COMM_CHAR:PROC      ; GET A CHARACTER FROM A COMM BUFFER
EXTRN   NETCLOSE:PROC           ; EXTERNAL PROCEDURE TO CLOSE A CONNECTION
EXTRN   NETWRITE:PROC           ; EXTERNAL PROCEDURE TO WRITE TO A CONNECTION
EXTRN   INT14OPEN:PROC          ; EXTERNAL PROCEDURE TO OPEN A CONNECTION
EXTRN   SPEEDUP:WORD            ; EXTERNAL TIMER SPEEDUP FACTOR
PNUM   DW NUM_COMM_PORTS DUP(0)             ; PORT WE ARE CONNECTED TO

ifdef QAK
INITIALIZED_FLAGS DB 0          ; A FLAG IN EACH BIT POSITION TO INDICATE THAT
                                ; COMM. PORT IS INITIALIZED (BIT 0 IS COMM PORT
                                ; 0, BIT 1 IF COMM. PORT 1, ETC..)
endif
CONNECTED_FLAGS DB 0            ; A FLAG IN EACH BIT POSITION TO INDICATE THAT
                                ; COMM. PORT IS CONNECTED (BIT 0 IS COMM PORT
                                ; 0, BIT 1 IS COMM. PORT 1, ETC..)
OPENING_FLAGS DB 0              ; A FLAG IN EACH BIT POSITION TO INDICATE THAT
                                ; COMM. PORT IS OPENING (BIT 0 IS COMM PORT
                                ; 0, BIT 1 IS COMM. PORT 1, ETC..)
PORT_BUFFER     DB  NUM_COMM_PORTS DUP (64 DUP (0)) ; SPECIFY THE FOUR BUFFERS FOR THE CONNECTION NAMES
BUFFER_OFFSET   DB  NUM_COMM_PORTS DUP (0)   ; THE OFFSETS INTO THE FOUR PORT BUFFERS
CHAR_TO_SEND    DB 0             ; SPACE TO STORE THE CHARACTER TO SEND ON THE NET
DATA_BEGIN      DD NUM_COMM_PORTS DUP(0); ARRAY OF POINTERS TO THE BEGINNING OF
                                ;   THE DATA BUFFERS FOR EACH COMM. PORT
DATA_END        DD NUM_COMM_PORTS DUP(0); ARRAY OF POINTERS TO THE END OF
                                ;   THE DATA BUFFERS FOR EACH COMM. PORT
DATA_MAX        DD NUM_COMM_PORTS DUP(0); ARRAY OF POINTERS TO THE MAX OF
                                ;   THE DATA BUFFERS FOR EACH COMM. PORT
DATA_START      DD NUM_COMM_PORTS DUP(0); ARRAY OF POINTERS TO THE START OF
                                ;   THE DATA BUFFERS FOR EACH COMM. PORT
CLOCK_TICK      DW  0           ;   The number of clock ticks which have elapsed
endif

ifdef Microsoft
.CODE
else
	ENDDS
    PSEG
endif	
;
;**************************************************************************
;
;  Routines to install and deinstall a routine which manages the
;       serial port
;
;
SERIALINT     EQU 4*14H       ; User hook to timer int

ifdef Microsoft
    PUBLIC  _INT14INST,_INT14DEINST
ifdef DEBUG
    PUBLIC  _INT14CHECK,_INT14RECEIVE
endif
    EXTRN   _int14netsleep:FAR   ; C routine which gets called from handler
    PUBLIC  _TIMEINST,_TIMEDEINST
else    
    PUBLIC  INT14INST,INT14DEINST
ifdef DEBUG
    PUBLIC  INT14CHECK,INT14RECEIVE
endif
    EXTRN   int14netsleep:FAR   ; C routine which gets called from handler
    PUBLIC  TIMEINST,TIMEDEINST
endif   
;
;**************************************************************************
;
;  Routines to install and deinstall a timer routine which calls
;  netsleep(0);
;  The timer is set to go off every 1/18 second to check for packets
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
;
;$$$ 01-05-1992 LJD + Stop using the timer routine. !!! THIS NEEDS FURTHER TESTING. !!!!
;
	pushf
	cli
	mov	CS:INTENTER,-1		; Prevent use of the timer routine while removing it from the system
	popf
;
;$$$ 01-05-1992 LJD -
;

	MOV		CX,CS:TIP		; GET OLD IP FROM SAVE SPOT
	MOV		DX,CS:TCS		; GET OLD CS FROM SAVE SPOT
	MOV		BX,TIMEINT		; INTERRUPT IN TABLE FOR TIMER

	PUSH	DS
	XOR		AX,AX			; SYSTEM INTERRUPT TABLE
	MOV		DS,AX		

	PUSHF					; $$$ 01-03-92 LJD +-
	CLI

	MOV		[BX],CX			; STORE OLD IP INTO THE TABLE
;
;$$$ 01-03-1992 ljd +
;
;	INC		BX
;	INC		BX				; MOVE POINTER IN INTERRUPT TABLE
;	MOV		[BX],DX			; STORE OLD CS INTO THE TABLE
	MOV		[BX+2],DX			; STORE OLD CS INTO THE TABLE

	POPF
;	STI
;
;$$$ 01-03-1992 LJD -
;
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
    MOV     CS:INTENTER,AL    ; CLEAR THIS FLAG
    MOV     CS:_TMYDS,DS    ; STORE FOR USE BY HANDLER
	MOV		BX,TIMEINT		; INTERRUPT IN TABLE FOR TIMER (1c)

	PUSH	DS
	XOR		AX,AX			; SYSTEM INTERRUPT TABLE
	MOV		DS,AX		
	MOV		AX,OFFSET THAND	; WHERE THE HANDLER IS
;
;$$$ 01-03-1992 ljd +
;	
	PUSHF					; save the current flag states
	CLI

	MOV		DX,[BX]			; KEEP COPY OF THE IP
	MOV		[BX],AX			; STORE IP INTO THE TABLE
;	INC		BX
;	INC		BX				; MOVE POINTER IN INTERRUPT TABLE
;	MOV		CX,[BX]			; KEEP COPY OF THE CS, TOO
	MOV		CX,[BX+2]			; KEEP COPY OF THE CS, TOO

	MOV		AX,CS
	MOV		[BX+2],AX			; STORE NEW CS INTO THE TABLE

	POPF					; restore the int. flag to what it was originally
;	STI
;
;$$$ 01-03-1992 LJD -
;
	POP	DS
	MOV	CS:TIP,DX			; STORE THEM AWAY
	MOV	CS:TCS,CX
	RET
;
;  Code segment addressable data for keeping track of the interrupt handler
;  stuff
;
;PUBLIC  _TMYDS
_TMYDS      DW  00H         ; THE DATA SEGMENT FOR THIS ASSEMBLY CODE
TICNT		DB	0			; COUNTER OF 1/18THS SEC
INTENTER    DB  00
TIP  		DW  00
TCS  		DW  00
;
;   The handler itself.
;
;$$$ 01-03-1992 ljd + Serious modifications made!
;
THAND:			       		; not a public name, only handles ints
	cli				; make sure interrupts are off!
	push	ax
	push	ds
	push	es

;
; Call the routine we replaced  to proceed with chaining process.
; (Properly simulate the INT instruction.)
;
	pushf
	cli
	call	dword ptr cs:tip

	cli					; disable interrupts

	CLD						; SET THE DIRECTION FLAG FORWARD

	cmp	cs:intenter,0
	jnz	time2				; leave if already active

	inc	cs:intenter
	
ifdef SLOW
    INC     CS:TICNT
    MOV     AL,CS:TICNT     ; COUNTER FOR US
	AND		AL,7			; SEE IF # MOD 8 = 0
	JNZ		TSKIP			; SKIP 7 OUT OF 8 TIMES
endif
;
ifdef OLD_WAY
	MOV		AL,60H			; EOI FOR TIMER INT
	OUT		20H,AL			; LET LOWER INTERRUPTS IN
endif
;
;  SET UP CORRECT DS & ES
;
    MOV     DS,CS:_TMYDS     ; GET CORRECT DS
    MOV     ES,CS:_TMYDS     ; GET CORRECT ES
;
;  do we have to set up our own stack here?
;
	MOV		AX,SS
	MOV     TIM_OLDSS,AX			; save the stack segment set upon entry
	MOV     TIM_OLDSP,SP			; save the stack pointer set upon entry
;
;                                /* (was SER_STCK)  rmg 931100 */
    MOV     AX,seg DGROUP:TIM_STCK
    MOV     SS,AX                          ; set a new stack segment
    MOV     SP,OFFSET DGROUP:TIM_STCK      ; set a new stack pointer
;    MOV     AX,seg DGROUP:SER_STCK
;        MOV     SS,AX                           ; set a new stack segment
;    MOV     SP,OFFSET DGROUP:SER_STCK   ; set a new stack pointer

ifndef Watcom
    mov     ax,_STKHQQ                   ; save old end of stack
    mov     TIM_OLDSTKHQQ,ax
    mov     ax,offset DGROUP:SER_NEWSTACK          ; load new end of stack
    mov     _STKHQQ,ax
endif
	STI						; enable interrupts

	PUSH	DS
	PUSH 	ES
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	DI
	PUSH	SI

ifdef Microsoft
    CALL _int14netsleep
else	
    CALL int14netsleep
endif

	POP 		SI
	POP		DI
	POP		DX
	POP		CX
	POP		BX
	POP		AX
	POP		ES
	POP		DS


	cli
	dec	cs:intenter
	jnz	time2

	MOV     DS,CS:_TMYDS     ; GET CORRECT DS

	MOV     AX,TIM_OLDSS
	MOV	SS,AX			; RESTORE STACK SEGMENT
	MOV     SP,TIM_OLDSP		; RESTORE STACK POINTER

ifndef Watcom
    mov     ax,TIM_OLDSTKHQQ                ; restore end of stack
    mov     _STKHQQ,ax
endif

comment #
TSKIP:
	XOR		AL,AL
    MOV     CS:INTENTER,AL    ; REENTER FLAG, DONE NOW
#

TIME2:
;
;   forward to any other existing routines
;
	pop	es
	pop	ds
	pop	ax
    IRET
;
;$$$ 01-03-1992 ljd -
;
ifdef Microsoft
_TIMEINST      ENDP
else
TIMEINST       ENDP
endif

ifdef DEBUG
;*************************************************************************
;
;  Check the int14 receive status
;
ifdef Microsoft
_INT14CHECK    PROC    FAR
else
INT14CHECK PROC    FAR
endif
    MOV     AH,3
    MOV     DX,PORT
    INT     14H
    XOR     AL,AL
	RET
ifdef Microsoft
_INT14CHECK    ENDP
else
INT14CHECK ENDP
endif

;*************************************************************************
;
;  Get a character from int14
;
ifdef Microsoft
_INT14RECEIVE    PROC    FAR
else
INT14RECEIVE PROC    FAR
endif
    MOV     AH,2
    MOV     DX,PORT
    INT     14H
	RET
ifdef Microsoft
_INT14RECEIVE   ENDP
else
INT14RECEIVE ENDP
endif
endif

;*************************************************************************
;
;  Take out the serial interrupt handler, restore previous value
;
ifdef Microsoft
_INT14DEINST    PROC    FAR
else
INT14DEINST PROC    FAR
endif
    MOV     CX,CS:SIP       ; GET OLD IP FROM SAVE SPOT
    MOV     DX,CS:SCS       ; GET OLD CS FROM SAVE SPOT
    MOV     BX,SERIALINT    ; INTERRUPT IN TABLE FOR TIMER
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
_INT14DEINST    ENDP
else
INT14DEINST ENDP
endif
;
;
;  install the serial interrupt handler, the handler is technically
;  part of this procedure.
;
ifdef Microsoft
_INT14INST  PROC    FAR
else
INT14INST   PROC    FAR
endif

;
;$$$ 01-05-1992 LJD + Don't do this twice!
;	XOR		AX,AX
;    MOV     CS:INTENTER,AL    ; CLEAR THIS FLAG
;
;$$$ 01-05-1992 LJD -
;
    MOV     CS:_SMYDS,DS     ; STORE FOR USE BY HANDLER
    MOV     BX,SERIALINT    ; INTERRUPT IN TABLE FOR TIMER (1c)
	PUSH	DS
	XOR		AX,AX			; SYSTEM INTERRUPT TABLE
	MOV		DS,AX		
    MOV     AX,OFFSET SHAND ; WHERE THE HANDLER IS
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
    MOV CS:SIP,DX           ; STORE THEM AWAY
    MOV CS:SCS,CX
	RET
;
;  Code segment addressable data for keeping track of the interrupt handler
;  stuff
;
;PUBLIC  _SMYDS
_SMYDS       DW  00H         ; THE DATA SEGMENT FOR THIS ASSEMBLY CODE
SICNT       DB  0           ; COUNTER OF 1/18THS SEC
;SENTER      DB  00
SIP         DW  00
SCS         DW  00
;
;   The handler itself.
;
;$$$ 01-05-1992 LJD + Modify to protect protocol layer from reentrancy problems
;
;
SHAND:                      ; not a public name, only handles ints
	cli			; make sure interrupts are off
	PUSH	DS
	PUSH 	ES
	PUSH	AX
	PUSH	BX
	PUSH	CX
	PUSH	DX
	PUSH	DI
	PUSH	SI

	CLD			; ALL MOVES WILL BE FORWARD
TRYAGAIN:
	cmp	cs:intenter,0
	jz	no_re_entry
	sti			; enable interrupts
	nop
	nop	
	cli
	jmp	tryagain

NO_RE_ENTRY:
	inc	cs:intenter	; increment the flags (lock out the timer interrupt)
;
;	Now it is safe to enable interrupts!
;
	STI
;
;
;
;  SET UP CORRECT DS and ES
;
    MOV     DS,CS:_SMYDS     ; GET CORRECT DS
    MOV     ES,CS:_SMYDS     ; GET CORRECT ES

;
;  do we have to set up our own stack here?
;
    MOV     BX,SS
    MOV     SER_OLDSS,BX
    MOV     SER_OLDSP,SP
	CLI
    MOV     BX,seg DGROUP:SER_STCK
    MOV     SS,BX
    MOV     SP,OFFSET DGROUP:SER_STCK
ifndef Watcom
    mov     bx,_STKHQQ                   ; save old end of stack
    mov     SER_OLDSTKHQQ,bx
    mov     bx,offset DGROUP:SER_NEWSTACK          ; load new end of stack
    mov     _STKHQQ,bx
endif
    STI

    MOV     CX,DX           ; MOVE THE COMM. PORT INTO THE CX REGISTER
    MOV     BL,1            ; MOVE A FLAG INTO THE BL REGISTER
    SHL     BL,CL           ; SHIFT THE FLAG TO LINE UP WITH THE CORRECT INITIALIZATION FLAG


    CMP     AH,03           ; CHECK FOR READ STATUS
    JNE     NOT_STATUS_CHECK; A COMMUNICATION REQUEST WHICH WE DON'T HANDLE


READ_STATUS:                ; CHECK FOR A CHARACTER
ifdef Microsoft
    TEST    BL,_CONNECTED_FLAGS; CHECK WHETHER THIS PORT IS CONNECTED
else
    TEST    BL,CONNECTED_FLAGS; CHECK WHETHER THIS PORT IN CONNECTED
endif
    JNE     GET_PORT_STATUS         ; GET THE PORT STATUS
    MOV     AX,2000H                ; REPORT ERROR CONDITION
    JMP     SHORT STATUS_FINISHED   ;

GET_PORT_STATUS:
ifdef Microsoft
    MOV     SI,OFFSET _DATA_BEGIN   ; GET THE POINTER TO THE BEGINNING OF THE DATA BUFFER
    MOV     DI,OFFSET _DATA_END     ; GET THE POINTER TO THE END OF THE DATA BUFFER
else
    MOV     SI,OFFSET DATA_BEGIN    ; GET THE POINTER TO THE BEGINNING OF THE DATA BUFFER
    MOV     DI,OFFSET DATA_END      ; GET THE POINTER TO THE END OF THE DATA BUFFER
endif
    SHL     DX,1                    ; MULTIPLY THE PORT NUMBER BY 4 TO GET THE
    SHL     DX,1                    ;   OFFSET TO THE CORRECT ARRAY ELEMENT
    ADD     SI,DX                   ; INCREMENT TO THE CORRECT ARRAY ELEMENT
    ADD     DI,DX

    MOV     AX,DS:[SI]              ; GET THE OFFSET INTO THE DATA BUFFER OF THE BEGINNING
    CMP     AX,DS:[DI]              ; CHECK WHETHER THERE ARE CHARACTERS IN THE BUFFER
    JE      CHARACTER_NOT_READY     ; IF THE TWO POINTERS ARE THE SAME, THEN THERE IS NO DATA

    MOV     AX,2100h                ; SET THE DATA READY FLAG

;    JMP     STATUS_FINISHED
    JMP     SSKIP2                  ; JUMP TO THE END OF THE ROUTINE
CHARACTER_NOT_READY:
    MOV     AX,2000H                ; RESET THE DATA READY FLAG

STATUS_FINISHED:
    JMP     SSKIP2                  ; JUMP TO THE END OF THE ROUTINE

NOT_STATUS_CHECK:
    CMP     AH,02           ; CHECK FOR RECEIVING CHARACTER
    JNE     NOT_RECEIVE_CHARACTER;  JUMP AROUND RECEIVING A CHARACTER

RECEIVE_CHARACTER:          ; GET A CHARACTER
ifdef Microsoft
    TEST    BL,_CONNECTED_FLAGS; CHECK WHETHER THIS PORT IN CONNECTED
else
    TEST    BL,CONNECTED_FLAGS; CHECK WHETHER THIS PORT IN CONNECTED
endif
    JNE     GET_PORT_CHARACTER; GET A CHARACTER FROM THE PORT
    MOV     AH,80H          ; REPORT ERROR CONDITION
    JMP     SHORT RECEIVE_FINISHED   ;

GET_PORT_CHARACTER:
ifdef Microsoft
    MOV     SI,OFFSET _DATA_BEGIN   ; GET THE POINTER TO THE array of the BEGINNING OF THE DATA BUFFERs
    MOV     DI,OFFSET _DATA_END     ; GET THE POINTER TO THE END OF THE DATA BUFFER
else
    MOV     SI,OFFSET DATA_BEGIN    ; GET THE POINTER TO THE BEGINNING OF THE DATA BUFFER
    MOV     DI,OFFSET DATA_END      ; GET THE POINTER TO THE END OF THE DATA BUFFER
endif
    SHL     DX,1                    ; MULTIPLY THE PORT NUMBER BY 4 TO GET THE
    SHL     DX,1                    ;   OFFSET TO THE CORRECT ARRAY ELEMENT
    ADD     SI,DX                   ; INCREMENT TO THE CORRECT ARRAY ELEMENT
    ADD     DI,DX
WAIT_FOR_CHARACTER:
    MOV     AX,DS:[SI]              ; GET THE OFFSET INTO THE DATA BUFFER OF THE BEGINNING
    CMP     AX,DS:[DI]              ; CHECK WHETHER THERE ARE CHARACTERS IN THE BUFFER
    JE      WAIT_FOR_CHARACTER      ; IF THE TWO POINTERS ARE THE SAME, THEN WAIT FOR DATA

    PUSH    DS
    MOV     DI,AX                   ; SAVE THE OFFSET OF THE BEGINING POINTER
    MOV     AX,DS:[SI+2]            ; GET THE SEGMENT OF THE BEGINNING POINTER
    MOV     DS,AX
    MOV     AL,BYTE PTR DS:[DI]     ; GET THE CHARACTER FROM THE BEGINNING OF THE QUEUE
    POP     DS                      ; RECOVER THE DATA SEGMENT
ifdef Microsoft
    MOV     DI,OFFSET _DATA_MAX     ; GET THE POINTER TO THE MAX OF THE DATA BUFFER
else
    MOV     DI,OFFSET DATA_MAX      ; GET THE POINTER TO THE MAX OF THE DATA BUFFER
endif
    ADD     DI,DX                   ; INCREMENT TO THE CORRECT ARRAY ELEMENT
    INC     WORD PTR DS:[SI]        ; INCREMENT THE BEGINNING OF THE QUEUE
    MOV     CX,DS:[SI]              ; GET THE BEGINNING OF THE QUEUE
    CMP     CX,DS:[DI]              ; CHECK FOR WRAPPING AROUND
    JL      NOT_WRAPPED             ; JUMP AROUND WRAP-AROUND FIX
ifdef Microsoft
    MOV     DI,OFFSET _DATA_START   ; GET THE POINTER TO THE MAX OF THE DATA BUFFER
else
    MOV     DI,OFFSET DATA_START    ; GET THE POINTER TO THE MAX OF THE DATA BUFFER
endif
    ADD     DI,DX                   ; INCREMENT TO THE CORRECT ARRAY ELEMENT
    MOV     CX,DS:[DI]              ; GET THE START OF THE QUEUE
    MOV     DS:[SI],CX              ; MOVE THE BEGINNING OF THE QUEUE AROUND

NOT_WRAPPED:
;    MOV     AH,21                   ; REPORT SUCCESS
        xor     ah,ah

RECEIVE_FINISHED:
    JMP     SSKIP2                  ; JUMP TO THE END OF THE ROUTINE

NOT_RECEIVE_CHARACTER:
    CMP     AH,01           ; CHECK FOR SEND CHARACTER
    JE      SEND_CHARACTER  ; JUMP TO SENDING THE CHARACTER
    JMP     NOT_SEND_CHARACTER; JUMP AROUND SENDING THE CHARACTER

SEND_CHARACTER:             ; SEND A CHARACTER
ifdef Microsoft
    TEST    BL,_CONNECTED_FLAGS; CHECK WHETHER THIS PORT IN CONNECTED
    JE      DONT_NET_SEND   ; CONNECTION NOT OPEN YET, DON'T SEND THE CHARACTER
    JMP     NET_SEND        ; OK, THE CONNECTION IS INITIALIZED, SEND THE CHARACTER
DONT_NET_SEND:
    TEST    BL,_OPENING_FLAGS; IF THE OPENING FLAG IS SET FOR THE PORT, CACHE THE CHARACTER IN A BUFFER
else
    TEST    BL,CONNECTED_FLAGS; CHECK WHETHER THIS PORT IN CONNECTED
    JE      DONT_NET_SEND   ; CONNECTION NOT OPEN YET, DON'T SEND THE CHARACTER
    JMP     NET_SEND        ; OK, THE CONNECTION IS INITIALIZED, SEND THE CHARACTER
DONT_NET_SEND:
    TEST    BL,OPENING_FLAGS; IF THE OPENING FLAG IS SET FOR THE PORT, CACHE THE CHARACTER IN A BUFFER
endif
    JNE     CACHE_PORT_NAME ; GO CACHE THE CHARACTER
              ; /* RMG */ Location of Hardcoded ^B
    CMP     AL,02           ; CHECK WHETHER THIS IS THE BEGINNING OF A PORT NAME
    JE      START_PORT_NAME ; START CACHING THE PORT NAME
    MOV     AH,80H          ; REPORT ERROR CONDITION
    JMP     SEND_FINISHED   ;

START_PORT_NAME:            ; INITIALIZE THE PORT NAME CACHING
ifdef Microsoft
    OR      _OPENING_FLAGS,BL; SET THE OPENING FLAG
else
    OR      OPENING_FLAGS,BL; SET THE OPENING FLAG
endif
    JMP     SEND_DONE       ; INDICATE A SUCCESSFUL CHARACTER SEND, BUT DON'T REALLY

CACHE_PORT_NAME:            ; STUFF THE CHARACTER INTO A BUFFER, OR OPEN A CONNECTION
    CMP     AL,03H          ; CHECK WHETHER THIS IS THE TERMINATING CHARACTER IN A PORT NAME
    JE      NET_OPEN        ; GO, OPEN THE CONNECTION
ifdef Microsoft
    MOV     DI,OFFSET _BUFFER_OFFSET; GET THE OFFSET INTO THE COMM. PORT BUFFER
else
    MOV     DI,OFFSET BUFFER_OFFSET; GET THE OFFSET INTO THE COMM. PORT BUFFER
endif
    ADD     DI,DX           ; INCREMENT TO THE CORRECT COMM. PORT OFFSET
    MOV     CL,AL           ; SAVE THE CHARACTER TEMPORARILY
    MOV     AL,64           ; MOVE THE SIZE OF THE PORT NAME BUFFER INTO AL
    MUL     DL              ; GET THE OFFSET OF THE PORT BUFFER TO USE
    ADD     AX,DS:[DI]      ; GET THE OFFSET TO STORE THE CHARACTER AT
    INC     BYTE PTR DS:[DI]    ; INCREMENT THE BUFFER OFFSET FOR THE COMM. PORT
    CMP     BYTE PTR DS:[DI],64 ; CHECK WHETHER WE HAVE TOO LONG OF A NAME
    JE      NAME_TOO_LONG   ; JUMP FOR TOO LONG OF A NAME
ifdef Microsoft
    MOV     SI,OFFSET _PORT_BUFFER; GET THE OFFSET INTO THE COMM. PORT BUFFER
else
    MOV     SI,OFFSET PORT_BUFFER; GET THE OFFSET INTO THE COMM. PORT BUFFER
endif
    ADD     SI,AX           ; GET THE CORRECT OFFSET TO STORE THIS CHARACTER AT
    MOV     AL,CL           ; RESTORE THE CHARACTER TO STORE
    MOV     [SI],AL         ; APPEND THE CHARACTER TO THE PORT NAME
    JMP     SHORT SEND_DONE       ; INDICATE A GOOD TRANSFER

NAME_TOO_LONG:              ; THE PORT NAME WAS TOO LONG
    MOV     BYTE PTR DS:[DI],0  ; RESET THE LENGTH OF THE PORT NAME
    NOT     BL              ;
ifdef Microsoft
    AND     _OPENING_FLAGS,BL; RESET THE OPENING FLAG
else
    AND     OPENING_FLAGS,BL; RESET THE OPENING FLAG
endif
    MOV     AL,CL           ; RESTORE THE CHARACTER TO SEND
    MOV     AH,80H          ; INDICATE ERROR CONDITION
    JMP     SHORT SEND_FINISHED   ; JUMP TO THE END OF THE ROUTINE

NET_OPEN:                   ; OPEN THE CONNECTION TO THE NETWORK
    NOT     BL              ;
ifdef Microsoft
    AND     _OPENING_FLAGS,BL; RESET THE OPENING FLAG
    MOV     DI,OFFSET _BUFFER_OFFSET; GET THE OFFSET INTO THE COMM. PORT BUFFER
else
    AND     OPENING_FLAGS,BL; RESET THE OPENING FLAG
    MOV     DI,OFFSET BUFFER_OFFSET; GET THE OFFSET INTO THE COMM. PORT BUFFER
endif
    ADD     DI,DX           ; INCREMENT TO THE CORRECT COMM. PORT OFFSET
    MOV     CL,AL           ; SAVE THE CHARACTER TEMPORARILY
    MOV     AL,64           ; MOVE THE SIZE OF THE PORT NAME BUFFER INTO AL
    MUL     DL              ; GET THE OFFSET OF THE PORT BUFFER TO USE
    ADD     AX,DS:[DI]      ; GET THE OFFSET TO STORE THE CHARACTER AT
ifdef Microsoft
    MOV     SI,OFFSET _PORT_BUFFER; GET THE OFFSET INTO THE COMM. PORT BUFFER
else
    MOV     SI,OFFSET PORT_BUFFER; GET THE OFFSET INTO THE COMM. PORT BUFFER
endif
    PUSH    BX              ; SAVE THE COMM. PORT FLAGS
    ADD     SI,AX           ; GET THE CORRECT OFFSET TO STORE THIS CHARACTER AT
    MOV     AL,CL           ; RESTORE THE CHARACTER TO STORE
    MOV     BYTE PTR [SI],0 ; APPEND THE TERMINATING ZERO

    PUSH    DX              ; PUSH THE COMM. PORT WE ARE OPENING

ifdef Microsoft
    CALL    _INT14OPEN      ; OPEN THE CONNECTION
else
    CALL    INT14OPEN       ; OPEN THE CONNECTION
endif
    POP     DX              ; RECOVER THE COMM. PORT WE USED
    POP     BX              ; RECOVER THE COMM. PORT FLAGS
    CMP     AX,1            ; CHECK FOR GOOD OPENING
    JNE     BAD_NET_OPEN    ; CONNECTION DIDN'T OPEN

    NOT     BL              ;
ifdef Microsoft
    OR      _CONNECTED_FLAGS,BL; SET THE CONNECTED FLAG
    MOV     SI,OFFSET _BUFFER_OFFSET; GET THE OFFSET INTO THE COMM. PORT BUFFER
else
    OR      CONNECTED_FLAGS,BL; SET THE CONNECTED FLAG
    MOV     SI,OFFSET BUFFER_OFFSET; GET THE OFFSET INTO THE COMM. PORT BUFFER
endif

    ADD     SI,DX           ; INCREMENT TO THE CORRECT COMM. PORT OFFSET
    MOV     BYTE PTR [SI],0     ; ZERO OUT THE BUFFER OFFSET FOR THE COMM. PORT
    JMP     SHORT SEND_DONE       ; INDICATE A SUCCESSFUL CONNECTION OPENING

BAD_NET_OPEN:
    MOV     AX,80           ; INDICATE A BAD NETWORK OPEN
    JMP     SHORT SEND_FINISHED   ; RETURN FROM THE INTERRUPT

NET_SEND:                   ; SEND THE CHARACTER IN AL ONTO THE NET
ifdef Microsoft
    MOV     _CHAR_TO_SEND,AL ; STORE THE CHARACTER TO SEND ON THE NET
else
    MOV     CHAR_TO_SEND,AL ; STORE THE CHARACTER TO SEND ON THE NET
endif

    MOV     AX,1            ; THE NUMBER OF CHARACTERS TO DROP ONTO THE NET
    PUSH    AX
    PUSH    DS              ; PUSH THE SEGMENT ADDRESS OF THE CHARACTER TO SEND
    MOV     AX,OFFSET _CHAR_TO_SEND ; PUSH THE ADDRESS OF THE CHARACTER TO SEND
    PUSH    AX
ifdef Microsoft
    MOV     SI,OFFSET _PNUM ; GET THE OFFSET INTO THE COMM. PORT NETWORK PORT NUMBERS
else
    MOV     SI,OFFSET PNUM  ; GET THE OFFSET INTO THE COMM. PORT NETWORK PORT NUMBERS
endif
    ADD     SI,DX           ; INCREMENT TO THE CORRECT COMM. PORT OFFSET
    ADD     SI,DX           ; TWICE BECAUSE THESE ARE INTEGERS, NOT BYTES
    PUSH    [SI]            ; PUSH THE NETWORK PORT NUMBER
ifdef Microsoft
    CALL    _NETWRITE       ; CLOSE THE CONNECTION
else
    CALL    NETWRITE        ; CLOSE THE CONNECTION
endif
    ADD     SP,8            ; RESTORE THE STACK FRAME
;   JMP     SEND_DONE       ; INDICATE A SUCCESSFUL DATA SEND

SEND_DONE:                  ; INDICATE A SUCCESSFUL CHARACTER SEND
;    MOV     AH,060h         ; INDICATE A HAPPY CHARACTER SEND
        xor     ah,ah
SEND_FINISHED:
    JMP     SHORT SSKIP2          ; JUMP TO THE END OF THE ROUTINE

NOT_SEND_CHARACTER:
    CMP     AH,00           ; CHECK FOR PORT INITIALIZATION
    JNE     SSKIP           ; A COMMUNICATIONS REQUEST WHICH WE DON'T HANDLE


INIT_COM_PORT:              ; INITIALIZE THE COMM. PORT
ifdef QAK
ifdef Microsoft
    TEST    BL,_INITIALIZED_FLAGS; CHECK WHETHER THIS CONNECTION HAS BEEN INITIALIZED
    JNE     PORT_ALREADY_INIT   ; JUMP TO THE CHECK FOR CLOSING THE PORT
    OR      _INITIALIZED_FLAGS,BL; SET THE INITIALIZED FLAG FOR THIS PORT
else
    TEST    BL,INITIALIZED_FLAGS; CHECK WHETHER THIS CONNECTION HAS BEEN INITIALIZED
    JNE     PORT_ALREADY_INIT   ; JUMP TO THE CHECK FOR CLOSING THE PORT
    OR      INITIALIZED_FLAGS,BL; SET THE INITIALIZED FLAG FOR THIS PORT
endif
    JMP     SHORT RESET_INIT      ; JUMP TO RESETING THE COMM. PORT BUFFER
endif

PORT_ALREADY_INIT:          ; THE PORT HAS ALREADY BEEN INITIALIZED
ifdef Microsoft
    TEST    BL,_CONNECTED_FLAGS; CHECK WHETHER WE WERE CONNECTED
else
    TEST    BL,CONNECTED_FLAGS; CHECK WHETHER WE WERE CONNECTED
endif
    JE      RESET_INIT      ; IGNORE RE-INITIALIZATION IF NOT CONNECTED
    NOT     BL              ; INVERT AL IN PREPERATION FOR TURNING OFF THE FLAGS
ifdef Microsoft
ifdef QAK
    AND     _INITIALIZED_FLAGS,BL; RESET THE INITIALIZATION FLAG FOR THIS PORT
endif
    AND     _CONNECTED_FLAGS,BL  ; RESET THE CONNECTED FLAG FOR THIS PORT
else
ifdef QAK
    AND     INITIALIZED_FLAGS,BL; RESET THE INITIALIZATION FLAG FOR THIS PORT
endif
    AND     CONNECTED_FLAGS,BL  ; RESET THE CONNECTED FLAG FOR THIS PORT
endif

ifdef Microsoft
    MOV     SI,OFFSET _PNUM ; GET THE OFFSET INTO THE COMM. PORT NETWORK PORT NUMBERS
else
    MOV     SI,OFFSET PNUM  ; GET THE OFFSET INTO THE COMM. PORT NETWORK PORT NUMBERS
endif
    ADD     SI,DX           ; INCREMENT TO THE CORRECT COMM. PORT OFFSET
    ADD     SI,DX           ; TWICE BECAUSE THESE ARE INTEGERS, NOT BYTES
    PUSH    DX
    PUSH    [SI]            ; PUSH THE NETWORK PORT NUMBER
ifdef Microsoft
    CALL    _NETCLOSE       ; CLOSE THE CONNECTION
else
    CALL    NETCLOSE        ; CLOSE THE CONNECTION
endif
    ADD     SP,2            ; GET RID OF THE PARAMETER WE PASSED
    POP     DX

RESET_INIT:
ifdef Microsoft
    MOV     SI,OFFSET _BUFFER_OFFSET; GET THE OFFSET INTO THE COMM. PORT BUFFER
else
    MOV     SI,OFFSET BUFFER_OFFSET; GET THE OFFSET INTO THE COMM. PORT BUFFER
endif
    ADD     SI,DX           ; INCREMENT TO THE CORRECT COMM. PORT OFFSET
    MOV     BYTE PTR [SI],0     ; ZERO OUT THE BUFFER OFFSET FOR THE COMM. PORT

INIT_FINISHED:
    MOV     AX,61B0H        ; MOVE THE RS-232 CONNECTED FLAGS INTO THE RETURN VALUE
    JMP     SHORT SSKIP2          ; JUMP TO THE END OF THE ROUTINE

SSKIP:
;
;$$$ 01-05-1992 LJD +
;
;    XOR     BL,BL
;    MOV     CS:INTENTER,BL    ; REENTER FLAG, DONE NOW


	CLI			; disable interrupts
	dec	cs:intenter	; reset the flag (unlock for the timer interrupt)
;
;$$$ 01-03-1992 LJD -
;
    MOV     BX,SER_OLDSS        ; restore the old stack
    MOV     SS,BX
    MOV     SP,SER_OLDSP
ifndef Watcom
    mov     bx,SER_OLDSTKHQQ                ; restore end of stack
    mov     _STKHQQ,bx
endif
    STI
SERIAL2:
    POP     SI
	POP		DI
	POP		DX
	POP		CX
	POP		BX
	POP		AX
	POP		ES
	POP		DS
    JMP     SHORT LEAVE_SERIAL    ; JUMP TO THE END OF THE SERIAL ROUTINES

SSKIP2:
;
;$$$ 01-05-1992 LJD +
;
;    XOR     BL,BL
;    MOV     CS:INTENTER,BL    ; REENTER FLAG, DONE NOW

	CLI			; disable interrupts
	dec	cs:intenter

    MOV     BX,SER_OLDSS        ; restore the old stack
    MOV     SS,BX
    MOV     SP,SER_OLDSP
ifndef Watcom
    mov     bx,SER_OLDSTKHQQ                ; restore end of stack
    mov     _STKHQQ,bx
endif
    STI
SERIAL3:
    POP     SI
	POP		DI
	POP		DX
	POP		CX
	POP		BX
    POP     ES              ; POP AX INTO ES TO PRESERVE THE RETURN VALUE IN AX
	POP		ES
	POP		DS
;
;
;   forward to any other existing routines
;
LEAVE_SERIAL:
ifdef QAK
    JMP     DWORD PTR CS:SIP
else
    IRET
endif
ifdef Microsoft
_INT14INST      ENDP
else
INT14INST       ENDP
endif

ifdef Microsoft
;_TEXT ends
else
	ENDPS
endif
	END

