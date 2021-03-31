        page    ,132
;  WD8003A driver code
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
;*      WD8003A driver adapted from WD8003 driver by                        *
;*      Alan Ballard, University of British Columbia                        *
;*      Alan_Ballard@mtsg.ubc.ca (128.189.103.1)                            *
;*                                                                          *
;****************************************************************************
;
        TITLE   NETSUPPORT -- LOW LEVEL DRIVERS FOR ETHERNET
;
;  Assembler support for WD8003/A  Ethernet I/O on the PC
;
;  Reads and writes from the 8K/16K buffer on the WD card.
;
;
;  Changes required for the 8003A (Micro Channel) driver are conditional,
;  based on defined symbol MCA.
;  These could easily be run-time determined via an initialization
;  test for MCA (BIOS int 15H function C0H) or an initialization call
;  from pctools.c based on the configuration file type.
;
;  The changes required are:
;       The 8003A supports only word-mode, even address access.
;       The DMA controller must be configured for word mode.
;       Shared memory is 16K bytes instead of 8K bytes
;
MCA     EQU     1

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

        NAME    NET8003
ifdef MSC6
		INCLUDE NET\ENET\NET8003.INC
else
		INCLUDE NET8003.INC
endif
        SUBTTL
ifdef Microsoft
X       EQU     6
else
        INCLUDE DOS.MAC
        SETX
endif
;
;  macros for writing to WD board
;
;***********************************************************************
;
;       Macros, from example driver
;
;***********************************************************************

;
; MACRO rd_wd8
;   Reads port specified on macro call. Leaves byte in AL.
;

rd_wd8  MACRO   port
        mov     DX, WDBASE
        add     DX, port                ; DX contains address of port
        in      AL, DX                  ; AL contains data read from port
        ENDM

;
; MACRO wr_wd8
;   Writes byte in AL to port specified on macro call.
;

wr_wd8  MACRO   port
        mov     DX, WDBASE
        add     DX, port                ; DX contains address of port
        out     DX, AL                  ; AL contains data to be written to port
        ENDM

;
ifdef Microsoft
DGROUP  group   _DATA
_DATA   segment public 'DATA'
        assume  DS:DGROUP
else
        DSEG
;       PUBLIC  RSTAT,BUFPT,BUFORG,BUFEND,BUFREAD,BUFBIG,BUFLIM
endif
        .sall                   ; suppress macro expansion listing
;
;  The pointers below are actually DWORDs but we access them two
;  bytes at a time.
;
; STAT change to RSTAT because of name clash with MSC library routine
ifdef Microsoft
        EXTRN   _RSTAT:BYTE     ; last status from read
        EXTRN   _BUFPT:WORD     ; current buffer pointer
        EXTRN   _BUFORG:WORD    ; pointer to beginning of buffer
        EXTRN   _BUFEND:WORD    ; pointer to end of buffer
        EXTRN   _BUFREAD:WORD   ; pointer to where program is reading
        EXTRN   _BUFBIG:WORD    ; integer, how many bytes we have
        EXTRN   _BUFLIM:WORD    ; integer, max bytes we can have
else
        EXTRN   RSTAT:BYTE      ; last status from read
        EXTRN   BUFPT:WORD      ; current buffer pointer
        EXTRN   BUFORG:WORD     ; pointer to beginning of buffer
        EXTRN   BUFEND:WORD     ; pointer to end of buffer
        EXTRN   BUFREAD:WORD    ; pointer to where program is reading
        EXTRN   BUFBIG:WORD     ; integer, how many bytes we have
        EXTRN   BUFLIM:WORD     ; integer, max bytes we can have
endif
;
;
;RSTAT  DB      00H             ; last status from read
;BUFBIG DW      00H             ; buffer space used
;BUFLIM DW      05000H          ; buffer space limit
;BUFPT  DW      00000H          ; where buffer pointer is, initialized safely
;BUFDS  DW      0a000H          ; where buffer is, ds
;BUFORG DW      00000H          ; start of buffer space
;BUFDS2 DW      0a000H          ; another ds
;BUFEND DW      06000H          ; end limit of allowable buffer space
;BUFDS3 DW      0a000H
;BUFREAD        DW      00000H          ; where the read pointer is
;BUFDS4 DW      0a000H

WDBASE  DW      00h             ; base ioaddr
WDADD   DW      00h             ; base shared mem addr
DEAF    DB      00H             ; when we can't handle any more packets
OFFS    DW      00H             ; how many times the handler was turned off

STOP_PAGE DB    STOP_PG         ; stop page number     (32 or 64)
;
ifdef Microsoft
_DATA   ends
else
        ENDDS
endif
;
;
;
;   The subroutines to call from C
;
ifdef Microsoft
NET8003A_TEXT   segment public  'CODE'
        assume CS:NET8003A_TEXT
        PUBLIC  _WARECV,_WAETOPEN,_WAETCLOSE,_WAGETADDR
ifdef NOT_USED
        PUBLIC  _WASETADDR
endif
        PUBLIC  _WAXMIT,_WAETUPDATE
else
        PSEG
        PUBLIC  WARECV,WAETOPEN,WAETCLOS,WAGETADD
ifdef NOT_USED
        PUBLIC  WASETADD
endif
        PUBLIC  WAXMIT,WAETUPDA
endif

;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,addr,ioaddr)
;           char s[6];       ethernet address
;           int irq,addr,ioaddr;
;                interrupt number (unused), base mem address and
;                i/o address to use
;
ifdef Microsoft
_WAETOPEN       PROC    FAR
else
WAETOPEN        PROC    FAR
endif
        PUSH    BP
        MOV     BP,SP
        mov     AX,[BP+X+8]             ; install ioaddr
        mov     WDBASE,AX
        mov     AX,[BP+X+6]             ; install shared mem addr
        mov     WDADD,AX

;
;  Reset the controller
;
        mov     ax,MSK_RESET
        wr_wd8  W83CREG
        mov     ax,0
        wr_wd8  W83CREG

;
;  Initialize shared memory.  According to the example driver,
;  we just set the enabled bit for the 8003A; for the 8003
;  it appears the address must be set as well. (NCSA driver for
;  8003 doesn't set the enable bit, so I guess it defaults enabled
;  for that board...)
;
ifdef MCA
        mov     ax,MSK_ENASH            ; enable pattern
else
        mov     ax,WDADD
        mov     cl,9
        shr     ax,cl                   ; adapt for MSR reg
endif
        wr_wd8  W83CREG                 ; ship it to offset 0 (MSR)
;
;  portions adapted from WD8STAR2.ASM example driver
;  Initializations as recommended by manufacturer and National Semi

;
;  initialize stop page
;
ifdef MCA
        mov     STOP_PAGE,STOP_PG+STOP_PG ; 16 K for 8003A
else
        mov     STOP_PAGE,STOP_PG       ; 8K for 8003
endif
;
;
; initial the LAN Controller register
;

; program for page 0
        mov     AL, MSK_PG0 + MSK_RD2
        wr_wd8  CMDR
; initial DCR data configuration
ifdef MCA
        mov     AL, MSK_BMS + MSK_FT10 + MSK_WTS  ; word mode
else
        mov     AL, MSK_BMS + MSK_FT10  ; select FIFO threshold = 8 bytes
endif
        wr_wd8  DCR
; clr RBCR0,1
        xor     AL, AL
        wr_wd8  RBCR0
        wr_wd8  RBCR1
; initial RCR to monitor mode
        mov     AL, MSK_MON
        wr_wd8  XRCR                    ; disable the rxer
; initial TCR
        xor     AL, AL
        wr_wd8  TCR                     ; normal operation
; initial rev buffer ring
        mov     AL, STOP_PAGE
        wr_wd8  PSTOP                   ; init PSTOP
        mov     AL, STRT_PG
        wr_wd8  PSTART                  ; init PSTART to the 1st page of ring
        wr_wd8  BNRY                    ; init BNRY
; clr ISR by 1's
        mov     AL, -1                  ; write FF
        wr_wd8  ISR
; initial IMR
        mov     AL, 00h                 ; ***NCSA Telnet does not
                                        ; ***need interrupts on
        wr_wd8  IMR

; program for page 1
        mov     AL, MSK_PG1 + MSK_RD2
        wr_wd8  CMDR
; initial physical addr
        mov     DX, WDBASE              ; get board io base
        push    DS
        mov     ax,[bp+X+2]             ; get seg from parms
        mov     ds,ax

        mov     CX, BPNA                ; should be 6 for Ethernet
        mov     BX, [BP+X]              ; ptr to adr in BX
        add     DX, PAR0                ; i/o address of PAR0 in DX
lopa:
        mov     AL, [BX]                ; get 1 byte into AL
        out     DX, AL                  ; write to PAR
        inc     BX
        inc     DX
        loop    lopa
        pop     DS

; initial multicast filter,  write all 0's  into MAR0 - MAR7
        mov     CX, 8
        mov     DX, WDBASE
        add     DX, MAR0                ; i/o address of MAR0 in DX
        xor     AL, AL
lopb:
        out     DX, AL
        inc     DX
        loop    lopb
; initial CURR = PSTART + 1
        mov     AL, STRT_PG + 1
        wr_wd8  CURR
; program for page 0
        mov     AL, MSK_PG0 + MSK_RD2
        wr_wd8  CMDR
; put 8390 on line
        mov     AL, MSK_STA + MSK_RD2           ; activate 8390
        wr_wd8  CMDR
; program RCR to normal operation (MSK_AB, no MSK_AM)
        mov     AL, MSK_AB                      ; accept broadcast
        wr_wd8  XRCR

        XOR     AX,AX
        POP     BP
        RET
ifdef Microsoft
_WAETOPEN       ENDP
else
WAETOPEN        ENDP
endif

ifdef NOT_USED
;
;******************************************************************
;  SETADDR
;    set the Ethernet address on the board to 6 byte ID code
;
;   usage:   setaddr(s,basea,ioa);
;             char s[6];           ethernet address to use
;             int basea;           shared memory base address
;             int ioa;             io address for board
;
ifdef Microsoft
_WASETADDR      PROC    FAR
else
WASETADD        PROC    FAR
endif
        PUSH    BP
        MOV     BP,SP
;
;  not used for this board, set during etopen
;
        POP     BP
        RET
ifdef Microsoft
_WASETADDR      ENDP
else
WASETADD        ENDP
endif
endif       ; NOT_USED
;
;*******************************************************************
;  GETADDR
;     get the Ethernet address off of the board
;
;   usage:  getaddr(s,address,ioaddr);
;       char s[6];           will get six bytes from the PROM
;       int address;
;       int ioaddr;      mem address and ioaddress to use
;
;   Original (Wd8003) version of this driver read the address from
;   port 0, not port ADDROM.  Apparently both work for the 8003, but
;   ADDROM must be used for the 8003A.
;
ifdef Microsoft
_WAGETADDR      PROC    FAR
else
WAGETADD        PROC    FAR
endif
        PUSH    BP
        MOV     BP,SP
        PUSH    DS

        MOV     AX,[BP+X+2]     ; SEG of where to put info
        MOV     DS,AX
        MOV     BX,[BP+X]       ; address of where to put info
        mov     cx,6            ; 6 byte ethernet address required
        mov     dx,[BP+X+6]     ; ioaddr for board
        add     dx,ADDROM       ; offset for ROM addr
getloop:
        in      al,dx
        mov     [bx],al         ; store where we want
        inc     dx
        inc     bx
        loop    getloop
;
        in      al,dx
        cmp     al,3            ; verification of board's existence
        jnz     geterr
        xor     ax,ax           ; success return
        jmp     SHORT noerr
geterr:
        mov     ax,-1           ; error return
noerr:
        POP     DS
        POP     BP
        RET
ifdef Microsoft
_WAGETADDR      ENDP
else
WAGETADD        ENDP
endif
;
;***********************************************************************
;  ETCLOSE
;        shut it down, remove the interrupt handler
;
;  usage:  etclose();
;
;
ifdef Microsoft
_WAETCLOSE      PROC    FAR
else
WAETCLOS        PROC    FAR
endif
        RET
ifdef Microsoft
_WAETCLOSE      ENDP
else
WAETCLOS        ENDP
endif
;
;************************************************************************
;   Receive
;   This is a CPU hook for boards that must be polled before we can
;   deliver packets into the receive buffer.  (i.e. no interrupts used)
;
;    usage:  recv();
;
ifdef Microsoft
_WARECV PROC    FAR
else
WARECV  PROC    FAR
endif
        push    bp
        PUSH    SI
        PUSH    DI
        push    es
;
;  check for data which can be read
;
        mov     AL, MSK_PG1 + MSK_RD2   ; read CURR reg
        wr_wd8  CMDR
        rd_wd8  CURR
        mov     BL, AL                  ; CURR in BL
        mov     AL, MSK_PG0 + MSK_RD2   ; read BNRY reg
        wr_wd8  CMDR
        rd_wd8  BNRY                    ; BNRY in AL
        add     AL, 1                   ; start page of frm in AL
        cmp     AL, STOP_PAGE           ; check boundary
        jne     go_cmp
        mov     AL, STRT_PG
go_cmp:
        cmp     AL, BL
        jne     gotone
        jmp     end_rx                  ; buff ring empty
gotone:
; ring not empty
        mov     BH, AL
        xor     BL, BL                  ; BX has the rx_frm pointer
        push    BX                      ; save the frm ptr
        mov     AX, WDADD               ; shared mem base
        mov     ES, AX                  ; ES has the shr seg paragraph
        mov     AX, ES:[BX]             ; (word fetch reqd) AL has the status byte
        test    AL, SMK_PRX             ; if rx good
        jnz     readit
        jmp     fd_bnry                 ; rx error, drop frm by forward bnry
readit:
;
;  set up to read the next packet from the net
;
;
;  get ready for next packet
;
        cld                     ; moves in fwd dir
;
;  check for buffer overrun or catching up with reader
;
;  implicit 64K max buffer, should stop before 64K anyway
;
ifdef Microsoft
        MOV     AX,_BUFBIG      ; how much stuff is in buffer
        MOV     BX,_BUFLIM      ; what is our size limit?
else
        MOV     AX,BUFBIG       ; how much stuff is in buffer
        MOV     BX,BUFLIM       ; what is our size limit?
endif
        CMP     AX,BX
        JNA     ISROOM          ; we are ok
;
;  no room at the Inn.
;
        JMP     SHORT fd_bnry         ; can't do much, we lose packets until restarted

;
;  wrap pointer around at end, we know that we have room
;
ISROOM:
ifdef Microsoft
        MOV     DI,word ptr [_BUFPT]    ; where buffer is
        MOV     DX,word ptr [_BUFEND]   ; right before 2K safety area
else
        MOV     DI,word ptr [BUFPT]     ; where buffer is
        MOV     DX,word ptr [BUFEND]    ; right before 2K safety area
endif
        CMP     DX,DI                   ; see if pointer is over limit
        JA      OKAYREAD                ; we are not at wrap-around

ifdef Microsoft
        MOV     AX,word ptr [_BUFORG]   ; wrap to here
        MOV     word ptr [_BUFPT],AX    ; wrap-around
else
        MOV     AX,word ptr [BUFORG]    ; wrap to here
        MOV     word ptr [BUFPT],AX     ; wrap-around
endif
        MOV     DI,AX                   ; di also

OKAYREAD:
;
;
;  start the copy of the new packet
;  pointer to the shared memory offset is in SI
;  At this offset, you will find:
;    1 byte - read status, usually 21h
;    1 byte - pointer, page # of next packet
;    2 bytes - length of data in packet, swapped for you already
;    n bytes - that many bytes of Ethernet packet spread
;       over n div 256 pages (allowing four lost bytes in first packet)
;
;
        mov     bl,STOP_PAGE    ; get stop page for below before ds changes.
        pop     si              ; get packet pointer back into si
        push    si              ; restore for fd_bnry to read
;
;  save regs while moving packet to buffer
;  set up ds for buffer, even though we switch it later
;
        push    es
        push    ds
ifdef Microsoft
        MOV     AX,word ptr [_BUFPT+2]  ; buffer's ds
else
        MOV     AX,word ptr [BUFPT+2]   ; buffer's ds
endif
        mov     ds,ax
;
;  here, DS:DI contains where we want to put the packet.
;
newpkt:
        add     si,2            ; offset for length field
        mov     dx,es:[si]      ; value of length of recd packet

        mov     [di],dx         ; put the accumulated size there
        inc     si
        inc     si
        inc     di
        inc     di              ; now it is the data pointer
;
;
;  Actually move the data
;    DX has packet size in bytes
;    ES:SI has the source pointer  } need to switch
;    DS:DI has the dest pointer    } es and ds
;    Remember, 256 byte pages wrap around at STOP_PAGE (in bl) and there
;    are max 252 bytes in the first page
;
        mov     cx,dx
        cmp     cx,252
        jng     shrt
        mov     cx,252          ; first page len
shrt:
        push    es              ; swap es and ds
        push    ds
        pop     es
        pop     ds

        push    dx              ; save a copy of data length

mvpg:                           ; start of page move loop
        mov     al,0            ; al is flag 1=odd byte to move
        sub     dx,cx
        shr     cx,1            ; convert to words
        jnc     iseven
        mov     al,1            ; remember there's an odd 1 to move
iseven:
        rep     movsw           ; move all words in one page
;  If we have an extra byte to move, must be done via word fetch for
;  MCA.  Works for 8003 also...
        cmp     al,0            ; odd byte required
        je      mvmore
        mov     ax,[si]         ; get a word
        stosb                   ; store a byte
mvmore:
        cmp     dx,0            ; how many left to move?
        jng     donepg
        mov     cx,dx
        cmp     cx,256
        jng     shrtr
        mov     cx,256          ; one more page
shrtr:
        mov     ax,si           ; look at source page
        cmp     ah,bl           ; compare to stop page
        jl      mvpg
        mov     ah,STRT_PG      ; wrap around at this page boundary
        mov     si,ax           ; put back in si for rest of packet
        jmp     mvpg

donepg:
        pop     dx              ; get original length back in dx
        pop     ds
        pop     es              ; put regs back so ES is shared mem

;
; update the pointer and length in the buffer
;  DI already points just past end of data just placed there
;
ifdef Microsoft
        MOV     word ptr [_BUFPT],di    ; it is here, now
        MOV     AX,word ptr [_BUFBIG]   ; total amount of stuff in buffer
else
        MOV     word ptr [BUFPT],di     ; it is here, now
        MOV     AX,word ptr [BUFBIG]    ; total amount of stuff in buffer
endif
        ADD     AX,DX           ; add in size of this packet
        INC     AX
        INC     AX              ; to cover the length value
ifdef Microsoft
        MOV     word ptr [_BUFBIG],AX   ; after adding in current packet size
else
        MOV     word ptr [BUFBIG],AX    ; after adding in current packet size
endif
;
;
;  signs that something is actually happening
;
;       push    es
;       MOV     AX,0B000H       ; screen
;       MOV     ES,AX
;       MOV     DI,3998         ; lower right corner
;       INC     cs:ICNT
;       MOV     al,cs:ICNT      ; character
;       STOSB
;       pop     es
;


; drop bad frame by forwarding the BNRY register
; or just normal BNRY update after frame read
; Again, for 8003A must always do word fetches; coded to be OK for
; both 8003A and 8003
;
fd_bnry:                                ; drop frm by forward BNRY
        pop     BX                      ; restore frm ptr in BX
;       add     BX, 1
        mov     AX, ES:[BX]             ; next frm start page in AX
        xchg    AH, AL                  ; AL = next page ptr
        sub     AL, 1                   ; new BNRY in AL
        cmp     AL, STRT_PG             ; check boundary
        jge     wrbnry
        mov     AL, STOP_PAGE
        dec     AL
wrbnry:
        wr_wd8  BNRY

end_rx:
        pop     es
        POP     DI
        POP     SI
        POP     BP

        RET                     ; for compatibility with other drivers
ICNT    db      0
ifdef Microsoft
_WARECV ENDP
else
WARECV  ENDP
endif
;
;************************************************************************
;  XMIT
;     send a packet to Ethernet
;     Is not interrupt driven, just call it when you need it.
;
;  usage:   xmit(packet,count)
;               char *packet;
;               int count;
;
;   Takes a packet raw, Ethernet packets start with destination address,
;   and puts it out onto the wire.  Count is the length of packet < 2048
;
;   checks for packets under the Ethernet size limit of 60 and handles them
;
ifdef Microsoft
_WAXMIT PROC    FAR
else
WAXMIT  PROC    FAR
endif
        PUSH    BP
        MOV     BP,SP
        PUSH    SI
        PUSH    DI
        cld
        push    es
        PUSH    DS              ; set up proper ds for the buffer
;
        mov     dx,WDADD        ; shared memory address in dx
        mov     es,dx           ; use es for this
;
;
;  move packet into position, set up regs
;
        MOV     AX,[BP+X+2]
        MOV     DS,AX
        MOV     SI,[BP+X]       ; offset for buffer

        MOV     AX,[BP+X+4]     ; count of bytes
        MOV     CX,AX           ; save a copy, might be less than 60, ok

        CMP     AX,60           ; minimum length for Ether
        JNB     OKLEN
        MOV     AX,60           ; make sure size at least 60
OKLEN:
;
;  Copy packet into transmit buffer
;  xmit buffer starts at offset zero in shared mem
;  If odd number of bytes to write, must round up and write even number
;  for MCA version.
;
        push    ax              ; xmit size here, CX has data size

        mov     di,0            ; set di to start of xmit buffer, 0 page
        shr     cx,1
        jnc     evenx
        inc     cx              ; move one more word
evenx:
        rep     movsw           ; copy all data into xmit buf
;
;  set up xmit length registers
;
        pop     ax
        pop     ds                      ; get back  DS for wr_wd8 macro
; set up TBCR0,1
        wr_wd8  TBCR0                   ; lower byte to TBCR0
        mov     AL, AH
        wr_wd8  TBCR1                   ; higher byte to TBCR1

; set page number to page 0
        xor     al,al                   ; page number

; set up TPSR
        wr_wd8  TPSR                    ; write start page into TPSR

; issue tx command
        mov     AL, MSK_TXP + MSK_RD2
        wr_wd8  CMDR                    ; start xmit
;
;
;  check to see if the last packet xmitted ok
;
        xor     cx,cx
waitxmit:
        rd_wd8  CMDR            ; command register
        test    al,MSK_TXP      ; xmit bit
        jz      oktogo          ; xmit is finished
        loop    waitxmit        ; waiting for xmit to complete
        mov     ax,-1
        jmp     SHORT getout
oktogo:
        xor     ax,ax
;
; go back for more
;
getout:
        pop     es
        POP     DI
        POP     SI
        POP     BP
        RET
ifdef Microsoft
_WAXMIT ENDP
else
WAXMIT  ENDP
endif
;
;
;*************************************************************************
;  ETUPDATE
;      update pointers and/or restart receiver when read routine has
;      already removed the current packet
;
;   usage:  etupdate();
;
ifdef Microsoft
_WAETUPDATE     PROC    FAR
else
WAETUPDA        PROC    FAR
endif
        PUSH    ES
ifdef Microsoft
        MOV     AX,word ptr [_BUFPT+2]  ; establish data segment to buffer
else
        MOV     AX,word ptr [BUFPT+2]   ; establish data segment to buffer
endif
        MOV     ES,AX           ; put that in es
;
ifdef Microsoft
        MOV     BX,_BUFREAD     ; where read pointer is now
else
        MOV     BX,BUFREAD      ; where read pointer is now
endif
        MOV     DX,ES:[BX]      ; get size of this packet
        INC     DX
        INC     DX              ; two more for length value

        ADD     BX,DX           ; increment bufread by size of packet

ifdef Microsoft
        MOV     CX,_BUFEND      ; right before 2K safety area
else
        MOV     CX,BUFEND       ; right before 2K safety area
endif
        CMP     BX,CX           ; see if pointer is over limit
        JB      NOWRAPRD        ; we are not at wrap-around

ifdef Microsoft
        MOV     BX,_BUFORG      ; wrap to here
NOWRAPRD:
        MOV     _BUFREAD,BX     ; buffer pointer has been updated
else
        MOV     BX,BUFORG       ; wrap to here
NOWRAPRD:
        MOV     BUFREAD,BX      ; buffer pointer has been updated
endif

;
;  DECREMENT TOTAL BUFFER SIZE
;
        CLI                     ; keep interrupt handler from bothering dec
ifdef Microsoft
        MOV     CX,_BUFBIG      ; size before removing packet
else
        MOV     CX,BUFBIG       ; size before removing packet
endif
        SUB     CX,DX           ; remove size of current packet
ifdef Microsoft
        MOV     _BUFBIG,CX      ; put it back
else
        MOV     BUFBIG,CX       ; put it back
endif
        STI
        POP     ES
        RET
ifdef Microsoft
_WAETUPDATE     ENDP
else
WAETUPDA        ENDP
endif

ifdef Microsoft
NET8003A_TEXT   ends
else
        ENDPS
endif
        END
