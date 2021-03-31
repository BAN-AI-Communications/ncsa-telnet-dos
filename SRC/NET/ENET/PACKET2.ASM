;/* cu-notic.txt         NCSA Telnet version 2.2C     2/3/89
;   Notice:
;        Portions of this file have been modified by
;        The Educational Resources Center of Clarkson University.
;
;        All modifications made by Clarkson University are hereby placed
;        in the public domain, provided the following statement remain in
;        all source files.
;
;        "Portions Developed by the Educational Resources Center, 
;                Clarkson University"
;
;        Bugs and comments to bkc@omnigate.clarkson.edu
;                                bkc@clgw.bitnet
;
;        Brad Clements
;        Educational Resources Center
;        Clarkson University
;*/


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

ifdef Microsoft
X	EQU	6

    EXTRN   _packet_vector:WORD

    .MODEL  LARGE
    .data

    PUBLIC  _driver_version,_driver_class,_driver_type,_driver_number
_driver_version  dw     1
_driver_class    db   2
_driver_type     dw   3
_driver_number   db   4

;_driver_version  WORD   1
;_driver_class    BYTE   2
;_driver_type     WORD   3
;_driver_number   BYTE   4

else
	INCLUDE	DOS.MAC
	SETX

    EXTRN   packet_vector:WORD

    EXTRN   driver_version:WORD
    EXTRN   driver_class:BYTE
    EXTRN   driver_type:WORD
    EXTRN   driver_number:BYTE

    DSEG
endif

vector_installed    DB  0       ; Whether we've installed the packet driver
                                ;   vector yet.

ifdef Microsoft
ifdef Watcom
    EXTRN   pkt_receiver2_:FAR
else
    EXTRN   _pkt_receiver2:FAR
endif
    .code
    PUBLIC  _pkt_receiver,_clear_int,_set_int
    PUBLIC  _pkt_access_type,_pkt_driver_info,_pkt_set_recv_mode
    PUBLIC  _pkt_release_type,_pkt_send_pkt,_pkt_get_address
else
    EXTRN   pkt_receiver2:FAR
    PSEG
	PUBLIC	pkt_receiver,clear_int,set_int
    PUBLIC  pkt_access_type,pkt_driver_info,pkt_set_recv_mode
    PUBLIC  pkt_release_type,pkt_send_pkt,pkt_get_address
endif

ifdef Microsoft
_PKT_RECEIVER	PROC	FAR
else
PKT_RECEIVER  PROC    FAR
endif
	PUSHF
ifdef Microsoft
ifdef Watcom
    CALL    pkt_receiver2_
else
    CALL    _pkt_receiver2
endif
else
    CALL    pkt_receiver2
endif
	RET
ifdef Microsoft
_PKT_RECEIVER	ENDP
else
PKT_RECEIVER	ENDP
endif

;************************************************************************
;  pkt_access_type
;
;  Sets the access type for a handler
;
ifdef Microsoft
_pkt_access_type proc far
else
pkt_access_type proc far
endif
    PUSH    BP
    MOV     BP,SP
    PUSH    DS
    PUSH    ES
    PUSH    SI
    PUSH    DI

    MOV     AX,_packet_vector   ; Get the packet driver vector
    CMP     AX,0                ; Check for bad vector
    JNE     ok_access
    MOV     AX,-1               ; Indicate a bad vector
    JMP     exit_pkt_access_type

ok_access:
    CMP     vector_installed,0  ; Check if the packet driver vector has already been installed
    JNE     installed_access
    MOV     BX,CS               ; Modify the code segment (Ugh...)
    MOV     DS,BX
    MOV     SI,OFFSET do_vector ; Get the offset of the interrupt procedure
    INC     SI
    MOV     DS:[SI],AL          ; Modify the interrupt to call

installed_access:
    MOV     AH,02h              ; Packet driver command
    MOV     AL,[BP+X]           ; Get the packet driver class
    MOV     BX,[BP+X+2]         ; Get the packet driver type
    MOV     DX,[BP+X+4]         ; Get the packet driver number
    LDS     SI,[BP+X+6]         ; Get the pointer to the packet type
    MOV     CX,[BP+X+10]        ; Get the length of the packet type info
    LES     DI,[BP+X+12]        ; Get the receiver function
    CALL    do_vector           ; Actually call the interrupt
    JNC     exit_pkt_access_type
    MOV     AX,-1               ; Carry set indicates error

exit_pkt_access_type:

    POP     DI
    POP     SI
    POP     ES
    POP     DS
    POP     BP
    RET
ifdef Microsoft
_pkt_access_type endp
else
pkt_access_type endp
endif

;************************************************************************
;  pkt_driver_info
;
;  Gets the packet driver information
;
ifdef Microsoft
_pkt_driver_info proc far
else
pkt_driver_info proc far
endif
    PUSH    BP
    MOV     BP,SP
    PUSH    DS
    PUSH    ES
    PUSH    SI
    PUSH    DI

    MOV     AX,_packet_vector   ; Get the packet driver vector
    CMP     AX,0                ; Check for bad vector
    JNE     ok_info
    MOV     AX,-1               ; Indicate a bad vector
    JMP     exit_pkt_driver_info

ok_info:
    CMP     vector_installed,0  ; Check if the packet driver vector has already been installed
    JNE     installed_info
    PUSH    DS
    MOV     BX,CS               ; Modify the code segment (Ugh...)
    MOV     DS,BX
    MOV     SI,OFFSET do_vector ; Get the offset of the interrupt procedure
    INC     SI
    MOV     DS:[SI],AL          ; Modify the interrupt to call
    POP     DS

installed_info:
    MOV     AX,001ffh           ; Packet driver command
    MOV     BX,[BP+X]           ; Get the packet type handle
    PUSH    DS
    CALL    do_vector           ; Actually call the interrupt
    POP     DS
    JNC     ok_pkt_driver_info
    XOR     AH,AH
    MOV     AL,DH               ; Carry set indicates error
    JMP     exit_pkt_driver_info

ok_pkt_driver_info:
ifdef Microsoft
    MOV     _driver_version,BX      ; Get the driver version
    MOV     _driver_class,CH        ; Get the driver class
    MOV     _driver_type,DX         ; Get the driver type
    MOV     _driver_number,CL       ; Get the driver number
else
    MOV     driver_version,BX       ; Get the driver version
    MOV     driver_class,CH         ; Get the driver class
    MOV     driver_type,DX          ; Get the driver type
    MOV     driver_number,CL        ; Get the driver number
endif
    MOV     AX,0                    ; Indicate no error

exit_pkt_driver_info:

    POP     DI
    POP     SI
    POP     ES
    POP     DS
    POP     BP
    RET
ifdef Microsoft
_pkt_driver_info endp
else
pkt_driver_info endp
endif

;************************************************************************
;  pkt_set_recv_mode
;
;  Sets the packet driver receive mode for a packet type
;
ifdef Microsoft
_pkt_set_recv_mode proc far
else
pkt_set_recv_mode proc far
endif
    PUSH    BP
    MOV     BP,SP
    PUSH    DS
    PUSH    ES
    PUSH    SI
    PUSH    DI

    MOV     AX,_packet_vector   ; Get the packet driver vector
    CMP     AX,0                ; Check for bad vector
    JNE     ok_recv
    MOV     AX,-1               ; Indicate a bad vector
    JMP     exit_pkt_set_recv_mode

ok_recv:
    CMP     vector_installed,0  ; Check if the packet driver vector has already been installed
    JNE     installed_recv
    MOV     BX,CS               ; Modify the code segment (Ugh...)
    MOV     DS,BX
    MOV     SI,OFFSET do_vector ; Get the offset of the interrupt procedure
    INC     SI
    MOV     DS:[SI],AL          ; Modify the interrupt to call

installed_recv:
    MOV     AX,0200h           ; Packet driver command
    MOV     BX,[BP+X]           ; Get the packet type handle
    MOV     CX,[BP+X+2]         ; Get the new mode
    CALL    do_vector           ; Actually call the interrupt
    JNC     ok_pkt_set_recv_mode
    XOR     AH,AH
    MOV     AL,DH               ; Carry set indicates error
    JMP     exit_pkt_set_recv_mode

ok_pkt_set_recv_mode:
    MOV     AX,0                    ; Indicate no error

exit_pkt_set_recv_mode:
    POP     DI
    POP     SI
    POP     ES
    POP     DS
    POP     BP
    RET
ifdef Microsoft
_pkt_set_recv_mode endp
else
pkt_set_recv_mode endp
endif

;************************************************************************
;  pkt_release_type
;
;  Tells the packet driver we don't need a packet type anymore
;
ifdef Microsoft
_pkt_release_type proc far
else
pkt_release_type proc far
endif
    PUSH    BP
    MOV     BP,SP
    PUSH    DS
    PUSH    ES
    PUSH    SI
    PUSH    DI

    MOV     AX,_packet_vector   ; Get the packet driver vector
    CMP     AX,0                ; Check for bad vector
    JNE     ok_release
    MOV     AX,-1               ; Indicate a bad vector
    JMP     exit_pkt_release_type

ok_release:
    CMP     vector_installed,0  ; Check if the packet driver vector has already been installed
    JNE     installed_release
    MOV     BX,CS               ; Modify the code segment (Ugh...)
    MOV     DS,BX
    MOV     SI,OFFSET do_vector ; Get the offset of the interrupt procedure
    INC     SI
    MOV     DS:[SI],AL          ; Modify the interrupt to call

installed_release:
    MOV     AX,0300h            ; Packet driver command
    MOV     BX,[BP+X]           ; Get the packet type handle
    CALL    do_vector           ; Actually call the interrupt
    JNC     ok_pkt_release_type
    XOR     AH,AH
    MOV     AL,DH               ; Carry set indicates error
    JMP     exit_pkt_release_type

ok_pkt_release_type:
    MOV     AX,0                    ; Indicate no error

exit_pkt_release_type:
    POP     DI
    POP     SI
    POP     ES
    POP     DS
    POP     BP
    RET
ifdef Microsoft
_pkt_release_type endp
else
pkt_release_type endp
endif

;************************************************************************
;  pkt_send_pkt
;
;  Sends a packet through the packet driver
;
ifdef Microsoft
_pkt_send_pkt proc far
else
pkt_send_pkt proc far
endif
    PUSH    BP
    MOV     BP,SP
    PUSH    DS
    PUSH    ES
    PUSH    SI
    PUSH    DI

    MOV     AX,_packet_vector   ; Get the packet driver vector
    CMP     AX,0                ; Check for bad vector
    JNE     ok_send
    MOV     AX,-1               ; Indicate a bad vector
    JMP     exit_pkt_send_pkt

ok_send:
    CMP     vector_installed,0  ; Check if the packet driver vector has already been installed
    JNE     installed_send
    MOV     BX,CS               ; Modify the code segment (Ugh...)
    MOV     DS,BX
    MOV     SI,OFFSET do_vector ; Get the offset of the interrupt procedure
    INC     SI
    MOV     DS:[SI],AL          ; Modify the interrupt to call

installed_send:
    MOV     AX,0400h            ; Packet driver command
    LDS     SI,[BP+X]           ; Get the buffer address
    MOV     CX,[BP+X+4]         ; Get the buffer length
    CALL    do_vector           ; Actually call the interrupt
    JNC     ok_pkt_send_pkt
    XOR     AH,AH
    MOV     AL,DH               ; Carry set indicates error
    JMP     exit_pkt_send_pkt

ok_pkt_send_pkt:
    MOV     AX,0                    ; Indicate no error

exit_pkt_send_pkt:
    POP     DI
    POP     SI
    POP     ES
    POP     DS
    POP     BP
    RET
ifdef Microsoft
_pkt_send_pkt endp
else
pkt_send_pkt endp
endif

;************************************************************************
;  pkt_get_address
;
;  Gets the net address of the interface for the packet handle and
;   puts it into the buffer
;
ifdef Microsoft
_pkt_get_address proc far
else
pkt_get_address proc far
endif
    PUSH    BP
    MOV     BP,SP
    PUSH    DS
    PUSH    ES
    PUSH    SI
    PUSH    DI

    MOV     AX,_packet_vector   ; Get the packet driver vector
    CMP     AX,0                ; Check for bad vector
    JNE     ok_get_addr
    MOV     AX,-1               ; Indicate a bad vector
    JMP     exit_pkt_get_address

ok_get_addr:
    CMP     vector_installed,0  ; Check if the packet driver vector has already been installed
    JNE     installed_get_addr
    MOV     BX,CS               ; Modify the code segment (Ugh...)
    MOV     DS,BX
    MOV     SI,OFFSET do_vector ; Get the offset of the interrupt procedure
    INC     SI
    MOV     DS:[SI],AL          ; Modify the interrupt to call

installed_get_addr:
    MOV     AX,0600h            ; Packet driver command
    MOV     BX,[BP+X]           ; Get the packet type handle
    LES     DI,[BP+X+2]         ; Get the buffer address
    MOV     CX,[BP+X+6]         ; Get the buffer length
    CALL    do_vector           ; Actually call the interrupt
    JNC     ok_pkt_get_address
    XOR     AH,AH
    MOV     AL,DH               ; Carry set indicates error
    JMP     exit_pkt_get_address

ok_pkt_get_address:
    MOV     AX,0                    ; Indicate no error

exit_pkt_get_address:
    POP     DI
    POP     SI
    POP     ES
    POP     DS
    POP     BP
    RET
ifdef Microsoft
_pkt_get_address endp
else
pkt_get_address endp
endif

; Procedure to call the packet driver interrupt
do_vector   PROC    NEAR
    INT     60h
	RET
do_vector   ENDP

ifdef Microsoft
_CLEAR_INT	PROC	FAR
else
CLEAR_INT	PROC	FAR
endif
	CLI
	RET
ifdef Microsoft
_CLEAR_INT	ENDP
else
CLEAR_INT	ENDP
endif

ifdef Microsoft
_SET_INT	PROC	FAR
else
SET_INT	PROC	FAR
endif
	STI
	RET
ifdef Microsoft
_SET_INT	ENDP
else
SET_INT	ENDP
endif
ifdef Microsoft
;_TEXT	ends
else
	ENDPS
endif
	END

