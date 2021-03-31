/*
*
*
*      File Transfer Protocal (FTP) definitions
*              (ftppi.c)
*  
*    National Center for Supercomputing Applications
*      by Gaige B. Paulsen, additions by Quincey Koziol
*
*      This file contains the definitions of ftp related things.
*
*      Version Date    Notes
*      ------- ------  ---------------------------------------------------
*        2.2   ??????  NCSA Telnet 2.2 - GBP
*        2.3   900527  NCSA Telnet 2.3 - QAK
*
*/

#ifndef FTPPI_H
#define FTPPI_H

#define NCMDS		51

#define	QMARK  		 1
#define BANG		 2
#define ACCOUNT      3
#define ASCII		 4
#define BELL		 5
#define BGET		 6
#define BINARY  	 7
#define BPUT		 8
#define BYE  		 9
#define CD   		10
#define CLOSE		11
#define DEL     	12
#define DEBUG		13
#define DIR  		14
#define GET  		15
#define GLOB 		16
#define HASH 		17
#define HELP 		18
#define INTERACTIVE 19
#define LCD  		20
#define LLS			21
#define LS			22
#define MDELETE 	23
#define MDIR 		24
#define MGET 		25
#define MKDIR		26
#define MLS  		27
#define MODE 		28
#define MORE		29
#define MPUT 		30
#define NONINTERACTIVE 31
#define OPEN 		32
#define PROMPT  	33
#define PUT  		34
#define PWD  		35
#define QUIT 		36
#define QUOTE		37
#define RECV 		38
#define REMOTEHELP 	39
#define RENAME  	40
#define RM			41
#define RMDIR		42
#define SEND 		43
#define SENDPORT 	44
#define SITE		45
#define SLASHFLIP 	46
#define STATUS  	47
#define STRUCT  	48
#define TYPE		49
#define USER 		50
#define VERBOSE 	51

static char *ftp_cmdlist[]={
	"?",
	"!",
	"account",
	"ascii",
	"bell",
	"bget",
	"binary",
	"bput",
	"bye",
	"cd",
	"close",
	"delete",
	"debug",
	"dir",
	"get",
	"glob",
	"hash",
	"help",
	"interactive",
	"lcd",
	"lls",
	"ls",
	"mdelete",
	"mdir",
	"mget",
	"mkdir",
	"mls",
	"mode",
	"more",
	"mput",
	"noninteractive",
	"open",
	"prompt",
	"put",
	"pwd",
	"quit",
	"quote",
	"recv",
	"remotehelp",
	"rename",
	"rm",
	"rmdir",
	"send",
	"sendport",
	"site",
	"slashflip",
	"status",
	"struct",
	"type",
	"user",
	"verbose"
};

static char *helpstrings[]={
	"?		print local help information",
	"!		escape to the shell",
	"account    allows entering of account data",
	"ascii     	set ascii transfer type",
	"bell      	beep when command completed - toggle",
	"bget		get a file in binary mode",
	"binary    	set binary transfer type",
	"bput		put a file in binary mode",
	"bye       	terminate ftp session and exit",
	"cd        	change remote working directory",
	"close     	terminate ftp session",
	"delete    	delete remote file - inquires if prompting is on",
	"debug     	toggle/set debugging mode",
	"dir       	list contents of remote directory",
	"get       	receive file",
	"glob      	toggle metacharacter expansion of local file names",
	"hash      	toggle printing `#' for each buffer transferred",
	"help      	print local help information",
	"interactive    turn on prompting for multiple commands",
	"lcd       	change local working directory",
	"lls		list contents of local directory",
	"ls        	nlist contents of remote directory",
	"mdelete   	delete multiple files",
	"mdir      	list contents of multiple remote directories",
	"mget      	get multiple files",
	"mkdir     	make directory on the remote machine",
	"mls       	nlist contents of multiple remote directories",
	"mode      	set file transfer mode",
	"more       toggle more mode",
	"mput      	send multiple files",
	"noninteractive turn off prompting on multiple commands",
	"open      	connect to remote tftp",
	"prompt    	toggle interactive prompting on multiple commands",
	"put       	send one file",
	"pwd       	print working directory on remote machine",
	"quit      	terminate ftp session and exit",
	"quote     	send arbitrary ftp command",
	"recv      	receive file",
	"remotehelp	get help from remote server",
	"rename    	rename file",
	"rm		remove file",
	"rmdir     	remove directory on the remote machine",
	"send      	send one file",
	"sendport  	toggle use of PORT cmd for each data connection",
	"site		site command for batch submissions",
        "slashflip      toggle changing / to \\ on outgoing commands",
	"status    	show current status",
	"struct    	set file transfer structure",
	"type      	set file transfer type",
	"user       send new user information",
	"verbose   	toggle verbose mode"
};

#endif
