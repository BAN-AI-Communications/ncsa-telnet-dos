;  1205, Wed 12 Sep 90
;
;  DNDLL2:  Interface to DECNET/DOS DLL
;
;  Nevil Brownlee,  n.brownlee@aukuni.ac.nz
;  Computer Centre,  University of Auckland.
;
;  int DLLfn (   /* Invoke DLL function */
;     int fn;           /* Function nbr */
;     struct dcb *dp);	/* Datalink Control Block */
;
;  void r_callback();  /* Receive callback routine */
;  void t_callback();  /* Transmit callback routine */
;		       /* Callback routines point ds to dgroup,
;			  then call a C routine passing the ucb */
;
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
ifdef Microsoft
X	EQU	6
	DOSSEG
	.MODEL	LARGE
else
	INCLUDE	DOS.MAC
	SETX
endif
;
ifdef Microsoft
ifdef TurboC				; !!! NB
		.code   dndll_text	; TurboC needs to know the segment
else					;    where the object is declared!
		.code			; Microsoft doesn't!
endif
ifdef Watcom
        extrn   c_r_callback_:far
        extrn   c_t_callback_:far
else
		extrn	_c_r_callback:far
		extrn	_c_t_callback:far
endif
;
;        	.code
		PUBLIC	_DLLfn,_r_callback,_t_callback
else
		PSEG
		extrn	c_r_callback:far
		extrn	c_t_callback:far
		PUBLIC	DLLfn,r_callback,t_callback
endif
;
DLLINT		equ	069h	; Id for the DECNET DLL driver Release 2.1
;				;    Earlier releases used 6D!
;
ifdef Microsoft
_DLLfn		PROC	FAR
else
DLLfn		PROC	FAR
endif
    push    bp
    mov bp,sp
    push    si      ; Turbo uses index regs for register variables
    push    di
    mov ax,X[bp]    ; Release 2.1 puts fn code in AL,
    mov ah,0Ah      ;    0Ah in AL
                    ; Earlier releases put fn code in AH!
    mov bx,X+2[bp]  ; dcb offset
    mov cx,X+4[bp]  ; dcb segment
    push    es
    mov es,cx
    int DLLINT      ; DECNET/DOS DLL process
    pop     es
    pop     di
    pop     si
    pop     bp
    ret
ifdef Microsoft
_DLLfn		endp
else
DLLfn		endp
endif
;
;
ifdef Microsoft
_r_callback	PROC	FAR
else
r_callback	PROC	FAR
endif
    push    ds          ; Save caller's registers
    push    es
    push    dx
ifdef Watcom
    push    ax          ; Preserve the AX register also for Watcom
endif
    mov     dx,dgroup   ; Point ds to dgroup
    mov     ds,dx
ifdef Watcom
    MOV     DX,ES       ; Watcom C passes variables in registers
    MOV     AX,BX       ;
else
    push    es          ; c_r_callback(&ucb);
    push    bx
endif
ifdef Microsoft
ifdef Watcom
    call    c_r_callback_
else
    call    _c_r_callback
endif
else
    call    c_r_callback
endif
ifndef Watcom
    pop dx      ; Cut back stack
    pop dx
endif
ifdef Watcom
    pop ax
endif
    pop dx      ; Restore caller's registers
    pop es
    pop ds
    ret
ifdef Microsoft
_r_callback	endp
else
r_callback	endp
endif
;
;
ifdef Microsoft
_t_callback	PROC	FAR
else
t_callback	PROC	FAR
endif
    push    ds      ; Save segment registers
    push    es
    push    dx
ifdef Watcom
    push    ax      ; Preserve the AX register also for Watcom C
endif
    mov     dx,dgroup   ; Point ds to dgroup
    mov     ds,dx
ifdef Watcom
    MOV     DX,ES       ; Watcom C passes variables in registers
    MOV     AX,BX       ;
else
    push    es      ; c_t_callback(&ucb);
    push    bx
endif
ifdef Microsoft
ifdef Watcom
    call    c_t_callback_
else
    call    _c_t_callback
endif
else
    call    c_t_callback
endif
ifndef Watcom
    pop dx      ; Cut back stack
    pop dx
endif
ifdef Watcom
    pop ax
endif
    pop dx      ; Restore caller's registers
    pop es
    pop ds
    ret
ifdef Microsoft
_t_callback	endp
else
t_callback	endp
endif
;
    end

