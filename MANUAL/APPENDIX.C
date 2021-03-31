C.1	NCSA Telnet 

Configuration File Summary	C.1


National Center for Supercomputing Applications

June 1991

                                                              

June 1991




Appendix C	Configuration File Summary



This appendix contains a quick reference list of the keyword 
values which NCSA Telnet allows in its configuration file. 

There are two sections in this appendix. The first list contains the 
PC configuration entries which apply to the overall program or to 
all sessions. The second section contains the list of attributes 
which you may assign to individual sessions. Each keyword has 
an example value and a one-line summary of its purpose. For 
details on allowed values, see Chapter 7, "Installation and 
Configuration." A template CONFIG.TEL file is included on the 
distribution disk.


Local Configuration

address=d000	gives segment address of the Ethernet 
	board's shared memory in hex.

arptime=3	gives local network timeout for ARP 
	requests.

bios=no	uses BIOS or write directly to screen.

broadcast=x.x.x.x	uses IP number for broadcast 
	packets.

capfile="\mycapture"	specifies a name for the capture file.

capfile=prn	specifies capture file is printer device 
	(PRN).

clock=off	displays clock on screen.

domain="ncsa.uiuc.edu"	gives default domain suffix.

domaintime=2	gives domain lookup timeout per retry.

domainretry=4	gives number of times to query domain 
	nameserver(s).

ftp=yes	enables or disables FTP server , default is 
	yes.

hardware=3C501	assigns type of Ethernet hardware to use:  
	3C501, PCNIC, NI5210, WD8003, 3C523, 
	NICPS2, etc. (See introduction for 
	complete list of Ethernet adapter board 
	list.)

hpfile=COM1	attaches HP plotter to COM1.

hpfile=hp.out	assigns filename for HPGL codes for 
	graphics to go to.

interrupt=3	gives interrupt number for the Ethernet 
	board.

ioaddr=360	gives I/O base address of the Ethernet 
	board in hex.

myip=BOOTP	queries the BOOTP server for my IP 
	address.

myip=RARP	queries the RARP server for my IP 
	address.

myip=10.0.0.51	assigns IP address to use for the PC.

netmask=255.255.255.0	sets subnet mask for your local 
	network. 

passfile="\mypasswd"	gives filename for FTP usernames and
	passwords.

psfile=ps.out	gives filename for PostScript output.

rcp=yes	enables and disables rcp server, default is 
	yes.

tekfile=tek.out	assigns filename for Tektronix 4014 codes 
	when output to disk.

tek=yes	enables and disables Tektronix 
	graphics mode.

termtype="vt100"	sends terminal type to all machines 
	you've attempted to connect to.

video=ega	assigns video display to use: cga, ega, 
	hercules, no9.

windowgoaway=yes	Close the telnet window without 
	prompting when a machine's session 
	closes.

wire=thin	determines which type of ethernet 
	connection (thin or thick) the 3C503 
	ethernet card is using.

Host-Specific Parameters

NOTE: The keyword name is special because it separates entries.

clearsave=yes	saves cleared screen into scrollback region.

contime=10	sets connection timeout in seconds.

copyfrom=nic	copies all missing parameters from a previous 
	session.

crmap=4.3BSDCRNUL	changes sets of codes for carriage return.

duplex=half	sets local echo, but sends all character 
	immediately.

erase=delete	sets BACKSPACE key to "backspace" or 
	"delete."

gateway=1	sets gateway precedence; starts at 1, goes up by 
	ones.

host=sri-nic.arpa	sets hostname or alternate name. Usually full 
	domain name.

hostip=10.0.0.51	sets IP address of the host. Required if 
	nameserver not used.

maxseg=512	sets limit on incoming packet size, given as TCP 
	MSS option.

mtu=512	establishes limit on outgoing packet size.

name=nic	sets session name, separates lists of 
	parameters for sessions.

nameserver=1	sets nameserver precedence; starts at 1, goes up 
	by ones.

nbcolor=black	gives normal background color.

nfcolor=white	gives normal foreground color.

rbcolor=white	gives reverse background color.

retrans=7	sets initial retransmit timeout in 18ths of a 
	second.

rfcolor=black	sets reverse, foreground color.

rwin=512	sets TCP receive window, can make up for low 
	performance hardware.

scrollback=100	establishes number of lines of scrollback for this 
	session.

ubcolor=black	designates underline, background color. Black, 
	blue, green, cyan, red, magenta, yellow, white.

ufcolor=blue	designates underline, foreground color.

