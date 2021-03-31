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
;

ifdef Microsoft
;
	.model	large
	.code
	PUBLIC	_EGAset
else
	PSEG
	PUBLIC	EGAset
endif

;NAME:		PIXELADDR10
;
;CALLER:	AX = Y-COORDINATE
;		BX = X-COORDINATE
;
;RETURNS:	AH = BITMASK
;			BX = BYTE OFFSET IN BUFFER
;			CL = NUMBER OF BITS TO SHIFT LEFT
;			ES = VIDEO BUFFER SEGMENT
;
;
;
BYTESPERLINE	EQU	80
ORIGINOFFSET	EQU	0
VIDEOBUFFERSEG	EQU	0A000H
PIXELADDR10	PROC	NEAR
	MOV		CL,BL
	PUSH	DX

	MOV		DX,BYTESPERLINE
	MUL		DX

	POP		DX
	SHR		BX,1
	SHR		BX,1
	SHR		BX,1
	ADD		BX,AX
	ADD		BX,ORIGINOFFSET

	MOV		AX,VIDEOBUFFERSEG
	MOV		ES,AX

	AND		CL, 7
	XOR		CL, 7
	MOV		AH,	1
	RET

PIXELADDR10	ENDP


;NAME		EGAset

ARGX	EQU	WORD PTR [BP+6]
ARGY	EQU	WORD PTR [BP+8]
ARGN	EQU	BYTE PTR [BP+10]

RMWBITS	EQU	00H

ifdef Microsoft
_EGAset	PROC	FAR
else
EGAset	PROC	FAR
endif
	PUSH	BP
	MOV	BP,SP
	PUSH	ES

	MOV	AX,ARGY
	MOV	BX,ARGX
	CALL PIXELADDR10
	
;SET GRAPHICS CONTROLLER BIT MASK REGISTER
	SHL	AH,CL
	MOV DX,3CEH
	MOV AL,8
	OUT DX,AX
	
;SET GRAPHICS CONTROLLER MODE REGISTER
	MOV AX,205H
	OUT DX,AX
	
;SET DATA ROTATE/FUNCTION SELECT REGISTER
	MOV AH,RMWBITS
	MOV AL,3
	OUT	DX,AX
	
;SET PIXEL VALUE
	MOV	AL,ES:[BX]
	MOV	AL,ARGN
	MOV	ES:[BX],AL
	
;RESTORE DEFAULT GRAPHICS CONTROLLER REGISTERS
	MOV	AX,0FF08H
	OUT DX,AX
	MOV AX,0005
	OUT DX,AX
	
	MOV AX,0003
	OUT DX,AX

	POP	ES
	MOV SP,BP
	POP BP
	RET
ifdef Microsoft	
_EGAset	ENDP
else
EGAset	ENDP
endif

ifdef Lattice
	ENDPS
endif
	END
