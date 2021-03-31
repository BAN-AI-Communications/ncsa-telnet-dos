		  page	   55,132
          title     Driver routines for 3C505 Ethernet board
;
; Driver routines for 3C505 Ethernet board
;
; Bruce Orchard
; Waisman Center on Mental Retardation and Human Development
; University of Wisconsin-Madison
;
; April 7, 1988
;  2/9/89      Changed to make compatible with Telnet 2.2 - Warren Van Houten
;  3/17/89     Changed to make compatible with msc 5.0
;  7/14/89     Fixed getting ether address (far/near mismatch) - krus@diku.dk
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

ifdef Microsoft
x          equ     6              ; Offset to parameters (skip bp, ip, cs)
else
    	NAME	NET
  	INCLUDE	DOS.MAC
  	SETX
endif

;  3C505 control register bit definitions

EC_ATTENTION             equ     0200q     ; Attention
EC_FLUSH_DATA            equ     0100q     ; Flush data register
EC_DMA_ENABLE            equ     0040q     ; DMA enable
EC_TO_HOST               equ     0020q     ; Direction:  To host
EC_TERMINAL_COUNT_ENABLE equ     0010q     ; Terminal count interrupt enable
EC_COMMAND_ENABLE        equ     0004q     ; Command intterupt enable
EC_FLAG2                 equ     0002q     ; Host status flag 2
EC_FLAG1                 equ     0001q     ; Host status flag 1

;  3C505 status register bit definitions

ES_DATA_READY            equ     0200q     ; Data register ready
ES_HOST_COMMAND_EMPTY    equ     0100q     ; Host command register empty
ES_ADAPTER_COMMAND_FULL  equ     0040q     ; Adapter command register full
ES_TO_HOST               equ     0020q     ; Direction:  To host
ES_DMA_DONE              equ     0010q     ; DMA done
ES_FLAG3                 equ     0004q     ; Adapter status flag 3
ES_FLAG2                 equ     0002q     ; Adapter status flag 2
ES_FLAG1                 equ     0001q     ; Adapter status flag 1

;  3C505 aux DMA control register bit definitions

EA_BURST                 equ     0001q     ; Burst mode DMA

; 8259 equates

IOCWR1        equ     20h          ; Command register address 1
IMR1          equ     21h          ; Interrupt mask register address 1
VEC1          equ     8            ; First vector for 8259 1
IOCWR2        equ     0a0h         ; Command register address 2
IMR2          equ     0a1h         ; Interrupt mask register address 2
VEC2          equ     70h          ; First vector for 8259 2
EOI           equ     60h          ; End of interrupt command

; Time out values (1/18 second ticks)

SECOND          EQU     18          ; Ticks in 1 second
RESDEL          EQU     3           ; Delay before checking reset status
RESTO           EQU     15*SECOND   ; Time out for reset completion
CMDBTO          EQU     3           ; Time out for command byte to be accepted
CMDCTO          EQU     3           ; Time out for command to be accepted
RETRYDELAY      EQU     3           ; Command retry delay
RCMDTO          EQU     3           ; Incoming command time out
RESPTO          EQU     3           ; Response time out



; BIOS data area

bios_data      segment     at 40h
org 06Ch
timer_low        dw        ?         ; BIOS timer counter
timer_high       dw        ?
timer_ofl        dw        ?
bios_data       ends

ifdef Microsoft
DGROUP	group	_DATA
_DATA	segment	public 'DATA'

;	PUBLIC	STAT,BUFPT,BUFORG,BUFEND,BUFREAD,BUFBIG,BUFLIM,OFFS
;
;  The pointers below are actually DWORDs but we access them two
;  bytes at a time.
;
; STAT change to RSTAT because of name clash with MSC library routine
;   EXTRN   _RSTAT:BYTE       ; last status from read
    EXTRN   _BUFPT:WORD       ; current buffer pointer
    EXTRN   _BUFORG:WORD      ; pointer to beginning of buffer
    EXTRN   _BUFEND:WORD      ; pointer to end of buffer
    EXTRN   _BUFREAD:WORD     ; pointer to where program is reading
    EXTRN   _BUFBIG:WORD      ; integer, how many bytes we have
    EXTRN   _BUFLIM:WORD      ; integer, max bytes we can have

          public     _c5_droptot
          public     _c5_wrapct
          public     _c5_nocmd     
          public     _c5_cmdito

_c5_droptot    dw     0          ; total buffers dropped
_c5_wrapct     dw     0          ; buffer wraparounds
_c5_nocmd      dw     0          ; interrupt with command register empty
_c5_cmdito     dw     0          ; incoming command timeout
	assume	DS:seg _c5_droptot

else
	DSEG

;   EXTRN   RSTAT:BYTE	; last status from read
    EXTRN   BUFPT:WORD	; current buffer pointer
    EXTRN   BUFORG:WORD	; pointer to beginning of buffer
    EXTRN   BUFEND:WORD	; pointer to end of buffer
    EXTRN   BUFREAD:WORD	; pointer to where program is reading
    EXTRN   BUFBIG:WORD	; integer, how many bytes we have
    EXTRN   BUFLIM:WORD	; integer, max bytes we can have

          public     c5_droptot
          public     c5_wrapct
          public     c5_nocmd     
          public     c5_cmdito

c5_droptot    dw     0          ; total buffers dropped
c5_wrapct     dw     0          ; buffer wraparounds
c5_nocmd      dw     0          ; interrupt with command register empty
c5_cmdito     dw     0          ; incoming command timeout
	assume	DS:seg c5_droptot

endif

ksegbios     dw     ?          ; bios data segment
ksegdata     dw     ?          ; DATA segment - might be different
                               ; from seg c5_drop
kseghere     dw     ?          ; segment for the begining of this data
                               ; group.
irq          dw     ?          ; Interrupt request level
ioadr        dw     ?          ; IO address
dma          dw     ?          ; DMA request level

ecommand     dw     ?          ; 3C505 command address
estatus      dw     ?          ; 3C505 status address
edata        dw     ?          ; 3C505 data address
econtrol     dw     ?          ; 3C505 control address
eauxdma      dw     ?          ; 3C505 aux DMA control address

eoi1         dw     ?          ; End of interrupt command for 8259 1
eoi2         dw     ?          ; End of interrupt command for 8259 2

imr          dw     ?          ; Interrupt mask register address
vec          dw     ?          ; Vector number
oldioff      dw     ?          ; Original interrupt offset
oldiseg      dw     ?          ; Original interrupt segment

pcblen       dw     ?          ; PCB length
pcbad        dw     ?          ; PCB address

cmdlen       dw     ?          ; Incoming command length

rbufct       dw     ?          ; receive buffer counter
rdropnew     dw     ?          ; receive buffers just dropped
newstart     dw     ?          ; number of receives to start

savemask     db     ?          ; Original interrupt mask
maskbit      db     ?          ; Interrupt mask bit
lastcon      db     ?          ; Last control to board

CBSH         equ     50        ; half of incoming command buffer
CBS          equ     CBSH*2    ; incoming command buffer size

icmdb        db     CBS dup (?)    ; Incoming command buffer
icmd         db     CBSH dup (?)   ; incoming command

fconc        db     0          ; Flag:  Configure 82586
fgeth        db     0          ; Flag:  Get Ethernet address
fseth        db     0          ; Flag:  Set Ethernet address
fxmit        db     0          ; Flag:  Transmit packet
fadin        db     0          ; Flag:  Adapter info
fstat        db     0          ; Flag:  Statistics

          even

cconc          db     02h        ; Command:  Configure 82586
               db     2          ; -- 2 more bytes
               dw     1          ; -- receive broadcasts

rconc          db  2 dup (?)     ; Response:  Configure 82586
rconc_st       dw     ?          ; -- status


cgeth          db     03h        ; Command:  Get Ethernet address
               db     00         ; 0 more bytes

rgeth          db  2 dup (?)     ; Response:  Get Ethernet address
rgeth_ad       db  6 dup (?)     ; -- address


cseth          db     10h        ; Command:  Set Ethernet address
               db     06         ; 6 more bytes
cseth_ad       db  6 dup (?)     ; -- address
rseth          db  2 dup (?)     ; Response:  Set Ethernet address
rseth_status   dw     ?          ; -- status

cxmit          db     09h        ; Command:  Transmit packet
               db     06         ; 6 more bytes
cx_offset      dw     ?          ; -- buffer offset
cx_segment     dw     ?          ; -- buffer segment
cx_length      dw     ?          ; -- buffer length

rxmit          db  2 dup (?)     ; Response:  Transmit packet
rx_offset      dw     ?          ; -- buffer offset
rx_segment     dw     ?          ; -- buffer segment
rx_status      dw     ?          ; -- completion status
rx_cstatus     dw     ?          ; -- 82586 status

cr             db     08h        ; Command:  Receive
               db     08         ; 8 more bytes
cr_offset      dw     ?          ; -- buffer offset
cr_segment     dw     ?          ; -- buffer segment
cr_length      dw     ?          ; -- buffer length
cr_timeout     dw     ?          ; -- timeout

rr             db  2 dup (?)     ; Response:  Receive
rr_offset      dw     ?          ; -- buffer offset
rr_segment     dw     ?          ; -- buffer segment
rr_dmalen      dw     ?          ; -- bytes to dma
rr_length      dw     ?          ; -- actual length
rr_status      dw     ?          ; -- completion status
rr_rstatus     dw     ?          ; -- 82586 receive status
rr_time        dd     ?          ; -- time tag

cadin          db     11h        ; Command:  Adapter info
               db     0          ; 0 more bytes
radin          db  2 dup (?)     ; Response:  Adapter info

ra_rom         dw     ?          ; -- ROM version
ra_cs          dw     ?          ; -- ROM checksum
ra_mem         dw     ?          ; -- RAM memory size
ra_freeoff     dw     ?          ; -- Free memory offset
ra_freeseg     dw     ?          ; -- Free memory segment

cstat          db     0ah        ; Command:  Network statistics
               db     0          ; 0 more bytes

rstat          db  2 dup (?)     ; Response:  Network statistics
rs_rec         dd     ?          ; -- Packets received
rs_tran        dd     ?          ; -- Packets sent
rs_crc         dw     ?          ; -- CRC error counter
rs_align       dw     ?          ; -- Alignment error counter
rs_nors        dw     ?          ; -- No resources error counter
rs_or          dw     ?          ; -- Overrun error counter

TURNOFF        db     08h
TURNON         db     0F7h
ifdef Microsoft
_DATA	ends
else
	ENDDS
endif
;
;
;  Macros for in and out
;
MOUT     MACRO     REG,STUFF       ; one byte to the given I/O register
     MOV  DX, REG
     MOV  AL, STUFF
     OUT  DX, AL
     ENDM
;
MOUTW     MACRO     REG, LO, HI     ; two bytes to the I/O double port
     MOV  DX, REG
     MOV  AL, LO
     OUT  DX, AL
     INC  DX
     MOV  AL, HI
     OUT  DX, AL
     ENDM
;
MIN MACRO  REG              ; get one byte to al
    MOV DX, REG
    IN  AL, DX
    ENDM


;
;
;
;   The subroutines to call from C
;
ifdef Microsoft
NET505_TEXT  segment  public  'CODE'
    
    assume CS:NET505_TEXT, ES:bios_data
    
	PUBLIC	_E5RECV, _E5ETOPEN, _E5ETCLOSE, _E5GETADDR
ifdef LOCAL_DEF
    PUBLIC  _E5SETADDR
endif
    PUBLIC  _E5XMIT, _E5ETUPDATE, _E5ETDMA
    public  _c5_get_adapter_info, _c5_get_statistics
else
    PSEG
	PUBLIC	E5RECV, E5ETOPEN, E5ETCLOS, E5GETADD
ifdef LOCAL_DEF
    PUBLIC  E5SETADD
endif
    PUBLIC  E5XMIT, E5ETUPDA, E5ETDMA
    public  c5getada, c5getsta
endif



		  subttl	 _E5etopen:  Initialize board
          page     +
;******************************************************************
;  ETOPEN
;     Initialize the Ethernet board, set receive type.
;
;  usage:  etopen(s,irq,addr,ioaddr)
;           char s[6];       ethernet address
;           int irq,addr,ioaddr;     
;                interrupt number, base mem address (unused) and
;                i/o address to use
;
;  _c5_init
;    Initialize the board, etc.
;
;  Arguments:
a_ethadr   equ     x             ; ethernet address
a_irq      equ     a_ethadr+4    ; Interrupt request level (int)
a_seg      equ     a_irq+2       ; Shared segment address (int)
a_ioadr    equ     a_seg+2       ; IO address (int)
;
ifdef Microsoft
_E5etopen  proc   far
else
E5ETOPEN	PROC	FAR
endif
          push     bp          ; save bp
          mov     bp,sp          ; bp -> return, parameters
          push     ds          ; save ds
          push     es          ; save es
          push     si          ; save si
          push     di          ; save di
ifdef Microsoft
          mov     ax, seg _c5_droptot     ; ax -> data segment
else
          mov     ax, seg c5_droptot      ; ax -> data segment
endif
          mov     ds, ax                  ; ds -> data segment
          mov     kseghere, ax
          
          mov     ax, seg _DATA
          mov     ksegdata, ax
          
          mov     ax, seg bios_data
          mov     ksegbios, ax
          
          mov     es, ksegbios            ; es -> bios data segment

          mov     ax, [bp+a_irq]          ; interrupt level -> ax
          mov     irq, ax                 ; save interrupt level

          mov     ax, [bp+a_ioadr]        ; IO address -> ax
          mov     ioadr, ax               ; save IO address
          mov     ax, ioadr               ; 3C505 IO address -> ax
          mov     ecommand, ax            ; save command address

          add     ax, 2                   ; status address -> ax
          mov     estatus, ax             ; save status address

          mov     eauxdma, ax             ; save aux dma address
          
          add     ax, 2                   ; data address -> ax
          mov     edata, ax               ; save data address

          add     ax, 2                   ; control address -> ax
          mov     econtrol, ax            ; save control address

          cli                             ; disable interrupts

; Set up the 8259 interrupt controller chip to what the 3c505 board is 
; set at.

          
          mov     ax, irq                 ; interrupt level -> ax
          cmp     ax, 8                   ; which 8259?
          jge     o_1                     ; 8259 2
          
          mov     bx, ax                  ; irq -> bx
          or      ax, EOI                 ; 8259 1: make first EOI command
          mov     eoi1, ax                ; save first EOI command
          mov     eoi2, 0                 ; no second EOI command
          mov     imr, IMR1               ; mask is in IMR1
          add     bx, VEC1                ; interrupt vector number -> bx
          mov     vec, bx                 ; save vector number
          jmp     SHORT o_2                     ; skip 8259 2 case

       ; 8259 2: just keep low 8 bits of interrupt number
o_1:
          and     ax, 07Q
          mov     bx, ax                  ; interrupt on 8259 -> bx
          or      ax, EOI                 ; put in EOI command
          mov     eoi2, ax                ; save second EOI command
          mov     eoi1, EOI+2             ; first EOI releases second
          mov     imr, IMR2               ; mask is in IMR2
          add     bx, VEC2                ; interrupt vector number -> bx
          mov     vec, bx                 ; save vector number
          mov     dx, IOCWR2              ; dx -> command register 2
          out     dx, al                  ; do EOI 2 just in case

o_2:
          mov     ax, eoi1                ; EOI 1 command -> ax
          mov     dx, IOCWR1              ; dx -> command register 1
          out     dx, al                  ; do EOI 1 just in case
          mov     ax, vec                 ; vector number -> ax

; Install the interrupt handler.

          call    IINST
          
; Save the old interrupt mask of the 8259 chip and then turn it on.

          mov     cx, irq                 ; interrupt level -> cx
          and     cx, 07q                 ; just keep level on 8259
          mov     ax, 1                   ; 1 -> ax
          shl     ax, cl                  ; make interrupt mask
          mov     maskbit, al             ; save mask bit
          mov     dx, imr                 ; mask register address -> dx
          in      al, dx                  ; get old mask
          mov     savemask, al            ; save mask
          mov     bl, maskbit             ; our interrupt bit -> bl
          not     bl                      ; want to unmask it
          and     al, bl                  ; combine with other interrupts
          out     dx, al                  ; unmask our interrupt
          sti                            ; turn interrupts on
          

;  Reset the 3c505 board - this takes about 15-20 seconds.

          mov     al, EC_ATTENTION OR EC_FLUSH_DATA; Master reset command -> al

          mov     dx, econtrol            ; dx -> control register
          out     dx, al                  ; do reset
          mov     ax, timer_low           ; current timer -> ax
          add     ax, RESDEL              ; + time to wait

wlp1:
          cmp     ax, timer_low           ; compare to current time
          ja      wlp1                    ; wait for reset to propagate

          mov     al, EC_COMMAND_ENABLE   ; command interrupt enable -> al
          mov     lastcon, al             ; save last command
          out     dx, al                  ; release reset
          mov     ax, timer_low           ; current timer -> ax
          add     ax, RESDEL              ; + time to wait

wlp2:
          cmp     ax, timer_low           ; compare to current time
          ja      wlp2                    ; wait for CPU to start reset
          mov     bx, timer_low           ; current timer -> ax
          add     bx, RESTO               ; + time out

wlp3:
          call     getstat                     ; get status

          and     ax, ES_FLAG1 OR ES_FLAG2     ; just keep flags
          cmp     ax, ES_FLAG1 OR ES_FLAG2     ; both on?
          jne     resdone                      ; no:  reset completed

          cmp     bx, timer_low                ; have we waited too long?
          ja      wlp3                         ; no
          jmp     SHORT openfail                     ; yes:  open failed

resdone:  

; Set up the receive buffers.

          mov     rbufct, 0                ; clear buffer counter
          mov     cr_length, 1600          ; buffer length:  1600

irb1:
          mov     ax, rbufct               ; buffer counter -> ax
          mov     cr_offset, ax            ; use buffer number for offset
          inc     ax                       ; count buffer
          mov     rbufct, ax               ; store buffer number
          mov     ax, 10                   ; pcb length -> ax
          mov     si, offset cr            ; si -> request

          call    outpcb                   ; pass pcb

          mov     ax, rbufct               ; buffer counter -> ax
          cmp     ax, 10                   ; start 10 receives
          jl     irb1                      ; loop if more buffers
          
          ; We use the same character string pointer for both of the
          ; next two calls, so we don't adjust the stack pointer
          ; until we're done.
          
          mov    ax, [bp+a_ethadr+2]
          push   ax
          mov    ax, [bp+a_ethadr]
          push   ax

; Get the hardware ethernet address.
          
          call   far ptr get_eth_addr
          or     ax, ax
          jz     callset
          add    sp,4
          jmp    SHORT openfail
callset:          

;  Set the 3c505 board to use that address

		  call	 far ptr _E5setaddr
          add    sp, 4
          or     ax, ax
          jnz    openfail

; Tell the 3c505 board to start receiving packets.

          
		  CALL	  E5OPEN
;	 ax = 0 E5open OK, ax = -1 then E5open failed
          jmp     SHORT openx                    ; go return

openfail:
          mov     ax, -1                   ; -1 -> ax, fail

openx:
          pop     di                      ; restore di
          pop     si                      ; restore si
          pop     es                      ; restore es
          pop     ds                      ; restore ds
          pop     bp                      ; restore bp
          ret
ifdef Microsoft
_E5etopen	  endp
else
E5ETOPEN	ENDP
endif



		  subttl	 _E5etDMA:	DMA request level.
          page     +

;******************************************************************
;  ETDMA
;     Initialize the DMA request level.
;
;  not needed at this time
;
a_dma      equ     a_ioadr+2     ; DMA request level
;
ifdef Microsoft
_E5etdma  proc	 far
else
E5ETDMA  PROC  FAR
endif

          push     bp          ; save bp
          mov     bp,sp          ; bp -> return, parameters

          push     ds                     ; save ds
ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
          mov     ds, ax
          
          mov     ax, seg bios_data
          mov     ksegbios, ax

          mov     ax, [bp+a_dma]          ; DMA level -> ax
          mov     dma, ax                 ; save IO address

          xor     ax, ax

          pop     ds                      ; restore ds

          pop     bp                      ; restore bp
          ret
ifdef Microsoft
_E5etdma	 endp
else
E5ETDMA   ENDP
endif




          subttl     open:  Open
          page     +
;  This routine turns tells the 3c505 board to start receiving packets.

E5open	   proc 	near

          push     bp                     ; save bp
          mov     bp, sp                  ; bp -> return, parameters
          push     ds                     ; save ds
          push     es                     ; save es
          push     si                     ; save si
          push     di                     ; save di
                                          
ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
          mov     ds, ax                  ; ds -> data segment
          mov     es, ksegbios            ; es -> bios data segment
          mov     si, offset cconc        ; si -> configure 82586 request
          mov     ax, 4                   ; request length -> ax
          mov     fconc, 0                ; clear response received flag
          call    outpcb                  ; send the pcb

          mov     ax, timer_low           ; current time -> ax
          add     ax, RESTO               ; + wait time
op_1:
          test    fconc, 0ffh             ; answered yet?
          jnz     op_2                    ; yes

          cmp     ax, timer_low           ; expired?
          ja      op_1                    ; no

          mov     ax, -1                  ; return fail
          jmp     SHORT op_x                    ; go return
op_2:
          mov     ax, 0                   ; 0 -> ax, success

op_x:
          pop     di                      ; restore di
          pop     si                      ; restore si
          pop     es                      ; restore es
          pop     ds                      ; restore ds
          pop     bp                      ; restore bp
          ret               ; Just return
E5open		  endp


		  subttl	 _E5etclose:  Close board
          page     +

;***********************************************************************
;  ETCLOSE
;        shut it down, remove the interrupt handler
;
;  usage:  etclose();
;
;
ifdef Microsoft
_E5ETCLOSE	   PROC 	FAR
else
E5ETCLOS	 PROC	  FAR
endif
     CLI
;
;
;  mask out IRQ on interrupt controller
;
     push ds
ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
     mov  ds, ax
     MIN  imr                  ; get current mask
     OR   AL, TURNOFF          ; force that bit on
     OUT  DX, AL               ; send it back to controller
     STI

     CALL  DEINST              ; restore old interrupt handler

     MOV   BL, savemask        ; get back saved setting of irq
     NOT   BL                  ; flip it
     
     CLI
     MIN   imr
     AND   AL, BL              ; restore setting of that bit
     OUT   DX, AL
     STI     
     
     xor   ax, ax
     pop   ds
     RET
ifdef Microsoft
_E5ETCLOSE	   ENDP
else
E5ETCLOS	 ENDP
endif


          subttl     _c5_getaddr:  Get Ethernet address
          page     +

;*******************************************************************
;  GETADDR
;     get the Ethernet address off of the board (This gets called 
;		before E5etopen)
;
;   usage:  getaddr(s,address,ioaddr);
;	char s[6];           will get six bytes from the PROM
;       int address;    (unused here)
;       int ioaddr;     mem address and ioaddress to use
;
; _E5getaddr
;    Get Ethernet address
;
; Arguments:

a_eadr          equ     x          ; Ethernet address (far char *)
ag_ioadr        equ     a_eadr + 6
ifdef Microsoft
_E5getaddr	   proc 	far
else
E5GETADD	PROC	FAR
endif

ret

ifdef Microsoft
_E5getaddr	   endp
else
E5GETADD	ENDP
endif


get_eth_addr   proc far
;
; This was the old getaddr routine.  But, to advoid changing the Telnet
; source code i moved it here.  The interrupt handler has to be installed
; before this routine is used.
;
          push     bp
          mov     bp, sp                  ; bp -> return, parameters
          push     ds
          push     es
          push     si
          push     di

ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
          mov     ds, ax                  ; ds -> data segment
          mov     es, ksegbios            ; es -> bios data segment

          mov     si, offset cgeth        ; si -> request ethernet address
          mov     ax, 2                   ; request length -> ax
          mov     fgeth, 0                ; clear response received flag

          call    outpcb                  ; send the pcb

          mov     ax, timer_low           ; current time -> ax
          add     ax, RESTO               ; + wait time
ga_1:
          test    fgeth, 0ffh             ; answered yet?
          jnz     ga_2                    ; yes

          cmp     ax, timer_low           ; expired?
          ja      ga_1                    ; no;

          mov     ax, -1                  ; return fail
          jmp     SHORT ga_x                    ; go return

ga_2:
          cld
          mov     di, [bp+a_eadr]         ; di -> destination offset
          push    es                      ; save es
          mov     es, [bp+a_eadr+2]       ; es -> destination segment
          mov     si, offset rgeth_ad     ; si -> response
          mov     cx, 6                   ; address length -> cx

          rep movsb                       ; return address

          pop     es
          mov     ax, 0                   ; 0 -> ax, success
ga_x:
          pop     di
          pop     si
          pop     es
          pop     ds
          pop     bp
          ret

get_eth_addr   endp
          subttl     _c5_setaddr:  Set Ethernet address
          page     +

;******************************************************************
;  SETADDR
;    set the Ethernet address on the board to 6 byte ID code
;
;   usage:   setaddr(s,basea,ioa);
;             char s[6];           ethernet address to use
;             int basea;           shared memory base address (unused)
;             int ioa;             io address for board (unused)
;
; _c5_setaddr
; _E5setaddr
;    Set Ethernet address
;
; Arguments:

a_eadr          equ     x          ; Ethernet address (far char *)

ifdef Microsoft
_E5setaddr	   proc 	far
else
E5SETADD	PROC	FAR
endif

          push     bp
          mov     bp, sp                  ; bp -> return, parameters
          push     ds
          push     es
          push     si
          push     di

ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
          mov     ds, ax                  ; ds -> data segment
          mov     es, ksegbios            ; es -> bios data segment
          push    es

          mov     di, offset cseth_ad     ; si -> command
          mov     ax, seg cseth_ad        ; ax -> command segment
          mov     es, ax                  ; es -> command segment
          push    ds

          mov     si, [bp+a_eadr]         ; di -> destination offset
          mov     ds, [bp+a_eadr+2]       ; es -> destination segment
          mov     cx, 6                   ; address length -> cx

          rep movsb                       ; return address

          pop     ds
          pop     es

          mov     si, offset cseth        ; si -> request ethernet address
          mov     ax, 8                   ; request length -> ax
          mov     fseth, 0                ; clear response received flag

          call    outpcb                  ; send the pcb

          mov     ax, timer_low           ; current time -> ax
          add     ax, RESTO               ; + wait time
sa_1:
          test     fgeth, 0ffh            ; answered yet?
          jnz     sa_2                    ; yes
          
          cmp     ax, timer_low           ; expired?
          ja      sa_1                    ; no
          
          mov     ax, -1                  ; return fail
          jmp     SHORT sa_x                    ; go return
sa_2:
          mov     ax,0                    ; 0 -> ax, success
sa_x:
          pop     di
          pop     si
          pop     es
          pop     ds
          pop     bp
          ret
ifdef Microsoft
_E5setaddr	   endp
else
E5SETADD	ENDP
endif

          subttl     _c5_get_adapter_info:  Get adapter information
          page     +
          
; I don't have the information to use this routine
;
; _c5_get_adapter_info
;    Get adapter information
;
; Arguments:

a_adin    equ     x        ; Adapter information (far struct r_adapter_info *)

_c5_get_adapter_info     proc     far

          push     bp                     ; save bp
          mov     bp, sp                  ; bp -> return, parameters
          push     ds                     ; save ds
          push     es                     ; save es
          push     si                     ; save si
          push     di                     ; save di
          
ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
          mov     ds, ax                  ; ds -> data segment
          mov     es, ksegbios            ; es -> bios data segment
          mov     si, offset cadin        ; si -> request adapter information
          mov     ax, 2                   ; request length -> ax
          mov     fadin, 0                ; clear response received flag

          call    outpcb                  ; send the pcb

          mov     ax, timer_low           ; current time -> ax
          add     ax, RESTO               ; + wait time
ai_1:     
          test     fadin, 0ffh            ; answered yet?
          jnz     ai_2                    ; yes

          cmp     ax, timer_low           ; expired?
          ja      ai_1                    ; no

          mov     ax, -1                  ; return fail
          jmp     SHORT ai_x                    ; go return
ai_2:     
          mov     di, [bp+a_adin]         ; di -> destination offset
          push     es                     ; save es
          mov     es, [bp+a_adin+2]       ; es -> destination segment
          mov     si, offset radin        ; si -> response
          mov     cx, 12                  ; address length -> cx

          rep movsb                       ; return address

          pop     es                      ; restore es
          mov     ax, 0                   ; 0 -> ax, success
ai_x:     
          pop     di                      ; restore di
          pop     si                      ; restore si
          pop     es                      ; restore es
          pop     ds                      ; restore ds
          pop     bp                      ; restore bp
          ret                           ; return
_c5_get_adapter_info     endp


          subttl     _c5_get_statistics:  Get statistics
          page     +

; I don't have the information to use this routine
;
; _c5_get_statistics
;    Get network statistics
;
; Arguments:

a_stat          equ     x          ; Statistics (far struct r_statistics *)

_c5_get_statistics     proc     far

          push     bp                     ; save bp
          mov     bp, sp                  ; bp -> return, parameters
          push     ds                     ; save ds
          push     es                     ; save es
          push     si                     ; save si
          push     di                     ; save di
          
ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
          mov     ds, ax                  ; ds -> data segment
          mov     es, ksegbios            ; es -> bios data segment
          mov     si, offset cstat        ; si -> request statistics
          mov     ax, 2                   ; request length -> ax
          mov     fstat, 0                ; clear response received flag
          
          call    outpcb                  ; send the pcb
          
          mov     ax, timer_low           ; current time -> ax
          add     ax, RESTO               ; + wait time
st_1:
          test     fstat, 0ffh            ; answered yet?
          jnz     st_2                    ; yes
          
          cmp     ax, timer_low           ; expired?
          ja      st_1                    ; no
          
          mov     ax, -1                  ; return fail
          jmp     SHORT st_x                    ; go return
st_2:
          mov     di, [bp+a_stat]         ; di -> destination offset
          push     es                     ; save es
          mov     es, [bp+a_stat+2]       ; es -> destination segment
          mov     si, offset rstat        ; si -> response
          mov     cx, 18                  ; statistics length -> cx

          rep movsb                       ; return address

          pop     es                      ; restore es
          mov     ax, 0                   ; 0 -> ax, success
st_x:
          pop     di                      ; restore di
          pop     si                      ; restore si
          pop     es                      ; restore es
          pop     ds                      ; restore ds
          pop     bp                      ; restore bp
          ret                           ; return
_c5_get_statistics     endp


		  subttl	 _E5recv:  Receive message
          page     +

;************************************************************************
;   Receive
;   This is a CPU hook for boards that must be polled before we can
;   deliver packets into the receive buffer.  (i.e. no interrupts used)
;
;   The 3COM 3C505 version uses interrupts, so this routine is a NOP
;   for this board.
;
;    usage:  recv();
;
ifdef Microsoft
_E5RECV PROC	FAR
else
E5RECV	PROC	FAR
endif

	RET			; for compatibility with other drivers

ifdef Microsoft
_E5RECV ENDP
else
E5RECV	ENDP
endif




          subttl     _c5_xmit:  Transmit message
          page     +

;************************************************************************
;  XMIT         
;     send a packet to Ethernet
;     Is not interrupt driven, just call it when you need it.
;
;  usage:   xmit(packet,count)
;		char *packet;
;		int count;
;
;   Takes a packet raw, Ethernet packets start with destination address,
;   and puts it out onto the wire.  Count is the length of packet < 2048
;
;   checks for packets under the Ethernet size limit of 60 and handles them
;
; _c5_xmit
; _E5xmit
;    Transmit message
;
; Arguments:

a_xaddr          equ     x          ; Pointer to buffer (far char *--must beeven)
a_xlength     equ     a_xaddr+4     ; Length in bytes (int)

ifdef Microsoft
_E5xmit 	proc	 far
else
E5XMIT	PROC	FAR
endif

          push     bp
          mov     bp, sp                  ; bp -> return, parameters
          push     ds
          push     es
          push     si
          push     di

ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
          mov     ds, ax                  ; ds -> data segment
          mov     es, ksegbios            ; es -> bios data segment
          mov     ax, [bp+a_xaddr]        ; ax -> buffer offset
          mov     cx_offset, ax           ; put in request
          mov     ax, [bp+a_xaddr+2]      ; ax -> buffer segment
          mov     cx_segment, ax          ; put in request

          mov     ax, [bp+a_xlength]      ; message length -> ax
          cmp     ax, 60                  ; is buffer too short?
          jg      xm_4                    ; no

          mov     ax, 60                  ; yes:  pad with garbage
xm_4:
          inc     ax                      ; round up
          sar     ax, 1                   ; divide by 2
          shl     ax, 1                   ; multiply by 2
          mov     cx_length, ax           ; put in request
          mov     fxmit, 0                ; clear transmit done flag
          mov     si, offset cxmit        ; si -> request
          mov     ax, 8                   ; request length -> ax

          call    outpcb                  ; send command

          mov     bx, estatus             ; bx -> status register
          mov     dx, edata               ; dx -> data register
          mov     cx, cx_length           ; length -> cx
          sar     cx, 1                   ; convert to words
          mov     si, cx_offset           ; offset -> si
          push     ds
          mov     ds, cx_segment          ; segment -> ds
xm_1:
          lodsw                           ; next word -> ax
          out     dx, ax                  ; output it
          xchg    dx, bx                  ; dx -> status register
xm_2:
          in      al, dx                  ; get status
          test    al, ES_DATA_READY       ; ready for next word?
          jz      xm_2                    ; no
          
          xchg    dx, bx                  ; dx -> data register
          dec     cx                      ; count word
          jnz     xm_1                    ; loop through buffer
          
          pop     ds
xm_3:
          test    fxmit, 0ffh             ; has transmit completed?
          jz      xm_3                    ; no
          
          mov     ax, rx_status           ; return status
xm_x:
          pop     di
          pop     si
          pop     es
          pop     ds
          pop     bp
          ret
ifdef Microsoft
_E5xmit 	endp
else
E5XMIT	ENDP
endif


          subttl     _c5_update:  Update receive buffer pointer
          page     +


;*************************************************************************
;  ETUPDATE
;      update pointers and/or restart receiver when read routine has
;      already removed the current packet
;
;   usage:  etupdate();
;
; _c5_update
; _E5etupdate
;    Update receive buffer pointer
;
;************  needs much more work to use with Lattice C
;
ifdef Microsoft
_E5etupdate 	proc	 far
else
E5ETUPDA	PROC	FAR
endif
          
          push     bp
          mov     bp, sp                  ; bp -> return, parameters
          push     ds
          push     es
          push     si
          push     di
          push     cx
          
ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot         ; ax -> data segment
endif
          mov     ds, ax
          mov     es, ksegbios               ; es -> bios data segment
          push    es                         ; save es
          
          mov     ds, ksegdata          
          les     di, dword ptr _bufread     ; es/di -> start of message
          mov     ax, es:[di]                ; message length -> ax
          pop     es
          add     di, ax                     ; advance by message length
          add     di, 2                      ; + 2 for message length
          cmp     di, _bufend                ; passed end?
          jb      up_1                       ; no
          
          mov     di, _buforg             ; yes:  start over
          inc     _c5_wrapct              ; count wraparound
up_1:     
          mov     _bufread, di         ; store pointer
          
          cli                             ; protect bufbig
          mov     bx, _bufbig          ; amount of buffer in use -> bx
          sub     bx, ax                  ; - size just released
          sub     bx, 2                   ; - 2 for message size
          mov     _bufbig, bx          ; store size left
          sti                             ; release bufbig
          
          cli                             ; protect drop count
          mov     ax, rdropnew            ; messages dropped recently -> ax
          mov     rdropnew, 0             ; clear drop count
          sti                             ; release interrupts
          
          inc     ax                      ; + 1 for buffer released
          mov     ax, newstart            ; = number to start
up_2:
          mov     ax, rbufct              ; buffer counter -> ax
          mov     cr_offset, ax           ; use buffer number for offset
          inc     ax                      ; count buffer
          mov     rbufct, ax              ; store buffer number
          mov     ax, 10                  ; pcb length -> ax
          mov     si, offset cr           ; si -> request
          
          call     outpcb                 ; pass pcb
          
          dec     newstart                ; count receive started
          jg     up_2                     ; loop if more buffers
up_x:
          pop     cx
          pop     di
          pop     si
          pop     es
          pop     ds
          pop     bp
          ret
ifdef Microsoft
_E5etupdate 	endp
else
E5ETUPDA	ENDP
endif


          subttl     getstat:  Get board status
          page     +
; Get board status, waiting for it to become stable
;
; Return:
;   al = status

getstat          proc     near

          push     bx
          push     dx
          
          mov     dx, estatus             ; dx -> status register
gs_1:
          in     al, dx                   ; status -> al
          mov     bl, al                  ; status -> bl
          in      al, dx                  ; status -> al
          cmp     al, bl                  ; same both times?
          jne     gs_1                    ; No:  try again
          
          pop     dx
          pop     bx
          ret
getstat          endp



          subttl     outpcb:  Send PCB to board
          page     +
; Send pcb to board, retry until accepted
;
; Entry:
;   ax = number of bytes in pcb
;   si = address of pcb

outpcb          proc     near

          mov     pcblen, ax                   ; save pcb length
          mov     pcbad, si                    ; save pcb address

ob_1:
          mov     cx, pcblen                   ; length -> cx
          mov     si, pcbad                    ; address -> si
          
          cli                                      ; Protect last command
          mov     al, lastcon                      ; last command -> ax
          and     al, NOT (EC_FLAG1 OR EC_FLAG2)   ; clear flags
          mov     lastcon, al                      ; save lastcom
          sti                                      ; enable interrupts
          
          mov     dx, econtrol                 ; dx -> control register
          out     dx, al                       ; send control
          mov     dx, ecommand                 ; dx -> command register
          
ob_2:
          mov     al, [si]                     ; next command byte -> al
          out     dx, al                       ; send command byte
          mov     bx, timer_low                ; current timer -> ax
          add     bx, CMDBTO                   ; + time out
          
wlp4:
          call     getstat                     ; get status
          
          and     al, ES_HOST_COMMAND_EMPTY    ; has command been taken?
          jne     ob_3                         ; yes:  go on
          
          cmp     bx, timer_low                ; have we waited too long?
          ja     wlp4                          ; no
          
          jmp     SHORT cmdretry                     ; go retry command
          
          
ob_3:
          inc     si                           ; increment source pointer
          dec     cx                           ; count byte
          jg     ob_2                          ; loop if more bytes
          
          mov     dx, econtrol                 ; dx -> control register
          
          cli                                  ; disable interrupts
          mov     al, lastcon                  ; last control -> al
          or     al, (EC_FLAG1 OR EC_FLAG2)    ; set end of command
          mov     lastcon, al                  ; save lastcon
          out     dx, al                       ; send flag bits
          
          mov     dx, ecommand                 ; dx -> command register
          mov     ax, pcblen                   ; pcb length -> ax
          out     dx, al                       ; send pcb length
          sti                                  ; enable interrupts
          
          
          mov     bx, timer_low                ; current time -> bx
          add     bx, CMDCTO            ; + time out for command to be accepted
wlp5:
          call     getstat                     ; get status
          
          and     al, (ES_FLAG1 OR ES_FLAG2)   ; just keep status flags
          cmp     al, 1                        ; accepted?
          je     cmdaccept                     ; yes
          
          cmp     al, 2                        ; rejected?
          je     cmdretry                      ; yes
          
          cmp     bx, timer_low                ; have we waited too long?
          ja     wlp5                          ; no
          
cmdretry:
          mov     ax, timer_low                ; current time -> ax
          add     ax, RETRYDELAY               ; + retry delay
wlp6:
          cmp     ax, timer_low                ; have we waited long enough?
          ja     wlp6                          ; no
          
          jmp     ob_1                         ; go do retry
          
cmdaccept:
          cli                                    ; protect last control
          mov     al, lastcon                    ; last control -> al
          and     al, NOT (EC_FLAG1 OR EC_FLAG2) ; turn off end of command flag
          mov     lastcon, al                    ; save last control
          sti                                    ; reenable interrupts
          
          mov     dx, econtrol                 ; dx -> control register
          out     dx, al                       ; pass control byte
          mov     ax, 0                        ; return 0, success
          ret
outpcb          endp


          subttl     Interrupt routine
          page     +

;*************************************************************************
;  Interrupt Handler
;  installation and deinstallation
;
;     the handler takes the receive packet out of the input buffer
;
DEINST     PROC     NEAR
     push ds
     
     MOV  ax, vec                  ; interrupt in table for 3com board
     MOV  dx, oldioff              ; get old ip from save spot
     MOV  ds, oldiseg              ; get old cs from save spot
     mov  ah, 25h
     int  21h

     POP  DS
     RET
DEINST     ENDP


;
IINST     PROC     NEAR

     MOV  CS:MYDS, DS              ; store for use by handler

     PUSH DS
     push es
     push bx
     
     mov  ax, vec                  ; get the current interupt vector
     mov  ah, 35h                  ; that we are using.
     int  21h
     
     mov  oldioff, bx              ; save the vector for later
     mov  oldiseg, es
     
     mov  dx, offset IHAND         ; set our interupt vector
     mov  ax, vec
     MOV  BX, CS
     MOV  DS, BX
     mov  ah, 25h
     int  21h
     
     pop  bx
     pop  es
     POP  DS

     RET
     
MYDS  DW 00H                     ; the data segment for this assembly code
ICNT  DB 00H

IHAND:                           ; not a public name, only handles ints

          push     ds
          push     es
          push     si
          push     di
          push     bp
          push     ax
          push     bx
          push     cx
          push     dx
          
          sti                             ; let other interrupts come in
          cld                             ; increment

ifdef Microsoft
          mov     ax, seg _c5_droptot           ; ax -> data segment
else
          mov     ax, seg c5_droptot           ; ax -> data segment
endif
          mov     ds, ax                  ; ds -> data segment


; Check to see if we have a command in the command register

icmdc:
          mov     dx, estatus                   ; dx -> status register
          in      al, dx                        ; status -> al
          and     al, ES_ADAPTER_COMMAND_FULL   ; command register full?
          jnz     icmd0                         ; yes
          
          inc     _c5_nocmd              ; count no command
          jmp     ir_y                   ; no:  no more commands this interrupt
          
; Yes we may have something.  Clear the flags and then check to see if 
; we really have something.

icmd0:
          mov     bx, timer_low                    ; current time -> bx
          add     bx, RCMDTO                       ; + time to wait
          mov     al, lastcon                      ; last control -> ax
          and     al, NOT (EC_FLAG1 OR EC_FLAG2)   ; clear flags
          mov     lastcon, al                      ; save last control
          mov     dx, econtrol                     ; dx -> control register
          out     dx, al                      ; clear flags in control register

;----------------------------------------------------------------------------
; This loop stores the command from the 3c505 board into the icmdb buffer.
;
          mov   di, offset icmdb               ; di -> incoming command buffer
icmd1:    
          mov   dx, estatus                    ; dx -> status register
          in    al, dx                         ; status -> al
          mov   cx, ax                         ; status -> cx
          test  al, ES_ADAPTER_COMMAND_FULL    ; command register full?
          jnz   icmd2                          ; yes
          
          cmp   bx, timer_low                  ; have we waited too long?
          ja    icmd1                          ; no
          
          inc   _c5_cmdito                     ; count time out
          jmp   ir_x                           ; yes:  give up
          
;      Yes we REALLY do have a command waiting from the 3c505 board.
icmd2:
          mov     dx, ecommand               ; dx -> command register
          in      al, dx                     ; get command byte
          and     cl, ES_FLAG1 OR ES_FLAG2   ; just keep flags
          cmp     cl, ES_FLAG1 OR ES_FLAG2   ; are both on?
          je      icmd3                      ; yes:  end of command
          
          mov     [di], al                   ; save byte
          inc     di                         ; increment pointer
          mov     ax, di                     ; current pointer -> ax
          sub     ax, offset icmdb           ; - start of buffer
          cmp     ax, CBS                    ; full?
          jl      icmd1                      ; no
          
          mov     si, (offset icmdb) + CBSH  ; si -> middle of buffer
          mov     di, offset icmdb           ; di -> start of buffer
          mov     cx, CBSH                   ; size of half buffer -> cx
          PUSH    ds
          POP     es
          rep movsb                          ; move buffer up
          jmp     icmd1                      ; loop for another byte

;-------------------------------------------------------------------------
; We've gotten the command from the board.
          
icmd3:    
          push    ds
          pop     es                      ; es -> segment of command area
          mov     ah, 0                   ; clear high byte of length
          mov     cmdlen, ax              ; save command length
          mov     si, di                  ; si -> command buffer
          sub     si, cmdlen              ; back up to start of command
          mov     di, offset icmd         ; di -> command area
          mov     cx, cmdlen              ; command length -> cx
          
          rep movsb                       ; move command
          
          mov     al, icmd                ; first byte of command -> al
          cmp     al, 32h                 ; configure 82586?
          je     ic_conc                  ; yes
          
          cmp     al, 33h                 ; get Ethernet address?
          je     ic_geth                  ; yes
          
          cmp     al, 38h                 ; receive complete?
          je     ic_rec                   ; yes
          
          cmp     al, 39h                 ; transmit complete?
          je      ic_xmit                 ; yes
          
          cmp     al, 3ah                 ; statistics response?
          je     ic_stat                  ; yes
          
          cmp     al, 40h                 ; set Ethernet address complete?
          jne     ic_j4                   ; no
          
          jmp     ic_seth                 ; yes
          
ic_j4:    
          cmp     al, 41h                 ; adapter information response?
          jne     ic_j5                   ; no
          
          jmp     ic_adin                 ; yes
          
ic_j5:    
          jmp     ir_x                    ; other:  just ignore it
          
ic_conc:  
          push    ds
          pop     es                   ; es -> configure 82586 response segment
          mov     si, offset icmd      ; si -> command received
          mov     di, offset rconc     ; di -> configure 82586 response
          mov     cx, 2                ; response length -> cx
          
          rep movsw                    ; move response
          
          mov     fconc, 1             ; flag response received
          jmp     ir_x                 ; go return from interrupt
          
ic_geth:  
          push    ds
          pop     es                  ; es -> Ethernet address response segment
          mov     si, offset icmd     ; si -> command received
          mov     di, offset rgeth    ; di -> Ethernet address response
          mov     cx, 4               ; response length -> cx
          
          rep movsw                   ; move response
          
          mov     fgeth, 1            ; flag response received
          jmp     ir_x                ; go return from interrupt
          
ic_xmit:  
          push    ds
          pop     es                      ; es -> transmit response segment
          mov     si, offset icmd         ; si -> command received
          mov     di, offset rxmit        ; di -> transmit response
          mov     cx, 5                   ; response length -> cx
          
          rep movsw                       ; move response
          
          mov     fxmit, 1                ; flag response received
          jmp     ir_x                    ; go return from interrupt
          
ic_stat:  
          push    ds
          pop     es                      ; es -> statistics response segment
          mov     si, offset icmd         ; si -> command received
          mov     di, offset rstat        ; di -> statistics response
          mov     cx, 5                   ; response length -> cx
          
          rep movsw                       ; move response
          
          mov     fstat, 1                ; flag response received
          jmp     ir_x                    ; go return from interrupt
          
ic_rec:   
          push    ds
          pop     es                      ; es -> receive response segment
          mov     si, offset icmd         ; si -> command received
          mov     di, offset rr           ; di -> receive response
          mov     cx, 9                   ; response length -> cx
          
          rep movsw                       ; move response
          
          push    ds
          mov     ds, ksegdata
          mov     ax, _buflim             ; buffer size -> ax
          sub     ax, _bufbig             ; - amount in use
          pop     ds
          sub     ax, rr_dmalen              ; - size of new message
          jl      ir_drop                    ; no room--drop it
          
          push    ds
          mov     ds, ksegdata
          les     di, dword ptr _bufpt    ; es/di -> buffer position
          cmp     di, _bufend             ; have we passed restart point
          jb      icr_2                      ; no     
          
          mov     di, _buforg             ; yes:  start over
icr_2:    
          pop     ds
          mov     ax, rr_dmalen               ; message size -> ax
          inc     ax                          ; + 1 to round up
          shr     ax, 1                       ; convert to words
          shl     ax, 1                       ; convert back to characters
          mov     rr_dmalen, ax               ; use it to update bufbig
          stosw                      ; store message length at front of message
          mov     cx, ax                      ; message length -> cx
          shr     cx, 1                       ; convert to words
          mov     al, lastcon                 ; last control -> al
          or      al, EC_TO_HOST OR EC_FLAG1  ; set direction and acknowledge 
                                                     ; response
          mov     lastcon, al                 ; save last control
          mov     dx, econtrol                ; dx -> control register
          out     dx, al                      ; pass direction
          
          mov     dx, estatus              ; dx -> status register
          mov     bx, edata                ; bx -> data register
icr_1:    
          in      al, dx                   ; get status
          test    al, ES_DATA_READY        ; is data ready?
          jz      icr_1                    ; no
          xchg    dx, bx                   ; dx -> data register
          in      ax, dx                   ; data word -> ax
          stosw                            ; store word in buffer
          xchg    dx, bx                   ; dx -> status register
          dec     cx                       ; count word
          jnz     icr_1                    ; loop if more words
          
          mov     al, lastcon                      ; last control -> al
          and     al, NOT (EC_TO_HOST OR EC_FLAG1) ; change direction to output
          mov     lastcon, al                      ; save last control
          mov     dx, econtrol                     ; dx -> control register
          out     dx, al                  ; send control
          
          mov     ax, rr_dmalen           ; data length-> ax
          push    ds
          mov     ds, ksegdata
          mov     _bufpt, di              ; store pointer
          add     ax, _bufbig             ; + bytes in buffer 
          add     ax, 2                   ; + 2 for size
          mov     _bufbig, ax             ; save buffer in use
          pop     ds
          jmp     SHORT ir_x                    ; go return from interrupt
ir_drop:
          inc     _c5_droptot             ; count dropped message
          inc     rdropnew                ; count so another read gets started
          
; eat the message
          mov     ax, rr_dmalen             ; message size -> ax
          inc     ax                        ; + 1 to round up
          shr     ax, 1                     ; convert to words
          shl     ax, 1                     ; convert back to characters
          mov     rr_dmalen, ax             ; use it to update bufbig
          stosw                             ; store message length at front 
                                                       ; of message
          mov     cx, ax                    ; message length -> cx
          shr     cx, 1                     ; convert to words
          mov     al, lastcon               ; last control -> al
          or     al, EC_TO_HOST OR EC_FLAG1 ; set direction and acknowledge 
                                                    ; response
          mov     lastcon, al               ; save last control
          mov     dx, econtrol              ; dx -> control register
          out     dx, al                    ; pass direction
          
          mov     dx, estatus               ; dx -> status register
          mov     bx, edata                 ; bx -> data register
icr_3:    
          in      al, dx                  ; get status
          test    al, ES_DATA_READY       ; is data ready?
          jz      icr_3                   ; no
          
          xchg    dx, bx                  ; dx -> data register
          in      ax, dx                  ; data word -> ax
          xchg    dx, bx                  ; dx -> status register
          dec     cx                      ; count word
          jnz     icr_3                   ; loop if more words
          
          mov     al, lastcon                      ; last control -> al
          and     al, NOT (EC_TO_HOST OR EC_FLAG1) ; change direction to output
          mov     lastcon, al                      ; save last control
          mov     dx, econtrol                     ; dx -> control register
          out     dx, al                           ; send control
          jmp     SHORT ir_x                             ; go return from interrupt
          
ic_seth:
          push    ds
          pop     es             ; es ->  set Ethernet address response segment
          mov     si, offset icmd         ; si -> command received
          mov     di, offset rseth        ; di -> set Ethernet address response
          mov     cx, 2                   ; response length -> cx
          
          rep movsw                       ; move response
          
          mov     fseth, 1                ; flag response received
          jmp     SHORT ir_x                    ; go return from interrupt

ic_adin:
          push    ds
          pop     es               ; es -> adapter information response segment
          mov     si, offset icmd    ; si -> command received
          mov     di, offset radin   ; di -> adapter information response
          mov     cx, 5              ; response length -> cx
          
          rep movsw                       ; move response
          
          mov     fadin, 1                ; flag response received
          jmp     SHORT ir_x                    ; go return from interrupt
          
ir_x:
          jmp     icmdc                   ; look for another interrupt
          
ir_y:
          mov     ax, eoi2                ; EOI command for 8259 2 -> ax
          jz     ir_1                     ; branch if none
          
          mov     dx, IOCWR2              ; dx -> 8259 2 command register
          out     dx, al                  ; do end of interrupt 2
ir_1:
          mov     ax, eoi1                ; EOI command for 8259 1 -> ax
          mov     dx, IOCWR1              ; dx -> 8259 1 command register
          out     dx, al                  ; do end of interrupt 1
          
          pop     dx
          pop     cx
          pop     bx
          pop     ax
          pop     bp
          pop     di
          pop     si
          pop     es
          pop     ds
          iret      
IINST     endp



NET505_text     ends
          end


