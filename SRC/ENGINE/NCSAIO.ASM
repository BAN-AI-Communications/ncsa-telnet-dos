;
;  ncsaio.asm
;  Support for BIOS calls in NCSA Telnet
;****************************************************************************
;*																			*
;*																			*
;*	  part of NCSA Telnet													*
;*	  by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer		*
;*																			*
;*	  National Center for Supercomputing Applications						*
;*	  152 Computing Applications Building									*
;*	  605 E. Springfield Ave.												*
;*	  Champaign, IL  61820													*
;*																			*
;*																			*
;****************************************************************************

	TITLE	NCSAIO	-- LOW-LEVEL I/O FOR SANE HARDWARE HANDLING
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

;
;   From original code by Tim Krauskopf	1984-1985
;
;   Modified and ported to Lattice C, Sept. 1986
;   ifdefs for Microsoft C, June 1987
;   Tim Krauskopf
;
;   Modified for version 2.3 release May 1990 by Heeren Pathak
;
;   National Center for Supercomputing Applications
;
	NAME	NIO

;
;  Macros for reading and writing I/O ports
;
MOUT	MACRO	REG,STUFF	   ; one byte to the given I/O register
	MOV		DX,REG
	MOV		AL,STUFF
	OUT		DX,AL
	ENDM
;
MIN	MACRO	REG		 		; get one byte to al
	MOV		DX,REG
	IN		AL,DX
	ENDM
;
;  Internal data 
;
X   EQU 6                       ;  for the large model programs
MONO_SEG   EQU     0b000h       ; Standard display segments
COLOR_SEG  EQU     0b800h

ifdef Microsoft
;DGROUP  group   _DATA
;_DATA   segment public 'DATA'
;    assume  DS:DGROUP
.MODEL  LARGE
.DATA

ifndef NET14
	PUBLIC _DTAPTR				; pointer to dta location
    PUBLIC _KEYBOARD_TYPE       ; Keyboard type installed
_DTAPTR 	DW	0000H			; DTA address for me
_DTADS		DW	0000H			; DS for DTA
endif
_KEYBOARD_TYPE	DB	?			; KEYBOARD TYPE (0) - is standard
								; (0x10) is enhanced
else
	INCLUDE	DOS.MAC
	DSEG
ifndef NET14
	PUBLIC	DTAPTR				; pointer to dta location
    PUBLIC  KEYBOARD_TYPE       ; Keyboard type installed
DTAPTR      DW  0000H           ; dta ADDRESS FOR ME
DTADS		DW	0000H			; ds FOR dta
endif
KEYBOARD_TYPE	DB	?			; KEYBOARD TYPE (0) - is standard
								; (0x10) is enhanced
endif

ATT			DB	7				; CURRENT DEFAULT ATTRIBUTE
TOPL		DB	00				; TOP LINE OF CURRENT WINDOW
BOTL            DB      24                              ; BOTTOM LINE OF CURRENT WINDOW
LEFTC		DB	00				; LEFT SIDE OF CURRENT WINDOW
RIGHTC		DB	79				; RIGHT SIDE OF CURRENT WINDOW
ROW			DB	00				; CURSOR POSITION ROW
COL			DB	00				; CURSOR POSITION COL
WRAP		DB	00				; WRAP AT END OR NOT
CURSOR  	DW  0607H			; DEFAULT CURSOR SHAPE
CURSOR_SAVE	DW	?				; PLACE TO SAVE THE USER'S CURSOR IN
BREAK_STATE	DB	?				; WHETHER THE BREAK IS SET TO ON OR OFF
EGAMODE 	DB  ?				; DEFAULT EGA MODE

video_segment   DW  COLOR_SEG   ; Display memory segment address
video_type      DB  0ffh        ; Display combination code
video_iscolor   DB  01h         ; 1=color, 0=monochrome
video_mode      DB  03h         ; Video display mode
video_page      DB  00h         ; Video Display Page
video_rows      DB  25          ; Number of Text rows
video_cols      DB  80          ; Number of Text columns
video_cursor    DW  ?           ; Cursor shape
video_font      DW  ?           ; Current font function

ifdef Microsoft
;_DATA   ends
else
	ENDDS
endif

;
;   The subroutines to call from C
;
ifdef Microsoft
;_TEXT   segment public  'CODE'
;    assume CS:_TEXT
.CODE
ifndef NET14
    PUBLIC  _n_color
endif
ifdef FTPBIN
    PUBLIC  _n_wrap
endif
;    PUBLIC  _n_erase
ifndef NET14
    PUBLIC  _n_getchar,_n_cur,_n_row,_n_col,_n_clear,_n_window
endif
    PUBLIC  _n_putchar
ifndef NET14
    PUBLIC  _n_chkchar
endif
;   PUBLIC  _n_savewin,_n_restwin
    PUBLIC  _n_puts,_n_sound
ifndef NET14
    PUBLIC  _n_findfirst,_n_findnext,_n_draw,_n_scrup,_n_scrdn,_n_cheat
endif
    PUBLIC  _n_clicks
ifndef NET14
    PUBLIC  _n_biosattr
endif
;   PUBLIC  _getdsk, _chgdsk
ifndef NET14
    PUBLIC  _ega43,_n_flags,_set_cur,_ega24,_vga50
    PUBLIC  _n_scrlck,_save_break,_restore_break,_n_gmode,_save_cursor
    PUBLIC  _restore_cursor,_install_cursor,_n_attr
    PUBLIC  _install_keyboard,_fix_vid,_get_mode,_set_mode,_get_size
    PUBLIC  _set_page,_initvideo,_getvconfig,_getvstate,_setvstate
endif


else            ; Lattice C
	PSEG
ifndef NET14
    PUBLIC  n_color
endif
ifdef FTPBIN
    PUBLIC  n_wrap
endif
;    PUBLIC  n_erase
ifndef NET14
    PUBLIC  n_getchar,n_cur,n_row,n_col,n_clear,n_window
endif
    PUBLIC  n_putchar
ifndef NET14
    PUBLIC  n_chkchar
endif
;   PUBLIC  n_savewin,n_restwin
    PUBLIC  n_puts,n_sound
ifndef NET14
    PUBLIC  n_findfirst,n_findnext,n_draw,n_scrup,n_scrdn,n_cheat
endif
    PUBLIC  n_clicks
ifndef NET14
    PUBLIC  n_biosattr
endif
    PUBLIC  n_srclck
ifndef NET14
    PUBLIC  ega43,n_flags,set_cur,ega24,vga50
    PUBLIC  n_scrlck,save_break,restore_break,n_gmode,save_cursor
    PUBLIC  restore_cursor,install_cursor,n_attr
    PUBLIC  install_keyboard,fix_vid,get_mode,set_mode,get_size
    PUBLIC  set_page,initvideo,getvconfig,getvstate,setvstate
endif
endif


ifndef NET14
;****************************************************************
; install_keyboard
;
; check for enhanced keyboard, and set keyboard type variable
;
;
ifdef Microsoft
_install_keyboard	proc 	far
else
install_keyboard	proc	far
endif
	PUSH	DS
	PUSH	SI
	MOV		AX,040H				; load the address of the keyboard flag
	MOV		DS,AX
	MOV		SI,096H
	LODSB						; get the keyboard flag
	POP		SI
	POP		DS
	AND		AL,10H				; mask off all bits except the enhanced flag
ifdef Microsoft
	MOV		_KEYBOARD_TYPE,AL	; store the keyboard type
else
	MOV		KEYBOARD_TYPE,AL	; store the keyboard type
endif
    RET
ifdef Microsoft
_install_keyboard	endp
else
install_keyboard	endp
endif

;****************************************************************
; install cursor
;
; put in user defined cursor settings
;
;
ifdef Microsoft
_install_cursor	proc 	far
else
install_cursor	proc	far
endif
    push    bp      ; save the base pointer
    mov bp,sp       ; point to the stack

    mov AX,[bp+x]   ; read our first parameter
    mov CURSOR,AX   ; save into the cursor variable

    pop bp          ; get the old base
    ret
ifdef Microsoft
_install_cursor	endp
else
install_cursor	endp
endif

;/***************************************************************/
; scrlck
;   returns whether scroll lock is on or not
;
ifdef Microsoft
_n_scrlck	proc	far
else
n_scrlck	proc	far
endif

		mov		ax,0200h		; get shift states
		int		16h				; keyboard int
		and		al,010h			; scroll lock state bit
		xor		ah,ah

		ret	
ifdef Microsoft
_n_scrlck	endp
else
n_scrlck	endp
endif

;*********************************************************************
;*	  save_break
;*	  record the state of the DOS BREAK status, and turn BREAK ON
;*
ifdef Microsoft
_save_break	proc	far
else
save_break	proc 	far
endif

	MOV		AX,3300H			; request current break state
	INT		21H					;
	MOV		BREAK_STATE,DL		; preserve the break state

	MOV		AX,3301H			; set break state
	MOV		DL,1				; turn break on
	INT		21H					;

	RET
ifdef Microsoft
_save_break 	endp
else
save_break	endp
endif	

;*********************************************************************
;*	  restore_break
;*	  restore the previous state of the DOS BREAK state
;*
ifdef Microsoft
_restore_break	proc	far
else
restore_break	proc 	far
endif

	MOV		AX,3301H			; set break state
	MOV		DL,BREAK_STATE		; return break state to previous value
	INT		21H					;

	RET
ifdef Microsoft
_restore_break 	endp
else
restore_break	endp
endif	

;*********************************************************************
;*	  save_cursor
;*	  record the state of the DOS keyboard cursor for page 0
;*
ifdef Microsoft
_save_cursor	proc	far
else
save_cursor	proc 	far
endif

	MOV		AX,0300h			; GET THE CURRENT CURSOR TYPE
	MOV		BX,0000h			; INFO FOR PAGE 0
	INT	10h
	MOV		CURSOR_SAVE,CX

	RET
ifdef Microsoft
_save_cursor 	endp
else
save_cursor	endp
endif

;*********************************************************************
;*	  restore_cursor
;*	  restore the previous state of the DOS keyboard cursor
;*
ifdef Microsoft
_restore_cursor	proc	far
else
restore_cursor	proc 	far
endif

	MOV		AH,01H			; set the keyboard cursor
	MOV		CX,CURSOR_SAVE	; get the user's keyboard cursor
	INT		10H				; set the cursor

	RET
ifdef Microsoft
_restore_cursor 	endp
else
restore_cursor	endp
endif	

;*********************************************************************
;*	  n_gmode
;*	  change the type of video display being used
;*
ifdef Microsoft
_n_gmode	PROC	FAR
else
n_gmode		PROC	FAR
endif
	PUSH	BP
	MOV		BP,SP
	
	MOV		AX,[BP+X]		;GET SCREEN MODE TO SWITCH TO
	XOR		AH,AH			;ENTER VIDEO_IO ROUTINE (SET MODE=0)
	INT		10H				;VIDEO INTERUPT

	POP	BP
	RET
ifdef Microsoft	
_n_gmode	ENDP
else
n_gmode	ENDP
endif

;*********************************************************************
;*	  ega24
;*	  set char set for 24 line mode
;*

ifdef Microsoft
_ega24	proc	far
else
ega24	proc 	far
endif

    MOV     BL,30H          ;Select scan lines for alphanumeric modes.
    MOV     AX,1202H        ;Set 400 scan lines.
    INT     10H

    MOV     AX,3                ; reset egamode
	INT 	10H

	MOV		AX,0100H			; SET CURSOR
	MOV		CX,CURSOR			; TURN ON
	INT 	10H

	RET
ifdef Microsoft
_ega24 	endp
else
ega24	endp
endif	

;****************************************************************
;*   set_cur
;*   sets cursor on or off
;*
ifdef Microsoft
_set_cur 	proc	far
else
set_cur		proc	far
endif
	PUSH 	BP
	MOV		BP,SP

	MOV		AL,[BP+X]			;get var
	OR		AL,AL
	JNZ		CUR					; MAKE CURSOR
	MOV		AX,0100H			; TURN OFF
	MOV		CX,2000H			; BEGIN END OF CURSOR
	INT 	10h
    JMP     SHORT EXIT                ; we are done
CUR:	
    MOV     CX,CURSOR           ; get cursor type
    MOV     AX,0100h            ; reset cursor
	INT 	10h
EXIT:
	POP 	BP
	RET
ifdef Microsoft
_set_cur endp
else
set_cur endp
endif

;****************************************************************
;*   n_flags
;*	 returns the keyboard status flags
;*
ifdef Microsoft
_n_flags	proc	far
else
n_flags 	proc	far
endif

	MOV		AX,0200h			; get shift states
	INT		16h					; keyboard INT
	XOR		AH,AH

	RET	
ifdef Microsoft
_n_flags	endp
else
n_flags	endp
endif

;****************************************************************
;*   ega43
;*   code for 43 line ega mode
;*	 
ifdef Microsoft
_ega43 proc far
else
ega43 proc far
endif

ifdef OLD_WAY
	MOV		AH,00H				; SET TO 80X25 CHAR MODE
	MOV		AL,03H
	INT		10H

	MOV		AX,1112H			; CHAR. GENERATOR BIOS ROUTINUE
	MOV		BL,00H				; LOAD 8 BY 8 DOUBLE DOT CHARACTER FONT
	INT 	10H					; VIDEO CALL

	MOV		AX,1200H			; ALTERNATE SCREEN ROUTINE
    MOV     BL,20H              ; SELECT ALTERNATE PRINT SCREEN ROUTINE
	INT 	10H					; VIDEO CALL
else
    MOV     BL,30H              ; Select scan lines for alphanumeric modes
    MOV     AX,1201H            ; Set 350 scan lines
    INT     10H

    MOV     AX,3                ; 80x25 color text
    INT     10H

    MOV     AX,1112H            ; Char. generator BIOS routine
    MOV     BL,00H              ; Load 8x8 double dot character font
    INT     10H                 ;

endif

	MOV		AX,0007H	
    MOV     CURSOR,AX           ; full cursor

	RET	
ifdef Microsoft
_ega43 	endp
else
ega43	endp
endif

;****************************************************************
;*   vga50
;*   code for 50 line vga mode
;*	 
ifdef Microsoft
_vga50 proc far
else
vga50 proc far
endif

    MOV     BL,30H              ; Select scan lines for alphanumeric modes
    MOV     AX,1202H            ; Set 400 scan lines
    INT     10H

    MOV     AX,3                ; 80x25 color text
    INT     10H

    MOV     AX,1112H            ; Char. generator BIOS routine
    MOV     BL,00H              ; Load 8x8 double dot character font
    INT     10H                 ;

	MOV		AX,0007H	
    MOV     CURSOR,AX           ; full cursor

	RET	
ifdef Microsoft
_vga50  endp
else
vga50   endp
endif
endif       ; NET14

ifdef NOT_USED
;*****************************************************************
;*   getDSk
;*   gets the active drive
;*	 
ifdef Microsoft
_getdsk proc	far

	MOV		AH,19H
	INT		21h

	RET
_getdsk 	endp	
endif 		

;*****************************************************************
;*	 chgdsk
;*   changes current drive
;*

ifdef Microsoft
_chgdsk	proc	far
	PUSH	BP
    MOV     BP,SP

	MOV		AH,0Eh
	MOV		DL,[BP+X]
	INT		21H

	POP		BP
	RET
_chgdsk	endp
endif
endif

ifndef NET14
;*****************************************************************
;*  n_attr
;*  replaces char. attribute
;*
ifdef Microsoft
_n_attr	proc	far
else 
n_attr	proc	far
endif
    PUSH    BP
	MOV		BP,SP
	PUSH 	ES
	PUSH	DI
;
;  for now, assume that the cursor location has ALready been set
;  (cursor is there)
;
    MOV     AX,video_segment
    MOV     ES,AX               ; set up for segment where screen is

	MOV		AL,ROW				; row where chars go
	XOR		AH,AH
	MOV		BL,160
	MUL		BL					; AX now equALs start of row
	XOR		BH,BH

	MOV		BL,COL
	SHL		BL,1				; COL*2
	ADD		AX,BX				; add in column offset
	INC		AX
	MOV 	DI,AX
	MOV		AL,[BP+X]			; ATTRIBUTE For CHARS
	STOSB						; STorE aTTR

	POP		DI
	POP		ES
	POP		BP
	RET
ifdef Microsoft
_n_attr 	endp
else
n_attr	  endp
endif
		
;*****************************************************************
;*  n_biosattr
;*  replaces char. attribute
;*  uses bios instead of directly screen write
;*

ifdef Microsoft
_n_biosattr	proc	far
else 
n_biosattr	proc	far
endif
	PUSH 	BP
    MOV     BP,SP

	MOV	 	AH,8h				; we want to read
	MOV	 	BH,0H				; GET FROM PAGE ZERO
	INT 	10H
	MOV		BL,[BP+X]			; GET CHANGED ATTRIBUTE
	MOV		AH,9H	 			; WE WANT TO WRITE
	MOV		CX,1H				; ONLY ONCE
	INT 	10H

	POP 	BP
	RET
ifdef Microsoft
_n_biosattr 	endp
else
n_biosattr	  endp
endif

;/***************************************************************/
;
;   Change the vALue of my current color
;
ifdef Microsoft
_n_color	proc	far
else
n_color	proc	far
endif
	PUSH	BP
	MOV		BP,SP

	XOR		AX,AX
	MOV		DL,[BP+X]			; PARAMETER, ONE BYTE
	MOV		AL,ATT				; RETURN OLD COLor
	MOV		ATT,DL

	POP		BP
	RET
ifdef Microsoft
_n_color	endp
else
n_color	endp
endif
endif           ; NET14

;/*****************************************************************/
;   Determine the current type of DISPlay
;
;   ask the BIOS which screen is currently active
;
ifdef Microsoft
_n_which	proc	far
else
n_which	proc	far
endif

    MOV     AX,0300h            ; GET THE CURRENT CURSOR
	MOV		BX,0000h			; INFO FOR PAGE 0
	INT	10h
	MOV		CURSOR,CX

	RET
ifdef Microsoft
_n_which	endp
else
n_which	endp
endif

ifdef FTPBIN
;
;/***************************************************************/
;
;   Set whether word wrap will be on or not
;   usage:  n_wrap(flag);
;
ifdef Microsoft
_n_wrap	proc	far
else
n_wrap	proc	far
endif
	PUSH	BP
	MOV		BP,SP

	MOV		DL,[BP+X]			; PARAMETER, ONE BYTE FLAG
	MOV		AL,WRAP				; RETURN OLD COLor
	MOV		WRAP,DL

	POP		BP
	RET
ifdef Microsoft
_n_wrap	endp
else
n_wrap	endp
endif
endif

ifndef NET14
;/***************************************************************/
;
;   Move the cursor somewhere
;
ifdef Microsoft
_n_cur	proc	far
else
n_cur	proc	far
endif
	PUSH	BP
	MOV		BP,SP

    MOV     DH,[BP+X]       ; row poSItion
    MOV     DL,[BP+X+2]     ; column poSItion
    MOV     ROW,DH          ; save a copy for me
    MOV     COL,DL          ; save column too
    MOV     AX,0200h        ; ah=2 FUNCTION CalL
    XOR     BX,BX           ; video page 0
    INT     10H             ; set cursor poSItion

	POP		BP
	RET
ifdef Microsoft
_n_cur	endp
else
n_cur	endp
endif

;/***************************************************************/
;
;  set the window boundaries
;
;  usage:  n_window(ulrow,ulcol,lrrow,lrcol);
;    sets window SIze for future operations.
;
;
ifdef Microsoft
_n_window	proc	far
else
n_window	proc	far
endif
	PUSH		BP
	MOV			BP,SP

	MOV			AL,[BP+X]		; UPPER LEFT Y
	MOV			TOPL,AL
	MOV			AL,[BP+X+2]		; COLUMN POsiTION
    MOV         LEFTC,AL        ; save a copy for me
	MOV			AL,[BP+X+4]
	MOV			BOTL,AL			; bottom row
	MOV			AL,[BP+X+6]
	MOV			RIGHTC,AL		; keep this one too
ifdef Microsoft
	CALL		_N_WHICH		; what kind of screen?
else
	CALL		N_WHICH			; WHAT KIND OF SCREEN?
endif

	POP			BP
	RET
ifdef Microsoft
_n_window	endp
else
n_window	endp
endif
endif       ; NET14

ifdef NOT_USED
;/***************************************************************/
;
;  erase portion of the screen
;
;  usage:  n_erase(ulrow,ulcol,lrrow,lrcol);
;
;
ifdef Microsoft
_n_erase	proc	far
else
n_erase	proc	far
endif
	PUSH	BP
	MOV		BP,SP

	MOV		CH,[BP+X]			; UPPER LEFT X
	MOV		CL,[BP+X+2]			; COLUMN POsiTION
	MOV		DH,[BP+X+4]
	MOV		DL,[BP+X+6]
	XOR		AX,AX				; clear area
	MOV		AH,6				; scroll up function
	MOV		BH,ATT				; ATTRIBUTE For ERAsiNG IS SAME AS PRint
	INT		10H					; CalL bios

	POP		BP
	RET
ifdef Microsoft
_n_erase	endp
else
n_erase	endp
endif
endif

ifndef NET14
;/***************************************************************/
;  scroll up and down
;  given the number of lines to scroll and the DImenSIons of the
;  box to scroll.
;  
ifdef Microsoft
_n_scrup	proc	far
else
n_scrup	proc	far
endif
	PUSH	BP
	MOV		BP,SP

    MOV     AL,[BP+X]           ; NUMBER OF LINES TO SCROLL
    MOV     CH,[BP+X+2]         ; UPPER LEFT X
	MOV		CL,[BP+X+4]			; COLUMN POsiTION
	MOV		DH,[BP+X+6]
	MOV		DL,[BP+X+8]
	MOV		AH,6				; scroll up function
	MOV		BH,ATT				; ATTRIBUTE For BLANK LINE IS SAME AS PRint
	INT		10H					; CalL bios

	POP		BP
	RET
ifdef Microsoft
_n_scrup	endp
else
n_scrup	endp
endif

ifdef Microsoft
_n_scrdn	proc	far
else
n_scrdn	proc	far
endif
	PUSH	BP
	MOV		BP,SP

    MOV     AL,[BP+X]           ; number of lines to scroll
    MOV     CH,[BP+X+2]         ; upper left x
	MOV		CL,[BP+X+4]			; column poSItion
	MOV		DH,[BP+X+6]
	MOV		DL,[BP+X+8]
	MOV		AH,7				; scroll down function
    MOV     BH,ATT              ; attribute for blank line is same as prINT
	INT		10H					; CalL bios

	POP		BP
	RET
ifdef Microsoft
_n_scrdn	endp
else
n_scrdn	endp
endif

;/***************************************************************/
; 
;  Find the cursor poSItion, return the row vALue
;  
ifdef Microsoft
_n_row	proc	far
else
n_row	proc	far
endif

    MOV     AX,0300H            ;video find cursor function
    XOR     BX,BX
    INT     10H                 ; find cursor
    MOV             ROW,DH
    MOV             COL,DL              ; save col for next routine
    XOR             AX,AX
    MOV             AL,DH                           ; return row location

	RET
ifdef Microsoft
_n_row	endp
else
n_row	endp
endif
endif       ; NET14

;/****************************************************************************/
  ;-----------------------------------------------------------
  ;  This routine was submitted by David T. Burhans, Jr.
  ;  It was obtained from an RBBS-PC electronic bulleton board 
  ;  deDIcated to the "C" language [(703) 321-7003].
  ;-----------------------------------------------------------
  ;
  ; SOUND - This routine produces a tone of a SPecified frequency 
  ; and duration on the SPeaker.
  ; The frequency in Hertz is moved INTo DI (21 to 65535 Hertz) 
  ; and the duration in hundredths of a second, is moved INTo BX 
  ; (0 to 65535).
  ; The routine is cALled with the following sequence:
  ;
  ;		UnSIgned		frequency, duration;
  ;
  ;	   n_sound(frequency, duration);
  ;
ifdef Microsoft
_n_sound	proc	far
else
n_sound	proc	far
endif
	PUSH	BP
	MOV		BP,SP
	PUSH	DI

    MOV     DI,[BP+X]       ; Frequency
    MOV     BX,[BP+X+2]     ; Duration
    CMP     DI, 21H         ; See if freq. is below minimum
    JB      DONE            ; iF LESS THAN LIMIT, THEN
                            ; clean up and return
    MOV     AL,0B6H         ; Write timer mode register
    OUT     43H, AL
    MOV     DX, 14H         ; Timer DIvisor =
    MOV     AX, 4F38H       ; 1331000/Frequency
    DIV     DI
    OUT     42H,AL          ; Write timer 2 count low byte
    MOV     AL,AH
    OUT     42H,AL          ; Write timer 2 count high byte
    IN      AL,61H          ; Get current Port B setting
    MOV     AH,AL           ; and save it in AH
    OR      AL,3            ; Turn SPeaker on
    OUT     61H,AL
HOLD:
    MOV     CX, 2801
SPKR_ON: 
    LOOP    SPKR_ON
    DEC     BX              ; Speaker-on count expired?
    JNZ     HOLD            ; iF NOT, KEEP spEAKER ON
    MOV     AL,AH           ; Otherwise, recover vALue of port
    OUT     61H,AL
DONE:
	POP		DI
    POP     BP              ; Restore cALler's BP
	RET						; rETURN TO CalLER
ifdef Microsoft
_n_sound	endp
else
n_sound	endp
endif

ifndef NET14
;/***************************************************************/
;
;  Find the column poSItion of the cursor.
;  Must be preceded by a n_row cALl!!!!
;  Will work when these routines ALready know the cursor poSItion
;
ifdef Microsoft
_n_col	proc	far
else
n_col	proc	far
endif
	XOR		AH,AH
	MOV		AL,COL				; RETURN COL LOCATION

	RET
ifdef Microsoft
_n_col	endp
else
n_col	endp
endif

;/***************************************************************/
;  n_clear
;  Clear what we think is the current window
;
ifdef Microsoft
_n_clear	proc	far
else
n_clear	proc	far
endif

	MOV		AL,0			; clear window
	MOV		AH,6			; scroll up function
	MOV		CH,TOPL			; TOP ROW TO CLEAR
	MOV		CL,LEFTC		; LEFT COLUMN
	MOV		DH,BOTL			; BOTTOM ROW TO CLEAR
	MOV		DL,RIGHTC		; RIGHT siDE TO CLEAR
	MOV		BH,ATT			; ATTRIBUTE TO USE For BLANKS
	INT		10H				; CalL bios

	RET
ifdef Microsoft
_n_clear	endp
else
n_clear	endp
endif
endif       ; NET14

;/***************************************************************/
;  n_upscroll
;  Scroll up what we think is the current window, n lines
;
ifdef Microsoft
_n_upscroll	proc	far
else
n_upscroll	proc	far
endif
	PUSH	BP
	MOV		BP,SP

	MOV		AL,[BP+X]			; number of lines to scroll
	MOV		AH,6				; scroll up function
	MOV		CH,TOPL				; top row to clear
	MOV		CL,LEFTC			; left column
	MOV		DH,BOTL				; bottom row to clear
	MOV		DL,RIGHTC			; RIGHT siDE TO CLEAR
	MOV		BH,ATT				; attribute to use
	INT		10H					; cALl BIOS

	POP		BP
	RET
ifdef Microsoft
_n_upscroll	endp
else
n_upscroll	endp
endif

;/***************************************************************/
;  n_dnscroll
;  Scroll up what we think is the current window, n lines
;
ifdef Microsoft
_n_dnscroll	proc	far
else
n_dnscroll	proc	far
endif
	PUSH	BP
	MOV		BP,SP

	MOV		AL,[BP+X]			; number of lines to scroll
	MOV		AH,7				; scroll down function
	MOV		CH,TOPL				; top row to clear
	MOV		CL,LEFTC			; left column
	MOV		DH,BOTL				; bottom row to clear
	MOV		DL,RIGHTC			; right SIde to clear
    MOV     BH,ATT              ; attribute to use for blank lines
	INT		10h					; CalL bios

	POP		BP
	RET
ifdef Microsoft
_n_dnscroll	endp
else
n_dnscroll	endp
endif

;/***************************************************************/
;  n_putchar(letter)
;     puts onto screen at current cursor location
;
ifdef Microsoft
_n_putchar	proc	far
else
n_putchar	proc	far
endif
	PUSH 	BP
	MOV		BP,SP

    MOV     AL,[BP+X]       ; char to write
    XOR     BX,BX           ;  set page number for ALl cursor addresses
	CMP  	AL,10		   	; line feed
	JNZ 	NXT
    MOV     AL,ROW          ; get current cursor row poSItion
    CMP             AL,BOTL

    JL      NOSCR           ; within window area
    MOV     AX,1            ; scroll up one line
	PUSH	AX
ifdef Microsoft
    CALL    _N_UPSCROLL     ; scrolls it up one
else
    CALL    N_UPSCROLL      ; scrolls it up one
endif
    POP     AX              ; take parameter off of stack
;   MOV     AL,LEFTC        ; get left SIde vALue
;   MOV     COL,AL          ; set cursor to left
        JMP             PUTCUR
NOSCR:
;   MOV     AL,LEFTC        ; return cursor to left SIde
;	MOV		COL,AL
	INC		ROW
    JMP     PUTCUR          ; set new cursor poSItion, return
NXT:
    CMP     AL,7            ; ctrl-G, bell
	JNZ  	NXT2
;
;  handle bell with a cALl to a SPeciAL routine
;
    MOV     AX,12           ; duration of tone
	PUSH	AX
	MOV		AX,1000	  		; frequency of tone
	PUSH 	AX
ifdef Microsoft
	CALL _N_SOUND
else
	CALL N_SOUND
endif
    ADD     SP,4            ; remove params from stack
HERE2:
	POP		BP
	RET
NXT2:
    CMP     AL,13           ; cr,  home the cursor
	JNZ		TRYTAB
    MOV     AL,LEFTC        ; new cursor poSItion
	MOV		COL,AL
	JMP		PUTCUR
TRYTAB:
    CMP     AL,9            ; tab character
    JNZ     NOTCRLF         ; ready for regular character
;  expand tabs
    MOV     DL,COL          ; get cursor poSItion
	MOV		CL,3
    SHR     DL,CL           ; DIvide cursor poSItion by 8
    INC     DL              ; increment cursor poSItion
    SHL     DL,CL           ; multiply back by 8
	MOV		COL,DL
                            ; check to see if past right SIde
    MOV     DL,COL          ; get where the cursor has moved to
    CMP     DL,RIGHTC       ; over the SIde yet?
    JA      TABOVER         ; set the new poSItion
	JMP		PUTCUR
TABOVER:
	MOV		DL,LEFTC
	MOV		COL,DL
    INC     ROW             ; TO NEXT ROW
    MOV     DL,ROW          ; WHAT ROW?
	CMP		DL,BOTL
    JG      TABNOS          ; we are okay
    JMP     SHORT PUTCUR
TABNOS:
    DEC     ROW             ; need to scroll
	MOV		AX,1
	PUSH	AX
ifdef Microsoft
    CALL    _N_UPSCROLL     ; scroll window up one line
else
    CALL    N_UPSCROLL      ; scroll window up one line
endif
	POP		AX
    MOV     AL,LEFTC        ; reset cursor poSItion to left
	MOV		COL,AL
    JMP     SHORT PUTCUR
NOTCRLF:
    CMP     AL,8            ; backSPace
	JNZ 	REGCHAR
    MOV     AL,COL          ; where is cursor?
    CMP     AL,LEFTC        ; is at left of screen?
	JZ		HERE2
    DEC     COL             ; decrement cursor poSItion
    JMP     SHORT PUTCUR    ; okay, move cursor where it belongs
REGCHAR:
    MOV     CX,1H           ; number of repetitions = 1
    MOV     BL,ATT          ; attribute of char
    MOV     BH,0            ; write to page zero
    MOV     AH,9            ; write char
	INT  	10H

    INC     COL             ; move cursor over one.
    MOV     DL,COL          ; get current poSItion
    CMP     DL,RIGHTC       ; is at right SIde of screen?
    JLE     PUTCUR          ; no, char can just be put out, cursor moved.
;
;  check wORd-wrap because we are at edge of window
;
	MOV		DL,WRAP
    OR      DL,DL           ; 0 = NO WRAP, 1 = WRAP
	JZ		NOWRAP
    MOV     DL,LEFTC        ; cursor will wrap around
    MOV     COL,DL          ; save the new column poSItion
    INC     ROW             ; to next row
	MOV		DL,BOTL
    CMP     ROW,DL          ; do we need to scroll?
    JNG     PUTCUR          ; no, okay for next row, normAL wrap
; scroll screen up one 
	MOV		AX,1
	PUSH	AX
ifdef Microsoft
    CALL    _N_UPSCROLL     ; scroll window up one line
else
    CALL    N_UPSCROLL      ; scroll window up one line
endif
	POP		AX
    DEC     ROW             ; scrolled up one
    JMP     SHORT PUTCUR    ; don't wrap
NOWRAP:
	DEC		COL
PUTCUR:
	MOV		DH,ROW
	MOV		DL,COL
   	MOV	 	AH,2h
   	XOR 	BX,BX
    INT     10H             ; set cursor poSItion

ESCAPEHERE:
	POP 	BP
	RET
ifdef not_used
TELE:
    MOV     BL,3            ; SEND WHATEVER CHAR
	MOV		AH,14
	INT 	10H

	POP		BP
	RET
endif
ifdef Microsoft
_n_putchar	endp
else
n_putchar	endp
endif

ifndef NET14
;/**********************************************************************/
;  draw
;  place characters on the screen-- checking for bounDS, etc.  ALl done
;  at a higher level.
;
;  n_draw(s,len)
;	char*s ; INT len;
;
ifdef Microsoft
_n_draw proc far
else
n_draw proc far
endif
	PUSH 	BP
	MOV		BP,SP
	PUSH 	ES
	PUSH	SI
	PUSH	DI
;
;  for now, assume that the cursor location has ALready been set
;  (cursor is there)
;
	LES		SI,[BP+X]			; get the segment and offset of the string
	MOV		DI,[BP+X+4]			; # OF CHARACTERS
;	OR		DI,DI				; check for no characters to draw
;	JZ		ENDDRAW				; exit if no characters to draw
    MOV     DH,ROW              ; row where cursor is
LOOPDRAW:
	MOV		AL,ES:[SI]			; get character to write from es:SI
	INC		SI
	MOV	 	CX,1H		  		; number of repetitions = 1
	XOR  	BH,BH				; dispLAY PAGE 0
   	MOV	 	BL,ATT 				; attribute of char
	MOV	 	AH,9		   		; write char
   	INT  	10h
	INC		COL					; MOVE THE CURSor OVER
	MOV		DL,COL
   	MOV	 	AH,2H
	XOR		BX,BX
	INT  	10H				 	; SET CURSor POsiTION
	DEC 	DI					; check counter
	JNZ 	LOOPDRAW			; GO THROUGH TO REQUIRED COUNT

ENDDRAW:
	POP		DI
	POP		SI
	POP 	ES
	POP		BP
	RET
ifdef Microsoft
_n_draw endp
else
n_draw endp
endif

;/**********************************************************************/
;  cheat
;  put characters on the screen as per n_draw, but write directly
;  to display memory.  Don't even check for retrace
;
ifdef Microsoft
_n_cheat proc	far
else
n_cheat proc	far
endif
	PUSH 	BP
	MOV		BP,SP
	PUSH 	ES
	PUSH 	DS
	PUSH	SI
	PUSH	DI
;
;  for now, assume that the cursor location has already been set
;  (cursor is there)
;
    MOV     AX,video_segment
    MOV     ES,AX               ; set up for segment where screen is

	MOV		AL,ROW				; row where chars go
	XOR		AH,AH
	MOV		BL,160
    MUL     BL                  ; ax now equals start of row

	XOR		BH,BH
	MOV		BL,COL
	SHL		BL,1				; COL*2
	ADD		AX,BX				; add in column offset
    MOV     DI,AX               ; store in di (es:di)

	MOV		CX,[BP+X+4]			; # of characters
;	OR		CX,CX				; check whether we really have ANY characters
;	JZ		ENDCHEAT			; exit if we have no characters
    ADD     COL,CL              ; COLUMN POSITION OF CURSOR
    MOV     AH,ATT              ; ATTRIBUTE FOR CHARS
	LDS		SI,[BP+X]			; get the segment and offset of the string
;
;  do the move
;
	CLD
LPCHEAT:
	LODSB						; GET NEXT BYTE
	STOSW						; STorE BYTE AND ATTRIBUTE
	LOOP 	LPCHEAT

ENDCHEAT:
	POP		DI
	POP		SI
	POP		DS
	POP		ES
	POP		BP
	RET
ifdef Microsoft
_n_cheat	endp
else
n_cheat	endp
endif
endif       ; NET14

;/**********************************************************************/
;
;  New keyboard handling

;	the interrupt and codes for the keyboard interface.

KEYBOARD	EQU	16H			; interrupt 16 to deal with keyboard
GETC	  	EQU	0			; code for reading a character
CHECKC		EQU	1			; code for keyboard status
SHIFTSTAT	EQU	2			; code for shift key status
SCAN		EQU	00100H		; scan code
SHIFT		EQU	00200H		; left or right shift key pressed
CONTROL		EQU	00400H		; control key pressed
ALT			EQU	00800H		; alt key pressed
ENHANCE		EQU	01000H		; enhanced keyboard special key

ifndef NET14
;**********************************************************************
;
;  Get a translated character from the keyboard.
;
;	Return the next character pressed with the new translation method for
;		kermit compatibility.
;	Quincey Koziol
;
;	Usage:
;       unsigned int n_getchar(void);
;
ifdef Microsoft
_n_getchar	proc	far
else
n_getchar	proc	far
endif
	PUSH  	BP

ifdef Microsoft
    MOV     AH,_KEYBOARD_TYPE   ; ask for a keyboard character
else
    MOV     AH,KEYBOARD_TYPE   ; ask for a keyboard character
endif
	INT		KEYBOARD
	MOV		BX,AX				; store the scan code character
	MOV		AH,SHIFTSTAT		; get the shift keys' status
	INT		KEYBOARD			; shift status returned in AX
	XOR		AH,AH				; clear the top part of the shift status
	XOR		DX,DX				; set local variables to zero
	XOR		CX,CX				; 
	CMP		BH,0				; check for enhanced key
	JE		NOT_ENHANCED1		; jump around first enchanced key checks
	CMP		BH,0E0H				; check for other part of enhanced key
	JNE		NOT_ENHANCED2
	XCHG	BH,BL				; shift key from bios left 8 bits
	XOR		BL,BL				; 
	OR		DX,ENHANCE			; set the enhanced bit
NOT_ENHANCED2:
	CMP		BL,0E0H				; check for second part of enhanced key
	JNE		NOT_ENHANCED1
	XOR		BL,BL				; clear the character code from the bios key
	OR		DX,ENHANCE			; set the enhanced bit
NOT_ENHANCED1:
	CMP		BL,0				; check for not scan code only being zero
	JE		NOT_ALIAS			; not one of the alias'ed keys from BIOS
	CMP		BX,(14*SCAN)+8		; check for Backspace
	JE		ALIAS_KEY
	CMP		BX,(55*SCAN)+'*'	; check for '*'
	JE		ALIAS_KEY
	CMP		BX,(74*SCAN)+'-'	; check for '-'
	JE		ALIAS_KEY
	CMP		BX,(78*SCAN)+'+'	; check for '+'
	JE		ALIAS_KEY
	CMP		BX,(71*SCAN)+'7'	; check for '7'
	JE		ALIAS_KEY
	CMP		BX,(72*SCAN)+'8'	; check for '8'
	JE		ALIAS_KEY
	CMP		BX,(73*SCAN)+'9'	; check for '9'
	JE		ALIAS_KEY
	CMP		BX,(75*SCAN)+'4'	; check for '4'
	JE		ALIAS_KEY
	CMP		BX,(76*SCAN)+'5'	; check for '5'
	JE		ALIAS_KEY
	CMP		BX,(77*SCAN)+'6'	; check for '6'
	JE		ALIAS_KEY
	CMP		BX,(79*SCAN)+'1'	; check for '1'
	JE		ALIAS_KEY
	CMP		BX,(80*SCAN)+'2'	; check for '2'
	JE		ALIAS_KEY
	CMP		BX,(81*SCAN)+'3'	; check for '3'
	JE		ALIAS_KEY
	CMP		BX,(82*SCAN)+'0'	; check for '0'
	JE		ALIAS_KEY
	CMP		BX,(83*SCAN)+'.'	; check for '.'
	JE		ALIAS_KEY
	CMP		BX,(83*SCAN)+','	; check for ','
	JE		ALIAS_KEY
	CMP		BX,(15*SCAN)+9		; check for Tab
	JE		ALIAS_KEY
    JMP     SHORT NOT_ALIAS           ; key didn't match one of the alias'ed keys
ALIAS_KEY:
	XOR		BL,BL				; clear ascii code for alias'ed keys
NOT_ALIAS:
	CMP		BL,0				; no ascii value, get a kermit scan code
	JNE		REGULAR_KEY
	MOV		DL,BH				; get the scancode
	OR		DX,SCAN				; set the SCAN flag
	TEST	AX,3				; check for shift key down
	JE		NOT_SHIFT			; shift key not down
	OR		CX,SHIFT			; set the SHIFT help flag
NOT_SHIFT:
	TEST	AL,20h				; check for NumLock set
	JE		NOT_NUMLOCK			; NumLock not down
	CMP		DX,71+SCAN			; on numeric keypad?
	JB		NOT_NUMLOCK
	CMP		DX,83+SCAN
	JA		NOT_NUMLOCK
	CMP		DX,74+SCAN			; not the grey - key
	JE		NOT_NUMLOCK
	CMP		DX,78+SCAN			; not the grey + key
	JE		NOT_NUMLOCK
	XOR		CX,SHIFT			; all true, xor the shift help
NOT_NUMLOCK:
	OR		DX,CX				; set the correct shift status
	TEST	AL,4				; check for the CONTROL key down
	JE		NOT_CONTROL
	OR		DX,CONTROL			; set the control flag
NOT_CONTROL:
	TEST	AX,8				; check for the ALT key down
	JE		NOT_ALT
	OR		DX,ALT				; set the alt flag
NOT_ALT:
	MOV		AX,DX				; set the return value
    JMP     SHORT END_NEWGETCHAR
	
REGULAR_KEY:					; we have an ascii value, return that
	XOR		BH,BH				;
	MOV		AX,BX				; return the key id

END_NEWGETCHAR:
	POP	BP
	RET
ifdef Microsoft
_n_getchar	endp
else
n_getchar	endp
endif

;**********************************************************************
;  Check for character present at the keyboard
;  If there is one available, return it, if not, return -1
;
;  translate any extended characters
;
ifdef Microsoft
_n_chkchar	proc	far
else
n_chkchar	proc	far
endif
ifdef Microsoft
	MOV		AH,_KEYBOARD_TYPE	; get the correct BIOS setting for the keyboard
else
	MOV		AH,KEYBOARD_TYPE	; get the correct BIOS setting for the keyboard
endif
	INC		AH					; increment for the setting to check the keyboard
	INT	 	KEYBOARD
	MOV		AX,-1
	JZ		NEW_NOKEY
ifdef Microsoft
	CALL	_N_GETCHAR			;get the coded character
else
	CALL	N_GETCHAR			;get the coded character
endif
NEW_NOKEY:
	RET
ifdef Microsoft
_n_chkchar	endp
else
n_chkchar	endp
endif
endif       ; NET14

ifdef NOT_USED
;/***************************************************************/
;  savewin
;	copy the current window INTo a buffer
;
;   usage:  n_savewindow(buffer);
;
ifdef Microsoft
_n_savewin	proc	far
else
n_savewin	proc	far
endif
	PUSH	BP
	MOV		BP,SP
	PUSH	ES
	PUSH	DS
	PUSH	SI
	PUSH	DI

    LES     DI,[BP+X]           ; pointer to buffer
	CLD
;
;  store parameters in first, for recall later
;
    MOV     AL,ROW              ; cursor position: row,col
	STOSB
	MOV		AL,COL
	STOSB
	MOV		AL,TOPL				; window boundaries
	STOSB
	MOV		AL,LEFTC
	STOSB
	MOV		AL,BOTL
	STOSB
	MOV		AL,RIGHTC
    STOSB                       ; STORE IN BUFFER TOO
;
;  calculate amount to move
;
;
;  calculate number of lines to move
;
    MOV     AL,TOPL         ; X1
    MOV     BL,BOTL         ; X2
    SUB     BL,AL           ; x2 = x2-x1
    INC     BL              ; ADD 1 LINE
    MOV     [BP+X],BL       ; KEEP IT SOMEWHERE SAFE
;
;  calculate screen offset  = 160*x1+2*y1;
;
;   al has topl in it
	XOR  	AH,AH
	MOV	 	BL,160
    MUL     BL              ; how many chars in x1 lines, in AX
	XOR  	CH,CH
    MOV     CL,LEFTC        ; chars to left SIde
    SHL     CL,1            ; *2 counts attributes
    ADD     AX,CX           ; add them
    MOV     SI,AX           ; place into source index
;
;  find number of characters to move each time
;
	MOV	 	CL,RIGHTC
	SUB  	CL,LEFTC	   		; Y2 = Y2-Y1
	INC  	CL			 		;  ADD 1
	MOV	 	[BP+X+1],CL			; SAFE PLACE AGAIN
;
;  find number to add to wrap around for each line
;
	MOV	 	BL,CL		  		; # OF CHARS MOVED EACH TIME
	SHL  	BL,1		   		; CHARS*2
	MOV	 	AL,160
	SUB  	AL,BL		  		; 160-chars*2
	MOV	 	[BP+X+2],AL			; safe place to keep it
;
;
    MOV     AX,video_segment    ; where source segment is (screen)
	MOV	 	DS,AX

    MOV     DX,03dAH            ; color card status port
	XOR  	BH,BH
	MOV	 	BL,[BP+X]		   ; howmany lines to move
	CMP  	AX,0B000H		   ; CHECK For MONO CARD
	JNZ 	GETLINE
    MOV     DX,03BAH            ; change status port vALue
GETLINE:
	MOV	 	CL,[BP+X+1]		 ; # OF CHARS TO MOVE
GETCHAR:
;   IN   	AL,DX			   ; GET STATUS BYTE
    TEST    AL,1                ; horizontal retrace
;   JNZ  	GETCHAR			 ; not there yet
;ISON:
	IN   	AL,DX
;   TEST 	AL,1
;   JZ   	ISON				; STILL ON, TRY AGAIN
    MOVSW                       ; NOW IS TIME, MOVE THE WORD
	LOOP 	GETCHAR			 ; DO ANOTHER UNTIL THIS LINE IS DONE
;
	DEC 	BX
	JZ  	ENDKEEP			 ; DONE, WE ARE OUT OF HERE
	MOV		AL,[BP+X+2]
	XOR		AH,AH
	ADD 	SI,AX			   
; go to next line, skip 160-n*2 bytes
	JMP 	GETLINE

ENDKEEP:
	POP		DI
	POP		SI
	POP  	DS
	POP  	ES
	POP  	BP
	RET
ifdef Microsoft
_n_savewin	endp
else
n_savewin	endp
endif

;/***************************************************************/
;  restwindow
;   restore the contents of the window to the screen
;
ifdef Microsoft
_n_restwin	proc	far
else
n_restwin	proc	far
endif
	PUSH	BP
	MOV		BP,SP
	PUSH	ES
	PUSH	DS
	PUSH	SI
	PUSH	DI

    LES     SI,[BP+X]           ; POINTER TO BUFFER
    PUSH    ES                  ; Will pop to ds later
	CLD
;
;  get back stored variables
;
    MOV     AL,ES:[SI]          ; get cursor row
	MOV		ROW,AL
	INC		SI
	MOV		AL,ES:[SI]
	MOV		COL,AL
	INC		SI
	MOV		AL,ES:[SI]			; window boundaries
	MOV		TOPL,AL
	INC		SI
	MOV		AL,ES:[SI]
	MOV		LEFTC,AL
	INC		SI
	MOV		AL,ES:[SI]
	MOV		BOTL,AL
	INC		SI
	MOV		AL,ES:[SI]
	MOV		RIGHTC,AL
    INC     SI                  ; now we are ready for data
;
;  calculate amount to move
;
;
;  calculate number of lines to move
;
    MOV     AL,TOPL         ; X1
    MOV     BL,BOTL         ; X2
    SUB     BL,AL           ; x2 = x2-x1
    INC     BL              ; ADD 1 LINE
    MOV     [BP+X],BL       ; KEEP IT SOMEWHERE SAFE
;
;  calculate screen offset  = 160*x1+2*y1;
;
;   AL has topl in it
	XOR 	AH,AH
	MOV		BL,160
    MUL     BL              ; HOW MANY CHARS IN X1 LINES, IN AX
	XOR 	CH,CH
    MOV     CL,LEFTC        ; CHARS TO LEFT SIDE
    SHL     CL,1            ; *2 COUNTS ATTRIBUTES
    ADD     AX,CX           ; add them
    MOV     DI,AX           ; place into source index
;
;  find number of characters to move each time
;
	MOV	 	CL,RIGHTC
    SUB     CL,LEFTC        ; Y2 = Y2-Y1
	INC  	CL			  	;  add 1
    MOV     [BP+X+1],CL     ; safe place again
;
;  find number to add to wrap around for each line
;
	MOV 	BL,CL		   	; # of chars moved each time
    SHL     BL,1            ; chars*2
	MOV 	AL,160
	SUB 	AL,BL		   	; 160-chars*2
	MOV 	[BP+X+2],AL	 	; safe place to keep it
;
;
ifdef CHECK_FOR_REFRESH
    MOV     AX,video_segment; screen will receive this data
	MOV	 	ES,AX
    POP     DS              ; set ds to where data is coming from
    MOV     DX,03DAH        ; COLOR CARD STATUS PorT
	XOR  	BH,BH
    MOV     BL,[BP+X]       ; howmany lines to move
    CMP     AX,0B000H       ; check for mono card
	JNZ 	RGETLINE
    MOV     DX,03BAH        ; change status port vALue
RGETLINE:
    MOV     CL,[BP+X+1]     ; # of chars to move
RGETCHAR:
    IN      AL,DX           ; GET STATUS BYTE
    TEST    AL,1            ; horizontal retrace
    JNZ     RGETCHAR        ; NOT THERE YET
RISON:
	IN   	AL,DX
	TEST 	AL,1
    JZ      RISON           ; STILL ON, TRY AGAIN
    MOVSW                   ; NOW IS TIME, MOVE THE WORD
    LOOP    RGETCHAR        ; DO ANOTHER UNTIL THIS LINE IS DONE
else
    MOV     AX,video_segment; screen will receive this data
	MOV	 	ES,AX
    POP     DS              ; set ds to where data is coming from
;   MOV     DX,03DAH        ; COLOR CARD STATUS PORT
	XOR  	BH,BH
    MOV     BL,[BP+X]       ; howmany lines to move
;   CMP     AX,0B000H       ; check for mono card
;	JNZ 	RGETLINE
;   MOV     DX,03BAH        ; change status port vALue
RGETLINE:
	XOR		CH,CH
    MOV     CL,[BP+X+1]     ; # of chars to move
RGETCHAR:
;   IN      AL,DX           ; GET STATUS BYTE
;   TEST    AL,1            ; horizontal retrace
;   JNZ     RGETCHAR        ; NOT THERE YET
;RISON:
;	IN   	AL,DX
;   TEST 	AL,1
;   JZ      RISON           ; STILL ON, TRY AGAIN
;   MOVSW                   ; NOW IS TIME, MOVE THE WORD
;   LOOP    RGETCHAR        ; DO ANOTHER UNTIL THIS LINE IS DONE
    REP     MOVSW           ; move all the words
endif
;
	DEC 	BX
    JZ      RENDKEEP        ; DONE, WE ARE OUT OF HERE
	MOV		AL,[BP+X+2]
	XOR		AH,AH
    ADD     DI,AX           ; go to next line, skip 160-n*2 bytes
	JMP 	RGETLINE

RENDKEEP:
	POP		DI
	POP		SI
	POP  	DS
    MOV     DH,ROW          ; LOAD STORED CURSOR POSITION
	MOV		DL,COL
   	MOV	 	AH,2H
   	XOR 	BX,BX
    INT     10H             ; SET CURSOR POSITION TO SAVED VALUE
    POP     ES
	POP  	BP
	RET
ifdef Microsoft
_n_restwin	endp
else
n_restwin	endp
endif
endif       ; NOT_USED

;************************************************************************
;  n_puts
;  Window-compatible puts, uses n_putchar and ALso translates
;  \n to CRLF
;
;    usage:  identical to puts()
;
ifdef Microsoft
_n_puts	proc	far
else
n_puts	proc	far
endif
	PUSH	BP
	MOV		BP,SP
    PUSH    ES
    PUSH    DI

;   MOV     DX,[BP+X+2]         ; ds OF STRING
;   MOV     ES,DX
;   MOV     BX,[BP+X]           ; PTR TO STRING
    LES     DI,[BP+X]           ; PTR TO STRING
NEXTC:
;   MOV     AL,[BX]             ; GET CHARACTER
    MOV     AL,ES:[DI]          ; GET CHARACTER
    XOR     AH,AH               ; clear ah

	OR		AL,AL				; is it end of string?
	JZ		DONEST
;   INC     WORD PTR [BP+X]     ; INCREMENT POINTER FOR NEXT ONE
    INC     DI                  ; INCREMENT POINTER FOR NEXT ONE
	CMP		AL,10				; newline?
	JNZ		DOCHAR
	MOV		AL,13
	PUSH	AX
ifdef Microsoft
	CALL	_N_PUTCHAR
else
        CALL    N_PUTCHAR
endif
	POP		AX
	MOV		AX,10
DOCHAR:
	PUSH	AX
ifdef Microsoft
	CALL	_N_PUTCHAR
else
	CALL	N_PUTCHAR
endif
	POP		AX					; take off of the stack
	JMP		NEXTC

DONEST:
	MOV		AX,13				; CR
	PUSH	AX
ifdef Microsoft
	CALL	_N_PUTCHAR
else
	CALL	N_PUTCHAR
endif
	POP		AX

	MOV		AX,10				; LF
	PUSH	AX
ifdef Microsoft
	CALL	_N_PUTCHAR
else
	CALL	N_PUTCHAR
endif
	POP		AX

    POP     DI
    POP     ES
	POP		BP
	RET
ifdef Microsoft
_n_puts	endp
else
n_puts	endp
endif

ifndef NET14
;**********************************************************************
;
;  find first
;   make dos find file names accorDIng to wildcards
;  n_findfirst(filename,attr)
;	char *filename; INT attr;
;
ifdef Microsoft
_n_findfirst	proc	far
else
n_findfirst		proc	far
endif
	PUSH	BP
	MOV		BP,SP
	PUSH	ES
	PUSH	DS

    MOV     AH,02FH             ; DOS FUNCTION GET dta
	INT		21H
ifdef Microsoft
    MOV     _DTAPTR,BX          ; SQUIRREL A COPY FOR ME
	MOV		AX,ES
	MOV 	_DTADS,AX
else
    MOV     DTAPTR,BX           ; SQUIRREL A COPY FOR ME
	MOV		AX,ES
	MOV		DTADS,AX
endif
    MOV     AX,[BP+X+2]         ; DS OF FILENAME PTR
	MOV		DS,AX
	MOV		DX,[BP+X]			; PTR PART OF FILENAME
    MOV     CX,[BP+X+4]         ; ATTRIBUTE TO SEARCH FOR
    MOV     AH,04EH             ; FIND MATCHING FILE DOS CALL
	INT		21H
    JC      BADRET              ; AX ALREADY CONTAINS ERROR CODE
	XOR		AX,AX

BADRET:
	POP		DS
	POP		ES
	POP		BP
	RET
ifdef Microsoft
_n_findfirst	endp
else
n_findfirst	endp
endif
;
;  n_findnext()
;  will find entries that follow findfirst
;  no need to respecify file name
;
ifdef Microsoft
_n_findnext	proc	far
else
n_findnext	proc	far
endif

    MOV     AH,04FH             ; FIND NEXT DOS CALL
	INT		21H
	JC		NBADRET
	XOR		AX,AX
NBADRET:
	RET
ifdef Microsoft
_n_findnext	endp
else
n_findnext	endp
endif
endif       ; NET14

;
;  get the number of timer clicks from BIOS
;
ifdef Microsoft
_n_clicks  proc	far			 ; must be declared long n_clicks()
else
n_clicks  proc	far			 ; must be declared long n_clicks()
endif
	MOV		AH,0
	INT		1AH
ifdef Microsoft
    MOV     AX,DX               ; msc USES AX-LO, DX-HI
    MOV     DX,CX               ; return values from INTerrupt 1A
else
    MOV     AX,CX               ; Lattice uses AX-HI, BX-LO
	MOV		BX,DX				; Lattice returns in BX, MSC in DX and switched
endif

	RET
ifdef Microsoft
_n_clicks endp
else
n_clicks endp
endif

ifndef NET14
;
;	void fix_vid(void)
;
ifdef Microsoft
_fix_vid proc far
else
fix_vid proc far
endif
    push es

    mov ax,40h
    mov es,ax
    mov bx,10h
	mov al,es:[bx]		; get equipment flags
	and al,0ch
    cmp al,0ch
    je done_vid_fix     ; if mono, we don't do anything

	mov bx,89h
	mov al,es:[bx]		; get VDD flag byte
	and al,0fdh			; Make sure bit 1 (gray scale summing enable)
                        ;   is UNSET
    mov es:[bx],al      ; put new flag byte in VDD

	mov ah,0fh
    int 10h             ; get current mode
	xor ah,ah
    int 10h             ; reset current mode (new flags take affect)

done_vid_fix:
	pop es
	ret
ifdef Microsoft
_fix_vid endp
else
fix_vid endp
endif

ifdef Microsoft
_get_mode proc far
else
get_mode proc far
endif
    push bp
    mov bp,sp
    push es

    les bx,dword ptr [bp+6]

    mov cx,7
    mov ax,1c01h
    int 10h
    mov ax,1c02h            ; immediately restore the state
    int 10h

    pop es
    pop bp
    ret

ifdef Microsoft
_get_mode endp
else
get_mode endp
endif


ifdef Microsoft
_set_mode proc far
else
set_mode proc far
endif

    push bp
    mov bp,sp
    push es

    les bx,dword ptr [bp+6]

    mov cx,7
    mov ax,1c02h
    int 10h

    pop es
    pop bp
    ret

ifdef Microsoft
_set_mode endp
else
set_mode endp
endif

ifdef Microsoft
_get_size proc far
else
get_size proc far
endif

    mov ax,1c00h
    mov cx,7
    int 10h
    mov ax,bx

    ret
ifdef Microsoft
_get_size endp
else
get_size endp
endif

;************************************************************************
;  set_page
;
;  Sets the video page
;
ifdef Microsoft
_set_page proc far
else
set_page proc far
endif
    PUSH    BP
    MOV     BP,SP

    MOV     AL,[BP+X]           ; Get the page to switch to
    MOV     AH,05h
    INT     10h

    POP     BP
    RET
ifdef Microsoft
_set_page endp
else
set_page endp
endif

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

    MOV     AH,03H          ; get cursor pos'n & shape
    INT     10H             ;   ..
    MOV     video_cursor,CX ; save shape for later restore

    MOV     AX,1130H        ; get font information
    XOR     BH,BH           ; for current font
    INT     10H             ;   ..
    MOV     AX,1112H        ; assume we're using small font
    CMP     CX,8            ; using 8 * 8 font?
    JE      setfont         ; if yes, good
    MOV     AX,1114H        ; no. try for the 8 * 16 font
    CMP     CX,16           ;   ..
    JE      setfont         ;   ..
    MOV     AX,1111H        ; no. must be the 8 * 14 font
setfont:
    MOV     video_font,AX   ; save function code to restore this font

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
;   This routine fills a buffer with the current video parameter values.
;   Note: initvideo() must be called first in order for this procedure
;   to return meaningful values.
;
;   Usage:      void getvconfig(struct vidinfo *)
;
;   Where:  struct vidinfo {
;               unsigned int segment;
;               int type;
;               int iscolor;
;               int mode;
;               unsigned int page;
;               unsigned int rows;
;               unsigned int columns;
;               unsigned int cursor;
;               unsigned int font_func;
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

    MOV     CX,6            ; Copy six more word values
    XOR     AH,AH           ; Clear the top part of the transfer register
Copy_Loop:
    LODSB                   ; Move each byte from the video parameters
    STOSW                   ; Into a word in the structure
    LOOP    Copy_Loop

    MOVSW                   ; Copy the cursor
    MOVSW                   ; Copy the font function

    POP     SI
    POP     DI
    POP     ES
    POP     BP
    RET
ifdef Microsoft
_getvconfig endp
else
getvconfig  endp
endif

;************************************************************************
;   getvstate
;
;   This routine fills a buffer with the current video parameter values.
;   This routine does not depend on initvideo() for anything.
;
;   Usage:      void getvstate(struct vidstate *)
;
;   Where:  struct vidstate {
;               char mode;
;               unsigned char page;
;               unsigned int cursor;
;               unsigned int curpos;
;               unsigned int font_func;
;           };
;
ifdef Microsoft
_getvstate proc    far
else
getvstate  proc    far
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

    MOV   AH,0FH            ; get video mode
    INT   10H               ;   ..
    MOV   BYTE PTR ES:[DI],AL      ; save original mode
    MOV   BYTE PTR ES:[DI+1],BH    ; save original display page
    MOV   AH,03H            ; get cursor pos'n & shape
    INT   10H               ;   ..
    MOV   WORD PTR ES:[DI+2],CX     ; save shape for later restore
    MOV   WORD PTR ES:[DI+4],DX     ; save position for later restore

    MOV   AX,1130h          ; get font information
    XOR   BH,BH             ; for current font
    PUSH  ES                ; Preserve the ES from being blown away
    INT   10h               ;   ..
    POP   ES                ; Get the ES back
    MOV   AX,1112h          ; assume we're using small font
    CMP   CX,8              ; using 8 * 8 font?
    JE    setvfont           ; if yes, good

    MOV   AX,1114h          ; no. try for the 8 * 16 font
    CMP   CX,16             ;   ..
    JE    setvfont           ;   ..

    MOV   AX,1111h          ; no. must be the 8 * 14 font
setvfont:
    MOV   WORD PTR ES:[DI+6],AX ; save function code to restore this font

    POP     SI
    POP     DI
    POP     ES
    POP     BP
    RET
ifdef Microsoft
_getvstate endp
else
getvstate  endp
endif

;************************************************************************
;   setvstate
;
;   This routine takes variables from a video state structure and
;       sets that video state
;   This routine does not depend on initvideo() for anything.
;
;   Usage:      void setvstate(struct vidstate *)
;
;   Where:  struct vidstate {
;               char mode;
;               unsigned char page;
;               unsigned int cursor;
;               unsigned int font_func;
;           };
;
ifdef Microsoft
_setvstate proc    far
else
setvstate  proc    far
endif
    PUSH    BP
    MOV     BP,SP
    PUSH    ES
    PUSH    DI

    CLD                 ; All moves forward
IF @DataSize
    LES     DI,[BP+X]   ; Get the Pointer to the buffer
ELSE
    MOV     DI,[BP+X]   ; Get the Pointer to the buffer
    PUSH    DS
    POP     ES
ENDIF

    MOV     AL,BYTE PTR ES:[DI] ; set video mode
    XOR     AH,AH
    INT     10H                 ;  ..

    MOV     AX,0500h            ; Force display page 0
    INT     10h

    MOV     AX,WORD PTR ES:[DI+6]   ; Restore original font
    XOR     BL,BL               ; character block 0
    INT     10h

    MOV     AL,BYTE PTR ES:[DI+1]   ; Restore original display page
    MOV     AH,5
    INT     10h

    MOV     CX,WORD PTR ES:[DI+2]   ; Restore cursor shape
    MOV     AH,01h
    INT     10h

    MOV     AH,02                   ; Restore cursor position for the page
    MOV     BH,BYTE PTR ES:[DI+1]
    MOV     DX,WORD PTR ES:[DI+4]
    INT     10h

    POP     DI
    POP     ES
    POP     BP
    RET
ifdef Microsoft
_setvstate endp
else
setvstate  endp
endif

endif       ; NET14

;-----------------------------------------------------------------

ifdef Microsoft
;_TEXT   ENDS
else
	endps
endif
	end

