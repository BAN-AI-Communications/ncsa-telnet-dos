F.1     NCSA Telnet 


Obtaining NCSA Software F.1


National Center for Supercomputing Applications


June 1991


                                                              


June 1991





Appendix  F     Obtaining NCSA Software



Overview
This appendix uses NCSA HDF to outline the procedures for 
obtaining NCSA software via FTP, an archive server, or by 
regular mail.


Obtaining NCSA Software

FTP
If you are connected to Internet (NSFNET, ARPANET, MILNET, 
etc.) you can download HDF software and documentation at no 
charge from an anonymous file transfer protocol (FTP) server at 
NCSA. The steps you should follow to do so are enumerated below. 
If you have any questions regarding the connection or procedure, 
consult your local system administrator or network expert.

1.      Log on to a host at your site that is connected to Internet and is 
running software supporting the FTP command.

2.      Invoke FTP on most systems by entering the Internet address of 
the server:

        %  ftp  ftp.ncsa.uiuc.edu

        or

        %  ftp  141.142.20.50

3.      Log in by entering anonymous for the name.

4.      Enter your e-mail address for the password.

5.      Enter get README.FIRST to transfer the instructions file 
(ASCII) to your local host.

6.      Enter quit to exit FTP and return to your local host.

7.      Review the README.FIRST file for complete instructions 
concerning the organization of the FTP directories and the 
procedures you should follow to download the README files 
specific to the application you want.

Your login session should resemble the following sample, where 
the remote user's name is smith and user entries are indicated in 
boldface type.
harriet_51%  ftp ftp.ncsa.uiuc.edu
Connected to zaphod.
220 zaphod FTP server (Version 4.173 Tue Jan 31 08:29:00 CST 
1989) ready.
Name (ftp.ncsa.uiuc.edu: smith):  anonymous
331 Guest login ok, send ident as password.
Password:  smith
230 Guest login ok, access restrictions apply.
ftp>  get README.FIRST
200 PORT command successful.
150 Opening ASCII mode data connection for README.FIRST (10283 
bytes).
226 Transfer complete.
local:  README.FIRST  remote:  README.FIRST
11066 bytes received in .34 seconds (32 Kbytes/s)
ftp>  quit
221 Goodbye.
harriet_52%


The README.FIRST file instructs you to copy the HDF README 
file to your directory and read it before proceeding. Your FTP 
session should resemble the one listed below:

ftp>    cd HDF
250 CWD command successful.
ftp>    get README
200 PORT command successful.
150 Opening ASCII mode data connection for README (10283 bytes)
226 Transfer complete.
local:  README  remote:  README
2080 bytes received in .14 seconds (15 Kbytes/s)
ftp>    quit
221 Goodbye.
harriet_52%


The HDF README file explains how to copy the contents of the 
HDF directory to your home directory via remote login or 
anonymous FTP. The precise file transfer procedure varies 
according to the type of operating system under which you will use 
HDF—UNICOS or other.


Archive Server
To obtain NCSA software via an archive server:

1.      E-mail a request to:

        archive-server@ncsa.uiuc.edu

2.      Include in the subject or message line, the word "help."

3.      Press RETURN.

4.      Send another e-mail request to:

        archive-server@ncsa.uiuc.edu

5.      Include in the subject or message line, the word "index."
6.      Press RETURN.

For example, if you use the UNIX mailing system, your login 
session should resemble the following sample, where user entries 
are indicated in boldface type.

yoyodyne_51% mail archive-server@ncsa.uiuc.edu
Subject:  help

.
EOT
Null message body; hope that's ok
yoyodyne_52% mail archive-server@ncsa.uiuc.edu
Subject:  index
.
EOT
Null message body; hope that's ok


The information you receive from both the help and index 
commands will give you further instructions on obtaining NCSA 
software. This controlled-access server will e-mail the 
distribution to you one segment at a time.


Mail
HDF software and manuals are available for purchase—either 
individually or as part of the anonymous FTP reel or cartridge 
tapes—through the NCSA Technical Resources Catalog. Orders 
can only be processed if accompanied by a check in U.S. dollars 
made out to the University of Illinois. To obtain a catalog, contact:

NCSA Documentation Orders
152 Computing Applications Building
605 East Springfield Avenue
Champaign, IL  61820
(217) 244-0072



