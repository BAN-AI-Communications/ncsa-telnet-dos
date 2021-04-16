# NCSA PC Telnet FAQ

## **NCSA Telnet** Frequently Asked Questions

_**NOTE:**_ **NCSA Telnet** for _DOS_ development and technical
support has been discontinued, effective Feb. 1, 1995.

### What are the system requirements for Telnet?

A iX286 running a minimum of _DOS_ 2.0 or later using a packet driver for
the Network adapter or modem.

### What are packet drivers?

A packet driver is program that allows Telnet to send and receive data
packets through a network adapter or communications card. For example,
the 3Com509 card is one such network adapter or comm card.

**NCSA Telnet** offers internal support for \"some\" of the many
communications cards that are in use. The cards that Telnet supports
directly are:

- 3Com501, 3Com503, 3Com523
- Western Digital WD8003A, WD8003E, WD8003EB
- AT&T Starlan 10
- Ungermann-Bass PC-NIC, Ungermann-Bass NICps/2
- Micom NI5210

Most of these cards were built during the _Stone Age_ (_1988_-_1991_\'ish) and
are probably associated with the iX286 and early iX386 systems. Due to
the increasing number of cards that came to market and the creation of
the Packet Driver Specification (_PDS_) developed by _FTP_ _Software_, we got
out of driver development. If you have a card other than the few
mentioned above, you will need to download a packet driver. These
drivers are freely available at the anonymous ftp site oak.oakland.edu
in the /SimTel/msdos/pktdrvr directory.

### How do I use packet drivers?

To use a packet driver with **NCSA Telnet**, you would install the driver
according to its instructions. Next edit your `config.tel` file to reflect
the following information:

1. Ensure all other `hardware=` options are commented out. To comment out
   a line in you `config.tel` place the pound symbol, \#, at the
   beginning of the line.
2. Add or Edit these lines:
   - `hardware=packet`
   - `ioaddr=[software interrupt of the driver]`

For example if you load the packet driver using 0x60 for the software
interrupt, the entry would read:

- `hardware=packet`
- `ioaddr=60`

For more information about which software interupt to select, see the
installation instruction of your packet driver for the default setting.
If you still cannot determine the software interrupt, most drivers
provide help if you attempt to invoke the driver without any options or
with the `/help` or `/?` switch. For example, if you use the 3c509 driver,
cd to the directory that contains the driver and enter the following
command at the _DOS_ prompt:

`3c509` or

`3c509 /help` or

`3c509 /?`

### How do I run Telnet with a Novell network?

NCSA doesn\'t use a Novell LAN. However, one of our users wrote:

There is a packet driver that sits on top of the _ODI_ interface called
`odipkt`. `odipkt.com` is available from _hsdndev.harvard.edu_
(_128.103.202.40_) in `/pub/odipkt`.

There is a sample `net.cfg` file in that dir. A note should be made that
the order of the envelope statements is the order that they are
assigned. (I found the order _backwards_ from the documentation).

Load:

- `lsl` (odi driver from vendor or from wsgen disk odi dir)

- `odipkt 1` (If envelope for ethernet_ii is the second one in the `net.cfg` file, `odipkt 0` otherwise)

- `ipxodi`

- `netx`

Now _Telnet_ and _FTP_ work fine while connected to the Novell network.

### Will Telnet run in Windows?

NCSA\'s PC Telnet was not designed to be run in MS Windows. Some users
have reported success when they increase the size of memory available in
the _PIF_ file\....others have not gotten it to work at all. You must also
check the background option when you load Telnet.

If you are looking for a native Windows telnet application, try _WinQVT_.
This application is a \"Shareware\" program that offers _Telnet_, _FTP_,
_News_, _Mail_, and _LPR_ utilities. You can find the latest copy of this
program at the anonymous ftp site _oak.oakland.edu_ in the
`/pub/win3/winsock` directory. We also keep a copy of this file on our
anonymous ftp server. You can find `qvtws398.zip` in the
`/PC/Windows/Contrib` directory of _ftp.ncsa.uiuc.edu_.

### Does Telnet support TN3270 teminal emulation?

NCSA\'s PC Telnet does not support _TN3270_. However, Clarkson University
has modified our source code to support the _TN3270_ and they have
released it as _CUTCP_. For more information about the latest release of
_CUTCP_, see the `readme` file available from the anonymous ftp server
_omnigate.clarkson.edu_. You can find the file in the `/pub/cutcp`
directory.

### Can I obtain and modify the source code?

NCSA\'s PC Telnet source code is in the **public domain**, and you are
welcome to **modify and redistribute** it.

The source code was developed using Microsoft\'s
C/C++, version 7.0 and MASM 5.0.

### Can I use Telnet over a serial connection (modem)?

Yes, you can find a number of _SLIP_ drivers and dialers available from
the `/SimTel/msdos/pktdrvr` directory of the anonymous ftp site
_oak.oakland.edu_. Select a _SLIP_ packet driver from the 00_index.txt file.

### Can I use Telnet with AppleTalk?

Using an _AppleTalk_ network involves some special considerations. First,
you must load the _AppleTalk_ driver into memory. Version 1.0 of the
`ATALK.EXE` driver was used in the development of **NCSA Telnet**.

The second consideration involves the `interrupt=` line. The
`interrupt=` line in your `CONFIG.TEL` file refers to the software
interrupt the AppleTalk driver is using, not the hardware interrupt the
card is set to. For example, if your AppleTalk card is set to IRQ2, you
should NOT set the `interrupt=` line to `2`. Instead, the value
should be set to the software interrupt, usually `interrupt=60` or
`interrupt=5C`.

Static addressing does not work at the current time in **NCSA Telnet** v2.3
using the AppleTalk driver. Therefore, **NCSA Telnet** ignores any IP
address you set in your `CONFIG.TEL` file, and assigns an IP address to
your PC by the AppleTalk gateway.

Some AppleTalk users have been more successful with _v2.3.03_ of Telnet.
If you would like to try _v2.3.03_, it\'s available on our anonymous ftp
server in the `/Telnet/DOS/contributions` directory.

One of our users wrote:

To load Telnet from the DOS prompt, with nothing Telnet-specific in
the `config.sys` or `autoexec.bat`, we use the following sequence:

```text
lsl.com
ltalk.com
atalk.com
ashare.com
compat.com
d:\network\telnet\telbin -n -h d:\network\telnet\config.tel
```

where all of the atalk stuff would be in the current directory and all
of the telnet stuff is in `d:\network\telnet`.

```text
broadcast=255.255.255.255
netmask=255.255.255.0
hardware=atalk # network adapter board (AppleTalk)
interrupt=60   # I have an Apple or Farralon card and PhoneNET Talk
               # remember to run COMPAT.COM for NCSA to run on LocalTalk
#interrupt=5C  # I have a TOPS Flashcard
mtu=512        # maximum transmit unit in bytes
maxseg=512     # largest segment we can receive
rwin=512       # most bytes we can receive without ACK
```

### What do I do when Telnet is running out of memory?

The latest version of Telnet takes around 400k to run. If you have
memory problems, reduce or comment out the number of lines that set the
scrollback buffer in the the `config.tel` file. For example:

`scrollback=100 # number of lines of scrollback per session`
`# Default is (0)`

### How do I scroll back the screen?

You can scroll in one line increments using the scroll lock. Press the
scroll lock in and use the arrow keys to increment by one line. To turn
off this feature, just turn off the scroll lock.

### How do I remap keys?

When remapping keys refer to _Appendix E_ of the 2.3 docs. There is also
information in chapter 7, _Installation and Configuration_, page 7.8 and
the sample `config.tel` file we provide with Telnet.

From Chapter 7:

- `keyfile=filename`
  - Specifies an additional keyboard mapping file to provide move key definitions. This file over-rides the definitions in the telnet.key file.

From the `config.tel` file:

- `keyfile=keymap.key`
  - Pathname of your keyboard re-mapping file.

Re-mapping will override the default `telnet.key` file and you will have
to include all keys in this new file. You can easily do this by copying
the contents of the telnet.key file into the newfile and then add the
key remap information.

### Can you explain rwin, mss, and mtu in the `config.tel` file?

Rwin is the TCP sliding window. The window allows transfers to proceed
without waiting for an acknowldgement for every packet, but rather
transmitting (up to) a window of data before waiting for acknowlegments.
If the window size equals the mss, then there is no window, because
every packet must be acknowledged. If large windows are selected and
data is lost, the entire window may have to be resent, hence the warning
that larger is not always better. During a transfer, both sides of the
connection continually advertise what their free space in the window is,
so that the transfer side can control data flow to only send what the
receiving side can accept.

Mss is the maximum segment size that the TCP connection advertises to
the other side. The other side then sends packets up to this size. In
_FTP_, all of the data packets except the last will probably be this big.

Mtu is the maximum size of outgoing packets on the TCP connection. When
transmitting _FTP_ data to a host, packets will be this big, unless the
host advertizes a smaller mss. For Telnet and the _FTP_ control
connection, packets are sent per character, so this is never an issue.

### Can I capture what I see into a file or send it to the printer?

Text that appears on the screen can be captured and sent to a file or
the local printer. When you press _Alt-C_ capture is turned on. Pressing
_Alt-C_ again turns capture off. Any text that appears on the screen is
captured and appended to the capture file. **NCSA Telnet** never erases the
capture file, only appends to it. The default capture file is
`capfile`, but this may be changed by using the Parameter menu
(_Alt-P_). Changing the capture filename to prn will send all captured
text to a local printer.

**Note:** you can capture text to a file or the printer in any session,
but not for more than one session at a time. When capture is active for
a session in the background, you cannot invoke capture in the current
session.

If you press _Alt-D_, **NCSA Telnet** dumps the contents of the current
session screen into the capture file.

**NOTE**: You cannot paste from the capture file into a Telnet session.

### How do the cut and paste features of **NCSA Telnet** work?

The cut and paste functions allow you to copy blocks of text from one
session to another, or within the same session. To use these functions,
follow these steps:

1. Enter Scrollback mode via Scrl_Lock or the right mouse button.
2. Move the cursor to the beginning position of the text you wish to
   copy, and press the space bar.
3. Move the cursor to the end of the text and press the space bar
   again. This action selects the area to copy.
4. Press _ALT-C_ while still in scrollback mode. This action copies the
   text into a buffer.
5. Exit Scrollback mode, and switch sessions by pressing _ALT-N_ or
   _ALT-B_, if you desire.
6. Position the cursor where you wish to insert the text, and press
   _ALT-V_. This action inserts the text at the current position as if
   you had typed it in.

You can use the copy and paste functions without touching the keyboard
if you have a Microsoft-compatible mouse attached and driver loaded.

1. Press the right mouse button to enter Scrollback mode.
2. Scroll to the beginning of the text you wish to copy, and press the
   left mouse button.
3. Move the cursor to the end of the text, and press the left mouse
   button again.
4. Press and hold the left mouse button, press the right mouse button,
   then release both buttons. This action copies the text into the
   buffer.
5. Exit Scrollback mode by pressing the right mouse button.

To paste the buffer to the screen, press and hold the right mouse
button, then press the left mouse button, then release them both. The
text should appear as if you typed it in.

**Note:** You cannot paste from the capture file. The capture file and
scrollback buffer are NOT the same.

### Can I FTP or rcp to my PC from a remote site?

Yes, if you are using Telnet\'s server mode. Invoking telnet with the -s
option enters the server mode. In this mode, you can establish remote
connections as usual with telnet, except that telnet stays active even
when all remote sessions have been closed. The reason for this is that
when in server mode Telnet waits for external _FTP_ and _rcp_ requests. This
allows you to leave your PC and access files there from a remote
location.

### Can I restrict access to my machine when in server mode?

If you want to restrict access to your machine when in server mode, you
will need to run telpass and create a password file. Start up Telpass
from _DOS_ with the name of the password file that you wish to edit.

Example:

- `C:\telpass pwfile`

Follow the instructions in Telpass to create/edit the password file.

Once you have a password file, you must add a passfile option to your
`config.tel` file so that Telnet knows where to find the password file.
You must include the full path to the password file.

Example:
`passfile="c:\bat\ftppass"`

### Can I permanently change my screen colors?

You can make color changes permanent for particular sessions. To do so,
simply add the options below to the host information contained in the
`config.tel` file.

- `nfcolor=white`

  - (normal forground)

- `nbcolor=black`

  - (normal background)

- `rfcolor=black`

  - (reverse forground)

- `rbcolor=white`

  - (reverse background)

- `ufcolor=blue`

  - (underline forground)

- `ubcolor=black`
  - (underline background)

Put these options after the keyword name, to associate colors to the
named session. The parameters are installed whenever a connection is
opened with that session name.

For machines with EGA or better graphics adapters, the following colors
are also available:

`BLACK`, `BLUE`, `GREEN`, `CYAN`, `RED`, `MAGENTA`, `YELLOW`, `WHITE`

These colors are in all caps, and for the forground colors they are the
highlighted version of the lowercase colors. For background colors, they
make the foreground blink.

### What does the Local host or gateway not responding error mean?

The possible reasons for this message are:

1. Network problem
2. Configuration file problem
3. Host is down
4. Gateway is down

The possible solutions to this problem are:

1. Check to see that the network is up and running
2. If the computer is not on your local network, check to see if the
   gateway is up and running.
3. Ask the system administrator to check the specification of the
   gateway (`gateway=`) in your configuration file.
4. Check the IP number of the computer you are trying to connect to.
5. Check to make sure that your computer is attached to the network.
6. Check the integrity of the network cable.

### Why can\'t I get BOOTP working on the latest version of telnet?

We have heard reports that _BOOTP_ may still be broken, so that some
systems cannot use it. If you are having problems, you may want to try
_2.3.03_ instead. This version is available via our ftp site in the
`/Telnet/DOS/contributions` directory.

---

© 2003 Board of Trustees of the University of Illinois. All rights reserved.
