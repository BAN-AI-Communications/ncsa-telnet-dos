#define MK_FP(__o,__s) (void far *)(((unsigned long)(__s)<<16)|(unsigned)(__o))
#define CHEK_GSS	{	printf("Gray scale summing is %s\n",((*((char far *)MK_FP(0x89,0x40))) & 2) ? "set":"unset"); getch(); }

#ifndef DNRINDEX
#define DNRINDEX 0
#endif
