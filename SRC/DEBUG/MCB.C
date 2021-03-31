#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include "externs.h"

struct MCB {
	char chain;
	unsigned int pid,
		psize;
	char unused[11];
  };

typedef struct MCB huge *PTRMCB;

static void far *ffmcb(void);
static void prn_header(void);
static void prn_mcb(PTRMCB pm);
static void prn_pid_own(unsigned int pid, unsigned int parent);
#ifdef QAK
static void huge *Normalize_Addr(void huge *mem_addr);
#endif

void mcb(void )
{
	PTRMCB ptrmcb;

	ptrmcb=(PTRMCB)ffmcb();
    prn_header();
    prn_mcb(ptrmcb);

	do {
		ptrmcb+=ptrmcb->psize+1;
#ifdef QAK
		ptrmcb=(PTRMCB)Normalize_Addr((void huge *)ptrmcb);
#endif
        prn_mcb(ptrmcb);
	  }	while(ptrmcb->chain == 'M');
	printf("===========================================================");
    puts  ("=============");
    getch();
}	/* end mcb() */

static void far *ffmcb(void)
{
	union REGS regs;
	struct SREGS sregs;
	unsigned far *segmptr;

	regs.h.ah=0x52;
	intdosx(&regs,&regs,&sregs);

	segmptr=MK_FP(sregs.es,regs.x.bx-2);
	return(MK_FP(*segmptr,0));
}	/* end ffmcb() */

static void prn_header(void )
{
	printf("===========================================================");
	puts  ("=============");
	puts("MCB MCB  ID PID      MB PAR- ENV    OWNER");
	puts("NO. SEG            SIZE ENT  BLK?");
	printf("===========================================================");
	puts  ("=============");
}	/* end prn_header() */

static void prn_mcb(PTRMCB pm)
{
	static unsigned int cnt=0;
	static unsigned int mcbnum=1;
	unsigned int parid;
	unsigned int mcbseg;
	char envf;
	unsigned int envseg;

	parid=*(unsigned far *)MK_FP(pm->pid,0x16);
	mcbseg=FP_SEG(pm);
	envseg=*(unsigned far *)MK_FP(pm->pid,0x2C);

    envf=((mcbseg+1)==envseg) ? 'Y' : 'N';

	if(parid==pm->pid)
		cnt++;

	if(!envseg && cnt==2)
		envf='Y';

    printf("%2.2u%06.4X%2.1c%06.4X%7lu%5.4X %-5.1c",mcbnum++,mcbseg,pm->chain,pm->pid,(unsigned long)pm->psize*(unsigned long)16,parid,envf);

    prn_pid_own(pm->pid,parid);
}	/* end prn_mcb() */

static void prn_pid_own(unsigned int pid, unsigned int parent)
{
	unsigned far *envsegptr;
	char far *envptr;
	unsigned far *envsizeptr;
	unsigned int envsize;

	static unsigned char ccnum=0;

	static unsigned int prev_pid=0xFFFF;

	switch(pid) {
		case 0:
            puts("FREE MEMORY CONTROL BLOCK");
			return;

		case 8:
            puts("IBMDOS.COM/MSDOS.SYS");
			return;
	  }	/* end switch */

	envsegptr=(unsigned far *)MK_FP(pid,0x2C);

	envptr=(char far *)MK_FP(*envsegptr,0);

	envsizeptr=(unsigned far *)MK_FP(*envsegptr-1,0x3);

	envsize=*envsizeptr*16;

	if(pid==parent) {
		if(prev_pid!=pid)
			ccnum++;
        printf("COMMAND.COM COPY #%-2u\n",(unsigned int)ccnum);
		prev_pid=pid;
		return;
	  }	/* end if */

	while(envsize) {
		while(--envsize && *envptr++);
		if(!*envptr && *(unsigned far *)(envptr+1) == 0x1) {
			envptr+=3;
			break;
		  }	/* end if */
	  }	/* end while */

	if(envsize) {
		while(*envptr)
			putchar(*envptr++);
		putchar('\n');
	  }	/* end if */
	else
		puts("UNKNOWN OWNER");
}	/* end prn_pid_own() */

#ifdef QAK
void huge *Normalize_Addr(void *mem_addr)
{
	unsigned int seg_val,		/* the segment of the address we are normalizing */
		offset_val,				/* the offset of the address we are normalizing */
		temp_val;				/* temporary value for calculating the amount to add to the segment */

	seg_val=FP_SEG(mem_addr);
	temp_val=offset_val=FP_OFF(mem_addr);
	temp_val>>=4;
	seg_val+=temp_val;
	offset_val-=(temp_val<<4);
	return((void huge *)MK_FP(seg_val,offset_val));
}	/* end Normalize_Addr() */
#endif
