/*
*    externs.h
*
****************************************************************************
*                                                                          *
*                                                                          *
*      NCSA Telnet                                                         *
*      by Tim Krauskopf, VT100 by Gaige Paulsen, Tek by Aaron Contorer     *
*                                                                          *
*      National Center for Supercomputing Applications                     *
*      152 Computing Applications Building                                 *
*      605 E. Springfield Ave.                                             *
*      Champaign, IL  61820                                                *
*                                                                          *
****************************************************************************
*
*   External definitions for functions in NCSA Telnet.  This file contains
*	ALL of the external function definitions for Telnet, lets try to keep it
*	this way, ok?  This include file needs to go after other include files
*	which define variables in the functions.  It also needs to go after
*	#define's which define xxxMASTER.
*
*		Added - May 26, 1990 by Quincey Koziol
*/

#ifndef EXTERNS_H
#define EXTERNS_H

/*      #ifdef TELBIN   Put into makefile 3/30/93 -CGW
        #define MOUSE
        #endif
*/
#if defined(MSC) && !defined(__TURBOC__) && !defined(__WATCOMC__)
#define MK_FP(__o,__s) (void far *)(((unsigned long)(__s)<<16)|(unsigned)(__o))
#endif

#include <stdio.h>

#ifdef __WATCOMC__
#include "pragma.h"
#include <i86.h>
#define CDECL       cdecl
#else
#define CDECL
#endif

#ifdef MEM_DEBUG
#include "memdebug.h"
#endif

#ifdef MAL_DEBUG
#include "maldebug.h"
#endif

#ifdef STR_DEBUG
#include "strdebug.h"
#else
#define real_strncmp    strncmp
#endif

/* Global NCSA Telnet definitions */
#include "ncsa.h"

/* mcb.c */
extern void mcb(void );

/* mapout.c */
extern int read_output_file(char *output_file);
extern void initoutputfile(void );

/* keymap.c */
#include "keymap.h"
extern key_node *find_key(unsigned int c);
extern int read_keyboard_file(char *key_file);
extern int initkbfile(void );

/* ipasm.asm */
extern int CDECL ipcheck(char *s,int len);
extern int CDECL tcpcheck(char *psptr,char *tcpptr,int tcplen);
#ifndef MEM_DEBUG
extern void CDECL movebytes(void *to,void *from,int len);
#endif
extern int CDECL comparen(char *s1,char *s2,int len);
extern long CDECL longswap(long l);
extern unsigned int CDECL intswap(int i);
extern void CDECL tinst(void );
extern void CDECL tdeinst(void );

/* ncsaio.asm */
#include "vidinfo.h"
extern int CDECL n_color(int color);
extern int CDECL n_wrap(int wrap);
extern void CDECL n_erase(int ulrow,int ulcol,int lrrow,int lrcol);
extern unsigned int CDECL n_getchar(void);
extern void CDECL n_cur(int row,int col);
extern char CDECL n_row(void );
extern char CDECL n_col(void );
extern void CDECL n_clear(void );
extern void CDECL n_window(int uly,int ulx,int lry,int lrx);
extern void CDECL n_putchar(char c);
extern int CDECL n_chkchar(void );
extern void CDECL n_savewin(char *);
extern void CDECL n_restwin(char *);
extern void CDECL n_puts(char *s);
extern void CDECL n_sound(unsigned int freq,unsigned int len);
extern int CDECL n_findfirst(char *name,int type);
extern int CDECL n_findnext(void );
extern void CDECL n_draw(char *s,int len);
extern void CDECL n_scrup(int num,int ulx,int uly,int lrx,int lry);
extern void CDECL n_scrdn(int num,int ulx,int uly,int lrx,int lry);
extern void CDECL n_cheat(char *s,int len);
extern long CDECL n_clicks(void );
extern void CDECL n_biosattr(int );
extern unsigned int CDECL getdsk(void );
extern void CDECL chgdsk(unsigned int);
extern void CDECL ega43(void );
extern int CDECL n_flags(void );
extern void CDECL set_cur(int type);
extern void CDECL ega24(void );
extern void CDECL vga50(void );
extern int CDECL n_scrlck(void );
extern void CDECL save_break(void );
extern void CDECL restore_break(void );
extern void CDECL n_gmode(int mode);
extern void CDECL save_cursor(void );
extern void CDECL restore_cursor(void );
extern void CDECL install_cursor(int );
extern void CDECL n_attr(int );
extern void CDECL install_keyboard(void );
extern void CDECL fix_vid(void);
extern void CDECL get_mode(char *);
extern void CDECL set_mode(char *);
extern int CDECL get_size(void);
extern void CDECL clear_int(void );
extern void CDECL set_int(void );
extern void CDECL initvideo(void );
extern void CDECL getvconfig(struct vidinfo *);
extern void CDECL getvstate(struct vidstate *);
extern void CDECL setvstate(struct vidstate *);
extern void CDECL set_page(int );

/* ncsabrk.asm */
extern void CDECL install_break(int * break_addr);
extern void CDECL remove_break(void );

/* mouse routines */
#ifdef MSC
extern void CDECL mousecml(int *,int *,int *,int *);
#else
extern void mousecl(int *,int *,int *,int *);
#endif

/* egaset.asm */
extern void CDECL EGAset(int x,int y,int color);

/* vgaset.asm */
extern void CDECL VGAset(int x,int y,int color);

/* net.c */
#include "protocol.h"
extern  void statcheck(void );
extern  int netparms(uint irq,uint address,uint ioaddr);
extern  void netconfig(char *s);
extern  int netarpme(char *s);
extern  int initbuffer(void );
extern  int demux(int all);
extern  int dlayersend(struct ether *ptr,unsigned int size);
extern  int dlayerinit(void );
extern  void dlayershut(void );
extern  void pcgetaddr(char *s,int x,int y);

/* bootp.c */
extern int bootp(void );

/* ltalk.c */
#include "ltalk.h"
extern int KIParp(unsigned char ipnum[4],AddrBlk *addrloc);
extern void KIPsetzone(char *s);
extern int  CDECL LTopen(AddrBlk *s,unsigned int irq,unsigned int addr,unsigned int ioaddr);
extern int  CDECL LTgetaddr(AddrBlk *s,unsigned int addr,unsigned int ioaddr);
extern int  CDECL LTxmit(DLAYER *ptr,unsigned int size);
extern void CDECL LTrecv(void );
extern void CDECL LTupdate(void );
extern int  CDECL LTclose(void );
extern void CDECL listen1_c(unsigned char socket,unsigned int datalen,unsigned char header[],unsigned int *bufflen,unsigned char *bptr[]);
extern void CDECL listen2_c(unsigned int bufflen,struct ether *bptr);


/* ip.c */
extern  int ipinterpret(struct ip *p);
extern  int neticmpsend(unsigned char *machine, unsigned char type,unsigned char code, unsigned char *buffer, int n, int (*func)());
extern  int icmpunreach(struct tcp *p);

/* user.c */
extern  int netread(int pnum,char *buffer,int n);
extern  int netprintf(int pnum,char *fmt,...);
#if defined(NET14)
extern  int CDECL netwrite(int pnum,char *buffer,int n);
#else
extern  int netwrite(int pnum,char *buffer,int n);
#endif
extern  int netpush(int pnum);
extern  int netqlen(int pnum);
extern  int netroom(int pnum);
extern  int netsegsize(int newsize);
extern  int netquench(int newcredit);
extern  void netarptime(int t);
extern  void netsetip(unsigned char *st);
extern  void netgetip(unsigned char *st);
extern  void netsetbroad(unsigned char *st);
extern  void netsetmask(unsigned char *st);
extern  void netgetmask(unsigned char *st);
extern  void netfromport(int port);
extern  int netest(int pn);
extern  int netlisten(unsigned int serv);
extern  void netgetftp(int a[],int pnum);
extern  int netopen(unsigned char *s,unsigned int tport);
extern  int netxopen(unsigned char *machine,unsigned int service,unsigned int rto,unsigned int mtu,unsigned int mseg,unsigned int mwin);
extern  int doconnect(int pnum,int service,int mseg);
extern  int netopen2(int pnum);
#if defined(NET14)
extern  int CDECL netclose(int pnum);
#else
extern  int netclose(int pnum);
#endif
extern  int netinit(void );
extern  void netshut(void );

/* rspc.c */
extern  unsigned int RSokmem(int size);
extern  void RSbell(int w);
extern  void RSvis(int w);
extern  void RSinitall(void );
extern  void RSinsstring(int w,int x,int y,int attrib,int len,char *s);
extern  void RSdelchars(int w,int x,int y,int n);
extern  void RSbufinfo(int w,int numlines,int top,int bottom);
extern  void RSdrawsep(int w,int y,int data);
extern  void RSmargininfo(int w,int x,int data);
extern  void RSdelcols(int w,int n);
extern  void RSinscols(int w,int n);
extern  void RScursoff(int w);
extern  void RScurson(int w,int y,int x);
extern  void RSdraw(int w,int y,int x,int a,int len,unsigned char *ptr);
extern  void RSsetatt(int a,int w);
extern  void RSdellines(int w,int t,int b,int n,int select);
extern  void RSerase(int w,int y1,int x1,int y2,int x2);
extern  void RSinslines(int w,int t,int b,int n,int select);
extern  void RSsendstring(int w,char *ptr,int len);
extern  int translate(int ch);
extern  void vt100key(unsigned int c);
extern  int RSgets(int w,char *s,int lim, char echo);
extern  char *nbgets(char *s,int lim);
extern  int nbgetch(void );
extern  int nbget(void );
extern  void ftpstart(char dir,char *buf);

/* tools.c */
extern  int CDECL netsleep(int n);
extern  int enqueue(struct window *wind,char *buffer,int nbytes);
extern  int dequeue(struct window *wind,char *buffer,int nbytes);
extern  int rmqueue(struct window *wind,int nbytes);
extern  int transq(struct port *prt);
#ifdef OLD_WAY
extern  int comparen(unsigned char *s1,unsigned char *s2,int n);
#endif
extern  void netposterr(int num);
extern  int netgetevent(unsigned char mask,int *retclass,int *retint);
extern  int netputevent(int class,int what,int dat);
extern  int netputuev(int class,int what,int dat);
extern  char *neterrstring(int err);

/* services.c */
extern  void inv_port_err(int service, int port, uint8 *ip);
extern  char *service_file(char *path);

/* udp.c */
extern  int udpinterpret(struct udp *p,int ulen);
extern  int neturead(char *buffer, int udpindex, unsigned int nbytes);
extern  int netulisten(int port, int udpindex);
extern  int netusend(uint8 *machine,uint16 port,uint16 retport,uint8 *buffer,int n,int udpindex);
extern  int newudp(void);

/* arp.c */
extern  int reqarp(unsigned char *tipnum);
extern  int arpinterpret(struct plummer *p);
extern  int cachelook(unsigned char *ipn,int gate,int doarp);
extern  uint8 *netdlayer(unsigned char *tipnum);
extern  int netgetrarp(void );
extern  uint8 *getdlayer(unsigned char *tipnum);
extern  int netsetgate(unsigned char *ipn);

/* domain.c */
#include "domain.h"
extern  int Sdomain(char *mname);
extern  int udpdom(void );
extern  int domto(int num);

/* bkgr.c */
extern  void unsetrshd(void );
extern  void setrshd(void );
extern  void rshd(int code);
extern  void setftp(void );
extern  void unsetftp(void );
extern  void setftpwrt(int mode);
extern  void rftpd(int code);
extern  void ftpgo(void );
extern  void ftpd(int code,int curcon);
extern  void Sftpname(char *s);
extern  void Sftpuser(char *user);
extern  void Sftppass(char *user);
extern  int SftpDirection(void);
extern  void Sftphost(char *host);
extern  void Sftpstat(long *byt);

/* util.c */
#include "hostform.h"
extern  int Snetinit(void );
extern  int Snetopen(struct machinfo *m,int tport);
extern  void Scwritemode(int mode);
extern  int Scmode(void );
extern  void Stekmode(int mode);
extern  int Stmode(void );
extern  void Srcpmode(int mode);
extern  int Srmode(void );
extern  int Sftpmode(int mode);
extern  int Sfmode(void );
extern  int Snewcap(char *s);
extern  int Snewpsfile(char *s);
extern  int Snewhpfile(char *s);
extern  int Snewtekfile(char *s);
extern  int Snewkbfile(char *s);
extern  FILE *Sopencap(void );
extern  void Stask(void );
extern  int Stimerset(int class,int event,int dat,int howlong);
extern  int Stimerunset(unsigned char class,unsigned char event,int dat);
extern  int Scheckpass(char *us,char *ps);
extern  int Sneedpass(void );
extern  int Scompass(char *ps,char *en);
extern  int Sgetevent(int class,int *what,int *datp);
extern 	char *fixdirnm(char *name);
extern  int safedir(char *d, char *user, int mode);
extern  int gosafedir(char *user);

/* tcp.c */
extern  int tcpinterpret(struct tcp *p,int tlen);
extern  int tcpsend(struct port *pport,int dlen);

/* pcutil.c */
extern  int direxist(char dirname[]);
extern  char *firstname(char path[],int type);
extern  char *nextname(int type);
extern  void getdrive(unsigned int *d);
extern  void setdrive(unsigned int d);
extern  void dopwd(char *p,int l);
extern  int chgdir(char *file_name);
extern  void Scolorset(char *thecolor,char *st);
extern  char *colorlook(int cl);
extern	int octal_to_int(char *octal_str);
extern	int hex_to_int(char *hex_str);

/* protinit.c */
extern  void protinit(void );
extern  void neteventinit(void );
extern  void etherinit(void );
extern  void arpinit(void );
extern  void ipinit(void );
extern  void udpinit(void );
extern  void tcpinit(void );
extern  int makeport(void );

/* look.c */
#include "windat.h"
#if !defined LPR && !defined FTPMASTER && !defined MINITEL && !defined PASS && !defined NET14
extern  int main(int argc,char *argv[]);
#endif
extern  void examineCommandLine(int argc, char *argv[]);
extern  void vhead(int v);
extern  int dosessions(void );
extern  int inprocess(struct twin *tw);
extern  void endall(void );
extern  void errhandle(void );
extern  void vprint(int w,char *s);
extern  int tprintf(int w,char *s,...);
extern  void parsewrite(struct twin *tw,char *dat,int len);
extern  int newkey(struct twin *t1);
extern  void screendump(struct twin *t1,int scrollback);
extern  int dokey(struct twin *t1,int c);
extern  int dosescape(void );
extern  struct twin *creatwindow(void );
extern  void wrest(struct twin *t);
extern  void statline(void );
extern  void inswindow(struct twin *t,int wtype);
extern  void delwindow(struct twin *t,int wtype);
extern  void freewin(struct twin *t);
extern  int hexbyte(char *st);
extern  int tekinit(char *dev);
extern  void fdump(char *str);
extern  void fdumpc(int c);
extern  void endump(void );
extern  int graphit(void );
extern  int addsess(char *st);
#ifdef USETEK
extern  int leavetek(void );
#endif
extern  void capstat(char *s, int i);
extern  void set_vtwrap(int vs_win,int value);
extern  void strchar(char *s,char c);
extern  void sendmacro(void);
extern  void check_null_area(void );
extern  void heapdump(void );
extern  void print_windowlist(void );
extern  void CDECL (*attrptr)(int );

/* negotiat.c */
extern  void start_negotiation(struct twin *tw,int cvs);
extern  void parse(struct twin *tw,unsigned char *st,int cnt);

/* linemode.c */
extern  int LMgets(struct twin *t);
extern  int LMinterp_char(struct twin *t,int c);

/* scrlback.c */
extern  void scrollback(struct twin *tw );
extern  int setattr(int row,int attrn,int attrh);
extern  void resetscr(struct twin *tw );
extern  void updatescr(int attr);
extern  int chkmouse(void );
extern  int initmouse(void );
extern  void setmouse(void );
extern  void m_cur(int row,int col);
extern  int is_in(int row,int vsrow,int col);
extern  void remark(int row,int col,int vsrow, struct twin *tw );

/* menu.c */
#include "menu.h"
extern  void menuit(struct pt p[],int n);
extern  int makechoice(struct pt p[],int max_nump,int spec);
extern  void makespecial(void );
extern  void parmchange(void );
extern  void dispgr(void );
extern  void helpmsg(void );
extern  void help2(void );

/* confile.c */
extern  void Sgetconfig(struct config *cp);
extern  int Sreadhosts(void );
extern  int ncstrcmp(char *sa,char *sb);
extern  void Serrline(int n);
extern  int Scontoken(int c);
extern  int Sconfile(char *s);
extern  void Scopyfrom(char *s);
extern  struct machinfo *Smadd(char *mname);
extern  void Shostfile(char *ptr);
extern  void Shostpath(char *ptr);
extern  struct machinfo *Sgethost(char *machine);
extern  struct machinfo *Shostlook(char *hname);
extern  struct machinfo *Slooknum(int num);
extern  struct machinfo *Slookip(unsigned char *ipnum);
extern  int Sissep(int c);
extern  int Snewns(void );
extern  int Ssetns(unsigned char ipn[4]);
extern  void Ssetgates(void );

/* vsinterf.c */
#include "vsdata.h"
extern  int VSinit(int max_num);
extern  struct VSscrn *VSwhereis(int i);
extern  int VSnewscreen(int maxlines,int screensave,int maxwid,int IDC);
extern  int VSdestroy(int w);
extern  void VSdetatch(int w);
extern  void VSiclrbuf(void );
extern  int VSredraw(int w,int x1,int y1,int x2,int y2);
extern  int VSwrite(int w,char *ptr,int len);
extern  int VSdirectwrite(int w,char *ptr,int len);
extern  int VSclear(int w);
extern  int VSpossend(int w,int x,int y,int echo);
extern  int VSkbsend(int w,unsigned char k,int echo);
extern  int VSclearall(int w);
extern  int VSreset(int w);
extern  char *VSgetline(int w,int y);
extern  int VSsetrgn(int w,int x1,int y1,int x2,int y2);
extern  int VSscrolback(int w,int in);
extern  int VSscrolforward(int w,int n);
extern  int VSscrolright(int w,int n);
extern  int VSscrolleft(int w,int n);
extern  int VSscrolcontrol(int w,int scrolon,int offtop);
extern  int VSgetrgn(int w,int *x1,int *y1,int *x2,int *y2);
extern  int VSsnapshot(int w);
extern  int VSvalids(int w);
extern  int VSmaxwidth(int w);
extern  struct VSline *VSIGetLineStart(int w,int y1);
extern  char *VSIstrcopy(char *src,int len,char *dest,int table);
extern  long VSgettext(int w,int x1,int y1,int x2,int y2,char *charp,long max_num,char *EOLS,int table);
extern  int VSgtlines(int w);
extern  int VSsetlines(int w,int lines);

/* vsem.c */
extern  void VSem(unsigned char *c,int ctr);

/* vsintern.c */
extern  int VSIclip(int *x1,int *y1,int *x2,int *y2,int *n,int *offset);
extern  int VSIcdellines(int w,int top,int bottom,int n,int scrolled);
extern  int VSIcinslines(int w,int top,int bottom,int n,int scrolled);
extern  void VSIcurson(int w,int x,int y,int ForceMove);
extern  void VSIcuroff(int w);
extern  struct VSline *VSInewline(void );
extern  void VSIlistndx(struct VSline *ts,struct VSline *as);
extern  void VSIscroff(void );
extern  void VSIelo(int s);
extern  void VSIes(void );
extern  void VSItabclear(void );
extern  void VSItabinit(void );
extern  void VSIreset(void );
extern  void VSIlistmove(struct VSline *TD,struct VSline *BD,struct VSline *TI,struct VSline *BI);
extern  void VSIdellines(int n,int s);
extern  void VSIinslines(int n,int s);
extern  void VSIscroll(void );
extern  void VSIindex(void );
extern  void VSIwrapnow(int *xp,int *yp);
extern  void VSIeeol(void );
extern  void VSIdelchars(int x);
extern  void VSIrindex(void );
extern  void VSIebol(void );
extern  void VSIel(int s);
extern  void VSIeeos(void );
extern  void VSIebos(void );
extern  void VSIrange(void );
extern  void VTsendpos(void );
extern  void VTsendstat(void );
extern  void VTsendident(void );
extern  void VTalign(void );
extern  void VSIapclear(void );
extern  void VSIsetoption(int toggle);
extern  void VSItab(void );
extern  void VSIinschar(int x);
extern  void VSIinsstring(int len,char *start);
extern  void VSIsave(void );
extern  void VSIrestore(void );
extern  void VSIdraw(int VSIwn,int x,int y,int a,int len,char *c);
extern  void VSIstring(unsigned char *c,int len);

/* rg0.c */
extern  int RG0newwin(void );
extern  void RG0clrscr(int w);
extern  void RG0close(int w);
extern  void RG0point(int w,int x,int y);
extern  void RG0drawline(int w, int a, int b, int c, int d);
extern  void RG0pagedone(int w);
extern  void RG0dataline(int w,char *data,int count);
extern  void RG0pencolor(int w,int color);
extern  void RG0charmode(int w,int rotation,int size);
extern  void RG0showcur(void );
extern  void RG0lockcur(void );
extern  void RG0hidecur(void );
extern  void RG0bell(int w);
extern  char *RG0devname(void );
extern  void RG0uncover(int w);
extern  void RG0init(void );
extern  void RG0info(int w,int a,int b,int c,int d,int v);
extern  void RG0gmode(void );
extern  void RG0tmode(void );

/* rge.c */
extern  void RGEgmode(void );
extern  void RGEtmode(void );
extern  void RGEclrscr(int w);
extern  int RGEnewwin(void );
extern  void RGEclose(int w);
extern  void RGEpoint(int w,int x,int y);
extern  void RGEpagedone(int w);
extern  void RGEdataline(int w,char *data,int count);
extern  void RGEpencolor(int w,int color);
extern  void RGEcharmode(int w,int rotation,int size);
extern  void RGEshowcur(void );
extern  void RGElockcur(void );
extern  void RGEhidecur(void );
extern  void RGEdrawline(int w,int x0,int y0,int x1,int y1);
extern  void RGEbell(int w);
extern  char *RGEdevname(void );
extern  void RGEinit(void );
extern  void RGEuncover(int w);
extern  void RGEinfo(int w,int a,int b,int c,int d,int v);

/* rgv.c */
extern  int RGVnewwin(void );
extern  void RGVclrscr(int w);
extern  void RGVclose(int w);
extern  void RGVpoint(int w,int x,int y);
extern  void RGVdrawline(int w,int x0,int y0,int x1,int y1);
extern  void RGVpagedone(int w);
extern  void RGVdataline(int w,char *data,int count);
extern  void RGVpencolor(int w,int color);
extern  void RGVcharmode(int w,int rotation,int size);
extern  void RGVshowcur(void );
extern  void RGVlockcur(void );
extern  void RGVhidecur(void );
extern  void RGVbell(int w);
extern  char *RGVdevname(void );
extern  void RGVuncover(int w);
extern  void RGVinit(void );
extern  void RGVinfo(int w,int a,int b,int c,int d,int v);
extern  void RGVgmode(void );
extern  void RGVtmode(void );

/* rgh.c */
extern  void RGHgmode(void );
extern  void RGHtmode(void );
extern  void RGHclrscr(int w);
extern  int RGHnewwin(void );
extern  void RGHclose(int w);
extern  void RGHpoint(int w,int x,int y);
extern  void RGHdrawline(int w,int x0,int y0,int x1,int y1);
extern  void RGHpagedone(int w);
extern  void RGHdataline(int w,char *data,int count);
extern  void RGHpencolor(int w,int color);
extern  void RGHcharmode(int w,int rotation,int size);
extern  void RGHshowcur(void );
extern  void RGHlockcur(void );
extern  void RGHhidecur(void );
extern  void RGHbell(int w);
extern  char *RGHdevname(void );
extern  void RGHinit(void );
extern  void RGHuncover(int w);
extern  void RGHinfo(int w,int a,int b,int c,int d,int v);

/* rg9.c */
extern  void RG9gmode(void );
extern  void RG9tmode(void );
extern  void RG9drawline(int w,int x0,int y0,int x1,int y1);
extern  void RG9clrscr(int w);
extern  int RG9newwin(void );
extern  void RG9close(int w);
extern  void RG9point(int w,int x,int y);
extern  void RG9pagedone(int w);
extern  void RG9dataline(int w,char *data,int count);
extern  void RG9pencolor(int w,int color);
extern  void RG9charmode(int w,int rotation,int size);
extern  void RG9showcur(void );
extern  void RG9lockcur(void );
extern  void RG9hidecur(void );
extern  void RG9bell(int w);
extern  char *RG9devname(void );
extern  void RG9init(void );
extern  void RG9uncover(int w);
extern  void RG9info(int w,int a,int b,int c,int d,int v);

/* rgp.c */
extern  void RGPoutfunc(void (*f)(char *));
extern  int RGPnewwin(void );
extern  void RGPclrscr(int w);
extern  void RGPclose(int w);
extern  void RGPpoint(int w,int x,int y);
extern  void RGPdrawline(int w,int x0,int y0,int x1,int y1);
extern  void RGPpagedone(int w);
extern  void RGPdataline(int w,char* data,int count);
extern  void RGPpencolor(int w,int color);
extern  void RGPcharmode(int w,int rotation,int size);
extern  void RGPshowcur(void );
extern  void RGPlockcur(void );
extern  void RGPhidecur(void );
extern  void RGPbell(int w);
extern  void RGPuncover(int w);
extern  char *RGPdevname(void );
extern  void RGPinit(void );
extern  void RGPinfo(int w,int a,int b,int c,int d,int v);
extern  void RGPgmode(void );
extern  void RGPtmode(void );

/* rgc.c */
extern  void RGCgmode(void );
extern  void RGCtmode(void );
extern  void RGCdrawline(int w,int x0,int y0,int x1,int y1);
extern  void RGCclrscr(int w);
extern  int RGCnewwin(void );
extern  void RGCclose(int w);
extern  void RGCpoint(int w,int x,int y);
extern  void RGCpagedone(int w);
extern  void RGCdataline(int w,char *data,int count);
extern  void RGCpencolor(int w,int color);
extern  void RGCcharmode(int w,int rotation,int size);
extern  void RGCshowcur(void );
extern  void RGClockcur(void );
extern  void RGChidecur(void );
extern  void RGCbell(int w);
extern  char *RGCdevname(void );
extern  void RGCinit(void );
extern  void RGCuncover(int w);
extern  void RGCinfo(int w,int a,int b,int c,int d,int v);

/* rgep.c */
extern  void RGEPgmode(void );
extern  void RGEPtmode(void );
extern  void RGEPclrscr(int w);
extern  int RGEPnewwin(void );
extern  void RGEPclose(int w);
extern  void RGEPpoint(int w,int x,int y);
extern  void RGEPpagedone(int w);
extern  void RGEPdataline(int w,char *data,int count);
extern  void RGEPpencolor(int w,int color);
extern  void RGEPcharmode(int w,int rotation,int size);
extern  void RGEPshowcur(void );
extern  void RGEPlockcur(void );
extern  void RGEPhidecur(void );
extern  void RGEPdrawline(int w,int x0,int y0,int x1,int y1);
extern  void RGEPbell(int w);
extern  char *RGEPdevname(void );
extern  void RGEPinit(void );
extern  void RGEPoutfunc(void (*f)(char ));
extern  void RGEPuncover(int w);
extern  void RGEPinfo(int w,int a,int b,int c,int d,int v);

/* rghp.c */
extern  void RGHPoutfunc(void (*f)(char *));
extern  int RGHPnewwin(void );
extern  void RGHPclrscr(int w);
extern  void RGHPclose(int w);
extern  void RGHPpoint(int w,int x,int y);
extern  void RGHPdrawline(int w,int x0,int y0,int x1,int y1);
extern  void RGHPpagedone(int w);
extern  void RGHPdataline(int w,char* data,int count);
extern  void RGHPpencolor(int w,int color);
extern  void RGHPcharmode(int w,int rotation,int size);
extern  void RGHPshowcur(void );
extern  void RGHPlockcur(void );
extern  void RGHPhidecur(void );
extern  void RGHPbell(int w);
extern  void RGHPuncover(int w);
extern  char *RGHPdevname(void );
extern  void RGHPinit(void );
extern  void RGHPinfo(int w,int a,int b,int c,int d,int v);
extern  void RGHPgmode(void );
extern  void RGHPtmode(void );

/* vgtek.c */
extern  void showmouse(void);
extern  void hidemouse(void);
extern  void VGinit(void );
extern  void VGuncover(int vw);
extern  int VGdevice(int vw,int dev);
extern  int VGnewwin(int device);
extern  void VGclrstor(int vw);
extern  void VGdumpstore(int vw,void (*func)(int data));
extern  void VGdraw(int vw,char c);
extern  void VGpage(int vw);
extern  int VGpred(int vw,int dest);
extern  void VGstopred(int vw,int dest);
extern  void VGredraw(int vw,int dest);
extern  void VGzoom(int vw,int x0,int y0,int x1,int y1);
extern  void VGwhatzoom(int vw,int *px0,int *py0,int *px1,int *py1);
extern  void VGzcpy(int src,int dest);
extern  void VGclose(int vw);
extern  int VGwrite(int vw,char *data,int count);
extern  void VGgiveinfo(int vw);
extern  char *VGrgname(int rgdev);
extern  void VGgmode(int rgdev);
extern  void VGtmode(int rgdev);
extern  void VGgindata(int vw,int x,int y,char c,char *a);
extern  int ginmouse(void );
extern  void setgin(void );
extern  void resetgin(void );

/* tekstor.c */
#include "tekstor.h"
extern STOREP newstore(void );
extern void freestore(STOREP s);
extern int addstore(STOREP s,char d);
extern void topstore(STOREP s);
extern int nextitem(STOREP s);
extern int unstore(STOREP s);

#ifdef MINITEL
/* minitel */
extern  void main(int argc,char * *argv);
extern  int dochoice(int c);
extern  void noecho(char *s);
extern  void passwrite(char *s);
extern  int passlist(char *s);
extern  void Sencompass(char *ps);
extern  void main(int argc,char * *argv);
extern  void errhandle(void );
#endif

/* netbicc.c */
#include "bicc.h"
extern unsigned int CDECL anr_c(struct acb *acb_ptr);
extern int CDECL ILetopen(unsigned char *s,unsigned int irq,unsigned int addr,unsigned int ioaddr);
extern int CDECL ILgetaddr(unsigned char *s,unsigned int address,unsigned int ioaddr);
extern int CDECL ILetclose(void );
extern void CDECL ILrecv(void );
extern void CDECL ILetupdate(void );
extern int CDECL ILxmit(DLAYER *packet,unsigned int count);

/* packet.c */
extern  int CDECL pketopen(unsigned char *s,unsigned int irq,unsigned int address,unsigned int ioaddr);
extern  int CDECL pkgetaddr(unsigned char *s,unsigned int address,unsigned int ioaddr);
extern  int CDECL pkxmit(DLAYER *packet,unsigned int length);
extern  int CDECL pketclose(void );
extern  void CDECL pkrecv(void );
extern  void CDECL pketupdate(void );
#ifdef __TURBOC__
extern  void interrupt pkt_receiver2(unsigned int bp,unsigned int di,unsigned int si,unsigned int ds,unsigned int es,unsigned int dx,unsigned int cx,unsigned int bx,unsigned int ax);
#elif __WATCOMC__
extern void interrupt pkt_receiver2(union INTPACK r);
#elif __ZTC__
#include <int.h>
extern void pkt_receiver2(struct INT_DATA *pd);
#else
extern  void interrupt pkt_receiver2(unsigned int es,unsigned int ds,unsigned int di,unsigned int si,unsigned int bp,unsigned int sp,unsigned int bx,unsigned int dx,unsigned int cx,unsigned int ax);
#endif

/* packet2.asm */
extern  void pkt_receiver();
extern  int  CDECL pkt_access_type(int if_class,int if_type,int if_number,char *type,int typelen,void (*receiver)());
extern  int  CDECL pkt_driver_info(int handle);
extern  int  CDECL pkt_set_recv_mode(int handle,int mode);
extern  int  CDECL pkt_release_type(int handle);
extern  int  CDECL pkt_get_address(int handle,char *storage,int len);
extern  int  CDECL pkt_send_pkt(char *packet,int len);
extern  void CDECL clear_int(void );
extern  void CDECL set_int(void );

/* dndll.c */
#include "decnet.h"
extern void c_r_callback(struct ucb *u);
extern void c_t_callback(struct ucb *u);
extern void p_farptr(unsigned char *fp);
extern void p_en_addr(unsigned char *e);
extern unsigned char *p_hex(unsigned char *fp,int n);
extern void dcbdump(struct dcb *d);
extern unsigned char *nbcpy(unsigned char *d,unsigned char *s,int n);
extern void dll_read_chan(struct dcb *d);
extern int dll_deallocate(struct dcb *d,unsigned char *b);
extern int dll_open(unsigned int prot,int nb);
extern struct dcb *dcb_for_prot(unsigned int prot);
extern struct dcb *dcb_for_ucb(struct ucb *u);
extern int check_tx(void );
extern int CDECL DNetopen(unsigned char *s,unsigned int irq,unsigned int addr,unsigned int ioaddr);
extern int CDECL DNgetaddr(unsigned char *s,unsigned int address,unsigned int ioaddr);
extern int CDECL DNetclose(void );
extern void CDECL DNrecv(void );
extern void CDECL DNetupdate(void );
extern int CDECL DNxmit(DLAYER *pkt,unsigned int count);

/* net503.asm */
extern void E4setwire(int);

#ifdef LPR
/* lp.c */
extern struct machinfo *lookup(char *host);
extern int open_connection(struct machinfo *machine_record,int source_port,int dest_port);
extern void crash(char *msg,...);
extern void checkerr(void );
extern int lgetevent(int class, int *what, int *datp);
extern int nprintf(int connection_id, char *format,...);
extern int nread(int connection_id, char *buff, int buff_size);
extern void vprint(int w, char *s);
extern int breakstop(void );
#endif

#ifdef NET14
/* run14.c */
extern void CDECL print_int(int );
extern void CDECL print_int2(int );
extern int CDECL get_comm_char(int );
extern int CDECL int14open(int );
extern void CDECL int14netsleep(void );

/* int14.asm */
extern void CDECL int14inst(void );
extern void CDECL int14deinst(void );
extern int CDECL int14check(void );
extern int CDECL int14receive(void );
extern void CDECL timeinst(void );
extern void CDECL timedeinst(void );

/* init14.c */
extern int int14init(void );

extern int go_tsr(void );

/* netutils.c */
extern int connect_sock(struct machinfo *m, int, int);
#endif

/* mouse.c */
extern int nm_getchar(void);
extern int nm_chkchar(void);
extern int nm_scrlck(void);
extern int nm_chkmouse(void);
extern int nm_initmouse(void);
extern int nm_mousespeed(int change);

/* scr_rest.c */
extern void init_text(void);
extern void end_text(void);

#ifdef TELBIN
#include "mouse.h"		/* for new mouse routines- replaces n_chkchar & n_scrlck with nm_chkchar & nm_scrlck */
#endif

#endif
