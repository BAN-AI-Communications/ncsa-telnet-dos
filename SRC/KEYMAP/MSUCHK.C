/* scanchek.c */
/*
** By: Phil Benchoff, June 1986
** v2.0: 31 Aug 1987, Update Kermit to v2.29C, add Turbo-C support.
** v3.0: 31 Aug 1987 merge header and main body, telescope code, add
**    some revisons and Bios support for most C compilers. Joe R. Doupnik
** v3.1: 01 Sep 1987, add #ifndef before bioskey() function.
** v4.0: 22 Jan 1988, Computer Innovations C86 support dropped.
**                    Enhanced keyboard support added
**                    Mods to match MS-Kermit 2.30
**                    A few JRD suggestions
** v4.1: 20 Jan 1989, fix bug with NumLock/Shift, match MS-Kermit 2.32
**                    Matthias Reichling
**
** This program displays various information about the keys pressed.
** It's original intent is to be used in testing the new IBM keyboards
** (you know the ones, with the F keys on the top).
**
** Enhanced keyboard:
** For more information on the enhanced keyboard see:
** 'Keying on a Standard', _PC_TECH_JOURNAL_, July 1987, pp. 134-156
** Minor error in article,  Table 1:  Numeric Keypad:
**    Buffer codes with lsb=0xF0 are actually translated to have
**    lsb=00 by int 0x16.
**
** The BIOS interrupt 0x16 is used for keyboard services. The particular
** service is determined by the service code in AH when the interrupt
** is invoked.
**
** AH = 0 returns the next available keyboard character.
**        AL = extended-ASCII character code
**        AH = keyboard scan code ( 0 if the alt-numeric input used )
**  or
**        AL = 0
**        AH = special character code ( F1-F10, Home, End, etc.)
** AH = 1 test for a character ( don't wait )
**        returned values same as above
** AH = 2 returns the current shift states
**        AL = shift states
** AH = 0x10 or 0x11
**        enhanced keyboard versions of AH=0 and AH=1
**
** The MS-Kermit (2.32) 'key ident' is also printed.  This value
** is used in the SET KEY command.  Note that Kermit uses the shift
** bits differently than the BIOS routines,  so multiple shifts
** (e.g. Ctrl-Alt-F1) can be used with the same key in some cases.
*/

#include <stdio.h>
#include <string.h>
#include <dos.h>

/* Kermit-MS keyboard shift flags (most significant byte of key_ident) */
#define SCAN    0x0100  /* scan code */
#define SHIFT   0x0200  /* left or right shift */
#define CONTROL 0x0400  /* Ctrl shift */
#define ALT     0x0800  /* Alt shift */
#define ENHANCE 0x1000  /* Enhanced keyboard special key */

#define BS 0x08
#define TAB 0x09
#define EOS '\0'
/*
** These two tables are useful for relating ascii and scan codes
** to the characters entered or keys pressed.
** There are several types of characters included.
** ASCII characters:
** - Characters 0x00 to 0x1F are control characters.
**   The array ascii_chars[i] contains the names of these
**   control characters. 'Space' is included for lack of
**   a better place to put it. The index i should be
**   the ascii code for the character. Note that the ascii
**   character NUL (0x00) cannot be entered on the IBM-PC keyboard.
** - The character codes 0x20 - 0xff are printable on the
**   IBM-PC and are not considered here (except 'space').
** Special characters:
**   For some characters, no ascii value is returned by BIOS interrupt
**   0x16. The scan codes of these characters identify them.
**   They include F keys, arrow keys, and alt keys.
**   With the enhanced keyboard,  BIOS may return the following:
**   - scancode = 0xe0, ascii = non-enhanced scancode
**   - scancode = non-enhanced scancode, ascii = 0xe0 of 0xf0
**
**   The array special_chars[i] contains tha names of these keys.
**   The index i should be the scan code for the key.
**   The array is 166 elements long, but not all of the
**   scan codes in that range are special keys.
**
** Phil Benchoff, June 1986
** revised Jan 88
*/

char *ascii_chars[] = {                 /* ANSI names of control codes */
   "NUL", "SOH", "STX", "ETX", "EOT", "ENQ",  /* ^@, ^A, ^B, ^C, ^D, ^E */
   "ACK", "BEL", "BS" , "HT" , "LF" , "VT",   /* ^F, ^G, ^H, ^I, ^J, ^K */
   "FF" , "CR" , "SO" , "SI" , "DLE", "DC1",  /* ^L, ^M, ^N, ^O, ^P, ^Q */
   "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",  /* ^R, ^S, ^T, ^U, ^V, ^W */
   "CAN", "EM" , "SUB", "ESC", "FS" , "GS",   /* ^X, ^Y, ^Z, ^[, ^\, ^] */
   "RS" , "US" , "Space" } ;                  /* ^^, ^_, space */

char *special_keys[] = {            /* common text for given scan code */
   "", "Alt-Esc", "", "NUL", "", "", "", "",                    /* 0-7 */
   "", "", "Ctrl-Num-Ent", "", "Alt-Enter", "Num-Enter",        /* 8-13 */
   "Alt-BS", "Shift-Tab",                                       /* 14-15 */
   "Alt-Q", "Alt-W", "Alt-E", "Alt-R", "Alt-T",                 /* 16-20 */
   "Alt-Y", "Alt-U", "Alt-I", "Alt-O", "Alt-P",                 /* 21-25 */
   "Alt-[", "Alt-]", "Alt-Enter", "",                           /* 26-29 */
   "Alt-A", "Alt-S", "Alt-D", "Alt-F", "Alt-G",                 /* 30-34 */
   "Alt-H", "Alt-J", "Alt-K", "Alt-L",                          /* 35-38 */
   "Alt-;", "Alt-'", "Alt-`", "", "Alt-\\",                     /* 39-43 */
   "Alt-Z", "Alt-X", "Alt-C", "Alt-V", "Alt-B",                 /* 44-48 */
   "Alt-N", "Alt-M",                                            /* 49-50 */
   "Alt-,", "Alt-.", "Alt-/", "", "Alt-Num-*", "", "", "",      /* 51-58 */
   "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", /* 59-68 */
   "", "",                                                      /* 69-70 */
   "Home", "U-arrow", "PgUp", "Alt-NumMinus",                   /* 71-74 */
   "L-arrow", "Num-5", "R-arrow",                               /* 75-77 */
   "Alt-Num-+", "End", "D-arrow", "PgDn", "Ins", "Del",         /* 78-83 */
   "Shift-F1", "Shift-F2", "Shift-F3", "Shift-F4", "Shift-F5",  /* 84-88 */
   "Shift-F6", "Shift-F7", "Shift-F8", "Shift-F9", "Shift-F10", /* 89-93 */
   "Ctrl-F1",  "Ctrl-F2",  "Ctrl-F3",  "Ctrl-F4",  "Ctrl-F5",   /* 94-98 */
   "Ctrl-F6",  "Ctrl-F7",  "Ctrl-F8",  "Ctrl-F9",  "Ctrl-F10",  /* 99-103 */
   "Alt-F1",   "Alt-F2",   "Alt-F3",   "Alt-F4",   "Alt-F5",    /* 104-108 */
   "Alt-F6",   "Alt-F7",   "Alt-F8",   "Alt-F9",   "Alt-F10",   /* 109-113 */
   "Ctrl-PrtSc", "Ctrl-L-arrow", "Ctrl-R-arrow",                /* 114-116 */
   "Ctrl-End",   "Ctrl-PgDn",    "Ctrl-Home",                   /* 117-119 */
   "Alt-1", "Alt-2", "Alt-3", "Alt-4", "Alt-5", "Alt-6",        /* 120-125 */
   "Alt-7", "Alt-8", "Alt-9", "Alt-0", "Alt--", "Alt-=",        /* 126-131 */
   "Ctrl-PgUp", "F11", "F12", "Shift-F11",  "Shift-F12",        /* 132-136 */
   "Ctrl-F11", "Ctrl-F12", "Alt-F11", "Alt-F12",                /* 137-140 */
   "Ctrl-U-arrow", "Ctrl-NumMinus", "Ctrl-Num-5",               /* 141-143 */
   "Ctrl-Num-+", "Ctrl-D-Arrow", "Ctrl-Insert",                 /* 144-146 */
   "Ctrl-Delete", "Ctrl-Tab", "Ctrl-Num-/", "Ctrl-Num-*",       /* 147-150 */
   "Alt-Home", "Alt-U-arrow", "Alt-PgUp", "",                   /* 151-154 */
   "Alt-L-arrow", "", "Alt-R-Arrow", "", "Alt-End",             /* 155-159 */
   "Alt-D-Arrow", "Alt-PgDn", "Alt-Insert", "Alt-Del",          /* 160-163 */
   "Alt-Num-/", "Alt-Tab", "Alt-Num-Ent."                       /* 164-166 */
};
#define NSPECIALS 166  /* length of special_keys[] */

#define MK_FP(__s,__o) ((void far *)(((unsigned long)(__s)<<16)|(unsigned)(__o)))

main()
{
   unsigned int count, ascii, scan;
   int read_fn, enh_key;
   static char *comment();
   unsigned int bios_key, bios_shifts, key_ident, kerm_key();

   explain();  /* display instructions */

   /*
   ** Determine if we have an enhanced keyboard.
   ** Select the function to use when calling the keyboard interrupt.
   **  read_fn = peekb(seg,offset) may need replacing by
   **  peek(seg,offset,&read_fn,1); read_fn &= 0x10;  for Lattice C.
   */

#ifdef QAK_FOR_NOW
   read_fn = peekb(0x40,0x96) & 0x10; /* get BIOS enhanced keyboard flag */
             /* read_fn is either 0x00 or 0x10 */
#else
    read_fn = (*(char far *)MK_FP(0x40,0x96)) & 0x10;
#ifdef OLD_WAY
	read_fn=0x10;
#endif
#endif
   if ( 0 == read_fn ) {              /* flag = 0,  standard keyboard */
      printf("Standard keyboard:\n");
   }
   else {                             /* flag = 1, enhanced keyboard */
      printf("Enhanced keyboard:\n");
   }

   count = 1;

   while ( 1 ) {
      if (1 == count ) {
         header();
         count = 21;
      } else
         count--;

      enh_key=0; /* set this to 1 in the following code if this key */
                 /* is only available on the enhanced keyboard */
      bios_key = bioskey(read_fn);
      bios_shifts = bioskey(2);

      ascii = bios_key & 0x00FF;     /* ASCII value is in low byte */
      scan  = bios_key >> 8;         /* Scan code is in high byte  */
      key_ident = kerm_key(bios_key,bios_shifts);

      /* Note: You can't enter a NUL (ascii 00) from the keyboard */

      if ( 0xe0 == scan ) {                       /* enhanced keyboard */
         enh_key = 1;
         /*
         ** The IBM memorial exception:
         ** The buffer code for Numeric-/ is 0xe02f; however 0x2f is
         ** the scan code for Alt-V,  so we can't just use al as the
         ** scan code.
         */
         if ( 0x2f == ascii )
            printf("| %-13s ", "Numeric-/");
         else if (ascii <= NSPECIALS)
            printf("| %-13s ", special_keys[ascii]);
         else
            printf("| * unknown *   ");
      } else if ( (0!=scan) && (0xe0==ascii) ) {  /* enhanced keyboard */
         enh_key = 1;
         if (scan <= NSPECIALS)
            printf("| %-13s ", special_keys[scan]);
         else
            printf("| * unknown *   ");
      } else if ( (ascii >= 33) && (ascii <= 255) ) {  /* Printable char. */
         printf("| %-13c ", ascii );
      } else if ( (ascii > 0) && (ascii <= 31) ) {  /* control char */
         printf("| Ctrl-%c %-6s ", ascii + 64, ascii_chars[ascii]);
      } else if (ascii == 32) {                /* Space       */
         printf("| Space         ");
      } else if ( ascii == 0 ) {      /* special key (no ascii value) */
         if (scan <= NSPECIALS) {              /* in table? */
            if ( 0x84 < scan )        /* enhanced keyboard if scan > 84 */
               enh_key = 1;
            else {
               switch (scan) {
                  /*
                  ** The following keys produce a scan code and
                  ** no ascii value,  but are only available on
                  ** the enhanced keyboard.
                  */
                  case 0x01:      /* Alt-ESC */
                  case 0x0e:      /* Alt-Backspace */
                  case 0x1a:      /* Alt-[ */
                  case 0x1b:      /* Alt-] */
                  case 0x1c:      /* Alt-Enter */
                     /* Alt-Enter produces scancode 12 (dec.) on my */
                     /* PC Limited AT-8 w/standard keyboard */
                  case 0x27:      /* Alt-; */
                  case 0x28:      /* Alt-' */
                  case 0x29:      /* Alt-` */
                  case 0x2b:      /* Alt-\ */
                  case 0x33:      /* Alt-, */
                  case 0x34:      /* Alt-. */
                  case 0x35:      /* Alt-/ */
                  case 0x37:      /* Alt-* */
                  case 0x4a:      /* Alt-Num- */
                  case 0x4c:      /* Num-5 */
                  case 0x4e:      /* Alt-Num+ */
                     enh_key = 1;
                     break;
                  default:
                     break;
               };
            }
            printf("| %-13s ", special_keys[scan]);
         }
         else
            printf("| * unknown *   ");
      } else {
         printf("| Out of range  ");
      }

      printf("| %3d | 0x%02x |%c| \\%-4d ", scan, ascii,
              enh_key ? '*' : ' ', key_ident);

      /* print Kermit shift bit status */
      if ( SCAN & key_ident ) {
         printf("| %c", (key_ident & ENHANCE) ? 'E' : '-');
         printf("%c",(key_ident & ALT) ? 'A' : '-');
         printf("%c",(key_ident & CONTROL) ? 'C' : '-');
         printf("%c ",(key_ident & SHIFT) ? 'S' : '-');
      } else printf("|      ");

      printf("| %-29s|\n", comment(scan,ascii) );
        if (key_ident == 1280) break;         /* Control-Break exit */
   }
   exit(0);
}

/*
** This function should match 'getkey' in MSUIBM.ASM.
**
** Kermit-MS determines the 'key ident' from the value returned in ax
** (ah=scan code, al=ascii value) from BIOS call 0x16 function 0
** (standard keyboard) or 0x10 (enhanced keyboard), the
** status of various shift keys, and a table of special keys (aliaskey).
** The aliaskey table handles cases where more than one key may generate
** the same ascii value (i.e. the numeric keypad).  The entries in table
** aliaskey are words with the high byte holding a scan code and the low
** byte holding an ascii value.  The general method is as follows:
**
**    BIOS int 0x16 function 0 or 0x10   returns key's code in register ax.
**    if (ah!=0 and al=0xe0)             enhanced
**       al = 0                          zero ascii
**       key_ident |= ENHANCE            set enhanced flag
**    else if (ah==0xe0)                 enhanced
**       ah=al                           put scan in ah
**       al=0                            zero ascii
**       key_ident |= ENHANCE            set enhanced flag
**    if (ax is in aliaskey list)
**       al = 0                    clear normal ascii to simulate special key
**    ascii = al                         do this in either case
**    if ( ascii == 0 ) {                now, if the key is a special one ...
**    scancode = ah                      work with scan code instead
**       key_ident = scancode + SCAN     set SCAN code flag bit
**       if ( (LeftShift || RightShift) && (no NumKeypadWhiteKey)
**           || (NumKeypadWhiteKey && NumLock && no Shift)
**           || (NumKeypadWhiteKey && no NumLock && Shift) )
**          key_ident |= SHIFT           set smart SHIFT key flag bit
**       if ( Ctrl )
**          key_ident |= CONTROL         set CONTROL key flag bit
**       If ( Alt )
**          key_ident |= ALT             set ALT key flag bit
**    } else
**       key_ident = ascii
*/
unsigned int kerm_key(bios_key,bios_shifts)
unsigned int bios_key, bios_shifts;
{
   unsigned  key_id, shift_hlp;

   key_id = 0;
   shift_hlp = 0;
   /* getky1a */
   if ( 0 != (bios_key & 0xff00) ) {        /* not alt-numeric */
      if ( 0xe000 == (bios_key & 0xff00) ) {
         bios_key <<= 8;
         key_id |= ENHANCE;
      }
      /* getky1b */
      if ( 0x00e0 == (bios_key & 0x00ff) ) {
         bios_key &= 0xff00;
         key_id |= ENHANCE;
      }
   }
   if ( 0 != (bios_key & 0x00ff) ) {        /* not scan code only */
      /* getky1c */
      /* aliaskey processing */
      switch (bios_key)
      {
         case ( 14 * SCAN ) + BS:
         case ( 55 * SCAN ) + '*':
         case ( 74 * SCAN ) + '-':
         case ( 78 * SCAN ) + '+':
         case ( 71 * SCAN ) + '7':
         case ( 72 * SCAN ) + '8':
         case ( 73 * SCAN ) + '9':
         case ( 75 * SCAN ) + '4':
         case ( 76 * SCAN ) + '5':
         case ( 77 * SCAN ) + '6':
         case ( 79 * SCAN ) + '1':
         case ( 80 * SCAN ) + '2':
         case ( 81 * SCAN ) + '3':
         case ( 82 * SCAN ) + '0':
         case ( 83 * SCAN ) + '.':
         case ( 83 * SCAN ) + ',':
         case ( 15 * SCAN ) + TAB:
                   bios_key &= 0xff00;     /* clear ascii low byte */
                   break;
         default:                        /* key is not in the list */
                   break;
      };
   }
   if ( (bios_key & 0x00ff) == 0 ) {
                           /* No ASCII value, get a kermit scan code. */
      key_id |= ((bios_key >> 8) | SCAN);       /* set scancode flag */
      if (bios_shifts & 3)                      /* left or right shift?*/
         shift_hlp |= SHIFT;                    /* yes, set shift flag */
      if ((bios_shifts & 0x20)                  /* NumLock set? */
             && ( key_id >= ( 71 + SCAN ) )     /* on numeric keypad ? */
             && ( key_id <= ( 83 + SCAN ) )
             && ( key_id != ( 74 + SCAN ) )     /* not the grey - key? */
             && ( key_id != ( 78 + SCAN ) ) )   /* not the grey + key? */
         shift_hlp ^= SHIFT;                    /* all true, xor shift */
      key_id |= shift_hlp;
      if (bios_shifts & 0x04)                   /* Ctrl-key pressed? */
         key_id |= CONTROL;
      if (bios_shifts & 0x08)                   /* Alt-key pressed? */
         key_id |= ALT;
   } else
      /* We have an ASCII value,  return that */
      key_id = bios_key & 0xff;

   return key_id;
}

char *comment(scan,ascii)
int  scan, ascii;
{
    static char line[40];

    line[0] = '\0';                     /* start with an empty line */

   if ( (0xe0 == ascii) && (0 != scan) ) {
      ascii = 0;
   } else if ( 0xe0 == scan ) {
      scan = ascii;
      ascii = 0;
   }
    if ( (ascii !=0) && (scan >= 71) )
       strcpy(line,"Numeric Keypad");
    else if ( (ascii == 0) &&  (scan != 0) )
       strcpy(line,"Special Key");
    else if ( (ascii != 0) && (scan == 0) )
       strcpy(line,"Alt-numeric method.");
    return(line);
}

header() {
   char *dash38 = "--------------------------------------",
        *left1  = "|           B I O S            |Kermit-",
        *right1 = "MSv2.32|                              |",
        *left2  = "|Key            |Scan |ASCII |E|KeyIdnt",
        *right2 = "|Flags |Notes                         |";

   printf("+%s%s+\n%s%s\n%s%s\n+%s%s+\n",dash38,dash38,left1,right1,
           left2,right2,dash38,dash38);

}

explain()
{
/* Tell what the program does, and how to get out */
   printf("ScanChek 4.1 (20 Jan 89)\n\n");
   printf("This program displays the scan code, ASCII code, and\n");
   printf("the Kermit 'key ident' used in the Kermit-MS SET KEY command. ");
   printf("Keycodes are \nobtained from the IBM-PC keyboard BIOS interrupt");
   printf(" 16H, function 0 or 0x10.\n");
   printf("Do not type ahead of the display.\n\n");
   printf("Key    - The key pressed. Determined from ascii value or");
   printf(" BIOS scan code.\n");
   printf("Scan   - The BIOS scan code (ah).\n");
   printf("ASCII  - The ASCII value (al).\n");
   printf("E      - * if available on enhanced keyboard only.\n");
   printf("KeyIdnt- The Kermit-MS v2.32 key ident.\n");
   printf("Flags  - Flags present in KeyIdnt. Valid only if \"Kermit\" field");
   printf(" is > 255\n         (E=Enhanced, A=Alt, C=Ctrl, S=Shift)\n");
   printf("E      - * if key ident enhanced bit is set.\n");
   printf("\nPress Control-BREAK to exit.\n\n");
}

bioskey(function)                       /* For most C compilers [jrd] */
int function;                           /* do Interrupt 16H via int86() */
{
   static union REGS reg;                       /* for Bios int86 calls */
   reg.h.ah = function;
   int86(0x16, &reg, &reg);
   return (reg.x.ax);
}

