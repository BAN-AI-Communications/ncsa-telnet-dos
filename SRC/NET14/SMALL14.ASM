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

X	EQU		6
PORT    EQU 1
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

OLDINT14SEG     dw ?
OLDINT14OFF     dw ?
OLDSS           dw ?
OLDSP           dw ?

ifdef Microsoft
;_DATA ends
;
;_TEXT	segment public 'CODE'
;	assume CS:_TEXT
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
    PUBLIC  _INT14CHECK,_INT14RECEIVE
else	
    PUBLIC  INT14CHECK,INT14RECEIVE
endif   
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

ifdef Microsoft
;_TEXT ends

else
	ENDPS
endif
	END
