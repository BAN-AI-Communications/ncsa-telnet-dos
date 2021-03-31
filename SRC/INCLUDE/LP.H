/*
*	Header file for Line Printer Utilities
*/

#ifndef LP_H
#define LP_H

#define MAX_PRIV_PORT   1023    /* privileged TCP ports are 0..1023 */
#define PRINTER_PORT    515     /* port number for LPD daemon */
#define DEFAULT_PRINTER "lp"    /* standard Unix default */

/* LPD request codes */
#define LPD_CHECK_QUEUE             '\1'
#define LPD_PRINT_JOB               '\2'
#define LPD_DISPLAY_SHORT_QUEUE     '\3'
#define LPD_DISPLAY_LONG_QUEUE      '\4'
#define LPD_REMOVE_JOB              '\5'
#define LPD_END_TRANSFER            '\0'
#define LPD_ABORT                   '\1'
#define LPD_RECEIVE_CONTROL_FILE    '\2'
#define LPD_RECEIVE_DATA_FILE       '\3'

/* LPD return codes */
#define LPD_OK                  '\0'
#define LPD_ERROR               '\1'
#define LPD_NO_SPOOL_SPACE      '\2'

extern int debug;     /* 1 = print debugging info, 0 = no info */

#endif
