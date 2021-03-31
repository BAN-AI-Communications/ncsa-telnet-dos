//Really small program to test net14.exe with - RMG 931100

#include <stdio.h>
#include <dos.h>
#ifdef SERIALCOM
#include <bios.h>
#endif
#include <conio.h>

#define PORT 0
#define HOST "void"

void main(int argc, char *argv[])
{
  char c=0,in,stat;
  void initb(char *);
  int  chkb(void);
  int  getb(char *);
  int  putb(char);

  if(argc == 2) {
    if(strcmp(argv[1],"-o"))
      initb(argv[1]);
  }
  else {
    printf("Usage: st {-o | <hostname>}\n");
    exit(1);
  }

  while(1) {
    while(!kbhit()) {
      stat=chkb();
      if(stat & 1) {
        in=getb(&c);
        putch(c);
        if(in)
          printf("error %d getting byte\n",in);
      }
    }
    c=getch();
    putb(c);
  }

return;
}


int getb(char *c)
{
  char local_c;
#ifdef SERIALCOM
  int i;
#endif
  union _REGS inregs,outregs;
  struct _SREGS segregs;

#ifdef SERIALCOM
  i = (char) _bios_serialcom(_COM_RECEIVE, PORT, 0);
  local_c = i;
#else
  inregs.h.ah = (char) 2;
  inregs.x.dx = (char) PORT;
  _int86x( 0x14, &inregs, &outregs, &segregs);
  local_c = (char) outregs.h.al;
#endif
  *c = local_c;

return((int) outregs.h.ah);
}


int chkb(void)
{
  union _REGS inregs, outregs;
  struct _SREGS segregs;

  inregs.h.ah = (char) 3;
  inregs.x.dx = (char) PORT;
  _int86x( 0x14, &inregs, &outregs, &segregs);

return((int) outregs.h.ah);
}


int putb(char c)
{
  union _REGS inregs,outregs;
  struct _SREGS segregs;

  inregs.h.ah = (char) 1;
  inregs.h.al = (char) c;
  inregs.x.dx = (char) PORT;
  _int86x( 0x14, &inregs, &outregs, &segregs);

return((int) outregs.h.ah);
}


void initb(char *host)
{
  int i,l;
  union _REGS inregs,outregs;
  struct _SREGS segregs;

  inregs.h.ah = 0;
  inregs.x.dx = PORT;
  _int86x( 0x14, &inregs, &outregs, &segregs);

  putb(2);
  l=strlen(host);
  for(i=0;i<l;i++)
    putb(host[i]);
  putb(3);

return;
}
