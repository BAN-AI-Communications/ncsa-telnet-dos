/*  -------------------------------------------------------------------
    lpr - line printer

    Used to print files on remote printers using the LPD protocol.
    Built on top of the NCSA TCP/IP package (version 2.2tn for MS-DOS).

    Paul Hilchey   May 1989

    Copyright (C) 1989  The University of British Columbia
    All rights reserved.

    history:
     8/17/89    Relax option parsing to allow spaces
     8/21/89    Expand wildcards in filenames
    10/23/89    Default title to filename with -p format
	  1/6/90		 Microsoft Port by Heeren Pathak (NCSA)
    -------------------------------------------------------------------
*/

#ifdef __TURBOC__
#include "turboc.h"
#include <dir.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <stdarg.h>
#include <io.h>
#include <time.h>
#ifdef MSC
#define EXIT_SUCCESS 0
#ifndef __TURBOC__
#define ffblk find_t
#define ff_name name
#define findnext _dos_findnext
#define findfirst(path,fileblock,attribute) _dos_findfirst((path),(attribute),(fileblock))
#endif
#include <signal.h>
#ifndef __TURBOC__
#include <direct.h>
#endif
#include <malloc.h>
#else 
#include <dir.h>
#endif

#define WINMASTER

#ifdef MEMORY_DEBUG
#include "memdebug.h"
#endif
#include "whatami.h"
#include "hostform.h"
#include "windat.h"
#include "lp.h"
#include "externs.h"

#define MAX_COPIES          10
#define DEFAULT_INDENT      8
#define CONTROL_FILE_SIZE   1024    /* max size in bytes */

/*  Function prototypes  */

void main(int argc,char * *argv);
static void start_protocol(char *host,char *rname);
static void finish_protocol(void );
static void print_file(char *filename);
static void print_one_file(char *filename);
static void send_file(int connection_id,FILE *data_file,char *spool_name,int is_text);
static void check_ack(int connection_id);
static long get_size_lpr(FILE *data_file,int is_text);
#ifndef __TURBOC__
static void randomize(void );
#endif

/*  global variables */

char *remote_name = NULL, /* printer name on remote system */
     *remote_host = NULL, /* address of remote host        */
     *class = NULL,       /* job classification            */
     *job = NULL,         /* job name                      */
     *title = NULL,       /* job title                     */
     filter = 'f';        /* default filter                */
int copies = 1,           /* number of copies              */
    indent = -1,          /* indent, -1 means not set      */
    width = 0,            /* width, 0 means not set        */
    noburst = 0;          /* 1 = skip burst page           */

int     connection_id;
char    control_file[CONTROL_FILE_SIZE];  /* control file to be sent after data files, built in a string */
int     cf_length = 0;      /* current length of control_file */
int     sequence_number;    /* sequence number for spooled file names */
struct config *cp;          /* configuration information */
char    username[9];        /* name of user */
int     debug = 0;          /* 1 = print debugging info; set with -D option */

int     ftppassword,        /* not used; just to avoid unresolved external */
		bypass_passwd=0;	/* whether to bypass the password check */

unsigned char path_name[_MAX_DRIVE+_MAX_DIR],		/* character storage for the path name */
	temp_str[20],s[_MAX_DIR],temp_data[30];


/****************************************************************
 *  Main program.                                               *
 *     lpr  [option ...] file1 ...                              *
 ****************************************************************/
void main(int argc,char *argv[])
{
    int i,first_time, temp;
    char *ptr;

#ifdef __TURBOC__
	fnsplit(argv[0],path_name,s,temp_str,temp_data);	/* split the full path name of telbin.exe into it's components */
#else
    _splitpath(argv[0],path_name,s,temp_str,temp_data); /* split the full path name of telbin.exe into it's components */
#endif
	strcat(path_name,s);	/* append the real path name to the drive specifier */

#if defined(MSC) && !defined(__TURBOC__)
	signal(SIGINT,breakstop);		/* Microsoft intercept of break */
#else
    ctrlbrk(breakstop);     /* set up ctrl-c handler */
#endif


    /* Do session initialization.  Snetinit reads config file. */
    ptr = getenv("CONFIG.TEL");
    if(ptr!=NULL)
		Shostfile(ptr);

	if(argc>1) {
		if(!(strcmp(argv[1],"-h")))
			Shostfile(argv[2]);
	}

    if(i=Snetinit()) {
		if(i==-3)		/* check for BOOTP server not responding */
			netshut();	/* release network */
		crash("network initialization failed.");
	  }	/* end if */

    first_time = 1;          /* reset once we've found something to print */

    /* select default printer */
    remote_name = getenv("PRINTER");
#ifdef AUX /* RMG */
fprintf(stdaux, " remote printer name is %s ",remote_name);
#endif
    if (remote_name == NULL) remote_name = DEFAULT_PRINTER;
#ifdef AUX /* RMG */
fprintf(stdaux, " remote printer name set to %s ",remote_name);
#endif

    remote_host = getenv("SERVER");

    /* Files sent to the remote system need to have a name of the
       form   df<letter><sequence number><host name>  (data files)
         or   cf<letter><sequence number><host name>  (control files).
       It isn't convenient for to keep track of the sequence number
       from when lpr was last used, so we just use a random number.
       It is unlikely but possible that we could get into trouble
       doing this.                                                    */
    randomize();
    sequence_number = rand() % 1000;

    /* get info from configuration file */
    cp = (struct config *)malloc(sizeof(struct config));
    Sgetconfig(cp);

    /* check that the machine name was set in the configuration file */
    if (0 == strlen(cp->me)) crash("`myname' not set in config file.");

    /* set user name.  use first part of machine name if nothing else. */
    ptr = getenv("USER");
    if (NULL != ptr) {
        strncpy(username,ptr,8);
        username[8]='\0';
    }
    else {
        i = min(strcspn(cp->me,"."),sizeof(username)-1);
        strncpy(username,cp->me,i);
        username[i]='\0';
    }

    /* Loop through command line arguments */
    for (i=1; i<argc; ++i)

        if (argv[i][0] == '-')        /* setting an option */

            switch(argv[i][1]) {

            case '#':                   /* set number of copies */
                if (argv[i][2])
                    temp = atoi(&argv[i][2]);
                else if (i+1 < argc)
                    temp = atoi(argv[++i]);
                else
                    temp = 1;

                if (temp < 1 || temp > MAX_COPIES) {
                    fprintf(stderr,"Unreasonable number of copies requested.  Reset to 1.\n");
                    temp = 1;
                }
                copies = temp;
                break;

            case 'P':                   /* set printer name */
                if (argv[i][2])
                    remote_name = &argv[i][2];
                else if (i+1 < argc)
                    remote_name = argv[++i];
                break;

            case 'S':                   /* select server */
                if (argv[i][2])
                    remote_host = &argv[i][2];
                else if (i+1 < argc)
                    remote_host = argv[++i];
                break;

            case 'C':                   /* set job classification */
                if (argv[i][2])
                    class = &argv[i][2];
                else if (i+1 < argc)
                    class = argv[++i];
                break;

            case 'J':                   /* set job name */
                if (argv[i][2])
                    job = &argv[i][2];
                else if (i+1 < argc)
                    job = argv[++i];
                break;

            case 'T':                   /* set job title */
                if (argv[i][2])
                    title = &argv[i][2];
                else if (i+1 < argc)
                    title = argv[++i];
                break;

            case 'i':                   /* set indent */
                temp = argv[i][2] ? atoi(&argv[i][2]) : DEFAULT_INDENT;
                if (temp < -1 || temp > 255)
                    fprintf(stderr,"Unreasonable indent requested.  Indent not changed.\n");
                else
                    indent = temp;
                break;

            case 'w':                   /* set page width */
                if (argv[i][2])
                    temp = atoi(&argv[i][2]);
                else if (i+1 < argc)
                    temp = atoi(argv[++i]);
                else
                    break;

                if (temp < 0 || temp > 512)
                    fprintf(stderr,"Unreasonable page width requested.  Width not changed.\n");
                else
                    width = temp;
                break;

            case 'p':   /* pr */
            case 'v':   /* raster */
            case 'c':   /* cifplot */
            case 'g':   /* graph */
            case 'd':   /* dvi */
            case 'n':   /* ditroff */
            case 't':   /* troff */
            case 'l':   /* long (with control characters) */
                filter = argv[i][1];
                break;

            case 'f':                   /* fortran */
                filter = 'r';  /* f selects filter r */
                break;

            case 'h':                   /* suppress burst page */
                noburst = 1;
                break;

            case 'D':                   /* turn debugging output on */
                debug = 1;
                break;

            default:
                fprintf (stderr,"Unrecognized option: %s ignored.\n",argv[i]);
                break;
            }
        else {                          /* name of file to print */
			if(!n_findfirst(argv[i],0)) {	/* check whether the file exists before trying to print it */
	            if (first_time) {           /* open connection on first job */
    	            if (NULL == remote_host) crash("server not specified.");
        	        if (NULL == job) job=argv[i];  /* default job name */
            	    start_protocol(remote_host,remote_name);
	                first_time = 0;
    	        }
        	    print_file(argv[i]);
			  }	/* end if */
			else {
                fprintf (stderr,"Cannot access: %s.\n",argv[i]);
			  }	/* end else */
        }

    if (first_time) {
        puts("Name:  lpr - send a job to a remote printer; version 1.3");
        puts("Usage: lpr  [ options ... ]  file1 ... ");
        puts("Options are:");
        puts("  -P<printer>     -S<server>      -C<class>      -J<job>");
        puts("  -T<title>       -i<indent>      -w<width>      -#<copies>");
        puts("  -p              -l              -d             -f");
        puts("  -h");
    }
    else {
        finish_protocol();
        puts("Done.");
    }
    netshut();
    exit(EXIT_SUCCESS);
}

/*****************************************************************
 *  start_protocol                                               *
 *  Open the TCP connection and start building the control file. *
 *  Aborts with an error message if someting goes wrong.         *
 *  parameters: null terminated name or IP address of the server *
 *              name of the printer on the server                *
 *****************************************************************/
static void start_protocol(char *host,char *rname)
{
    struct machinfo *server_info_record;

    server_info_record = lookup(host);
    if (0 == server_info_record) crash("domain lookup failed for %s.",host);

    netfromport(rand() % 1023);  /* source should be a privileged port */
    connection_id = open_connection(server_info_record, rand() % MAX_PRIV_PORT, PRINTER_PORT);
    if (0 > connection_id)
        crash("unable to open connection.");

    /* Tell LPD to receive a job. */
    nprintf(connection_id,"%c%s\n", LPD_PRINT_JOB, rname);

    if (debug) puts("told LPD to receive job, awaiting ack");
    check_ack(connection_id);

    cf_length = 0;
               cf_length += sprintf(control_file+cf_length,"H%s\n",cp->me);
               cf_length += sprintf(control_file+cf_length,"P%s\n",username);
#ifdef OLD_WAY
    if (job)   cf_length += sprintf(control_file+cf_length,"J%s\n",job);
               cf_length += sprintf(control_file+cf_length,"C%s\n",class ? class : cp->me);
               cf_length += sprintf(control_file+cf_length,"L%s\n",username);
#else
	if(!noburst) {
		if (job) 
			cf_length += sprintf(control_file+cf_length,"J%s\n",job);
		cf_length += sprintf(control_file+cf_length,"C%s\n",class ? class : cp->me);
		cf_length += sprintf(control_file+cf_length,"L%s\n",username);
	  }	/* end if */
#endif
}

/******************************************************************
 * finish_protocol                                                *
 * Called after last data file has been sent.  Sends the control  *
 * file then closes the network connection.                       *
 ******************************************************************/
static void finish_protocol(void )
{
    nprintf(connection_id,"%c%d cfA%03d%s\n", LPD_RECEIVE_CONTROL_FILE, cf_length,
            sequence_number, cp->me);
    if(debug)
		puts("Told LPD we're sending control file, awaiting ack.");
    check_ack(connection_id);
    netwrite(connection_id,control_file,cf_length);
    nprintf(connection_id,"%c",LPD_END_TRANSFER);
    if(debug)
		puts("Sent control file, awaiting ack.");
    check_ack(connection_id);

    netclose(connection_id);
}

/**************************************************************** *
 * print_file                                                     *
 * Changes any forward slashes in the filename to backwards ones, *
 * and then expands any wildcards before calling print_one_file.  *
 ******************************************************************/
static void print_file(char *filename)
{
    int done;
    struct ffblk ffblk;
    char *p;
    char *start_of_name;
    char *expanded_name;

    /* change forward slashes in filenames to suit backwards MS-DOS */
    p=filename;
    while (*p != '\0') {
        if (*p == '/') *p = '\\';
        p++;
    }

    /* make a copy of the path portion of the filename with room
       to substitute any expanded wildcard filenames at the end  */
    expanded_name = (char *)malloc(strlen(filename)+13);
    strcpy(expanded_name,filename);
    start_of_name = strrchr(expanded_name,'\\');
#ifdef OLD_WAY
    if (start_of_name == NULL)
       start_of_name = expanded_name;
    else
       start_of_name++;
#else
    if (start_of_name == NULL)
		if((start_of_name =strrchr(expanded_name,':'))!=NULL)	/* skip name past drive specifier */
			start_of_name++;
		else
			start_of_name=expanded_name;
    else
       start_of_name++;
#endif

    /* expand wildcards */
    done = findfirst(filename, &ffblk, 0);
    if (done)
        fprintf(stderr,"%s doesn't exist.\n",filename);
    else
        do {
            strcpy(start_of_name,ffblk.ff_name);
            print_one_file(expanded_name);
        } while (0 == findnext(&ffblk));

    free(expanded_name);
}


/******************************************************************
 *  Print a file                                                  *
 *  Adds records to the control file as appropriate for the       *
 *  various option settings, then sends the data file.            *
 ******************************************************************/
static void print_one_file(char *filename)
{
    char spool_file_name[40];
    int is_text,i;
    FILE *data_file;

    if (debug) printf("printing file %s\n",filename);

    /* for `non-binary' filters, we do a little bit of filtering beforehand */
#ifdef OLD_WAY
    is_text = (filter == 'f' || filter == 'l' || filter == 'p' || filter == 'r');
#else
    is_text = (filter == 'f' || filter == 'p' || filter == 'r');
#endif

    /* open the file in the appropriate mode (text or binary) */
    if (is_text)
        data_file = fopen(filename,"rt");
    else
        data_file = fopen(filename,"rb");

    if (data_file == NULL) {
        fprintf(stderr,"Trouble opening file %s.\n",filename);
        return;
    }
    printf("Sending %s ",filename);

    /* make up name for spooled data file */
    sequence_number = (sequence_number + 1) % 1000;
    sprintf(spool_file_name,"dfA%03d%s",sequence_number,cp->me);

    /* add records in control file describing job */
    if (width != 0)   cf_length += sprintf(control_file+cf_length,"W%d\n",width);
    if (indent != -1) cf_length += sprintf(control_file+cf_length,"I%d\n",indent);
    if (filter == 'p') cf_length += sprintf(control_file+cf_length,"T%s\n", (title != NULL) ? title : filename);
    for (i=1; i <= copies; i++)
        cf_length += sprintf(control_file+cf_length,"%c%s\n",filter,spool_file_name);
    cf_length += sprintf(control_file+cf_length,"U%s\n",spool_file_name);
    cf_length += sprintf(control_file+cf_length,"N%s\n",filename);

    send_file(connection_id, data_file, spool_file_name, is_text);
}

/****************************************************************
 * send_file                                                    *
 * Does the actual work of sending the file to the daemon.      *
 * parameters: the connection id, as returned from Snetopen     *
 *             a file handle for the data file to be sent       *
 *             the name of the spool file on the remote system  *
 *             a flag indicating if this is a text file         *
 ****************************************************************/
static void send_file(int connection_id, FILE *data_file, char *spool_name, int is_text)
{
/*    char buf[1024]; */
	char *buf;
    long length, step;
    int xp, towrite, i;
    int file_handle;

	if ((buf = (char *)malloc(1024)) == NULL) crash("Out of heap...");
    length = get_size_lpr(data_file, is_text);
    nprintf(connection_id, "%c%ld %s\n", LPD_RECEIVE_DATA_FILE, length, spool_name);
    check_ack(connection_id);

    file_handle = fileno(data_file);
    towrite = xp = 0;
    step = max(length / 5, 1);  /* print one dot for each 1/5 of the file sent */

    while(1) {
        if (towrite <= xp) {
            towrite = read(file_handle, buf, 1024);
            if (towrite == 0) break;
            xp = 0;
        }

        i = netwrite(connection_id,&buf[xp], towrite-xp);
        if (debug && i) printf("send_file sent %d bytes\n",i);
        Stask();
        checkerr();
        if (i < 0) crash("transfer failed.");

        if (i > 0) {
            int j;

            /* print out dots indicating how much sent */
            for (j=1; j<=(int)(((length+step-5L)/step - (length+step-(long)i-5L)/step)); j++)
                putchar('.');
            xp += i;
            length -= i;
        }
    }
    putchar('\n');
    if (length != 0)
        crash ("file length discrepancy.");

    /* send end mark */
    nprintf(connection_id, "%c", LPD_END_TRANSFER);
    check_ack(connection_id);
}

/************************************************************************
 * Return the size of a file.  For text files, we have to read through  *
 * the whole thing, as we are going to send LF as line terminator,      *
 * rather than CRLF.                                                    *
 ************************************************************************/
static long get_size_lpr(FILE *data_file, int is_text)
{
    char buf[1024];
    long total;
    int handle;
    int count;

    handle = fileno(data_file);
    if (is_text) {
        total = 0;
        while ((count = read(handle, buf, 1024)) > 0)
            total += count;
        rewind(data_file);
        return(total);
    }
    else
        return(filelength(handle));
}

/*********************************************************
 *  Wait for a one character reply from LPD.  Abort with *
 *  an error message if it not an OK acknowledgement.    *
 *********************************************************/
static void check_ack(int connection_id)
{
    char ack_buff[80];
    int len;

    while(1) {
       len = nread(connection_id, ack_buff, 80);
       if (len <= 0)
           crash("connection closed by server.");
       switch (ack_buff[0]) {
           case LPD_OK :
               return;
           case LPD_ERROR :
               crash("error code from server.");
           case LPD_NO_SPOOL_SPACE :
               crash("server unable to accept job at this time.");
           default :   /* may be an error message */
               fprintf(stderr,"%.*s",len,ack_buff);
        }
     }
}

#if defined(MSC) && !defined(__TURBOC__)
/******************************************************************
*
* randomize()
*
* replicates the randomize function of Turbo C
* MSC 5.1 does not contain it so we have to write it ourselves.
*
*/

static void randomize(void )
{
	srand((unsigned)time(NULL));
}
#endif

