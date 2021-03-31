;
;  ncsabrk.asm
;  Support for BREAK interupts in NCSA Telnet
;****************************************************************************
;*																			*
;*																			*
;*	  part of NCSA Telnet													*
;*	  by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer		*
;*	  	Kurt Mahan, Heeren Pathak, and Quincey Koziol						*
;*																			*
;*	  National Center for Supercomputing Applications						*
;*	  152 Computing Applications Building									*
;*	  605 E. Springfield Ave.												*
;*	  Champaign, IL  61820													*
;*																			*
;*																			*
;****************************************************************************

	TITLE	NCSABRK	-- LOW-LEVEL I/O FOR SANE HARDWARE HANDLING
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
;	Support for the BREAK interupt added, June 1990, Quincey Koziol
;
;   National Center for Supercomputing Applications
;
	NAME	NBREAK

;
;  Internal data 
;
X 	EQU	6					;  for the large model programs
;  Match the model directive with the application model
.model large

;-------------------------------------------------------------------------------------------
;	MACROs and EQUATES
;-------------------------------------------------------------------------------------------

sim_int macro	num
	pushf
	call	cs:orig_&num
endm

TRUE	equ	1
FALSE	equ	0

;  large model flag parameter equates
if @CODESIZE
	FLG_OFF	equ	[bp+6]
	FLG_SEG	equ	[bp+8]

;  small model flag parameter equates
else
	FLG_OFF	equ	[bp+4]
	FLG_SEG	equ	[bp+6]
endif

;
;   The subroutines to call from C
;
ifdef Microsoft
;_TEXT	segment	public	'CODE'
;	assume CS:_TEXT
.code
	PUBLIC	_install_break,_remove_break
else
	PSEG
	PUBLIC	install_break,remove_break
endif

;-------------------------------------------------------------------------------------------
;	The replaced addrss and the flag pointer are in the code
;	segment so they will be accessable to the interrupt 
;	replacement code.
;-------------------------------------------------------------------------------------------

;  far address of application program's Ctrl-C/Break
;  detected flag
flag		dd	0

orig_16h	dd	0		;  far addresses of
orig_1bh	dd	0		;  the original
orig_23h	dd	0		;  vectors

;  flag to let capture () & release () know there are 
;  valid addresses in org_??h
replaced	db	FALSE


assume	ds:@curseg

;**************************************************************************
;
;  Routines to install and deinstall a BREAK routine which intersepts ctrl-c's
;
;
BREAKINT		EQU	4*23H		; User hook to break int

;----------------------------------------------------------------------------------------------
;	The following "installation" prodecure is a near call even 
;	in the large model enironment
;----------------------------------------------------------------------------------------------
install_vectors	proc	near

;  get current int 16h vector
	mov	ax, 3516h
	int	21h
	mov	word ptr orig_16h, bx

;  save the vector we found
	mov	word ptr orig_16h+2, es

;  get address of new handler
	lea	dx, sixteen_handler

;  set the vector to point
;  to our routine
	mov	ax, 2516h
	int	21h

;---------------------------------------------------------------------------------------------
;	Replacement of 1bh is mandatory if you want to prevent int
;	1bh (Ctrl-Break) from setting the flag DOS looks at
;----------------------------------------------------------------------------------------------

;  get current int 1bh vector
	mov	ax, 351bh
	int	21h
	mov	word ptr orig_1bh, bx

;  save the vetor we found
	mov	word ptr orig_1bh+2, es

;  get address of new handler
	lea	dx, int1b_handler

;  set the vector to point
;  to our routine
	mov	ax, 251bh
	int	21h

;---------------------------------------------------------------------------------------------
;	Replacement of 23h is *not* necessary to trap Ctrl C or
;	Ctrl-Break, however, if you want to break from your code
;	using Ctrl-2 or Alt-3, int 23h is where we'll make a call to
;	release () so the installed interrupt handlers will be un-
;	installed, thus preventing a system hang on return to DOS
;----------------------------------------------------------------------------------------------

;  get current int 23h vector
	mov	ax, 3523h
	int	21h
	mov	word ptr orig_23h, bx

;  save the vector we found
	mov	word ptr orig_23h+2, es

;  get address of new handlers
	lea	dx, int23_handler

;  set the vector to  point
;  to our routine
	mov	 ax, 2523h
	int	21h
	ret
install_vectors endp

;****************************************************************
; install_break
;
;  install the break interrupt handler, the handler is technically
;  part of this procedure.
;
ifdef Microsoft
_install_break	proc 	far
else
install_break	proc	far
endif
;  establish the stack frame
	push	bp
	mov	bp, sp

;  save the application programs ds & es registers
	push	ds
	push	es

;  make ds point to the code segment for vector swaps
	push	cs
	pop	ds

;  check if already installed
	cmp	replaced, TRUE
	jz	capture_exit

;  get the offset and segment of application "break_flag"
	mov	ax, word ptr FLG_OFF
	mov	word ptr flag, ax
	mov	ax, word ptr FLG_SEG
	mov	word ptr flag+2, ax

;  install the replacements
;  NOTE:  near overides for when large model
	call	near ptr install_vectors

;  flag that things have changed
	mov	byte ptr replaced, TRUE

;  restore registers and stack frame
capture_exit:
	pop	es
	pop	ds
	pop	bp
	ret
ifdef Microsoft
_install_break	endp
else
install_break	endp
endif

assume cs:@curseg, ds:nothing, es:nothing

;****************************************************************
; remove_break
;
;  restores interrupt 16h, 1bh, 23h to what they were before installing our
;	break handler
;
ifdef Microsoft
_remove_break	proc 	far
else
remove_break	proc	far
endif
;  save regs used locally
	push	ds
	push	dx

;  save the flags in case this routine has been called 
;  by the int 23h handler
	pushf

;  check that _capture() has installed the handlers
	cmp	cs:replaced, TRUE
	jnz	release_exit

;  ds:dx gets the address of the saved original
;  interrupt 16h vector
	lds	dx, cs:orig_16h

;  reset the int 16h vector
	mov	ax, 2516h
	int	21h

;  ds:dx gets the address of the saved original
;  interrupt 1bh vector
	lds	dx, cs:orig_1bh
	
;  reset the int 1bh vector
	mov	ax, 251bh
	int	21h

;  ds:dx gets the address of the saved original
;  interrupt 23h vector
	lds	dx, cs:orig_23h

;  reset the int 23h vector
	mov	ax, 2523h
	int	21h

;  indicate that vectors are no longer replaced
	mov	cs:replaced, FALSE

release_exit:
;  restore flags, dx & ds
	popf
	pop	dx
	pop	ds
	ret
ifdef Microsoft
_remove_break	endp
else
remove_break	endp
endif

assume cs:@curseg, ds:nothing, es:nothing

;  place to store the int 16h function parameter
;  re-entrance is not a problem
save_funct	db	?

;------------------------------------------------------------------------------------------------
;	Sixteen_handler is a far proc regardless of the memory model
;	specified in the ".model" directive since it is an interrupt 
;	replacement routine.
;------------------------------------------------------------------------------------------------

sixteen_handler proc far

;  save the function value
	mov	cs:save_funct, ah

;  convert to the non-extended numbers
	and	ah, 11101111b

; is it a shift status request
	cmp	ah, 2

;  lower than shift status request, we'll take care of it
	jb	not_shift_status_req

;  put back the callers function, pass it to the BIOS &
;  don't come back here
	mov	ah, cs:save_funct
	jmp	cs:orig_16h

not_shift_status_req:
;  if it is a "is_keyready" call, handle it in the 
;  keyready_call block of code
	cmp	ah, 1
	jz	keyready_call

;  must be a "get_key" request
get_key_call:

;  restore the callers original function value
	mov	ah, cs:save_funct

; simulate an interrupt
	sim_int	16h

;  did the BIOS return the Ctrl-C keycode
	cmp	ax, 2e03h

;  no, so we can return to caller
	jnz	iret_back

;  the BIOS returned a Ctrl-C keycode, so
;  set the flag in the application program
	call	near ptr set_flag

;  the Ctrl-C dey is thrown away so go bakc and get
;  another key
	jmp	get_key_call

keyready_call:
;  restore the callers original function value
	mov	ah, cs:save_funct

; simulate an interrupt
	sim_int	16h

;  if the zero flag is set (by the BIOS), the keyboard
;  buffer is empty - ok to return to caller
	jz	ok_to_go_back

;  compare the key at the heaad of the keyboard buffer
;  with Ctrl-C keycode.  This compare will leave the Z flag
; indicating a key is available.
	cmp	ax, 2e03h
	jnz	ok_to_go_back

;  key was Ctrl-C, set the application program flag
	call	near ptr set_flag

;  remove Ctrl-C keycode from the keyboard buffer
	mov	ah, 0
	sim_int	16h

;  log back to see if a non Ctrl-C key is ready
	jmp	keyready_call

ok_to_go_back:
;  throw away flags of our caller and return
	ret	2

iret_back:
;  restore callers flags on return
	iret
sixteen_handler endp


;------------------------------------------------------------------------------------------
;	int1b_handler is a far proc regardless of memory model
;	set the application program flag and return
;------------------------------------------------------------------------------------------
int1b_handler proc far
	call	near ptr set_flag
	iret
int1b_handler endp

;------------------------------------------------------------------------------------------
;	int23_handler is a far proc regardless of memory model
;	used here to allow Ctrl-2 or Alt-3 to "break" the program
;	execution.
;
;	restore the original vectors and execute the original Ctrl-C
;	interrupt handler
;------------------------------------------------------------------------------------------
int23_handler proc far
;	call _release
	jmp	cs:orig_23h
int23_handler endp

;------------------------------------------------------------------------------------------
;	set_flag is a near procedure regardless of memory model
;
;	Use the address passed to _capture() and set the integer
;	refrenced to one.
;------------------------------------------------------------------------------------------

set_flag proc near
	push ds
	push si

;  get the address of the application break flag
	lds	si,cs:flag

;  set the flag to one
	mov	word ptr [si], 1
	pop	si
	pop	ds
	ret
set_flag endp

ifdef Microsoft
;_TEXT	ENDS
else
	endps
endif
	end
