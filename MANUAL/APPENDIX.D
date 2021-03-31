D.2     NCSA Telnet


FTPBIN Command Reference        D.3


National Center for Supercomputing Applications


June 1991

                                                              


June 1991





Appendix D      FTPBIN Command Reference



        This appendix describes the FTPBIN commands that 
        NCSA Telnet uses.

Command Action

?       prints  help information.

!       escapes to a MS-DOS shell.

account allows entering of account data for hosts which 
        require this information to log in.

ascii   sets the transfer type to ASCII--this command will 
        do some translation, so do not use this mode to 
        transfer files which are not ASCII text.

bget    gets (transfers from remote machine to local PC) a 
        specified file in binary mode.

binary  sets the transfer type to binary--this mode will do 
        no translation of end-of-line characters, etc., so do 
        not use it to transfer ASCII text files.

bput    puts (transfers from local PC to remote host) a 
        specified file in binary mode.

bye     closes any open session and quits FTPBIN.

close   closes any open session, but does not exit FTPBIN.

delete  deletes a file on the remote machine. Prompts for 
        confirmation if prompting is on.

debug   sets the level of debugging (default 0). This 
        is mainly for developmental purposes, and 
        should not be necessary for normal use.

dir     gives a directory listing of the remote directory.

get     gets (transfers form remote host to local PC) a 
        single specified file.

glob    toggles expansion of wildcard characters in local 
        filenames.

hash    toggles hashing mode.  If hashing mode is on, 
        every time a full buffer (usually 1024 bytes-1K 
        bytes) is transferred, a "#" symbol will be printed.
help    performs the same function as the "?" command.

interactive     puts FTPBIN into interactive mode--i.e. turns 
        prompting on.

lls     gives a listing of the local directory.

ls      gives a listing of the remote directory (same as 
        dir).

mdelete sets multiple deletion of remote files.

mdir    lists of multiple remote directories (i.e. runs "ls" on 
        more than one remote directory).

mget    gets (transfers from remote host to local PC) 
        multiple files. For example, "mget*" will copy all 
        files in the current remote directory to the current 
        local directory.

mkdir   makes a directory on the remote host's drive.

mls     lists the contents of multiple remote directories.

mode    sets file transfer mode. Currently, only stream 
        mode is supported, so this command does not 
        function.

more    toggles (or expressly turns "on" or "off", if "on" 
        or "off" is specified after the more command) the 
        "more" paging of directories.  That is, if "more" is 
        turned on, you will see listed directories a page at a 
        time, instead of all of the data being scrolled 
        through without stopping.

mput    puts (transfers from local PC to remote host) 
        multiple files. For example, "mput*" will transfer 
        all files in the current local directory to the current 
        remote directory.

noninteractive  puts FTPBIN into non-interactive mode--i.e. turns 
        prompting off.

open    opens a connection to the specified host.

prompt  toggles the current prompting mode--if prompting 
        is on, it is turned off, and if it is off, it is turned on. 
        Prompting means that FTPBIN will prompt you 
        or every file if you request a multiple file transfer, 
        deletion, or directory.

put     puts (transfers from local PC to remote host) a 
        single specified file.

pwd     displays the current directory on the remote 
        machine.
quit    closes any open session and quits FTPBIN. (same 
        as the command "bye").

quote   sends an arbitrary FTP command.

recv    receives a file.

remotehelp      queries the remote host for help information.

rename  renames a file on the remote host.

rm      deletes a file on the remote host.

rmdir   deletes a directory on the remote host.

send    performs same functions as the "put" command--
        transfers a file from the local PC to the remote 
        host.

sendport        toggles the use of the FTP PORT command for 
        each data connection/transfer.

site    supports the SITE command for hosts that require 
        SITE information.

slashflip       toggles changing of "\" on outgoing commands.

status  displays the current status of FTPBIN--i.e. 
        if connected (and to where), current transfer 
        mode, debugging level, prompting mode, 
        slashflipping, verbosity, more mode-basically, the 
        status of anything that can be changed in an 
        FTPBIN session.

struct  sets the file transfer structure. Currently only file 
        structure is supported.

type    sets transfer type.  See "binary" and "ascii" 
        commands.

user    prompts for user information (i.e. login ID and 
        password)-this command is useful if your first 
        login attempt fails. Type user and retype your 
        login information.

verbose toggles verbose mode (i.e. sets the amount of 
        information you will receive on your display).

