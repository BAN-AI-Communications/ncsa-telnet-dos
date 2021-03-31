/*
*
*	net.c
*	Network interface(between IP and the hardware routines)
*/

#include	<stdio.h>

/*
*	Global Variables
*/

/*
*	Netparms(irq, address, ioaddr)
*
*	Set network parameters(good for all cards)
*
*/
static int nnirq=3,nnaddr=0xd000,nnioaddr=0x300;
netparms(irq,address,ioaddr)

int	irq;											/* interrupt level */
int	address;										/* shared memory address */
int	ioaddr;											/* io address */
{
	printf("netparms : irq = %d, address = %8x, ioaddr = %8x\n",irq,address,ioaddr);
	return(0);
}

/*
*	netconfig(s)
*	configure the network based on the hardware type(s)
*/
void netconfig(s)
char *s;
{
	printf("netconfig : s = %s\n",s);
}

/*
*	demux(all)
*
*	Generic routine for people to call to process the network
*(if *all* then keep processing until all packets are gone)
*/
void demux(all)

int	all;
{
#ifdef	UNDEF
	DLAYER *firstlook;
	int	getcode;
	if(0){
		firstlook=(DLAYER *)rxBuf[xRxOut].data;
		getcode=firstlook->type;			/* where does it belong? */
		switch(getcode){					/* what to do with it? */
			case EARP:
			case ERARP:
				printf("Got an ARP\n");
				break;

			case EIP:
				printf("Got an IP\n");
				ipinterpret(firstlook);
				break;

			default:
				printf("Got something BOGUS\n");
				break;
  		  }
		xRxOut=(xRxOut+1)&RXMASK;
	  }
#endif
}

/*
*	dlayersend(ptr, size)
*
*	Send the packet *ptr* of size *size* out the appropriate hardware
*
*/
dlayersend(ptr, size)

char	*ptr;
int	size;
{
	printf("dlayersend : size=%d\n",size);
	return	0;
}

/*
*	dlayerinit()
*
*	Initialize the hardware
*
*/

dlayerinit()
{
	printf("dlayerinit : \n");
	return	0;
}

/*
*	dlayershut()
*
*	shut down the hardware 
*
*/
void dlayershut()
{
	printf("dlayershut : \n");
}

/*
*	pcgetaddr(s, ioaddr, memloc)
*
*	get the pc address of the ethernet card and return in s
*
*/
void pcgetaddr(s, x, y)
char s[];
int	x,y;
{
	int	i;

	for(i=0; i<6; i++)
		s[i]=0x55;
}

/*
*	netarpme(s)
*
*   send myself an arp
*
*/
netarpme(s)

char *s;
{
	printf("netarpme : \n");
	return 0;
}

