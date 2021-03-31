;************************************************************************
;*									*
;*	ZYPDEFS.INC -- ZYPHER Definitions				*
;*									*
;*  Copyright (C) Ungermann-Bass, Inc. 1986, 1987.			*
;*  All rights reserved.						*
;*									*
;************************************************************************
;*	Revision History						*
;*	----------------						*
;*  03/16/87	JF	Added "Get_Xmt_Bfr", "Write_Xmt_Bfr", "Send_	*
;*			Xmt_Bfr", and "Release_Xmt_Bfr" definitions.	*
;*  03/19/87	JF	Added definitions for "incremental mode" of	*
;*			"Receive" command.				*
;*  03/29/87	JF	Added "Schedule" command definitions.		*
;*  04/02/87	JF	Added "Set_Multicast" command definitions.	*
;*									*
;************************************************************************

	;************************************************
	;*	82586 System Configuration Pointer	*
	;************************************************

SCP_	STRUC

SCP_SysBus		db	0	; System data bus width.
			db	0	; [Unused by us.]
			dw	0	;	"
			dw	0	;	"
SCP_ISCP_Address	dw	0	; Low order 16 bits of ISCP address.
SCP_ISCP_Address_MSB	db	0	; High order 6 bits of ISCP address.
			db	0

SCP_	ENDS

	;************************************************
	;*   82586 Intermediate System Control Pointer	*
	;************************************************

ISCP_	STRUC

ISCP_Busy		db	0	; Initialization-in-progress flag.
			db	0	; [Unused.]
ISCP_SCB_Offset		dw	0	; Offset of SCB from SCB Base.
ISCP_SCB_Base		dw	0	; Low order bits of SCB base address.
ISCP_SCB_Base_MSB	db	0	; High order bits of SCB base address.
			db	0	; [Unused.]

ISCP_	ENDS

	;************************************************
	;*	82586 System Control Block		*
	;************************************************

SCB_	STRUC

SCB_Status	dw	0		; STAT, CUS, and RUS.
SCB_Command	dw	0		; ACK, CUC, RESET, and RUC.
SCB_CBL_offset	dw	0		; Command Block List.
SCB_RFA_offset	dw	0		; Receive Frame Area.
SCB_CRCERRS	dw	0		; Cyclic Redundancy Check.
SCB_ALNERRS	dw	0		; Alignment.
SCB_RSCERRS	dw	0		; Resource.
SCB_OVRNERRS	dw	0		; Overrun.

SCB_	ENDS

SCB_CUS_Ready_bit	equ	0200h
SCB_RUS_Ready_bit	equ	0040h
RUS_No_Resources	equ	0020h
CUC_Start_Command	equ	0100h
RUC_Start_Command	equ	0010h
CX_bit			equ	8000h
CNA_bit			equ	2000h

	;************************************************
	;*	82586 Transmit Command Block		*
	;************************************************

TCB_	STRUC

TCB_Status	dw	0		; C, B, and other status.
TCB_Command	dw	0		; EL, S, I, and CMD.
TCB_next_TCB	dw	0		; Pointer to next TCB (always FFFF).
TCB_1st_TBD	dw	0		; Address of active TBD, or FFFF.

TCB_	ENDS

TCB_Completion_Status		equ	BYTE PTR TCB_Status + 1
TCB_Complete_bit		equ	80h
TCB_Complete_and_No_Error_bits	equ	80h+20h
TCB_No_Error_bit		equ	20h
TCB_Hang_bit			equ	8
TCB_EL_bit			equ	8000h

CB_Status	equ	WORD PTR 0
CB_C_bit	equ	8000h
CB_OK_bit	equ	2000h

	;************************************************
	;*	82586 Transmit Buffer Descriptors	*
	;************************************************

TBD	STRUC

TBD_EOF_and_Length	dw	0	; EOF bit and byte count.
TBD_next_TBD		dw	0	; Pointer to next TBD (always FFFF).
TBD_Buffer		dw	0	; 16 LSBs of the buffer address.
TBD_Buffer_MSB		db	0	; 4 MSBs of the buffer address.
TBD_State		db	0	; [Used as the buffer's state.]
TBD_Link		dw	0	; [Used to link TBDs on queues.]
TBD_Index		db	0	; [This TBD's TBD_Table index.]
			db	0	; [Unused.]
TBD_ZCB_Address		dw	0,0	; [Holds address of a Transmit_ZCB.]

TBD	ENDS

TBD_Bfr_Ptr	equ	TBD_ZCB_Address
TBD_Bfr_Room_Left equ	TBD_ZCB_Address+2

Max_TBDs		EQU	4

TBD_EOF_bit	equ	80h
TBD_Length_bits	equ	3FFFh
Multicast_byte	equ	BYTE PTR 0
Multicast_bit	equ	1

TBD_State_Free	equ	0	; Not in use -- on "Free_TBD_Queue".
TBD_State_User	equ	1	; Assigned to user for filling with data.
TBD_State_Wait	equ	2	; Ready to transmit -- on "Waiting_TBD_Queue".
TBD_State_Xmtg	equ	3	; Being transmitted now.

	;************************************************
	;*	82586 Receive Frame Descriptors		*
	;************************************************

RFD	STRUC

RFD_Status	dw	0	; C, B, and other status bits of this RFD.
		db	0	; Unused.
RFD_EOL		db	0	; End-of-List status byte.
RFD_next_RFD	dw	0	; Link to next RFD.
RFD_1st_RBD	dw	0	; Pointer to the first RBD.

RFD	ENDS

RFD_Complete_bit		equ	8000h
RFD_Complete_and_No_Error_bits	equ	8000h+2000h
RFD_EOL_bit			equ	80h


	;************************************************
	;*	82586 Receive Buffer Descriptors	*
	;************************************************

RBD	STRUC

RBD_EOF_F_and_Length	dw	0	; EOF & F bits and rec'vd byte count.
RBD_next_RBD		dw	0	; Pointer to next RBD.
RBD_Buffer		dw	0	; 16 LSBs of the buffer address.
RBD_Buffer_MSB		db	0	; 4 MSBs of the buffer address.
RBD_State		db	0	; [Used as the buffer's state.]
RBD_EOL_and_Size	dw	0	; EOL status and buffer size.
RBD_Link		dw	0	; [Used to link RBDs on queues.]
RBD_Frame_Length	dw	0	; [Total length of received frame.]
RBD_Rcv_Hdwr_Status	dw	0	; [Status from RFD.]

RBD	ENDS

RBD_EOF_and_F		equ	BYTE PTR RBD_EOF_F_and_Length + 1
RBD_EOL			equ	BYTE PTR RBD_EOL_and_Size + 1

RBD_EOF_bit	equ	80h
RBD_F_bit	equ	40h
RBD_EOL_bit	equ	80h
RBD_Size_bits	equ	3fffh
RBD_Length_bits	equ	3fffh

	;************************************************
	;*	82586 Configure Command Block		*
	;************************************************

CFG_	STRUC

CFG_Status		dw	0
CFG_Command		dw	0
CFG_next_CB		dw	0
CFG_Byte_Count		db	0
CFG_FIFO_Limit		db	0
CFG_BF_and_SRDY_ARDY	db	0
CFG_LPBK_PREAM_AL_and_LEN db	0
CFG_BOF_ACR_and_PRIO	db	0
CFG_Interframe_Spacing	db	0
CFG_Slot_Time_Low	db	0
CFG_Retries_and_Slot_Time db	0
CFG_BTSTF_TONOCRS_etc	db	0
CFG_CS_and_CD_things	db	0
CFG_Min_Frame_Length	db	0
			db	0
CFG_	ENDS

Bit_Stuffing_bit		EQU	40h
TONO_CRS_bit			EQU	08h
BIT_STUFF_and_TONO_CRS_bits	EQU	48h
Both_Loopback_bits		EQU	0C0h
External_Loopback_bit		EQU	80h
Internal_Loopback_bit		EQU	40h
Save_Bad_Frames_bit		EQU	80h
Alternative_Backoff_bit		EQU	80h

	;************************************************
	;*  82586 Set Individual Address Command Block	*
	;************************************************

SET_IA_	STRUC

IA_Status		dw	0
IA_Command		dw	0
IA_next_CB		dw	0
IA_Address		db	6 dup (0)

SET_IA_	ENDS

	;************************************************
	;*	82586 Multicast Setup Command Block	*
	;************************************************

MCB_	STRUC
		db	0
MCB_Status	db	0		; C, B, OK, and A status bits.
MCB_Command	dw	0		; EL, S, I, and CMD.
MCB_next_CB	dw	0		; Pointer to next Command Block.
MCB_Count	dw	0		; Count of Multicast Address bytes.

MCB_	ENDS

MCB_Addresses		equ	BYTE PTR MCB_Count + 2
MCB_Complete_bit	equ	80h


	    ;************************************************
	    ;*		ZYPHER Command Blocks		    *
	    ;************************************************

    ;********************************************************
    ;*    Command Block Header -- Common to all Commands    *
    ;********************************************************

ZCB_Header	STRUC

ZCB_Command		db	0
ZCB_Status		db	0
ZCB_Result		db	0
ZCB_Report_Code		db	0
ZCB_Options		dw	0
ZCB_Post_Routine	dd	0
ZCB_Link		dw	2 dup (0)

ZCB_Header	ENDS

ZCB_Post_Routine_offset	equ	WORD PTR ZCB_Post_Routine
ZCB_Post_Routine_segment equ	WORD PTR ZCB_Post_Routine + 2


    ;************************************
    ;*    Transmit Command Block	*
    ;************************************

Transmit_ZCB	STRUC
			db	SIZE ZCB_Header dup (0)
ZCB_Xmt_Data_Length	dw	0
ZCB_Xmt_Data_Address	dd	0
ZCB_Xmt_Hdwr_Status	dw	0
ZCB_Xmt_Bfr_ID		db	0,0
ZCB_Xmt_Bfr_Address	dw	0,0

Transmit_ZCB	ENDS



    ;************************************
    ;*    Receive Command Block		*
    ;************************************

Receive_ZCB	STRUC
			db	SIZE ZCB_Header dup (0)
ZCB_Rcv_Mode		db	0
ZCB_Rcv_Status		db	0
ZCB_Rcv_Buffer_Size	dw	0	; Size of user's buffer.
ZCB_Rcv_Buffer_Address	dd	0	; Address of user's buffer.
ZCB_Rcv_Data_Length	dw	0	; Bytes copied to user's buffer.
ZCB_Rcv_Frame_Count	dw	0	; Count of as-yet-uncopied bytes left
					;  in frame.
ZCB_Rcv_Hdwr_Status	dw	0	; Status reported by 82586.
ZCB_Rcv_Frame_ID	dw	0	; Frame ID for "incremental mode".
ZCB_Rcv_Bfr_Ptr		dw	0,0	; Address of next as-yet-uncopied byte
					;  of frame in on-card receive buffer.
ZCB_Rcv_Bfr_Count	dw	0	; Count of as-yet-uncopied bytes in
					;  current on-card receive buffer.
ZCB_Rcv_Descriptor	dw	0,0	; Address of 82586 RBD (Receive Buffer
					;  Descriptor) for current on-card
					;  receive buffer.

Receive_ZCB	ENDS

Rcv_Mode_Incremental	EQU	1


    ;************************************
    ;*    Initialize Command Block	*
    ;************************************

Initialize_ZCB	STRUC
			db	SIZE ZCB_Header dup (0)
ZCB_Init_Modes		dw	0	; Desired modes of operation.
ZCB_Max_Xmt_Length	dw	0	; Desired on-card xmit buffer size.
ZCB_Num_Xmt_Buffers	dw	0	;    "    number of on-card xmit bfrs.
ZCB_Max_Rcv_Size	dw	0	;    "    on-card recv buffer size.
ZCB_Num_Rcv_Buffers	dw	0	;    "    number of on-card recv bfrs.
ZCB_Max_Mcast_Addresses	dw	0	;    "  number of multicast addresses.
			dw	3 dup (0)	; [Reserved.] 
			dw	2 dup (0)	; [Reserved.] 
ZCB_82586_CFG_Address	dd	0	; Offset & Segment of 82586 CFG data.
ZCB_Acq_Modes		dw	0	; Acquired modes of operation.
ZCB_Acq_Max_Xmt_Length	dw	0	; Acquired on-card xmit buffer size.
ZCB_Acq_Num_Xmt_Buffers	dw	0	;    "    number of on-card xmit bfrs.
ZCB_Acq_Max_Rcv_Size	dw	0	;    "    on-card recv buffer size.
ZCB_Acq_Num_Rcv_Buffers	dw	0	;    "    number of on-card recv bfrs.
ZCB_Acq_Max_Mcast_Addresses dw	0	;    "    number of multicast addrs.
			dw	3 dup (0)	; [Reserved.] 

Initialize_ZCB	ENDS

    ;****************************************************
    ;*    User-Supplied 82586 Configuration Data Block	*
    ;****************************************************

;  If the "User_Configured_82586" bit is set in the "ZCB_Init_Modes" field
;  of the "Initialize_ZCB", and the "ZCB_82586_CFG_Address" field is not
;  equal to (0, 0), the contents of the "ZCB_82586_CFG_Address" field will
;  be assumed to be the address of a block of 14 bytes with the format given
;  below.  If the "UCFG_Signature" word contains 429Ah (i.e., 82586 decimal,
;  truncated to 16 bits), and the "UCFG_Byte_Count" field contains 12, the
;  12 bytes of the block starting with "UCFG_Byte_Count" will be used for
;  the "command specific" part of the CONFIGURE commands given to the 82586,
;  in place of the 82586 configuration data built into the ROM.  The user-
;  specified configuration will override the broadband/baseband decision that
;  has been made by "Determine_Baseband_or_Broadband".

User_CFG	STRUC

UCFG_Signature		dw	0
UCFG_Byte_Count		db	0
UCFG_FIFO_Limit		db	0
UCFG_BF_and_SRDY_ARDY	db	0
UCFG_LPBK_PREAM_AL_and_LEN db	0
UCFG_BOF_ACR_and_PRIO	db	0
UCFG_Interframe_Spacing	db	0
UCFG_Slot_Time_Low	db	0
UCFG_Retries_and_Slot_Time db	0
UCFG_BTSTF_TONOCRS_etc	db	0
UCFG_CS_and_CD_things	db	0
UCFG_Min_Frame_Length	db	0
			db	0
User_CFG	ENDS



    ;************************************
    ;*    Status Command Block		*
    ;************************************

Status_ZCB	STRUC
			db	SIZE ZCB_Header dup (0)
ZCB_Stat_State		dw	0
ZCB_Stat_Modes		dw	0	; Current modes of operation.
ZCB_Stat_Max_Xmt_Length	dw	0	; Current on-card xmit buffer size.
ZCB_Stat_Num_Xmt_Buffers dw	0	;    "    number of on-card xmit bfrs.
ZCB_Stat_Max_Rcv_Size	dw	0	;    "    on-card recv buffer size.
ZCB_Stat_Num_Rcv_Buffers dw	0	;    "    number of on-card recv bfrs.
ZCB_Stat_Unique_ID	db	6 dup (0)
ZCB_Stat_Total_Xmts	dw	2 dup (0)
ZCB_Stat_Total_Rcvs	dw	2 dup (0)
ZCB_Stat_CRC_Errors	dw	2 dup (0)
ZCB_Stat_ALN_Errors	dw	2 dup (0)
ZCB_Stat_RSC_Errors	dw	2 dup (0)
ZCB_Stat_OVR_Errors	dw	2 dup (0)
			dw	12 dup (0);	[Reserved.]

Status_ZCB	ENDS

    ;********************************************
    ;*    Setup-Scheduling Command Block	*
    ;********************************************

Scheduling_ZCB	STRUC
			db	SIZE ZCB_Header dup (0)
ZCB_Sched_Routine	dw	0,0
ZCB_Sched_Options	dw	0
ZCB_Sched_Interval	dw	0

Scheduling_ZCB	ENDS

Time_Elapsed		equ	1
Frame_Arrived		equ	2
Receive_Completed	equ	4
Transmit_Completed	equ	8
Explicit_Sched_Request	equ	100h

Sched_Options_bits	equ	Time_Elapsed+Frame_Arrived+Receive_Completed+Transmit_Completed+Explicit_Sched_Request

    ;********************************************
    ;*   Set-Multicast-Address Command Block	*
    ;********************************************

Multicast_ZCB	STRUC
			db	SIZE ZCB_Header dup (0)
ZCB_Multicast_Address	db	6 dup (0)

Multicast_ZCB	ENDS

	;************************************************
	;*	ZYPHER Command Codes			*
	;************************************************

Initialize_Cmd		EQU	40h
Transmit_Cmd		EQU	41h
Receive_Cmd		EQU	42h
Status_Cmd		EQU	43h
Cancel_Receives_Cmd	EQU	44h
Get_Xmt_Bfr_Cmd		EQU	45h
Write_Xmt_Bfr_Cmd	EQU	46h
Send_Xmt_Bfr_Cmd	EQU	47h
Release_Xmt_Bfr_Cmd	EQU	48h
Read_Frame_Data_Cmd	EQU	49h
Next_Rcv_Bfr_Cmd	EQU	4Ah
Flush_Frame_Cmd		EQU	4Bh
Setup_Scheduling_Cmd	EQU	4Ch
Schedule_Cmd		EQU	4Dh
Enable_Multicast_Cmd	EQU	4Eh
Disable_Multicast_Cmd	EQU	4Fh
Disable_All_Multicasts_Cmd EQU	50h

Min_Command_Code	EQU	Initialize_Cmd
Max_Command_Code	EQU	Disable_All_Multicasts_Cmd


	;************************************************
	;*	ZYPHER Result Codes			*
	;************************************************

Xmt_Length_Error	equ	10h	; Data length too long or too short.
Xmt_Address_Error	equ	11h	; Data starts at odd address.
Unknown_Command_Error	equ	12h	; Undefined command code in ZCB.
Unimplemented_yet_Error	equ	13h	; As-yet-unimplemented command code.
Xmt_Completion_Success	equ	14h	; Transmission successfully completed.
Xmt_Completion_Error	equ	15h	; Transmission completed with error.
Rcv_Completion_Success	equ	16h	; Frame received successfully.
Rcv_Frame_Truncated_Error equ	17h	; Receive buffer was too small.
Initialization_Complete	equ	18h	; Initialization completed.
Status_Complete		equ	19h	; Status completed.
Not_Yet_Initialized	equ	1Ah
Already_Initialized	equ	1Bh
Command_Cancelled	equ	1Ch
Cancel_Complete		equ	1Dh
Xmt_Bfr_Assigned	equ	1Eh	; "Get_Xmt_Bfr_Cmd" completed.
Xmt_Bfr_not_Assigned	equ	1Fh
Xmt_Bfr_Released	equ	20h	; "Release_Xmt_Bfr_Cmd" completed.
Xmt_Bfr_Written		equ	21h	; "Write_Xmt_Bfr_Cmd" completed.
More_Data_Available	equ	22h
No_More_Rcv_Bfrs	equ	23h
Frame_ID_Invalid	equ	24h
Flush_Complete		equ	25h
Frame_Table_Overflow	equ	26h
Scheduling_Started	equ	27h
Scheduling_Cancelled	equ	28h
Schedule_Done		equ	29h
Multicast_Setup_Done	equ	2Ah
Too_Many_Multicast_Addresses equ 2Bh

	;************************************************
	;*		System State Bits		*
	;************************************************

Initialized			EQU	8000h
Ready_to_Receive		EQU	1000h
Interrupt_Entry_Installed	EQU	0800h
Media_Test_Done			EQU	0002h
Media_Determined		EQU	0001h

	;************************************************
	;*		System Modes Bits		*
	;************************************************

Broadcasts_Disabled	EQU	0800h
Promiscuous_Mode	EQU	0400h
Accept_Bad_Frames	EQU	0200h
Broadband_Mode		EQU	0100h
User_Configured_82586	EQU	0080h

	;************************************************
	;*	ZE-NIC Control and Status Register bits	*
	;************************************************

CSR_Reset			EQU	1
CSR_Network_Loopback		EQU	2
CSR_Timer_Enable		EQU	4
CSR_82586_Enable		EQU	8
CSR_Soft_Interrupt		EQU	20h
CSR_Timer_Interrupt		EQU	40h
CSR_82586_Interrupt		EQU	80h
;