;
;  Video support routines
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
    NAME    INITVID
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

 X   EQU     6
 MONO_SEG   EQU     0b000h          ; Standard display segments
 COLOR_SEG  EQU     0b800h

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

    video_segment   DW  COLOR_SEG   ; Display memory segment address
    video_type      DB  0ffh        ; Display combination code
    video_iscolor   DB  01h         ; 1=color, 0=monochrome
    video_mode      DB  03h         ; Video display mode
    video_page      DB  00h         ; Video Display Page
    video_rows      DB  25          ; Number of Text rows
    video_cols      DB  80          ; Number of Text columns

ifdef Microsoft
;_DATA ends
;
;_TEXT	segment public 'CODE'
;	assume CS:_TEXT
.CODE
    PUBLIC  _INITVIDEO,_GETVCONFIG
else
	ENDDS
	PSEG
    PUBLIC  INITVIDEO,GETVCONFIG
endif	
;
;  Routines for general use
;
;
;************************************************************************
;  initvideo
;
;  Determines the active display adapter and various display parameters
;
ifdef Microsoft
_initvideo  proc    far
else
initvideo   proc    far
endif

    PUSH    BP
	MOV		BP, SP
    PUSH    DS
    PUSH    ES
	PUSH 	SI
	PUSH 	DI

    MOV     AH,0FH          ; Read Video information
    INT     10H
    MOV     video_mode,AL   ; Video Display Mode
    MOV     video_page,BH   ; Video Display Page
    MOV     video_cols,AH   ; Number of Text Columns
    MOV     video_segment,MONO_SEG  ; Assume monochrome for now
    MOV     video_iscolor,0 ;

    INT     11H             ; Read Equipment list
    AND     AL,00110000b    ; Isolate Video bits
    CMP     AL,00110000b    ; Was it mono?
    JE      find_adapter    ; Yes
    MOV     video_segment,COLOR_SEG ; Else, set color display
    MOV     video_iscolor,1 ;
find_adapter:
    CALL    ps2_state       ; Read PS/2 video state
    JNZ     adapter_set     ; Done, if supported
    CALL    ega_state       ; Read EGA video state
    JNZ     adapter_set     ; Done, is supported
    CALL    cga_state       ; Determine CGA or mono
adapter_set:
    SUB     AX,AX           ; Adjust dsplay segment for current vide page
    MOV     ES,AX           ;
    MOV     AX,ES:[044EH]   ;
    MOV     CL,4
    SHR     AX,CL
    ADD     video_segment,AX

	POP 	DI
	POP 	SI
    POP     ES
    POP     DS
	POP 	BP
    ret
ifdef Microsoft
_initvideo endp
else
initvideo endp
endif

;************************************************************************
;   ps2_state
;
;   This procedure attempts to access ps/2 compatible ROM BIOS video
;   services.  The zero flag is set if they aren't supported
;
ps2_state   proc    near
    MOV     AX,1A00H            ; Read PS/2 video state
    INT     10H
    CMP     AL,1AH              ; Was function supported?
    LAHF                        ; Toggle zero flag (zf=1 if al is not equal to 1ah)
    XOR     AH,01000000b        ;
    SAHF
    JZ      no_ps2              ; PS/2 BIOS not present
    MOV     video_type,BL       ; Save active display code
    MOV     AX,1130h            ; Read Font Code
    SUB     BH,BH               ; Font Code (not used)
    INT     10h                 ;
    INC     DL                  ; Adjust row count (clear zf)
    MOV     video_rows,DL       ;   and save
no_ps2:
    RET
ps2_state endp

;************************************************************************
;   ega_state
;
;   If PS/2 compatible ROM BIOS is not present, this procedure attempts
;   to access the EGA ROM BIOS video services
;
ega_state proc near
    MOV     AH,12H              ; Read EGA video state
    MOV     BL,10H
    INT     10H
    CMP     BL,10H              ; Was Function supported?
    JE      no_ega              ; No, EGA BIOS not present
    CMP     video_iscolor,BH    ; Is EGA the active display?
    JE      no_ega              ; No, find active display
    ADD     BH,4                ; Else, calculate display code
    MOV     video_type,BH       ;   and save
    MOV     AX,1130H            ; Read Font code
    SUB     BH,BH               ; Font Code (not used)
    INT     10H
    INC     DL                  ; Adjust row count (clear zf)
    MOV     video_rows,DL       ;   and save
no_ega:
    RET
ega_state endp

;************************************************************************
;   cga_state
;
;   If neither PS/2 compatible ROM BIOS nor EGA ROM BIOS is present,
;   this procedure is called.  It simply assumes 25 text rows and sets
;   video_type to MDA or CGA depending on the value of  video_iscolor
;
cga_state proc near
    MOV     video_rows,25       ; If we get here, must be 25 rows
    MOV     video_type,01H      ; Assume MDA adapter for now
    CMP     video_iscolor,0     ; Is it mono?
    JE      no_cga              ; Yes
    MOV     video_type,02H      ; Else set CGA display adapter
no_cga:
    RET
cga_state endp

;************************************************************************
;   getvconfig
;
;   This routine fills a buffer with the current videio paremeter values.
;   Note: initvideo() must be called first in order for this procedure
;   to return meaningful values.
;
;   Usage:      void getvconfig(struct vidinfo *)
;
;   Where:  struct vidinfo {
;               int segment;
;               int type;
;               int iscolor;
;               int mode;
;               int page;
;               int rows;
;               int columns;
;           };
;
ifdef Microsoft
_getvconfig proc    far
else
getvconfig  proc    far
endif
    PUSH    BP
    MOV     BP,SP
    PUSH    ES
    PUSH    DI
    PUSH    SI
    CLD                 ; All moves forward
IF @DataSize
    LES     DI,[BP+X]   ; Get the Pointer to the buffer
ELSE
    MOV     DI,[BP+X]   ; Get the Pointer to the buffer
    PUSH    DS
    POP     ES
ENDIF

    MOV     SI,OFFSET video_segment ; Get the offset of the start of the videio information
    MOVSW                   ; Copy the video segment

    MOV     CX,6            ; Copy six more byte values
    XOR     AH,AH           ; Clear the top part of the transfer register
Copy_Loop:
    LODSB                   ; Move each byte from the video parameters
    STOSW                   ; Into a word in the structure
    LOOP    Copy_Loop

    POP     DI
    POP     SI
    POP     ES
    POP     BP
    RET
ifdef Microsoft
_getvconfig endp
else
getvconfig  endp
endif

ifdef Microsoft
;_TEXT ends

else
	ENDPS
endif
	END

