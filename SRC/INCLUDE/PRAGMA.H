#ifndef PRAGMA_H
#define PRAGMA_H

#ifdef __WATCOMC__
#include "stddef.h"
#endif

/* mouse routines */
#pragma aux (cdecl) mousecml;

/* look.c */
#pragma aux (cdecl) attrptr;

/* net.c */
/* the function pointers through which the network board is called */
#pragma aux (cdecl) etopen;
#pragma aux (cdecl) getaddr;
#pragma aux (cdecl) setaddr;
#pragma aux (cdecl) etclose;
#pragma aux (cdecl) xmit;
#pragma aux (cdecl) recv;
#pragma aux (cdecl) etupdate;

/* tools.c */
#pragma aux (cdecl) netsleep;       /* Define as Microsoft until we can re-code the assembly which calls it */

/* user.c */
#if defined(__WATCOMC__) & defined(NET14)
#pragma aux (cdecl) netwrite;
#pragma aux (cdecl) netclose;
#endif

/* ltalk.c */
#pragma aux (cdecl) listen1_c;      /* define these as Microsoft functions until we get around to rewriting the parameter passing to them in the assembly code */
#pragma aux (cdecl) listen2_c;
#pragma aux (cdecl) LTopen;
#pragma aux (cdecl) LTgetaddr;
#pragma aux (cdecl) LTxmit;
#pragma aux (cdecl) LTrecv;
#pragma aux (cdecl) LTupdate;
#pragma aux (cdecl) LTclose;

#ifdef OLD_WAY
/* packet.c */
#pragma aux (cdecl) pkt_receiver2;  /* another routine for Watcomizing the assembly */
#endif

/* netbicc.c */
#pragma aux (cdecl) anr_c;          /* another routine waiting for Watcomizing the parameter passing from the assembly stuff */
#pragma aux (cdecl) ILetopen;
#pragma aux (cdecl) ILgetaddr;
#pragma aux (cdecl) ILetclose;
#pragma aux (cdecl) ILrecv;
#pragma aux (cdecl) ILetupdate;
#pragma aux (cdecl) ILxmit;

/* packet.c */
#pragma aux (cdecl) pketopen;
#pragma aux (cdecl) pkgetaddr;
#pragma aux (cdecl) pkxmit;
#pragma aux (cdecl) pketclose;
#pragma aux (cdecl) pkrecv;
#pragma aux (cdecl) pketupdate;

/* dndll.c */
#pragma aux (cdecl) DNetopen;
#pragma aux (cdecl) DNgetaddr;
#pragma aux (cdecl) DNetclose;
#pragma aux (cdecl) DNrecv;
#pragma aux (cdecl) DNetupdate;
#pragma aux (cdecl) DNxmit;

/* ipasm.asm */
#pragma aux (cdecl) ipcheck;
#pragma aux (cdecl) tcpcheck;
#pragma aux (cdecl) movebytes;
#pragma aux (cdecl) comparen;
#pragma aux (cdecl) longswap;
#pragma aux (cdecl) intswap;
#pragma aux (cdecl) dbg;
#pragma aux (cdecl) tinst;
#pragma aux (cdecl) tdeinst;

/* ncsabrk.asm */
#pragma aux (cdecl) install_break;
#pragma aux (cdecl) remove_break;

/* ncsaio.asm */
#pragma aux (cdecl) n_color;
#pragma aux (cdecl) n_wrap;
#pragma aux (cdecl) n_erase;
#pragma aux (cdecl) n_getchar;
#pragma aux (cdecl) n_cur;
#pragma aux (cdecl) n_row;
#pragma aux (cdecl) n_col;
#pragma aux (cdecl) n_clear;
#pragma aux (cdecl) n_window;
#pragma aux (cdecl) n_putchar;
#pragma aux (cdecl) n_chkchar;
#pragma aux (cdecl) n_savewin;
#pragma aux (cdecl) n_restwin;
#pragma aux (cdecl) n_puts;
#pragma aux (cdecl) n_sound;
#pragma aux (cdecl) n_findfirst;
#pragma aux (cdecl) n_findnext;
#pragma aux (cdecl) n_draw;
#pragma aux (cdecl) n_scrup;
#pragma aux (cdecl) n_scrdn;
#pragma aux (cdecl) n_cheat;
#pragma aux (cdecl) n_clicks;
#pragma aux (cdecl) n_biosattr;
#pragma aux (cdecl) getdsk;
#pragma aux (cdecl) chgdsk;
#pragma aux (cdecl) ega43;
#pragma aux (cdecl) n_flags;
#pragma aux (cdecl) set_cur;
#pragma aux (cdecl) ega24;
#pragma aux (cdecl) n_scrlck;
#pragma aux (cdecl) save_break;
#pragma aux (cdecl) restore_break;
#pragma aux (cdecl) n_gmode;
#pragma aux (cdecl) save_cursor;
#pragma aux (cdecl) restore_cursor;
#pragma aux (cdecl) install_cursor;
#pragma aux (cdecl) n_attr;
#pragma aux (cdecl) install_keyboard;
#pragma aux (cdecl) fix_vid;
#pragma aux (cdecl) get_mode;
#pragma aux (cdecl) set_mode;
#pragma aux (cdecl) get_size;

/* egaset.asm */
#pragma aux (cdecl) EGAset;

/* vgaset.asm */
#pragma aux (cdecl) VGAset;

/* dndll2.asm */
#pragma aux (cdecl) DLLfn;
#pragma aux (cdecl) r_callback;
#pragma aux (cdecl) t_callback;

/* ltlisten.asm */
#pragma aux (cdecl) listen1;
#pragma aux (cdecl) listen2;

/* packet2.asm */
#pragma aux (cdecl) pkt_receiver;
#pragma aux (cdecl) clear_int;
#pragma aux (cdecl) set_int;

/* netbicc2.asm */
#pragma aux (cdecl) ANR_ENTRY;

/* netzyp.asm */
#pragma aux (cdecl) U2recv;
#pragma aux (cdecl) U2etopen;
#pragma aux (cdecl) U2etclose;
#pragma aux (cdecl) U2getaddr;
#pragma aux (cdecl) U2xmit;
#pragma aux (cdecl) U2etupdate;

/* netub.asm */
#pragma aux (cdecl) U1recv;
#pragma aux (cdecl) U1etopen;
#pragma aux (cdecl) U1etclose;
#pragma aux (cdecl) U1getaddr;
#pragma aux (cdecl) U1xmit;
#pragma aux (cdecl) U1etupdate;

/* net3com.asm */
#pragma aux (cdecl) E3recv;
#pragma aux (cdecl) E3etopen;
#pragma aux (cdecl) E3etclose;
#pragma aux (cdecl) E3getaddr;
#pragma aux (cdecl) E3setaddr;
#pragma aux (cdecl) E3xmit;
#pragma aux (cdecl) E3etupdate;

/* net501.asm */
#pragma aux (cdecl) E1recv;
#pragma aux (cdecl) E1etopen;
#pragma aux (cdecl) E1etclose;
#pragma aux (cdecl) E1getaddr;
#pragma aux (cdecl) E1setaddr;
#pragma aux (cdecl) E1xmit;
#pragma aux (cdecl) E1etupdate;

/* net503.asm */
#pragma aux (cdecl) E4recv;
#pragma aux (cdecl) E4etopen;
#pragma aux (cdecl) E4etclose;
#pragma aux (cdecl) E4getaddr;
#pragma aux (cdecl) E4setaddr;
#pragma aux (cdecl) E4xmit;
#pragma aux (cdecl) E4etupdate;
#pragma aux (cdecl) E4setwire;

/* net505.asm */
#pragma aux (cdecl) E5recv;
#pragma aux (cdecl) E5etopen;
#pragma aux (cdecl) E5etclose;
#pragma aux (cdecl) E5getaddr;
#pragma aux (cdecl) E5setaddr;
#pragma aux (cdecl) E5xmit;
#pragma aux (cdecl) E5etupdate;
#pragma aux (cdecl) E4setdma;
#pragma aux (cdecl) c5_get_adapter_info;
#pragma aux (cdecl) c5_get_statistics;

/* net5210.asm */
#pragma aux (cdecl) M5recv;
#pragma aux (cdecl) M5etopen;
#pragma aux (cdecl) M5etclose;
#pragma aux (cdecl) M5getaddr;
#pragma aux (cdecl) M5xmit;
#pragma aux (cdecl) M5etupdate;

/* net523.asm */
#pragma aux (cdecl) E2recv;
#pragma aux (cdecl) E2etopen;
#pragma aux (cdecl) E2etclose;
#pragma aux (cdecl) E2getaddr;
#pragma aux (cdecl) E2xmit;
#pragma aux (cdecl) E2etupdate;

/* net8003.asm */
#pragma aux (cdecl) WDrecv;
#pragma aux (cdecl) WDetopen;
#pragma aux (cdecl) WDetclose;
#pragma aux (cdecl) WDgetaddr;
#pragma aux (cdecl) WDsetaddr;
#pragma aux (cdecl) WDxmit;
#pragma aux (cdecl) WDetupdate;

/* net8003.asm */
#pragma aux (cdecl) WArecv;
#pragma aux (cdecl) WAetopen;
#pragma aux (cdecl) WAetclose;
#pragma aux (cdecl) WAgetaddr;
#pragma aux (cdecl) WAsetaddr;
#pragma aux (cdecl) WAxmit;
#pragma aux (cdecl) WAetupdate;

/* net9210.asm */
#pragma aux (cdecl) M9recv;
#pragma aux (cdecl) M9etopen;
#pragma aux (cdecl) M9etclose;
#pragma aux (cdecl) M9getaddr;
#pragma aux (cdecl) M9xmit;
#pragma aux (cdecl) M9etupdate;

/* netatt10.asm */
#pragma aux (cdecl) ATrecv;
#pragma aux (cdecl) ATetopen;
#pragma aux (cdecl) ATetclose;
#pragma aux (cdecl) ATgetaddr;
#pragma aux (cdecl) ATxmit;
#pragma aux (cdecl) ATetupdate;

#ifdef NET14
#pragma aux (cdecl) print_int;
#pragma aux (cdecl) print_int2;
#pragma aux (cdecl) get_comm_char;
#pragma aux (cdecl) int14open;
#pragma aux (cdecl) int14netsleep;
#pragma aux (cdecl) int14inst;
#pragma aux (cdecl) int14deinst;
#pragma aux (cdecl) int14check;
#pragma aux (cdecl) int14receive;
#pragma aux (cdecl) timeinst;
#pragma aux (cdecl) timedeinst;
#endif

#endif  /* PRAGMA_H */
