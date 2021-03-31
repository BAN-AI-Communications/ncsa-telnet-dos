/************************************************************************
 *	services.c
 *
 *	part of NCSA Telnet
 *
 *		This file contains routines for figuring what service a tcp port 
 *		normally is used for.  It is used in telnet when a packet for an 
 *		invalid port is received to let the user know what service was 
 *		requested by what machine.
 *
 *		New 6/91	Jeff Wiedemeier
 */

#include <stdio.h>
#include <string.h>
#include "externs.h"

static char *get_name(int port);
static char *find_port(int port);
static char *services={"services.tel"};     /* Path to the services file */


/************************************************************************
 *	void inv_port_err(int service, int port, uint8 *ip)
 *
 *		This function has two purposes - 
 *			0 :		Display an error string based on information that
 *					was previously set - the port and ip arguments
 *					should be 0 when this is called in case of future
 *					use of these arguments for this service
 *
 *			1 :		set the port and source ip number for future service
 *					0 calls 
int service;         0 - display error string , 1 - set message info
int port;            should be 0 when service 0 is requested
uint8 *ip;           should be NULL when service 0 is requested
 */
void inv_port_err(int service,int port,uint8 *ip)
{
	static int portnum;
	static uint8 sourceip[4];
	char msg[81];

	if (!service) {		/* service 0 */
    sprintf(msg, "\t-Destination port %d (%s) - From host %d.%d.%d.%d\r\n",
                      portnum, get_name(portnum), sourceip[0], sourceip[1],
                      sourceip[2],sourceip[3]);
    tprintf(console->vs, msg);
  }
  else {      /* service 1 */
    int i;

		portnum = port;
		for(i = 0; i < 4; i++)
			sourceip[i] = *(ip + i);
	}
}

static char *get_name(int port)
{
	char *pname;

	if ((pname = find_port(port)) != NULL)
		return(pname);
	
  return ("");   /* just in case we get a NULL (should NEVER happen) */
}

static int is_service=1;

static char *find_port(int port)
{
	FILE *servfile;
	char line[101];
	static char name[30];
	int found = FALSE;
	int num;

	if (is_service == 0)
    return("unknown");

	if ((servfile = fopen(service_file(NULL), "r")) != NULL) {
    while((fgets(line, 100, servfile) != NULL) && !found) {
      sscanf(line,"%s %d/tcp", &name[0], &num);
			if ((line[0] != '#') && (num == port)) 
				found = TRUE;
    }

		fclose(servfile);
    if(found)
			return(name);
	} else {
    is_service = 0;
	}

	return ("unknown");
}

/****************************************************************************
 *	char *service_file(char *path)
 *
 *		This routine is used to maintain the path to the services file.
 *		If called with a path == NULL, it will return a (char *) to the
 *		current pathname of the services file.  Otherwise it will set 
 *		the pathname to the services file to path and return a pointer 
 * 		to the new name of the services file.
 */

char *service_file(char *path)
{
	if (path) 
		services = strdup(path);
	return(services);
}

