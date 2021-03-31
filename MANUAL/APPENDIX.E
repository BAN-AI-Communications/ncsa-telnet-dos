E.1     NCSA Telnet


Key-mapping     E.1


National Center for Supercomputing Applications


June 1991

                                                              


June 1991





Appendix  E     Key-mapping



Overview
NCSA Telnet for the PC incorporates many of the key-mapping 
elements found in the MS-Kermit program. Because of this 
adaptation, we've included the following Appendix, which except 
for minor changes, contains excerpts that appear exactly as they 
did in the MS-Kermit documentation. Copyright information 
granting the use of this segment is cited below.


Copyright Information
Copyright (C)  1981, 1988
Trustees of Columbia University in the City of New York

Permission is granted to any individual or institution to use, copy, 
or redistribute this document so long as it is not sold for profit, and 
provided this copyright notice is retained.


Kermit Verb Usage
NCSA Telnet contains a subset of the Kermit verbs listed below. 
Kermit verbs with a star are supported while unmarked verbs will 
cause an error if they are in a keyboard mapping file. Also 
included is a utility from the MS-Kermit 2.32 distribution which 
will determine what the actual kermit key code is for any 
combination of keys. Use this in the telnet keyboard mapping file 
as the code to map from. See the telnet.key file included with this 
release for an example keyboard mapping file.

NCSA Telnet does not support the extended SET KEY options such 
as:

SET KEY ON, SET KEY OFF, or SET KEY CLEAR

Also, it is impossible to map the ALT+alphabetical keys to 
anything with the keyboard mapping interface. NCSA Telnet traps 
these internally before they actually reach the mapping interface.


Set Key

Syntax
SET KEY key-specified {key-definition}


Uses of SET KEY
¥       You're used to having the ESC key in the upper-left corner of the 
keyboard, but your new PC keyboard has an accent grave (" ' ") 
key there. You can use SET KEY to make the accent key 
transmit an ESC, and you can assign accent grave to some other 
key.

¥       You send a lot of electronic mail, and always sign it the same 
way. You can put your "signature" on a single key to save 
yourself a lot of repetitive typing.

¥       You must set up your PC's function keys or numeric keypad to 
work properly with a host application.

The SET KEY command does these things and more, while SHOW 
KEY gives us assistance. A key can be defined to:

¥       send a single character other than what it would normally send
¥       send a string of multiple characters
¥       send itself again

SET KEY specifies that when you press the designated key during 
terminal emulation, the specified character or string is sent or the 
specified Kermit action verb is performed.

Using the SET KEY command, you create a key-specifier. The 
key-specifier is the identification of the key expressed in system-
dependent terms. This key can be a letter, such as Q for the key 
which produces an uppercase Q, or the numeric ASCII value of the 
letter in backslash notation (e.g., "\81"), or else the numerical 
"scan code: observed by the system when the key is pressed (e.g., 
"\3856" for CTRL-ALT-SHIFT-Q on an IBM PC). Material 
printed on keycaps is not necessarily a guide to what the key-
specifier should be.

A string definition is one or more characters, including 8-bit 
values expressed in backslash form, such as

SET KEY \315 directory\13       IBM F1 key sends        
                "directory<cr>"

SET KEY S  X    S key sends upper case X
                (a mean trick)

SET KEY T  \27 [m       T key sends three bytes:  
                        ESC, [, m

SET KEY \2336  {del } xxx       ALT-D sends "del"
SET KEY \324  \Kexit    F10 escapes back to Kermit-MS> 
                prompt.

The string begins with the first non-spacing character following 
the key identification and continues until the end of line, 
exclusive of any trailing spaces. You can use curly braces, {...} 
can be used to delimit the string in case you want the definition to 
include trailing spaces. The program ignores all text after the 
closing bracket.

This manual does not contain a list of all the scan codes for all the 
keys on all the keyboards on all the PCs supported by NCSA Telnet. 
In order to obtain the scancode for a key, you must use the scanchek 
utility.


Kermit Action Verbs

An action verb is the shorthand expression for a named Kermit 
procedure, such as "generate the proper sequence for a left arrow," 
"show status," "send a BREAK," and others; verbs are complex 
actions and each verb has a name. In a key definition precede the 
verb name by a backslash K (\K) to avoid being confused with a 
string. You cannot assign verbs and strings together on a key.

Entering the commands:

SET KEY \331 \Klfarr
SET KEY \2349 \Kexit

makes the IBM keyboard left arrow key execute the verb named 
lfarr, which sends the proper escape sequence for a VT102 left 
arrow key (which changes depending on the internal state of the 
VT102). The leading \K identifies the definition as a Kermit verb, 
so no string can start as \K or as \{K in upper or lower case (use 
\92K). The second example has ALT-X invoking the Leave-
Connect-Mode verb "exit" (same as Kermit escape  character "^]" 
followed by C).

Each system has its own list of verbs and predefined keys. Table 
E.1 shows those available for the IBM PC family (there are also 
some additional verbs for reassigning Heath or VT100 function 
keys, see section 1.17.2 of the original Kermit documentation). 
The SET KEY command shows the list of available verbs when a 
query mark (?)  is given as a definition. 

Table E.1       MS-Kermit Verbs 
for the IBM PC 
Family
Verb    Meaning

\Kupscn Roll up (back) to previous screen
\Kdnscn Roll down (forward) to next screen
\Khomscn        Roll up to top of screen memory
\Kendscn        Roll down to end of screen memory (current 
        position)
\Kupone Roll screen up one line
\Kdnone Roll screen down one line
\Kprtscn        Print the current screen
\Kdump  Append the current screen to dump file
\Kholdscrn      Toggle hold screen mode
\Klogoff        Turn off session logging
\Klogon Turn on session logging
\Ktermtype      Toggle terminal type
\Kreset Reset terminal emulator to initial state
\Kmodeline      Toggle modeline off/on
\Kbreak Send a BREAK signal
\Klbreak        Send a "long BREAK" signal
\Khangup        Drop dTR so modem will hang up phone
*\Knull Send a null (ASCII 0)
\Kdos   "Push" to DOS
\Khelp  Display CONNECT help message
\Kstatus        Display STATUS message
\Kterminals     Invoke user-defined macro TERMIANLS, if any
\Kterminalr     Invoke user-defined macro TERMINALR, if any
\Kexit  Escape back from CONNECT mode
*\Kgold, \Kpf1  VT102 keypad function key PF1
*\Kpf2..\Kpf4   VT102 keypad function keys
*\Kkp0..\Kkp9   VT102 keypad numeric keys

Other VT102 keypad keys:
*\Kkpdot, \Kkpminus, \Kkpcoma, \Kkpenter

VT102 cursor (arrow) keys:
*\Kuparr, \Kdnarr, \Klfarr, \Krtarr


NOTE:  * signifies that NCSA Telnet 2.3 supports the verb.


