TITLE 'NETBICC2.ASM'
;
;       These assembler routine provide an interface between the code
;       written in 'C' and the Multi Protocol Handler.
;

.model large

;
;       Data Structures
;
.data

;
;       The Asynchronous Notification Routine Stack
;
;       Size of stack is
STACK_SIZE      EQU     100h
;
                DW      STACK_SIZE      DUP     (?)
STACK_START     DW      ?

;
;       Area to save Multi Protocol Handler Stack Pointer
;
SS_SAVE         DW      ?
SP_SAVE         DW      ?
;

;
;       Start of Code
;


.code

EXTRN   _anr_c : FAR

PUBLIC  _ANR_ENTRY

_ANR_ENTRY      PROC    

;
;       This routine is the ANR entry point.  It is called by the
;       Multi Protocol Handler, via a far call.  Before calling the main
;       ANR routine (written in 'C'), it sets up the Data Segment, saves
;       the Multi Protocol Handler's stack, and allocates a new stack.
;       Before returning to the Multi Protocol Handler, it restores its
;       stack.
;
;       The ANR routine (written in 'C') must not make use of the heap, 
;       nor should it try to access to access automatic variables declared
;       in main(), because the stack it has been allocated is NOT the same
;       as the standard 'C' stack.
;

        MOV     AX, DGROUP      ; Get Data Segement
        MOV     DS, AX          ; Set up DS

        MOV     SS_SAVE, SS     ; Save Stack Pointers
        MOV     SP_SAVE, SP     ;

        MOV     SS, AX          ; Set up Stack Segment
        LEA     SP, STACK_START ; 

        PUSH    ES              ; The address of the ACB is passed as
        PUSH    BX              ; a far pointer.

        CLD

        CALL    _anr_c          ; Call 'C' ANR routine
                                ; _ANR_C returns a parameter in AX

        MOV     SS, SS_SAVE     ; Restore Stack Pointers
        MOV     SP, SP_SAVE

        RET
_ANR_ENTRY      ENDP
		END

